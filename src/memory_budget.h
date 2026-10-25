#pragma once
#include <QtGlobal>
#include <atomic>
#include <algorithm>
#include <limits>
// A shared estimate for asset operations, not an OS allocation/reservation limit.
// Atomics keep worker threads independent of QWidget/QSettings access.
namespace AssetMemory {
inline constexpr qint64 GiB=1024LL*1024*1024;
inline std::atomic<int> configuredGiB{8};
inline void setGiB(int value){configuredGiB.store(std::clamp(value,1,64));}
inline int gib(){return configuredGiB.load();}
inline qint64 bytes(){return qint64(gib())*GiB;}
// Separate retained image/texture allowance, derived from the user asset budget.
// A quarter of the configured asset budget is available for shared decoded images.
inline qint64 textureBytes(){return bytes()/4;}
inline qint64 vertices(){return std::min(bytes()/96,qint64(std::numeric_limits<int>::max()-1));}
inline qint64 indices(){return std::min(bytes()/32,qint64(std::numeric_limits<int>::max()-1));}
inline qint64 estimate(qint64 vertices,qint64 indices){
    if(vertices<0||indices<0||vertices>std::numeric_limits<int>::max()-1||indices>std::numeric_limits<int>::max()-1)return std::numeric_limits<qint64>::max();
    return vertices*96+indices*32;
}
inline bool fits(qint64 vertices,qint64 indices,qint64 input=0){auto limit=bytes();return input>=0&&input<=limit&&estimate(vertices,indices)<=limit-input;}
}
