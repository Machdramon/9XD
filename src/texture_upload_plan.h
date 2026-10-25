#pragma once
#include <QSize>
#include <QVector>
#include <QtGlobal>
// Preserve source images; reduce only the GPU preview when its storage-buffer range cannot hold them.
inline int texturePreviewDivisor(const QVector<QSize> &sizes,quint64 headerBytes,quint64 limit){
    if(headerBytes>limit)return 0;
    for(int divisor=1;divisor<=8192;divisor*=2){
        quint64 used=headerBytes;bool fits=true;
        for(auto size:sizes){if(!size.isValid())continue;quint64 bytes=quint64(qMax(1,size.width()/divisor))*qMax(1,size.height()/divisor)*4;if(bytes>limit-used){fits=false;break;}used+=bytes;}
        if(fits)return divisor;
    }
    return 0;
}
