#pragma once
#include "binary_document.h"
#include "interpretation.h"
#include <QJsonDocument>
#include <atomic>
struct MagicSignature {
    QString name, pattern, notes;
    qint64 relativeOffset=0;
    bool enabled=true;
};
struct MagicSpec {
    QList<MagicSignature> signatures;
    // With anchored=true, test only these region starts; otherwise scan the file.
    QVector<ByteRange> regions;
    bool anchored=false;
    PatchList patches;
};
struct MagicState {
    MagicSpec spec;
    std::atomic<bool> cancel{false};
    std::atomic<qint64> progress{0};
};
bool validateMagic(const MagicSignature &signature,QString &error);
QJsonDocument encodeMagicLibrary(const QList<MagicSignature> &signatures);
bool decodeMagicLibrary(const QByteArray &json,QList<MagicSignature> &signatures,QString &error);
QList<MagicSignature> defaultMagicLibrary();
ReadResult scanMagic(const ReadRequest &source,const MagicSpec &spec,std::atomic<bool> &cancel,std::atomic<qint64> &progress);
