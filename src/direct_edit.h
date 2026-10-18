#pragma once
#include "binary_document.h"
#include "interpretation.h"
struct DirectEditSpec {
    QVector<ByteRange> ranges;
    PatchList patches;
    QString character,type,encoding;
    bool hex=true,broadcast=false,bigEndian=false;
    int digit=0;
    QMap<qint64,QString> drafts;
};
ReadResult directEdit(const ReadRequest &,const DirectEditSpec &);

QByteArray encodeInputCharacter(const QString &,const QString &);
