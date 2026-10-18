#include "binary_reader.h"
#include <QFile>
#include <QFileInfo>
#include <QtConcurrentRun>
#include <exception>
#include <QThread>

namespace { QThreadPool *documentReadPool(){static QThreadPool pool;static const bool configured=[] {pool.setMaxThreadCount(qBound(2,QThread::idealThreadCount(),8));return true;}();Q_UNUSED(configured);return &pool;} }

QByteArray MemoryData::read(qint64 offset, qint64 length) const {
    QByteArray result; qint64 position=0;
    for (const auto &block : blocks) {
        const auto begin=qMax(offset,position), end=qMin(offset+length,position+block.size());
        if(end>begin) result.append(block.constData()+(begin-position),end-begin);
        position+=block.size(); if(position>=offset+length) break;
    }
    return result;
}
std::shared_ptr<const MemoryData> MemoryData::splice(qint64 offset,qint64 remove,const QByteArray &insert) const {
    auto next=std::make_shared<MemoryData>();
    auto append=[&](QByteArray bytes) {
        if(bytes.isEmpty())return;
        if(!next->blocks.isEmpty() && next->blocks.last().size()+bytes.size()<=BlockSize) next->blocks.last().append(bytes);
        else next->blocks.append(std::move(bytes));
    };
    auto slice=[&](qint64 start,qint64 count) {
        qint64 position=0;
        for(const auto &block:blocks) {
            const auto begin=qMax(start,position), end=qMin(start+count,position+block.size());
            if(end>begin) append(begin==position && end==position+block.size()?block:block.mid(begin-position,end-begin));
            position+=block.size(); if(position>=start+count)break;
        }
    };
    slice(0,offset);
    for(qsizetype at=0;at<insert.size();at+=BlockSize)append(insert.mid(at,BlockSize));
    slice(offset+remove,size-offset-remove);next->size=size-remove+insert.size();return next;
}

ReadResult readPage(const ReadRequest &request) {
    ReadResult result;
    result.request = request;
    if (request.length < 0 || request.length > 1024*1024) {
        result.error = "The requested read exceeds the 1 MiB limit."; return result;
    }
    if(request.memory) {
        result.size=request.memory->size; result.modified=request.expectedModified;
        if(request.offset<0||request.offset>result.size) {result.error="The offset is outside the document.";return result;}
        result.bytes=request.memory->read(request.offset,qMin(request.length,result.size-request.offset));return result;
    }
    QFile file(request.path);
    QFileInfo info(file);
    if (!info.isFile()) { result.error = "Select a regular file, not a folder or device."; return result; }
    if (!file.open(QIODevice::ReadOnly)) { result.error = file.errorString(); return result; }
    result.size = file.size();
    result.modified = file.fileTime(QFileDevice::FileModificationTime);
    if (request.expectedSize >= 0 &&
        (result.size != request.expectedSize || result.modified != request.expectedModified)) {
        result.error = "The file has changed. Reopen it to refresh the view.";
        return result;
    }
    if (request.offset < 0 || request.offset > result.size) {
        result.error = "The offset is outside the file."; return result;
    }
    if (!file.seek(request.offset)) { result.error = file.errorString(); return result; }
    const qint64 length = qMin(request.length, result.size - request.offset);
    result.bytes = file.read(length);
    if (result.bytes.size() != length || file.error() != QFileDevice::NoError)
        result.error = "Could not read the entire page: " + file.errorString();
    if (file.size() != result.size || file.fileTime(QFileDevice::FileModificationTime) != result.modified)
        result.error = "The file changed while reading. Reopen it.";
    if (!result.error.isEmpty()) result.bytes.clear();
    return result;
}

BinaryReader::BinaryReader(QObject *parent, ReadFunction read) : QObject(parent), read_(std::move(read)) {
    pool_.setMaxThreadCount(1);
    connect(&watcher_, &QFutureWatcher<ReadResult>::finished, this, [this] {
        const auto result = watcher_.result();
        running_ = false;
        ++activity_.completed;
        activity_.lastMs = elapsed_.elapsed();
        if (!result.error.isEmpty()) ++activity_.errors;
        if (stopping_ || activeGeneration_ != generation_) ++activity_.discarded;
        if (!stopping_ && activeGeneration_ == generation_) emit ready(result);
        if (!stopping_ && pending_) {
            auto next = std::move(*pending_);
            pending_.reset();
            start(std::move(next));
        } else if (!running_) emit idle();
    });
}

void BinaryReader::request(ReadRequest request) {
    if (stopping_) return;
    ++generation_;
    if (running_) {
        if (pending_) ++activity_.replaced;
        pending_ = std::move(request);
    }
    else start(std::move(request));
}

void BinaryReader::start(ReadRequest request) {
    running_ = true;
    ++activity_.started;
    activity_.offset = request.offset;
    activity_.length = request.length;
    elapsed_.start();
    activeGeneration_ = generation_;
    watcher_.setFuture(QtConcurrent::run(documentReadPool(), [read = read_, request] {
        try { return read(request); }
        catch (const std::exception &e) {
            ReadResult result; result.request = request;
            result.error = QString::fromUtf8(e.what()); return result;
        }
        catch (...) {
            ReadResult result; result.request = request;
            result.error = "Unexpected error while reading."; return result;
        }
    }));
}

BinaryReader::~BinaryReader(){disconnect(&watcher_,nullptr,this,nullptr);watcher_.waitForFinished();}

void BinaryReader::shutdown() {
    stopping_ = true;
    ++generation_;
    pending_.reset();
    if (!running_) emit idle();
}

void BinaryReader::invalidate() {
    ++generation_;
    pending_.reset();
}

ReaderActivity BinaryReader::activity() const {
    auto snapshot = activity_;
    snapshot.running = running_;
    snapshot.pending = pending_.has_value();
    snapshot.stopping = stopping_;
    snapshot.elapsedMs = running_ ? elapsed_.elapsed() : 0;
    return snapshot;
}
