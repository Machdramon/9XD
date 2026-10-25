#pragma once
#include "image_recipe.h"
#include <QByteArray>
#include <QtEndian>
#include <atomic>
// Genki transfer-form TIM2 is distinct from ordinary linear PictFormat=0.
// Admit only observed single-level indexed layouts whose TEX0 and sizes agree.
inline int tim2TransferBits(const QByteArray &b,qint64 at){
    if(at<0||at>b.size()-48)return 0;const auto *p=reinterpret_cast<const uchar *>(b.constData()+at);
    auto h=[&](int n){return qFromLittleEndian<quint16>(p+n);};auto u=[&](int n){return qFromLittleEndian<quint32>(p+n);};auto tex=qFromLittleEndian<quint64>(p+24);
    int bits=p[16]==0x10&&p[19]==4?4:p[16]==0x30&&p[19]==5?8:0,w=h(20),height=h(22);
    if(!bits||p[17]!=1||p[18]!=3||h(12)!=128||!imageValidSize(w,height)||(w!=32&&w!=64)||(height!=32&&height!=64&&height!=128)||((tex>>14)&63)!=2||((tex>>20)&63)!=(bits==4?20:19)||(1<<((tex>>26)&15))!=w||(1<<((tex>>30)&15))!=height||h(14)!=(bits==4?16:256)||u(8)!=quint32(w*height*bits/8)||u(4)!=(bits==4?128u:1024u)||u(0)<128+u(8)+u(4)||u(0)>b.size()-at)return 0;
    return bits==4?16:32;
}
inline QByteArray tim2TransferLinear(const QByteArray &raw,int w,int h,int bits,int transfer,const std::atomic_bool &cancel){
    if(!imageValidSize(w,h)||raw.size()!=qint64(w)*h*bits/8||(transfer!=16&&transfer!=32))return {};
    QByteArray memory(1024*1024,0),written(memory.size(),0),linear(raw.size(),0);int tw=w/2,th=h*bits*2/transfer,step=transfer/8;
    for(int y=0;y<th;++y){if(cancel.load())return {};for(int x=0;x<tw;++x){auto a=imageGsBitAddress(x,y,tw,transfer)/8,source=qint64(y*tw+x)*step;if(a<0||a+step>memory.size()||source+step>raw.size())return {};for(int k=0;k<step;++k){memory[a+k]=raw[source+k];written[a+k]=1;}}}
    for(int y=0;y<h;++y){if(cancel.load())return {};for(int x=0;x<w;++x){auto a=imageGsBitAddress(x,y,128,bits);if(a<0||a/8>=memory.size()||!written[a/8])return {};int value=(quint8(memory[a/8])>>(a%8))&((1<<bits)-1);auto dest=qint64(y*w+x)*bits;linear[dest/8]=char(quint8(linear[dest/8])|(value<<(dest%8)));}}
    return linear;
}
