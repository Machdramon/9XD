#pragma once
#include "binary_reader.h"
#include <QList>
#include <atomic>

struct BytePatch { qint64 offset; QByteArray bytes; };
using PatchList = QList<BytePatch>;
void applyPatches(QByteArray &bytes, qint64 offset, const PatchList &patches);

// Stores only replacement bytes. Undo removes a layer; the cached source stays immutable.
class EditHistory {
public:
    static constexpr qint64 MaxBytes = 20 * 1024 * 1024;
    bool push(PatchList patches, QString &error);
    void undo() { if (cursor_ > 0) --cursor_; }
    void redo() { if (cursor_ < commands_.size()) ++cursor_; }
    bool canUndo() const { return cursor_ > 0; }
    bool canRedo() const { return cursor_ < commands_.size(); }
    PatchList patches() const;
    void clear() { commands_.clear(); cursor_ = 0; }
private:
    QList<PatchList> commands_;
    qsizetype cursor_ = 0;
};

struct SaveState {
    QString destination;
    PatchList patches;
    std::atomic<qint64> progress{0};
};
ReadResult savePatchedFile(const ReadRequest &source, const QString &destination,
                          const PatchList &patches, std::atomic<qint64> *progress = nullptr);
