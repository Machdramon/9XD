#pragma once
#include <QByteArray>
#include <QString>
#include <QVector>

struct ByteRange { qint64 offset; qint64 length; };
struct ByteSelection {
    qint64 anchor = -1;
    qint64 cursor = -1;
    int stride = 16;
    bool rectangular = false;
    bool contains(qint64 offset) const;
    qint64 count(qint64 fileSize) const;
    // Bounded preview/copy: refuse ranges whose total output exceeds limit.
    QVector<ByteRange> ranges(qint64 fileSize, qint64 limit) const;
};
QString decodeText(const QByteArray &bytes, const QString &encoding);
QString decodeNumber(const QByteArray &bytes, const QString &type, bool bigEndian);
int numberWidth(const QString &type);
QString calculateInteger(const QString &left, const QString &operation, const QString &right);
