#include "binary_reader.h"
#include <QFile>
#include <QFileInfo>
#include <QtConcurrentRun>
#include <exception>

ReadResult readPage(const ReadRequest &request) {
    ReadResult result;
    result.request = request;
    if (request.length < 0 || request.length > 1024*1024) {
        result.error = "La lectura solicitada supera el límite de 1 MiB."; return result;
    }
    QFile file(request.path);
    QFileInfo info(file);
    if (!info.isFile()) { result.error = "Selecciona un archivo normal, no una carpeta o dispositivo."; return result; }
    if (!file.open(QIODevice::ReadOnly)) { result.error = file.errorString(); return result; }
    result.size = file.size();
    result.modified = file.fileTime(QFileDevice::FileModificationTime);
    if (request.expectedSize >= 0 &&
        (result.size != request.expectedSize || result.modified != request.expectedModified)) {
        result.error = "El archivo ha cambiado. Vuelve a abrirlo para actualizar la vista.";
        return result;
    }
    if (request.offset < 0 || request.offset > result.size) {
        result.error = "La posición está fuera del archivo."; return result;
    }
    if (!file.seek(request.offset)) { result.error = file.errorString(); return result; }
    const qint64 length = qMin(request.length, result.size - request.offset);
    result.bytes = file.read(length);
    if (result.bytes.size() != length || file.error() != QFileDevice::NoError)
        result.error = "No se pudo leer la página completa: " + file.errorString();
    if (file.size() != result.size || file.fileTime(QFileDevice::FileModificationTime) != result.modified)
        result.error = "El archivo cambió durante la lectura. Vuelve a abrirlo.";
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
    watcher_.setFuture(QtConcurrent::run(&pool_, [read = read_, request] {
        try { return read(request); }
        catch (const std::exception &e) {
            ReadResult result; result.request = request;
            result.error = QString::fromUtf8(e.what()); return result;
        }
        catch (...) {
            ReadResult result; result.request = request;
            result.error = "Error inesperado durante la lectura."; return result;
        }
    }));
}

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
