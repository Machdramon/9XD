#pragma once
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <QtEndian>
#include <array>
#include <cmath>
#include <cstring>

// Exact 0IVX node hierarchy (layout from the Gato's Lair IVX_rework research, verified on
// every Genki study case): header, node table, then per node an 80-byte record
// (flags, id, translation, Euler rotation, scale). Nodes with geometry flags carry a
// 48-byte part table; each part has a 160-byte descriptor (208 on LDMX00.1), a
// contiguous DMA packet span and a closing DMA RET.
namespace GenkiHierarchy {
struct Part { qint64 descriptor=0,first=0,end=0; int packets=0; };
struct Node {
    qint64 offset=0; quint32 flags=0,id=0;
    std::array<float,3> translation{},rotation{},scale{1,1,1};
    QVector<Part> parts;
    bool geometry()const{return (flags&0xf0)!=0;}
    bool identity()const{return translation==std::array<float,3>{}&&rotation==std::array<float,3>{}&&scale==std::array<float,3>{1,1,1};}
};
struct Model { qint64 base=0,end=0; QVector<Node> nodes; bool valid=false; QString error; };

inline Model walk(const QByteArray &bytes,qint64 base,qint64 limit){
    Model model;model.base=base;limit=qMin<qint64>(limit,bytes.size());
    auto fail=[&](const QString &error){model.valid=false;model.error=error;model.nodes.clear();return model;};
    auto u32=[&](qint64 p){return qFromLittleEndian<quint32>(bytes.constData()+p);};
    auto f32=[&](qint64 p){auto bits=u32(p);float value;std::memcpy(&value,&bits,4);return value;};
    if(base<0||base+48>limit)return fail("Truncated 0IVX header.");
    const bool v1=bytes.mid(base+16,8)=="LDMX00.1";
    const quint32 sections=u32(base+32),nodes=u32(base+36),tableBytes=u32(base+44);
    if(nodes<1||nodes>4096||sections>4096||qint64(tableBytes)>limit-base-48)return fail("0IVX node table counts are out of range.");
    qint64 p=base+48+qint64(tableBytes)+qint64(sections)*48;
    for(quint32 i=0;i<nodes;++i){
        if(p+80>limit)return fail(QString("Node %1 is truncated.").arg(i));
        Node node;node.offset=p;node.flags=u32(p);node.id=u32(p+4);
        for(int k=0;k<3;++k){node.translation[k]=f32(p+32+k*4);node.rotation[k]=f32(p+48+k*4);node.scale[k]=f32(p+64+k*4);}
        for(int k=0;k<3;++k)if(!std::isfinite(node.translation[k])||!std::isfinite(node.rotation[k])||!std::isfinite(node.scale[k])||std::abs(node.translation[k])>1e6f||std::abs(node.rotation[k])>100.f||std::abs(node.scale[k])>1e4f)return fail(QString("Node %1 has an invalid transform.").arg(i));
        p+=80;
        if(node.geometry()){
            if(p+48>limit)return fail(QString("Node %1 part table is truncated.").arg(i));
            const quint32 count=u32(p+12);if(count>1024)return fail(QString("Node %1 declares %2 parts.").arg(i).arg(count));p+=48;
            for(quint32 c=0;c<count;++c){
                Part part;part.descriptor=p;p+=112+(v1?48:0);
                if(p+48>limit)return fail("Part descriptor is truncated.");
                const quint32 packets=u32(p+4),span=u32(p+8);p+=48;part.first=p;
                if(packets<1||packets>4096||qint64(span)*16>limit-p)return fail(QString("Part at 0x%1 has an invalid packet span.").arg(part.descriptor,0,16));
                qint64 q=p;const qint64 end=p+qint64(span)*16;
                for(quint32 r=0;r<packets;++r){
                    if(q+16>end)return fail(QString("Packet %1 of part 0x%2 exceeds its span.").arg(r).arg(part.descriptor,0,16));
                    const quint32 tag=u32(q),kind=(tag>>28)&7,qwc=tag&0xffff;
                    if((kind!=1&&kind!=7)||tag!=((kind<<28)|qwc))return fail(QString("Invalid DMA tag at 0x%1.").arg(q,0,16));
                    q+=16+qint64(qwc)*16;
                }
                if(q!=end)return fail(QString("Part at 0x%1 does not fill its declared span.").arg(part.descriptor,0,16));
                if(q+16>limit||u32(q)!=0x60000000u||u32(q+4)||u32(q+8)||u32(q+12))return fail(QString("Part at 0x%1 is not closed by a DMA RET.").arg(part.descriptor,0,16));
                part.end=q;part.packets=int(packets);node.parts.append(part);p=q+16;
            }
        }
        model.nodes.append(node);
    }
    model.end=p;model.valid=true;return model;
}
// Raw concatenated track resources with a bounded trailing ID/XYZ table use the
// Y-down convention documented by the legacy 0IVX track reader. Do not apply this
// to standalone car/wheel models, directory containers or incomplete reads.
inline bool rawTrackYDown(const QByteArray &bytes){
    qint64 p=0;int models=0;
    while(bytes.mid(p,8)=="0IVX00.1"){
        if(bytes.mid(p+16,16)!="LDMX00.2MRON00.2")return false;
        auto model=walk(bytes,p,bytes.size());if(!model.valid||model.end<=p)return false;
        p=model.end;if(++models>4096)return false;
    }
    if(models<2||p>bytes.size()-4)return false;
    const auto count=qFromLittleEndian<quint32>(bytes.constData()+p);
    if(count<1||count>65536)return false;
    const qint64 end=p+4+qint64(count)*16;
    if(end>bytes.size()||((end+15)&~qint64(15))!=bytes.size())return false;
    for(qint64 at=end;at<bytes.size();++at)if(bytes[at]!=0)return false;
    for(quint32 i=0;i<count;++i)for(int k=0;k<3;++k){auto bits=qFromLittleEndian<quint32>(bytes.constData()+p+8+qint64(i)*16+k*4);float v;std::memcpy(&v,&bits,4);if(!std::isfinite(v)||std::abs(v)>1e7f)return false;}
    return true;
}
// Scale, then Euler rotation X, Y, Z (radians), then translation.
inline std::array<float,3> rotate(const std::array<float,3> &r,std::array<float,3> v){
    auto turn=[&](int a,int b,float angle){if(angle==0.f)return;const float c=std::cos(angle),s=std::sin(angle);const float x=v[a],y=v[b];v[a]=c*x-s*y;v[b]=s*x+c*y;};
    turn(1,2,r[0]);turn(2,0,r[1]);turn(0,1,r[2]);return v;
}
inline std::array<float,3> apply(const std::array<float,3> &t,const std::array<float,3> &r,const std::array<float,3> &s,std::array<float,3> v){
    for(int k=0;k<3;++k)v[k]*=s[k];v=rotate(r,v);for(int k=0;k<3;++k)v[k]+=t[k];return v;
}
// For R*S, inverse-transpose normals are R*(n/S). Singular scales have no
// inverse: return zero so the decoder reconstructs normals from transformed faces.
inline std::array<float,3> transformNormal(const std::array<float,3> &r,const std::array<float,3> &s,std::array<float,3> n){
    for(int k=0;k<3;++k)if(s[k]==0.f)return {};
    std::array<double,3> inv{};double length=0;for(int k=0;k<3;++k){inv[k]=double(n[k])/s[k];length=std::hypot(length,inv[k]);}
    if(!std::isfinite(length)||length==0)return {};
    for(int k=0;k<3;++k)n[k]=float(inv[k]/length);
    n=rotate(r,n);const double norm=std::hypot(double(n[0]),double(n[1]),double(n[2]));
    if(!std::isfinite(norm)||norm==0)return {};for(auto &v:n)v=float(v/norm);return n;
}
inline bool mirrored(const std::array<float,3> &s){return double(s[0])*s[1]*s[2]<0;}
inline QJsonArray json(const std::array<float,3> &v){return {double(v[0]),double(v[1]),double(v[2])};}
inline std::array<float,3> vec(const QJsonValue &value,float fallback){auto a=value.toArray();if(a.size()!=3)return {fallback,fallback,fallback};return {float(a[0].toDouble()),float(a[1].toDouble()),float(a[2].toDouble())};}
}
