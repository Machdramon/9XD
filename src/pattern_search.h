#pragma once
#include "binary_document.h"
#include <atomic>
struct PatternSpec {
    QString expression;
    bool regex=false, text=false;
    QString encoding="UTF-8";
    qint64 begin=0, length=0;
    PatchList patches;
};
struct PatternState {
    PatternSpec spec;
    std::atomic<bool> cancel{false};
    std::atomic<qint64> progress{0};
};
// Hex masks stream through the requested range. Regex operates on a bounded range.
ReadResult searchPattern(const ReadRequest &source,const PatternSpec &spec,
                         std::atomic<bool> &cancel,std::atomic<qint64> &progress);
