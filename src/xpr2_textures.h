#pragma once
// Xbox 360 XPR2 texture packages: header {"XPR2", header size, data size, count}, then 16-byte entries
// {type, offset, size, name offset} relative to byte 12. TX2D entries hold a D3D resource whose GPU texture
// fetch constant (at +28) gives format, endianness, tiling, base address and size. Data starts at 12 + header
// size. Tiled surfaces use the Xenos 2D tiling already implemented by texture_decode.h.
#include "texture_decode.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QtEndian>

namespace Xpr2 {
struct Entry { QString name; int format=0,endian=0,width=0,height=0,pitch=0; bool tiled=false; qint64 data=0; };
inline QString formatName(int f){return f==0x12?"BC1":f==0x13?"BC2":f==0x14?"BC3":f==0x31?"BC5":f==0x3B?"BC4":f==0x06?"ARGB8":f==0x04?"RGB565":f==0x02?"Gray8":QString();}
// Entries are positions relative to the start of the XPR2 package.
inline QList<Entry> entries(const QByteArray &bytes,QString *error=nullptr){
    QList<Entry> out;auto fail=[&](QString e){if(error)*error=e;return QList<Entry>();};
    if(bytes.size()<16||!bytes.startsWith("XPR2"))return fail("Not an XPR2 package.");
    const auto *p=reinterpret_cast<const uchar *>(bytes.constData());auto u=[&](qint64 at){return qFromBigEndian<quint32>(p+at);};
    const qint64 header=u(4),count=u(12);if(count<1||count>4096||16+count*16>bytes.size()||header<16+count*16||12+header>bytes.size())return fail("Invalid XPR2 directory.");
    for(qint64 i=0;i<count;++i){const qint64 e=16+16*i;Entry x;
        if(qstrncmp(bytes.constData()+e,"TX2D",4)){out.append(x);continue;}
        const qint64 resource=12+u(e+4),name=12+u(e+12);if(resource+52>bytes.size())return fail("XPR2 texture header is truncated.");
        for(qint64 k=name;k<bytes.size()&&k<name+128&&p[k];++k)x.name+=QChar(p[k]);
        const quint32 w0=u(resource+28),w1=u(resource+32),w2=u(resource+36);
        x.tiled=w0>>31;x.pitch=int((w0>>22)&0x1FF)*32;x.format=int(w1&63);x.endian=int((w1>>6)&3);x.data=12+header+qint64(w1>>12)*4096;x.width=int(w2&0x1FFF)+1;x.height=int((w2>>13)&0x1FFF)+1;
        out.append(x);}
    return out;
}
inline QImage decode(const QByteArray &bytes,const ImageRecipe &selection,QString &error,const std::atomic_bool &cancel){
    auto fail=[&](QString e){error=e;return QImage();};auto list=entries(bytes,&error);if(list.isEmpty())return {};
    if(selection.surface<0||selection.surface>=list.size())return fail("XPR2 texture index is out of range.");
    if(selection.mipLevel)return fail("Only the top XPR2 mip level is supported.");
    const auto &t=list[selection.surface];ImageRecipe r;r.format=formatName(t.format);if(r.format.isEmpty())return fail(QString("Unsupported Xbox 360 texture format 0x%1.").arg(t.format,0,16));
    r.width=t.width;r.height=t.height;r.layout=t.tiled?"Xbox 360 tiled":"Linear";r.byteSwap=t.endian==1?"Swap 8 in 16":"None";
    if(!t.tiled&&t.pitch){const bool block=imageBlockCompressed(r.format);r.rowStride=block?qint64(t.pitch/4)*imageBlockBytes(r.format):qint64(t.pitch)*imagePixelBits(r.format)/8;}
    QString e;const auto span=imageSpan(r,e);if(!e.isEmpty())return fail(e);if(t.data<0||t.data>bytes.size()-span)return fail("XPR2 texture data is truncated.");
    return decodeTextureData(bytes.mid(t.data,span),{},r,error,cancel);
}
// Image candidates for every XPR2 package found at the given offsets.
inline QJsonArray images(const QByteArray &bytes,const QList<qint64> &packages,int limit){
    QJsonArray out;
    for(auto at:packages){const auto list=entries(QByteArray::fromRawData(bytes.constData()+at,bytes.size()-at));
        for(int i=0;i<list.size()&&out.size()<limit;++i){const auto &t=list[i];if(formatName(t.format).isEmpty())continue;
            out.append(QJsonObject{{"name",QString("XPR2 / %1").arg(t.name.isEmpty()?QString("texture %1").arg(i):t.name)},{"basis",QString("Xbox 360 XPR2 TX2D (%1%2), decoded with Xenos untiling.").arg(formatName(t.format)).arg(t.tiled?", tiled":"")},{"width",t.width},{"height",t.height},
                {"settings",QJsonObject{{"offset",QString::number(at)},{"format","Encoded image"},{"container","Auto"},{"surface",i},{"width",1},{"height",1}}}});}}
    return out;
}
inline QList<qint64> find(const QByteArray &bytes,int limit=8){QList<qint64> out;for(qsizetype at=bytes.indexOf("XPR2");at>=0&&out.size()<limit;at=bytes.indexOf("XPR2",at+4))if(!entries(QByteArray::fromRawData(bytes.constData()+at,bytes.size()-at)).isEmpty())out<<at;return out;}
}
