#include "binary_document.h"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <cstring>

void applyPatches(QByteArray &bytes, qint64 offset, const PatchList &patches) {
    for (const auto &patch : patches) {
        const auto begin = qMax(offset, patch.offset);
        const auto end = qMin(offset + bytes.size(), patch.offset + patch.bytes.size());
        if (end > begin)
            std::memcpy(bytes.data() + (begin - offset), patch.bytes.constData() + (begin - patch.offset), size_t(end - begin));
    }
}
bool EditHistory::push(PatchList patches, QString &error) {
    qint64 total = 0; qsizetype regions = patches.size();
    for (qsizetype i = 0; i < cursor_; ++i) {
        regions += commands_[i].size();
        for (const auto &p : commands_[i]) total += p.bytes.size();
    }
    for (const auto &p : patches) total += p.bytes.size();
    if (total > MaxBytes || cursor_ >= 1024 || regions > 16384) {
        error = "History has reached its limit of 20 MiB, 1024 operations or 16384 regions. Save before continuing to edit.";
        return false;
    }
    while (commands_.size() > cursor_) commands_.removeLast();
    commands_.append(std::move(patches)); ++cursor_;
    return true;
}
PatchList EditHistory::patches() const {
    PatchList result;
    for (qsizetype i = 0; i < cursor_; ++i) result.append(commands_[i]);
    return result;
}
ReadResult savePatchedFile(const ReadRequest &source, const QString &destination,
                          const PatchList &patches, std::atomic<qint64> *progress) {
    ReadResult result; result.request = source;
    auto fail = [&](const QString &error) { result.error = error; return result; };
    if(source.memory) {
        if(QFileInfo(destination).isSymLink())return fail("Use Save As with a regular file path.");
        QSaveFile output(destination);output.setDirectWriteFallback(false);
        if(!output.open(QIODevice::WriteOnly))return fail(output.errorString());
        qint64 done=0;
        for(const auto &block:source.memory->blocks) {
            if(output.write(block)!=block.size())return fail(output.errorString());
            done+=block.size();if(progress)progress->store(done,std::memory_order_relaxed);
        }
        if(!output.commit())return fail(output.errorString());
        result.size=done;result.modified=QFileInfo(destination).lastModified();return result;
    }
    QFile input(source.path);
    if (!QFileInfo(input).isFile() || !input.open(QIODevice::ReadOnly))
        return fail("Could not open the original file: " + input.errorString());
    auto unchanged = [&] {
        return input.size() == source.expectedSize &&
            input.fileTime(QFileDevice::FileModificationTime) == source.expectedModified &&
            QFileInfo(source.path).size() == source.expectedSize &&
            QFileInfo(source.path).lastModified() == source.expectedModified;
    };
    if (!unchanged()) return fail("The original file has changed. Nothing was saved; your changes are still pending.");
    if (QFileInfo(destination).isSymLink())
        return fail("The destination is a symbolic link. Use Save As with a regular file path.");
    for (const auto &patch : patches)
        if (patch.offset < 0 || patch.offset > source.expectedSize || patch.bytes.size() > source.expectedSize - patch.offset)
            return fail("An edit is outside the file.");
    QSaveFile output(destination);
    output.setDirectWriteFallback(false);
    if (!output.open(QIODevice::WriteOnly)) return fail("Could not prepare the save: " + output.errorString());
    if (!output.setPermissions(input.permissions()))
        return fail("Could not preserve file permissions.");
    qint64 offset = 0;
    while (offset < source.expectedSize) {
        const auto length = qMin(qint64(1024 * 1024), source.expectedSize - offset);
        auto bytes = input.read(length);
        if (bytes.size() != length) return fail("Could not read the entire original file. The destination was not replaced.");
        applyPatches(bytes, offset, patches);
        if (output.write(bytes) != bytes.size()) return fail("Write error: " + output.errorString());
        offset += length;
        if (progress) progress->store(offset, std::memory_order_relaxed);
    }
    if (!unchanged()) return fail("The original file changed while saving. The destination was not replaced.");
    input.close(); // Allows replacement of the source on Windows.
    if (!output.commit()) return fail("Could not complete the save: " + output.errorString());
    const QFileInfo saved(destination);
    result.size = saved.size(); result.modified = saved.lastModified();
    return result;
}
