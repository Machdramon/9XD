#pragma once
#include "binary_document.h"
#include <atomic>
struct EntropySpec { qint64 begin=0,length=0,window=4096;PatchList patches; };
struct EntropyState { EntropySpec spec;std::atomic<bool> cancel{false};std::atomic<qint64> progress{0}; };
ReadResult analyzeEntropy(const ReadRequest &,const EntropySpec &,std::atomic<bool> &,std::atomic<qint64> &);
