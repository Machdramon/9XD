#pragma once
#include "memory_budget.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QtEndian>
#include <atomic>
#include <cmath>
#include <cstring>
#include <algorithm>

// Observed R-Tuned geometry.bin with pointer tables at byte 8 or 32.
// The second header word is the table offset, not a format version.
// Object placement/materials live in object.bin and are deliberately not inferred here.
namespace RTunedGeometry {
inline QJsonObject scan(const QByteArray &b,const std::atomic_bool &cancel){
    QJsonArray candidates,notes;
    auto range=[&](qint64 p,qint64 n){return p>=0&&n>=0&&p<=b.size()&&n<=b.size()-p;};
    auto u=[&](qint64 p){return qFromLittleEndian<quint32>(b.constData()+p);};
    if(b.size()<32||(u(4)!=8&&u(4)!=32))return {};
    qint64 count=u(0),table=u(4),tableEnd=table+count*4;
    if(count<1||count>4096||!range(table,count*4))return {};
    // Avoid interpreting generic count/version headers as this family.
    if(u(table)<tableEnd||u(table)%16||!range(u(table),128))return {};
    for(qint64 p=8;p<table;++p)if(b[p])return {};
    qint64 previous=tableEnd;
    for(int i=0;i<count;++i){
        if(cancel.load())return {};
        qint64 h=u(table+i*4),end=i+1<count?u(table+4+i*4):b.size();
        if(h<previous||h%16||!range(h,128)||end<h+128||end>b.size())return {};
        previous=end;
        auto flags=u(h),declaration=u(h+4);qint64 vertices=u(h+8),pos=u(h+12),normal=u(h+16),uv=u(h+28),indices=u(h+96),index=u(h+100);
        bool hasUv=flags!=3;
        if((flags!=3&&flags!=0x13&&flags!=0x33&&flags!=0x113)||(flags==3?declaration!=18:flags==0x13?declaration!=26:declaration!=34)||u(h+92)!=1||vertices<3||indices<3||!AssetMemory::fits(vertices,indices)||pos<h+128||index<pos+vertices*12||index+indices*2>end||!range(pos,vertices*12)||!range(index,indices*2))return {};
        if(normal<pos+vertices*12||normal+vertices*6>index||(hasUv&&(uv<normal+vertices*6||uv+vertices*8>index))||(!hasUv&&uv))return {};
        bool sane=true;for(qint64 p=0;p<vertices&&sane;++p){if(cancel.load())return {};for(int a=0;a<3;++a){auto bits=u(pos+p*12+a*4);float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f)||std::abs(double(f))>1e8)sane=false;}for(int a=0;hasUv&&a<2;++a){auto bits=u(uv+p*8+a*4);float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f)||std::abs(double(f))>1e8)sane=false;}}
        qint64 run=0,triangles=0;quint16 a=0,c=0;
        for(qint64 p=0;p<indices&&sane;++p){auto v=qFromLittleEndian<quint16>(b.constData()+index+p*2);if(v==65535){run=0;continue;}if(v>=vertices){sane=false;break;}if(run>=2&&a!=c&&a!=v&&c!=v)++triangles;a=c;c=v;++run;}
        if(!sane||!triangles)return {};
        QJsonObject settings{{"vertexOffset",QString::number(pos)},{"vertexCount",vertices},{"vertexStride",12},{"positionType","Float32"},{"positionScale",1},{"indexOffset",QString::number(index)},{"indexCount",indices},{"indexStride",2},{"indexType","UInt16"},{"topology","Triangle strip"},{"uv",hasUv},{"uvOffset",QString::number(uv)},{"uvStride",8},{"uvType","Float32"},{"normals",false}};
        candidates.append(QJsonObject{{"name",QString("R-Tuned geometry %1").arg(i)},{"profileId","native-rtuned-v8"},{"profileName","R-Tuned geometry"},{"sourceFlags",qint64(flags)},{"sourceDeclaration",qint64(declaration)},{"geometryId",i},{"triangleCount",triangles},{"objectOffset",QString::number(h)},{"settings",settings},{"basis","Bounded R-Tuned geometry buffers, finite positions/UVs and validated index ranges. Raw-buffer topology is provisional: independent draw boundaries require object.bin; scan the original archive for assembled course geometry. Packed source normals and materials unresolved."}});
    }
    QList<QJsonObject> sorted;for(auto v:candidates)sorted.append(v.toObject());std::stable_sort(sorted.begin(),sorted.end(),[](const auto &a,const auto &b){return a["settings"].toObject()["vertexCount"].toInteger()>b["settings"].toObject()["vertexCount"].toInteger();});candidates={};for(auto c:sorted)candidates.append(c);
    notes.append("R-Tuned local geometry: object.bin assembly, material bindings and source normals are not imported. Separate pieces may require placement.");
    return {{"candidates",candidates},{"notes",notes}};
}
}
