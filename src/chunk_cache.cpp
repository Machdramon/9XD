#include "chunk_cache.h"
#include <QMutexLocker>

ChunkCache::ChunkCache(ReadFunction backing) : backing_(std::move(backing)) {}
void ChunkCache::reset(const QString &path) {
    QMutexLocker lock(&mutex_);
    path_ = path; ++epoch_; entries_.clear();
}
int ChunkCache::chunkCount() const { QMutexLocker lock(&mutex_); return entries_.size(); }
qint64 ChunkCache::retainedBytes() const {
    QMutexLocker lock(&mutex_); qint64 bytes=0;
    for (const auto &entry:entries_) bytes+=entry.bytes.size();
    return bytes;
}
bool ChunkCache::contains(qint64 offset) const {
    QMutexLocker lock(&mutex_);
    for (const auto &entry:entries_) if(entry.offset==offset)return true;
    return false;
}
ReadResult ChunkCache::read(const ReadRequest &request) {
    ReadResult result;result.request=request;
    if(request.length<0||request.length>ChunkBytes){result.error="Lectura fuera del límite de 1 MiB.";return result;}
    quint64 epoch;
    {QMutexLocker lock(&mutex_);epoch=epoch_;}
    // Even hits check the source metadata, so ordinary external edits invalidate data.
    auto metadataRequest=request;metadataRequest.length=0;
    auto metadata=backing_(metadataRequest);
    if(!metadata.error.isEmpty()){
        QMutexLocker lock(&mutex_);
        if(epoch==epoch_&&request.path==path_)entries_.clear();
        result.error=metadata.error;return result;
    }
    result.size=metadata.size;result.modified=metadata.modified;
    if(request.offset<0||request.offset>result.size){result.error="Posición fuera del archivo.";return result;}
    const qint64 length=qMin(request.length,result.size-request.offset);
    qint64 position=request.offset;
    while(position-request.offset<length){
        const qint64 block=(position/ChunkBytes)*ChunkBytes;
        QByteArray bytes;
        {
            QMutexLocker lock(&mutex_);
            if(epoch==epoch_&&request.path==path_){
                for(qsizetype i=entries_.size();i>0;--i)
                    if(entries_[i-1].size!=result.size||entries_[i-1].modified!=result.modified)entries_.removeAt(i-1);
                for(qsizetype i=0;i<entries_.size();++i){
                    if(entries_[i].offset==block){auto entry=entries_.takeAt(i);bytes=entry.bytes;entries_.prepend(std::move(entry));break;}
                }
            }
        }
        if(bytes.isEmpty()){
            auto loaded=backing_({request.path,block,result.size,result.modified,ChunkBytes});
            if(!loaded.error.isEmpty()){result.bytes.clear();result.error=loaded.error;return result;}
            bytes=std::move(loaded.bytes);
            QMutexLocker lock(&mutex_);
            if(epoch==epoch_&&request.path==path_){
                for(qsizetype i=entries_.size();i>0;--i)if(entries_[i-1].offset==block)entries_.removeAt(i-1);
                // Evict BEFORE inserting to keep retained entries at or below 20 MiB.
                while(entries_.size()>=MaxChunks)entries_.removeLast();
                entries_.prepend({block,result.size,result.modified,bytes});
            }
        }
        const auto start=position-block;
        const auto count=qMin(length-(position-request.offset),qint64(bytes.size())-start);
        if(count<=0){result.bytes.clear();result.error="Bloque incompleto; vuelve a abrir el archivo.";return result;}
        result.bytes.append(bytes.constData()+start,count);position+=count;
    }
    metadataRequest.expectedSize=result.size;metadataRequest.expectedModified=result.modified;
    const auto finalCheck=backing_(metadataRequest);
    if(!finalCheck.error.isEmpty()){
        result.bytes.clear();result.error=finalCheck.error;
        QMutexLocker lock(&mutex_);if(epoch==epoch_&&request.path==path_)entries_.clear();
    }
    return result;
}
