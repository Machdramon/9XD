#pragma once
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QtEndian>
#include <atomic>
// Bounded TXRD2 picture records. Alias slots are physical 48-byte records even
// when they declare a 128-byte header; never read pixels from an empty slot.
namespace GenkiTextureBank {
inline QJsonObject headerless(const QByteArray &b,qint64 start,qint64 end,const std::atomic_bool *cancel=nullptr){
    if(start<0||end> b.size()||end-start<64)return {};qint64 p=start;QJsonArray images;
    while(end-p>16){
        if(cancel&&cancel->load())return {};if(end-p<48||images.size()>=4096)return {};
        const auto *v=reinterpret_cast<const uchar *>(b.constData()+p);auto u=[&](int n){return qFromLittleEndian<quint32>(v+n);};auto h=[&](int n){return qFromLittleEndian<quint16>(v+n);};
        qint64 total=u(0),palette=u(4),pixels=u(8);int hs=h(12),w=h(20),height=h(22);auto tex=qFromLittleEndian<quint64>(v+24);
        if(total<48||total>end-p||v[16]||v[17]!=1||v[18]!=3||v[19]!=5||h(14)!=256||w<1||height<1||qint64(w)*height>4194304||w>2048||height>2048||((tex>>20)&63)!=19||(1<<((tex>>26)&15))!=w||(1<<((tex>>30)&15))!=height)return {};
        bool alias=total==48&&pixels==0&&palette==0;
        if(alias){if(hs!=48&&hs!=128)return {};}
        else if(hs!=128||pixels!=qint64(w)*height||palette!=1024||total!=hs+pixels+palette)return {};
        images.append(QJsonObject{{"index",images.size()},{"width",w},{"height",height},{"storageWidth",w},{"storageHeight",height},{"pictureOffset",QString::number(p)},{"recordBytes",QString::number(total)},{"referenceOnly",alias},{"decodeSupported",!alias},{"pictureFormat",0}});p+=total;
    }
    if(images.isEmpty()||end-p!=16)return {};for(;p<end;++p)if(b[p])return {};
    return {{"valid",true},{"headerless",true},{"referenceNamespace",2},{"offset",QString::number(start)},{"end",QString::number(end)},{"images",images}};
}
inline QByteArray pictureTim2(const QByteArray &b,const QJsonObject &bank,int index,QString &error){
    const auto images=bank["images"].toArray();if(!bank["valid"].toBool()||!bank["headerless"].toBool()||index<0||index>=images.size()){error="Invalid Genki image bank or slot.";return {};}
    auto image=images[index].toObject();if(image["referenceOnly"].toBool()){error="Genki image slot is an unresolved external reference; it contains no pixels.";return {};}
    auto p=image["pictureOffset"].toString().toLongLong(),size=image["recordBytes"].toString().toLongLong();if(p<0||size<48||p>b.size()-size){error="Genki image record is outside the source.";return {};}
    // Adapt one already validated record to the common TIM2 decoder in memory.
    QByteArray bytes(16,0);bytes.replace(qsizetype(0),qsizetype(4),"TIM2",qsizetype(4));bytes[4]=4;qToLittleEndian<quint16>(1,bytes.data()+6);bytes+=b.mid(p,size);return bytes;
}
}
