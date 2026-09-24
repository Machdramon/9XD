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
        error = "El historial alcanza su límite de 20 MiB, 1024 operaciones o 16384 regiones. Guarda antes de seguir editando.";
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
    QFile input(source.path);
    if (!QFileInfo(input).isFile() || !input.open(QIODevice::ReadOnly))
        return fail("No se pudo abrir el archivo original: " + input.errorString());
    auto unchanged = [&] {
        return input.size() == source.expectedSize &&
            input.fileTime(QFileDevice::FileModificationTime) == source.expectedModified &&
            QFileInfo(source.path).size() == source.expectedSize &&
            QFileInfo(source.path).lastModified() == source.expectedModified;
    };
    if (!unchanged()) return fail("El archivo original ha cambiado. No se ha guardado; los cambios siguen pendientes.");
    if (QFileInfo(destination).isSymLink())
        return fail("El destino es un enlace simbólico. Usa Guardar como con una ruta de archivo normal.");
    for (const auto &patch : patches)
        if (patch.offset < 0 || patch.offset > source.expectedSize || patch.bytes.size() > source.expectedSize - patch.offset)
            return fail("Una modificación está fuera del archivo.");
    QSaveFile output(destination);
    output.setDirectWriteFallback(false);
    if (!output.open(QIODevice::WriteOnly)) return fail("No se pudo preparar el guardado: " + output.errorString());
    if (!output.setPermissions(input.permissions()))
        return fail("No se pudieron conservar los permisos del archivo.");
    qint64 offset = 0;
    while (offset < source.expectedSize) {
        const auto length = qMin(qint64(1024 * 1024), source.expectedSize - offset);
        auto bytes = input.read(length);
        if (bytes.size() != length) return fail("No se pudo leer el original completo. No se ha reemplazado el destino.");
        applyPatches(bytes, offset, patches);
        if (output.write(bytes) != bytes.size()) return fail("Error al escribir: " + output.errorString());
        offset += length;
        if (progress) progress->store(offset, std::memory_order_relaxed);
    }
    if (!unchanged()) return fail("El original cambió durante el guardado. No se ha reemplazado el destino.");
    input.close(); // Allows replacement of the source on Windows.
    if (!output.commit()) return fail("No se pudo finalizar el guardado: " + output.errorString());
    const QFileInfo saved(destination);
    result.size = saved.size(); result.modified = saved.lastModified();
    return result;
}
