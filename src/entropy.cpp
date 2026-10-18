#include "entropy.h"
#include <array>
#include <cmath>
ReadResult analyzeEntropy(const ReadRequest &source,const EntropySpec &spec,std::atomic<bool> &cancel,std::atomic<qint64> &progress){
    ReadResult out;out.request=source;
    if(spec.begin<0||spec.length<=0||spec.begin>source.expectedSize||spec.length>source.expectedSize-spec.begin||spec.window<256){out.error="Invalid analysis region or window.";return out;}
    const qint64 window=qMax(spec.window,1+(spec.length-1)/8192);out.analysisWindow=window;
    std::array<qint64,256> counts{};qint64 count=0,start=spec.begin;
    auto flush=[&]{double h=0;for(auto n:counts)if(n){double p=double(n)/count;h-=p*std::log2(p);}qint64 printable=0;for(int c=32;c<=126;++c)printable+=counts[c];out.entropy.append({start,count,h,100.0*counts[0]/count,100.0*printable/count});start+=count;count=0;counts.fill(0);};
    for(qint64 done=0;done<spec.length;){
        if(cancel.load()){out.cancelled=true;out.entropy.clear();return out;}
        auto request=source;request.offset=spec.begin+done;request.length=qMin(qint64(1048576),spec.length-done);auto data=readPage(request);if(!data.error.isEmpty())return data;
        applyPatches(data.bytes,request.offset,spec.patches);
        for(auto c:data.bytes){++counts[quint8(c)];if(++count==window)flush();}
        done+=data.bytes.size();progress.store(done);
    }
    if(count)flush();auto request=source;request.offset=0;request.length=0;auto meta=readPage(request);if(!meta.error.isEmpty())return meta;
    if(cancel.load()){out.cancelled=true;out.entropy.clear();}out.size=meta.size;out.modified=meta.modified;return out;
}
