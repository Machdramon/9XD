#pragma once
#include "binary_reader.h"
#include <QMutex>
#include <QList>

// Shared by the viewport, inspector and copy workers. No filesystem I/O under mutex.
// The retained cache is bounded; callers may briefly retain evicted blocks by value.
class ChunkCache {
public:
    static constexpr qint64 ChunkBytes = 1024 * 1024;
    static constexpr int MaxChunks = 20;
    using ReadFunction = BinaryReader::ReadFunction;
    explicit ChunkCache(ReadFunction backing = readPage);
    ~ChunkCache();
    void reset(const QString &path);
    ReadResult read(const ReadRequest &request);
    int chunkCount() const;
    qint64 retainedBytes() const;
    bool contains(qint64 blockOffset) const;
private:
    struct Entry { qint64 offset; qint64 size; QDateTime modified; QByteArray bytes; };
    mutable QMutex mutex_;
    QList<Entry> entries_;
    QString path_;
    quint64 epoch_ = 0;
    ReadFunction backing_;
};
