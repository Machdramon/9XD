#pragma once
#include "binary_document.h"
#include "interpretation.h"
#include <atomic>
struct CompareResult {
    QVector<ByteRange> ranges;
    qint64 differentBytes=0,processed=0,total=0;
    QString error;
    bool cancelled=false,truncated=false;
};
struct CompareState {std::atomic<bool> cancel{false};std::atomic<qint64> progress{0};};
// Positional comparison: insertions/deletions are not realigned.
inline CompareResult compareDocuments(ReadRequest left,ReadRequest right,PatchList patches,CompareState &state){
    CompareResult result;result.total=qMax(left.expectedSize,right.expectedSize);
    if(left.expectedSize<0||right.expectedSize<0){result.error="Open both sources before comparing.";return result;}
    auto read=[](ReadRequest source,qint64 start,qint64 count){source.offset=qMin(start,source.expectedSize);source.length=qMin(count,qMax(qint64(0),source.expectedSize-start));return readPage(source);};
    bool inRange=false;
    for(qint64 at=0;at<result.total;){
        if(state.cancel.load()){result.cancelled=true;result.ranges.clear();return result;}
        const qint64 count=qMin(qint64(1024*1024),result.total-at);
        auto a=read(left,at,count),b=read(right,at,count);
        if(!a.error.isEmpty()||!b.error.isEmpty()){result.error=!a.error.isEmpty()?a.error:b.error;result.ranges.clear();return result;}
        applyPatches(a.bytes,at,patches);
        for(qint64 i=0;i<count;++i){
            if((i&4095)==0&&state.cancel.load()){result.cancelled=true;result.ranges.clear();return result;}
            bool differs=i>=a.bytes.size()||i>=b.bytes.size()||a.bytes[i]!=b.bytes[i];
            if(differs){
                if(!inRange){if(result.ranges.size()==10000){result.truncated=true;result.processed=at+i;return result;}result.ranges.append({at+i,0});}
                ++result.ranges.last().length;++result.differentBytes;
            }
            inRange=differs;
        }
        at+=count;result.processed=at;state.progress.store(at);
    }
    auto a=read(left,0,0),b=read(right,0,0);
    if(!a.error.isEmpty()||!b.error.isEmpty()){result.error=!a.error.isEmpty()?a.error:b.error;result.ranges.clear();}
    return result;
}
