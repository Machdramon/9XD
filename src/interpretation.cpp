#include "interpretation.h"
#include <QStringDecoder>
#include <QtEndian>
#include <bit>
#include <limits>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#else
#include <iconv.h>
#endif

bool ByteSelection::contains(qint64 offset) const {
    if (anchor < 0 || cursor < 0 || offset < 0) return false;
    if (!rectangular) return offset >= qMin(anchor,cursor) && offset <= qMax(anchor,cursor);
    return offset / stride >= qMin(anchor/stride,cursor/stride) &&
        offset / stride <= qMax(anchor/stride,cursor/stride) &&
        offset % stride >= qMin(anchor%stride,cursor%stride) &&
        offset % stride <= qMax(anchor%stride,cursor%stride);
}
qint64 ByteSelection::count(qint64 size) const {
    if (anchor < 0 || cursor < 0 || size <= 0 || anchor >= size || cursor >= size) return 0;
    if (!rectangular) return qMax(anchor,cursor)-qMin(anchor,cursor)+1;
    const auto rows = qMax(anchor/stride,cursor/stride)-qMin(anchor/stride,cursor/stride)+1;
    const auto left = qMin(anchor%stride,cursor%stride);
    const auto right = qMax(anchor%stride,cursor%stride);
    const qint64 lastRow = qMax(anchor/stride,cursor/stride);
    const qint64 available = qMin(qint64(stride), size-lastRow*stride);
    return (rows-1)*(right-left+1) + qMax(qint64(0),qMin(right+1,available)-left);
}
QVector<ByteRange> ByteSelection::ranges(qint64 size, qint64 limit) const {
    QVector<ByteRange> result;
    const auto n = count(size);
    if (n <= 0 || n > limit) return result;
    if (!rectangular) return {{qMin(anchor,cursor), n}};
    const auto left = qMin(anchor%stride,cursor%stride);
    const auto right = qMax(anchor%stride,cursor%stride);
    for (qint64 row=qMin(anchor/stride,cursor/stride); row<=qMax(anchor/stride,cursor/stride); ++row) {
        const auto offset = row*stride+left;
        if (offset < size) result.append({offset,qMin(right-left+1,size-offset)});
    }
    return result;
}
QString decodeText(const QByteArray &bytes, const QString &encoding) {
    QString text;
    if (encoding == "ASCII") {
        for (unsigned char c : bytes) text += c >= 32 && c <= 126 ? QChar(c) : QChar('.');
        return text;
    }
    if (encoding == "CP932") {
#ifdef Q_OS_WIN
        const int length = MultiByteToWideChar(932, MB_ERR_INVALID_CHARS, bytes.constData(), int(bytes.size()), nullptr, 0);
        if (!length && !bytes.isEmpty()) return "[secuencia CP932 incompleta o no válida]";
        std::wstring output(length, L'\0');
        MultiByteToWideChar(932, MB_ERR_INVALID_CHARS, bytes.constData(), int(bytes.size()), output.data(), length);
        text = QString::fromStdWString(output);
#else
        iconv_t converter = iconv_open("UTF-8", "CP932");
        if (converter == (iconv_t)-1) return "[CP932 no disponible]";
        QByteArray output(bytes.size()*4+8, '\0');
        char *input = const_cast<char *>(bytes.constData()); char *out = output.data();
        size_t remaining = size_t(bytes.size()), capacity = size_t(output.size());
        const bool invalid = iconv(converter, &input, &remaining, &out, &capacity) == size_t(-1);
        iconv_close(converter);
        text = QString::fromUtf8(output.constData(), output.size()-qsizetype(capacity));
        if (invalid) text += " [secuencia incompleta o no válida]";
#endif
    } else {
        QStringDecoder decoder(encoding.toLatin1().constData(), QStringConverter::Flag::Stateless);
        if (!decoder.isValid()) return "[codificación no disponible]";
        text = decoder(bytes);
        if (decoder.hasError()) text += " [secuencia incompleta o no válida]";
    }
    text.replace(QChar(0), QStringLiteral("␀"));
    text.replace('\n', QStringLiteral("↵")); text.replace('\r', QStringLiteral("␍")); text.replace('\t', QStringLiteral("⇥"));
    return text;
}
int numberWidth(const QString &type) {
    if (type.endsWith("64")) return 8;
    if (type.endsWith("32")) return 4;
    if (type.endsWith("16")) return 2;
    return 1;
}
QString decodeNumber(const QByteArray &bytes, const QString &type, bool bigEndian) {
    const int width=numberWidth(type);
    if (bytes.size()<width) return QString("Faltan %1 bytes").arg(width-bytes.size());
    quint64 value=0;
    for (int i=0;i<width;++i) {
        const auto byte=quint8(bytes[i]);
        value |= quint64(byte) << (8*(bigEndian ? width-1-i : i));
    }
    if (type=="Float32") return QString::number(std::bit_cast<float>(quint32(value)), 'g', 9);
    if (type=="Float64") return QString::number(std::bit_cast<double>(value), 'g', 17);
    if (type.startsWith("Int")) {
        switch (width) {
        case 1: return QString::number(std::bit_cast<qint8>(quint8(value)));
        case 2: return QString::number(std::bit_cast<qint16>(quint16(value)));
        case 4: return QString::number(std::bit_cast<qint32>(quint32(value)));
        default: return QString::number(std::bit_cast<qint64>(value));
        }
    }
    return QString::number(value);
}
QString calculateInteger(const QString &left, const QString &operation, const QString &right) {
    auto parse=[](QString text, bool *ok) {
        text=text.trimmed();
        if (text.startsWith("0x",Qt::CaseInsensitive)) return text.mid(2).toULongLong(ok,16);
        return text.toULongLong(ok,10);
    };
    bool aOk=false,bOk=false;
    const quint64 a=parse(left,&aOk),b=parse(right,&bOk);
    if (!aOk || !bOk || left.trimmed().startsWith('-') || right.trimmed().startsWith('-')) return "Usa enteros sin signo de 64 bits, decimales o con 0x.";
    quint64 r=0; const auto max=std::numeric_limits<quint64>::max();
    if (operation=="+") { if (a>max-b) return "Desbordamiento"; r=a+b; }
    else if (operation=="−") { if (b>a) return "El resultado sería negativo"; r=a-b; }
    else if (operation=="×") { if (b && a>max/b) return "Desbordamiento"; r=a*b; }
    else if (operation=="÷") { if (!b) return "División por cero"; r=a/b; }
    else if (operation=="%") { if (!b) return "División por cero"; r=a%b; }
    else if (operation=="AND") r=a&b;
    else if (operation=="OR") r=a|b;
    else if (operation=="XOR") r=a^b;
    else return "Operación desconocida";
    return QString("Decimal: %1\nHexadecimal: 0x%2\nBinario: %3").arg(r).arg(r,0,16).arg(QString::number(r,2));
}
