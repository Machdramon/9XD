#pragma once
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QtEndian>
#include <QVector>
#include <QSet>
#include <atomic>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <QMap>
#include "genki_hierarchy.h"

// 0IVX recognition/position evidence is deliberately separate from topology admission.
// A version string or a syntactically valid VIF packet does not define draw controls.
namespace GenkiTopology {
inline constexpr auto C1Profile="c1-v2-ybit-tag-parity-v1";
struct Unpack {int command,count,immediate,cl,wl;qint64 data,size;};
inline bool c1Packet(const QByteArray &bytes,const QList<Unpack> &streams,int count,int parity,bool commandsSupported,bool kick,QString &reason){
    auto fail=[&](QString text){reason=text;return false;};
    if(!commandsSupported||!kick||streams.size()<3)return fail("C1 VIF command/stream signature did not match.");
    const auto &control=streams[0],&position=streams[1];
    if(control.command!=0x6c||control.count!=2||control.immediate!=0x8000||control.cl!=2||control.wl!=2||control.size!=32||position.command!=0x68||position.count!=count||position.immediate!=0x8002||position.cl!=4||position.wl!=1)return fail("C1 control/position layout did not match.");
    if(count<3||parity<0||parity>1||control.data<0||control.data+32>bytes.size()||position.data<0||position.data+qint64(count)*12>bytes.size())return fail("C1 packet bounds are invalid.");
    auto word=[&](qint64 p){return qFromLittleEndian<quint32>(bytes.constData()+p);};
    if(word(control.data+28)!=(0x8000u|quint32(count))||word(control.data+12)!=(parity?0xbf800000u:0x3f800000u))return fail("C1 count/parity controls disagree with the packet tag.");
    for(int offset:{0,4,8,16,20,24}){auto bits=word(control.data+offset);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value)||std::abs(value)>100000)return fail("Invalid C1 bounding/control coordinates.");}
    if(!(word(position.data+4)&1)||!(word(position.data+16)&1))return fail("C1 strip must start with two explicitly suppressed records.");
    int last=1;bool color=false;for(int i=2;i<streams.size();++i){const auto &s=streams[i];int slot=s.command==0x65&&s.immediate==0x8003?2:s.command==0x6a&&s.immediate==0x8004?3:(s.command==0x62||s.command==0x6e)&&s.immediate==0xc005?4:-1;
        if(slot<=last||s.count!=count||s.cl!=4||s.wl!=1)return fail("C1 optional attributes or final stream did not match.");last=slot;color|=slot==4;
    }
    if(!color||last!=4)return fail("C1 final stream is missing.");
    // Parity/count/control fields are not interpreted as coordinates. Only this
    // admitted profile is allowed to remove the embedded Y draw-suppression bit.
    for(int i=0;i<count;++i)for(int a=0;a<3;++a){auto bits=word(position.data+i*12+a*4);if(a==1)bits&=~quint32(1);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value)||std::abs(value)>100000)return fail("Nonfinite/out-of-range C1 positions.");}
    reason.clear();return true;
}
inline constexpr auto GifStripProfile="genki-v2-explicit-gif-strip-v1";
inline constexpr auto Txrd2Profile="txrd2-v1-mode-adc-strip-v1";
inline constexpr auto KaidoProfile="kaido-v2-plain-strip-v1";
inline bool supported(QString p){return p==C1Profile||p==GifStripProfile||p==Txrd2Profile||p==KaidoProfile;}
// Kaido Racer (LDMX00.2, 2-vector): same control block as C1, but no per-record draw marker.
// Each packet is one independent strip; UV Float32 @8003, normal V4-16 @8004, color @c005.
inline bool kaidoPacket(const QByteArray &bytes,const QList<Unpack> &streams,int count,int parity,bool commands,bool kick,QString &reason){
    auto fail=[&](QString r){reason=r;return false;};
    if(!commands||!kick||streams.size()<2||count<3||parity<0||parity>1)return fail("Kaido VIF command/count signature mismatch.");
    const auto &c=streams[0],&p=streams[1];
    if(c.command!=0x6c||c.count!=2||c.immediate!=0x8000||c.cl!=2||c.wl!=2||c.size!=32||p.command!=0x68||p.count!=count||p.immediate!=0x8002||p.cl!=4||p.wl!=1)return fail("Kaido control/position layout mismatch.");
    if(c.data<0||c.data>bytes.size()-32||p.data<0||p.data>bytes.size()-qint64(count)*12)return fail("Truncated Kaido packet.");
    auto word=[&](qint64 at){return qFromLittleEndian<quint32>(bytes.constData()+at);};
    if(word(c.data+28)!=(0x8000u|quint32(count))||word(c.data+12)!=(parity?0xbf800000u:0x3f800000u))return fail("Kaido count/parity controls disagree with the packet tag.");
    for(int offset:{0,4,8,16,20,24}){auto bits=word(c.data+offset);float v;std::memcpy(&v,&bits,4);if(!std::isfinite(v)||std::abs(v)>100000)return fail("Invalid Kaido controls.");}
    int previous=1;for(int i=2;i<streams.size();++i){const auto &a=streams[i];int slot=a.command==0x64&&a.immediate==0x8003?2:a.command==0x6d&&a.immediate==0x8004?3:(a.command==0x6e||a.command==0x62)&&a.immediate==0xc005?4:-1;if(slot<=previous||a.count!=count||a.cl!=4||a.wl!=1)return fail("Kaido optional stream layout mismatch.");previous=slot;}
    for(int i=0;i<count;++i)for(int k=0;k<3;++k){auto bits=word(p.data+i*12+k*4);float v;std::memcpy(&v,&bits,4);if(!std::isfinite(v)||std::abs(v)>100000)return fail("Nonfinite/out-of-range Kaido positions.");}
    reason.clear();return true;
}
inline bool gifStripPacket(const QByteArray &bytes,const QList<Unpack> &streams,int count,int parity,bool commands,bool kick,QString &reason){auto fail=[&](QString r){reason=r;return false;};if(!commands||!kick||streams.size()<2||count<3||parity<0||parity>1)return fail("GIF strip command/count signature mismatch.");auto c=streams[0],p=streams[1];if(c.command!=0x6c||c.count!=4||c.immediate!=0x8000||c.cl!=4||c.wl!=4||c.size!=64||p.command!=0x68||p.count!=count||p.immediate!=0x8004||p.cl!=4||p.wl!=1)return fail("GIF strip control/position layout mismatch.");if(c.data<0||c.data>bytes.size()-64)return fail("Truncated GIF strip controls.");auto word=[&](int n){return qFromLittleEndian<quint32>(bytes.constData()+c.data+n);};auto tag=qFromLittleEndian<quint64>(bytes.constData()+c.data+48),clip=qFromLittleEndian<quint64>(bytes.constData()+c.data);if((tag&0xffff)!=quint64(0x8000|count)||((tag>>46)&1)!=1||((tag>>47)&7)!=4||((tag>>58)&3)!=0||(tag>>60)!=3||(word(56)&0xfff)!=0x412||clip!=(tag|(quint64(1)<<47))||word(8)!=word(56)||word(12)!=word(60)||word(60)!=(parity?0xbf800001u:0x3f800000u))return fail("Native GIF primitive/count/register/parity fields disagree.");for(int offset:{16,20,24,32,36,40}){auto bits=word(offset);float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f)||std::abs(f)>100000)return fail("Invalid GIF strip bounds.");}if(word(28)!=0x3f800000||word(44)!=0x3f800000)return fail("GIF strip bound controls mismatch.");if(p.data<0||p.data>bytes.size()-qint64(count)*12)return fail("Truncated GIF strip positions.");for(int k=0;k<3;++k){float low,high;auto l=word(16+k*4),h=word(32+k*4);std::memcpy(&low,&l,4);std::memcpy(&high,&h,4);if(low>high)return fail("Inverted GIF strip bounds.");for(int i=0;i<count;++i){auto bits=qFromLittleEndian<quint32>(bytes.constData()+p.data+i*12+k*4);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value)||value<double(low)-.001||value>double(high)+.001)return fail("GIF strip position exceeds its native bounds.");}}int previous=1;for(int i=2;i<streams.size();++i){auto a=streams[i];int slot=a.command==0x64&&a.immediate==0x8005?2:a.command==0x6d&&a.immediate==0x8006?3:a.command==0x6e&&a.immediate==0xc007?4:-1;if(slot<=previous||a.count!=count||a.cl!=4||a.wl!=1)return fail("Unsupported GIF strip attribute layout.");previous=slot;}reason.clear();return true;}
inline QVector<quint32> gifStripFaces(const QByteArray &b,qint64 position,int count,int parity){QVector<quint32> out;if(position<0||count<3||count>256||position>b.size()-qint64(count)*12||parity<0||parity>1)return out;auto point=[&](int i){std::array<double,3> p;for(int k=0;k<3;++k){auto bits=qFromLittleEndian<quint32>(b.constData()+position+i*12+k*4);float f;std::memcpy(&f,&bits,4);p[k]=f;}return p;};for(int i=2;i<count;++i){int a=i-2,c=i,d=i-1;if(((i-2)&1)^parity)std::swap(a,d);auto p=point(a),q=point(d),r=point(c);double u[3],v[3];for(int k=0;k<3;++k){u[k]=q[k]-p[k];v[k]=r[k]-p[k];}double x=u[1]*v[2]-u[2]*v[1],y=u[2]*v[0]-u[0]*v[2],z=u[0]*v[1]-u[1]*v[0];if(x*x+y*y+z*z>1e-20)out+={quint32(a),quint32(d),quint32(c)};}return out;}
inline bool txrd2Packet(const QByteArray &b,const QList<Unpack> &streams,int count,int parity,bool commands,bool kick,qint64 &flag,int &stride,bool &yFlag,QString &reason){
    auto fail=[&](QString r){reason=r;return false;};flag=-1;stride=0;yFlag=false;
    if(!commands||!kick||streams.size()<3||count<3||parity<0||parity>1)return fail("TXRD2 VIF command/count mismatch.");auto c=streams[0],p=streams[1];
    if(c.command!=0x6c||c.count!=2||c.immediate!=0x8000||c.cl!=2||c.wl!=2||c.size!=32||p.command!=0x68||p.count!=count||p.immediate!=0x8002||p.cl!=4||p.wl!=1||c.data<0||c.data>b.size()-32)return fail("TXRD2 control/position layout mismatch.");
    auto u=[&](int k){return qFromLittleEndian<quint32>(b.constData()+c.data+k);};if(u(28)!=(0x8000u|quint32(count))||u(12)!=(parity?0xbf800000u:0x3f800000u))return fail("TXRD2 count/parity controls disagree.");
    for(int k:{0,4,8,16,20,24}){auto bits=u(k);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value)||std::abs(value)>100000)return fail("Invalid TXRD2 controls.");}
    int previous=1;bool normal=false;const Unpack *color=nullptr;for(int i=2;i<streams.size();++i){const auto &a=streams[i];int slot=(a.command==0x65||a.command==0x6d)&&a.immediate==0x8003?2:a.command==0x6a&&a.immediate==0x8004?3:(a.command==0x62||a.command==0x6e)&&a.immediate==0xc005?4:-1;if(slot<=previous||a.count!=count||a.cl!=4||a.wl!=1)return fail("Unsupported TXRD2 optional stream.");previous=slot;normal|=slot==3;if(slot==4)color=&a;}
    yFlag=normal;if(normal){flag=p.data+4;stride=12;}else if(color){flag=color->data+(color->command==0x6e?3:0);stride=color->command==0x6e?4:1;}else return fail("TXRD2 packet has no draw-control stream.");
    if(flag<0||flag+qint64(count-1)*stride>=b.size()||!(quint8(b[flag])&1)||!(quint8(b[flag+stride])&1))return fail("TXRD2 strip reset markers are missing.");reason.clear();return true;
}
inline QVector<quint32> txrd2Faces(const QByteArray &b,qint64 position,int count,int parity,qint64 flags,int stride,bool yFlag){
    QVector<quint32> out;if(position<0||count<3||count>256||position>b.size()-qint64(count)*12||flags<0||stride<1||flags+qint64(count-1)*stride>=b.size())return out;
    auto point=[&](int i){std::array<double,3> p;for(int k=0;k<3;++k){auto bits=qFromLittleEndian<quint32>(b.constData()+position+i*12+k*4);if(yFlag&&k==1)bits&=~quint32(1);float f;std::memcpy(&f,&bits,4);p[k]=f;}return p;};
    for(int i=2;i<count;++i){if(quint8(b[flags+qint64(i)*stride])&1)continue;int a=i-2,d=i-1,c=i;if(((i-2)&1)^parity)std::swap(a,d);auto p=point(a),q=point(d),r=point(c);double u[3],v[3];for(int k=0;k<3;++k){u[k]=q[k]-p[k];v[k]=r[k]-p[k];}double x=u[1]*v[2]-u[2]*v[1],y=u[2]*v[0]-u[0]*v[2],z=u[0]*v[1]-u[1]*v[0];if(x*x+y*y+z*z>1e-20)out+={quint32(a),quint32(d),quint32(c)};}return out;
}
inline QVector<quint32> chunkFaces(const QByteArray &b,const QJsonObject &c);
inline QVector<quint32> c1Faces(const QByteArray &bytes,qint64 position,int count,int parity){
    QVector<quint32> faces;if(position<0||count<3||count>256||position>bytes.size()||qint64(count)*12>bytes.size()-position||parity<0||parity>1)return faces;
    auto word=[&](qint64 p){return qFromLittleEndian<quint32>(bytes.constData()+p);};auto xyz=[&](int i){std::array<double,3> out;for(int a=0;a<3;++a){auto bits=word(position+i*12+a*4);if(a==1)bits&=~quint32(1);float f;std::memcpy(&f,&bits,4);out[a]=f;}return out;};
    for(int i=2;i<count;++i){if(word(position+i*12+4)&1)continue;int a=i-2,b=i-1,c=i;if(((i-2)&1)^parity)std::swap(a,b);auto x=xyz(a),y=xyz(b),z=xyz(c);double u[3],v[3];for(int k=0;k<3;++k){u[k]=y[k]-x[k];v[k]=z[k]-x[k];}double nx=u[1]*v[2]-u[2]*v[1],ny=u[2]*v[0]-u[0]*v[2],nz=u[0]*v[1]-u[1]*v[0];if(nx*nx+ny*ny+nz*nz<=1e-20)continue;faces+={quint32(a),quint32(b),quint32(c)};}return faces;
}
inline QVector<quint32> chunkFaces(const QByteArray &b,const QJsonObject &c){auto pos=c["positionsOffset"].toString().toLongLong();int n=c["vertexCount"].toInt(),parity=c["tagParity"].toInt();auto profile=c["topologyProfile"].toString();if(profile==Txrd2Profile)return txrd2Faces(b,pos,n,parity,c["drawFlagOffset"].toString().toLongLong(),c["drawFlagStride"].toInt(),c["drawFlagInY"].toBool());return profile==C1Profile?c1Faces(b,pos,n,parity):profile==GifStripProfile||profile==KaidoProfile?gifStripFaces(b,pos,n,parity):QVector<quint32>();}
}
inline QJsonObject genkiMeshEvidence(const QByteArray &bytes,qint64 sourceSize,const std::atomic_bool *cancel=nullptr){
    auto hex=[](qint64 n){return QString("0x%1").arg(n,0,16);};if(bytes.size()<32)return {};
    auto u32=[&](qint64 p){return qFromLittleEndian<quint32>(bytes.constData()+p);};
    QVector<qint64> directory;auto count=u32(0);if(count>=1&&count<=256&&4+qint64(count)*4<=bytes.size()){qint64 previous=-1;bool valid=true;for(quint32 i=0;i<count;++i){auto at=qint64(u32(4+i*4));if(at<4+qint64(count)*4||at<=previous||at%16||at>=sourceSize){valid=false;break;}directory.append(at);previous=at;}if(!valid)directory.clear();}
    auto parents=directory;for(int i=0;i<parents.size();++i){auto base=parents[i],end=i+1<parents.size()?parents[i+1]:qint64(bytes.size());if(base>bytes.size()-4||end>bytes.size())continue;auto n=u32(base);if(n<2||n>256||base+4+qint64(n)*4>end)continue;QVector<qint64> offsets;qint64 previous=base+4+qint64(n)*4-1;bool valid=true,model=false;for(quint32 j=0;j<n;++j){auto at=base+qint64(u32(base+4+j*4));if(at<=previous||at%16||at>=end){valid=false;break;}offsets.append(at);previous=at;model|=bytes.mid(at,8)=="0IVX00.1";}if(valid&&model)for(auto at:offsets)if(!directory.contains(at))directory.append(at);}std::sort(directory.begin(),directory.end());
    QVector<qint64> models;qsizetype search=0;bool capped=false;int attempts=0;for(;attempts<1024;++attempts){if(cancel&&cancel->load())return {{"error","Genki scan canceled."}};auto at=bytes.indexOf("0IVX00.1",search);if(at<0)break;search=at+8;if(at%16||at>bytes.size()-32||u32(at+8)||u32(at+12)||bytes.mid(at+16,4)!="LDMX"||bytes.mid(at+24,4)!="MRON")continue;models.append(at);if(models.size()>=128){capped=true;break;}}
    if(attempts==1024)capped=true;if(models.isEmpty())return {};
    QJsonArray samples,regions,chunks,sections,v1FlagGroups;QSet<QString> admittedProfiles;int v1Groups=0,v1FlagSet=0,v1FlagAgreements=0;int totalVertices=0,totalTriangles=0;bool allTopology=true;int rejectedPackets=0,withV3_8=0,withoutV3_8=0,resetWithV3_8=0,resetWithoutV3_8=0;
    auto region=[&](QString name,qint64 start,qint64 width,qint64 stride,qint64 repetitions,QString evidence){if(regions.size()<24)regions.append(QJsonObject{{"name",name},{"offset",QString::number(start)},{"length",QString::number(width)},{"step",QString::number(stride)},{"repetitions",repetitions},{"evidence",evidence}});};
    for(int modelIndex=0;modelIndex<models.size();++modelIndex){
        auto base=models[modelIndex];qint64 modelEnd=modelIndex+1<models.size()?models[modelIndex+1]:bytes.size();for(auto boundary:directory)if(boundary>base){modelEnd=qMin(modelEnd,boundary);break;}
        auto ldmx=QString::fromLatin1(bytes.mid(base+16,8)),mron=QString::fromLatin1(bytes.mid(base+24,8));
        // The node hierarchy closes every part with a DMA RET; a complete walk establishes the
        // model boundary even when no container directory lists embedded models.
        auto hierarchy=GenkiHierarchy::walk(bytes,base,modelEnd);
        QMap<qint64,std::pair<int,int>> partStarts;if(hierarchy.valid)for(int n=0;n<hierarchy.nodes.size();++n)for(int k=0;k<hierarchy.nodes[n].parts.size();++k)partStarts.insert(hierarchy.nodes[n].parts[k].first,{n,k});
        bool boundaryKnown=base==0||directory.contains(base)||hierarchy.valid;bool versionC1=ldmx=="LDMX00.2"&&mron=="MRON00.2";bool modelKaido=versionC1&&hierarchy.valid;bool kaidoDistinct=false;
        bool versionV1=ldmx=="LDMX00.1"&&mron=="MRON00.1";bool modelV1=versionV1&&boundaryKnown;QSet<qint64> v1Covered;bool modelTopology=versionC1&&boundaryKnown,modelGif=modelTopology;QString reason=!versionC1?"This LDMX/MRON version requires its own complete packet/group topology validation.":!boundaryKnown?"Embedded model boundaries are not established by a container directory.":QString();int sectionStart=chunks.size(),modelVertices=0,modelTriangles=0,matchingPackets=0,sectionRejected=0;
        for(qint64 a=base+32;a+16<=modelEnd;a+=16){
            if(cancel&&cancel->load())return {{"error","Genki scan canceled."}};
            auto tag=u32(a),kind=(tag>>28)&7,qwc=tag&65535,vertices=u32(a+12);qint64 end=a+16+qint64(qwc)*16;
            if((kind!=1&&kind!=7)||tag!=((kind<<28)|qwc)||qwc<1||qwc>2048||u32(a+4)!=0||u32(a+8)>1||vertices<3||vertices>256)continue;
            if(end>modelEnd){modelTopology=false;modelGif=false;modelV1=false;modelKaido=false;++sectionRejected;if(reason.isEmpty())reason="A packet crosses its model boundary.";continue;}
            int cl=1,wl=1;bool valid=true,commandsC1=true,commandsGif=true,kick=false;qint64 position=-1,uv=-1,normal=-1,normal16=-1;QString uvType="Int16 / 4096";int uvStride=4;int controls=0;QList<GenkiTopology::Unpack> streams;qint64 p=a+16;
            while(p<end){if(end-p<4){valid=false;break;}auto w=u32(p);int command=(w>>24)&127,num=(w>>16)&255,immediate=w&65535;p+=4;qint64 extra=0;commandsC1&=command==0||command==1||command==0x17||command>=0x60;commandsGif&=command==0||command==1||command==0x17||command>=0x60;
                if(command==0&&(immediate||num)){commandsC1=false;commandsGif=false;}if(command==1){if(num||(kick&&immediate!=0x404)){commandsC1=false;commandsGif=false;}cl=immediate&255;wl=immediate>>8;if(!cl||!wl){valid=false;break;}}
                else if(command==0x20)extra=4;else if(command==0x30||command==0x31)extra=16;
                else if(command==0x4a)extra=(num?num:256)*8;else if(command==0x50||command==0x51)extra=qint64(immediate)*16;
                else if(command>=0x60){if(kick){commandsC1=false;commandsGif=false;}int components=((command>>2)&3)+1,bits=32>>(command&3),n=num?num:256;if(bits==4&&components!=4){valid=false;break;}int inputs=wl<=cl?n:(n/wl)*cl+qMin(n%wl,cl);extra=bits==4?((qint64(inputs)*2+3)/4)*4:((qint64(inputs)*components*bits+31)/32)*4;streams.append({command,n,immediate,cl,wl,p,extra});
                    if(command==0x68&&n==int(vertices)&&cl==4&&wl==1){if(position>=0){valid=false;break;}position=p;}
                    if((command==0x65||command==0x64)&&n==int(vertices)&&cl==4&&wl==1&&(immediate&0x4000)==0){uv=p;uvType=command==0x64?"Float32":"Int16 / 4096";uvStride=command==0x64?8:4;}if(command==0x6d&&n==int(vertices)&&cl==4&&wl==1&&immediate==0x8003&&ldmx=="LDMX00.1"){uv=p;uvType="Int16 / 4096";uvStride=8;}if(command==0x6a&&n==int(vertices)&&cl==4&&wl==1&&(immediate&0x4000)==0)normal=p;if(command==0x6d&&n==int(vertices)&&cl==4&&wl==1&&(immediate==0x8006||(immediate==0x8004&&ldmx=="LDMX00.2")))normal16=p;
                }else if(command==0x17){if(immediate||num||kick||streams.size()<3)commandsC1=false;if(immediate||num||kick||streams.size()<2)commandsGif=false;kick=true;}
                else if(command!=0&&command!=2&&command!=3&&command!=4&&command!=5&&command!=6&&command!=7&&command!=0x10&&command!=0x11&&command!=0x13&&command!=0x14&&command!=0x15){valid=false;break;}
                if(extra>end-p){valid=false;break;}p+=extra;
            }
            if(!valid||p!=end||position<0||streams.size()<2){++sectionRejected;modelTopology=false;modelGif=false;modelV1=false;modelKaido=false;if(reason.isEmpty())reason="A DMA-like geometry packet could not be completely validated.";continue;}
            for(quint32 i=0;i<vertices&&valid;++i){for(int k=0;k<3;++k){auto bits=u32(position+i*12+k*4);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value)||std::abs(value)>100000)valid=false;}if(u32(position+i*12+4)&1)++controls;}
            if(!valid){++sectionRejected;modelTopology=false;modelGif=false;modelV1=false;modelKaido=false;if(reason.isEmpty())reason="Nonfinite/out-of-range position packet.";continue;}
            if(uv>=0&&uvType=="Float32"){bool finite=true;for(quint32 i=0;i<vertices&&finite;++i)for(int k=0;k<2;++k){auto bits=u32(uv+i*8+k*4);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value)||std::abs(value)>100000)finite=false;}if(!finite){uv=-1;uvType="Invalid Float32";}}
            if(chunks.size()>=4096||totalVertices+int(vertices)>100000){capped=true;modelTopology=false;modelGif=false;modelV1=false;modelKaido=false;break;}
            QString packetReason;bool packetC1=versionC1&&boundaryKnown&&GenkiTopology::c1Packet(bytes,streams,vertices,u32(a+8),commandsC1,kick,packetReason);if(packetC1)++matchingPackets;else{modelTopology=false;if(reason.isEmpty())reason=packetReason.isEmpty()?"Topology profile not established for this packet.":packetReason;}
            QString gifReason;bool packetGif=versionC1&&boundaryKnown&&GenkiTopology::gifStripPacket(bytes,streams,vertices,u32(a+8),commandsGif,kick,gifReason);modelGif&=packetGif;
            QString kaidoReason;modelKaido&=versionC1&&boundaryKnown&&GenkiTopology::kaidoPacket(bytes,streams,vertices,u32(a+8),commandsGif,kick,kaidoReason);
            qint64 drawFlag=-1;int drawStride=0;bool drawY=false;QString v1Reason;bool packetV1=versionV1&&boundaryKnown&&GenkiTopology::txrd2Packet(bytes,streams,vertices,u32(a+8),commandsGif,kick,drawFlag,drawStride,drawY,v1Reason);modelV1&=packetV1;
            totalVertices+=vertices;modelVertices+=vertices;bool firstYBits=(u32(position+4)&1)&&(u32(position+16)&1);if(normal>=0){++withV3_8;resetWithV3_8+=firstYBits;}else{++withoutV3_8;resetWithoutV3_8+=firstYBits;}
            chunks.append(QJsonObject{{"txrd2PacketMatched",packetV1},{"drawFlagOffset",QString::number(drawFlag)},{"drawFlagStride",drawStride},{"drawFlagInY",drawY},{"positionsOffset",QString::number(position)},{"vertexCount",int(vertices)},{"uvOffset",QString::number(uv)},{"uvType",uvType},{"uvStride",uvStride},{"normalOffset",QString::number(normal)},{"normal16Offset",QString::number(normal16)},{"gifPacketSignatureMatched",packetGif},{"tagParity",int(u32(a+8))},{"dmaOffset",QString::number(a)},{"dmaEnd",QString::number(end)},{"v3_8AttributePresent",normal>=0},{"modelOffset",QString::number(base)},{"packetTopologyMatched",packetC1}});
            if(hierarchy.valid){auto it=partStarts.upperBound(a);if(it!=partStarts.begin()){--it;const auto &node=hierarchy.nodes[it.value().first];const auto &part=node.parts[it.value().second];if(a>=part.first&&a<part.end){auto c=chunks.last().toObject();c["nodeIndex"]=it.value().first;c["nodeId"]=qint64(node.id);c["partDescriptor"]=QString::number(part.descriptor);if(!node.identity()){c["nodeTranslation"]=GenkiHierarchy::json(node.translation);c["nodeRotation"]=GenkiHierarchy::json(node.rotation);c["nodeScale"]=GenkiHierarchy::json(node.scale);}chunks[chunks.size()-1]=c;}}}
            if(samples.size()<4){QJsonObject sample{{"dmaOffset",hex(a)},{"dmaEnd",hex(end)},{"positionsOffset",hex(position)},{"vertexCount",int(vertices)},{"positionStride",12},{"positionType","Float32 XYZ (raw until topology is validated)"},{"tagParity",int(u32(a+8))},{"yLowBitSetVertices",controls},{"ldmxVersion",ldmx},{"mronVersion",mron},{"c1PacketSignatureMatched",packetC1}};if(packetV1){sample["drawFlagOffset"]=hex(drawFlag);sample["drawFlagStride"]=drawStride;sample["drawFlagBit"]=0;sample["drawFlagStorage"]=drawY?"Position Y low bit (normal-present mode)":"Color/alpha byte low bit (no-normal mode)";region("TXRD2 face-suppression flags",drawFlag,1,drawStride,vertices,"Bit 0 suppresses the triangle ending at this record; packet and group mode checks must pass before applying it.");}if(uv>=0)sample["uvOffset"]=hex(uv);if(normal>=0)sample["normalOffset"]=hex(normal);if(!streams.isEmpty()){auto control=streams.first();sample["preVertexControlOffset"]=hex(control.data);sample["preVertexControlBytes"]=control.size;sample["preVertexControlHex"]=QString::fromLatin1(bytes.mid(control.data,qMin(control.size,qint64(64))).toHex(' '));sample["positionCommandOffset"]=hex(position-4);sample["positionCommandWord"]=hex(u32(position-4));sample["packetWord8Offset"]=hex(a+8);if(control.size>=32){sample["controlWord12Offset"]=hex(control.data+12);sample["controlWord12"]=hex(u32(control.data+12));sample["controlWord28Offset"]=hex(control.data+28);sample["controlWord28"]=hex(u32(control.data+28));}region("0IVX pre-vertex control block (semantics separate)",control.data,control.size,control.size,1,"Validated VIF payload before positions; count/parity candidates are evidence, not a universal face flag");}sample["firstTwoYLowBitsSet"]=firstYBits;sample["v3_8AttributeStreamPresent"]=normal>=0;samples.append(sample);region("0IVX sampled positions (topology separate)",position,12,12,vertices,"Bounded DMA/VIF V3-32 data; low bits are not generic draw flags");}
            a=end-16;
        }
        // The v1 mesh-header hypothesis is admitted as evidence only when all
        // three declared counts match a complete consecutive group of packets.
        // Its low flag bit correlates with stream layout in the supplied corpus;
        // that correlation is NOT a triangle-generation rule.
        if(ldmx=="LDMX00.1"&&mron=="MRON00.1"){
            qint64 previousGroupEnd=base;for(int first=sectionStart;first<chunks.size();){int next=first+1;auto head=chunks[first].toObject();qint64 start=head["dmaOffset"].toString().toLongLong(),end=head["dmaEnd"].toString().toLongLong(),vertices=head["vertexCount"].toInt();bool everyAttribute=head["v3_8AttributePresent"].toBool(),anyAttribute=everyAttribute;
                while(next<chunks.size()){auto c=chunks[next].toObject();if(c["dmaOffset"].toString().toLongLong()!=end)break;end=c["dmaEnd"].toString().toLongLong();vertices+=c["vertexCount"].toInt();everyAttribute&=c["v3_8AttributePresent"].toBool();anyAttribute|=c["v3_8AttributePresent"].toBool();++next;}
                if(start>=base+224&&start-224>=previousGroupEnd&&u32(start-48)==vertices&&u32(start-44)==quint32(next-first)&&qint64(u32(start-40))*16==end-start&&u32(start-36)==0){auto flags=u32(start-208);bool uniform=everyAttribute==anyAttribute,agrees=uniform&&((flags&1)==0)==everyAttribute;if(!agrees)modelV1=false;for(int k=first;k<next;++k)v1Covered.insert(chunks[k].toObject()["dmaOffset"].toString().toLongLong());++v1Groups;v1FlagSet+=bool(flags&1);v1FlagAgreements+=agrees;if(v1FlagGroups.size()<1024)v1FlagGroups.append(QJsonObject{{"modelOffset",hex(base)},{"firstDmaOffset",hex(start)},{"end",hex(end)},{"flagOffset",hex(start-208)},{"flagWord",hex(flags)},{"flagBit0",int(flags&1)},{"countHeaderOffset",hex(start-48)},{"vertices",vertices},{"packets",next-first},{"quadwords",qint64(u32(start-40))},{"v3_8EveryPacket",everyAttribute},{"streamLayoutCorrelationMatches",agrees},{"topologyValidated",false}});region("TXRD2 mesh stream-mode selector",start-208,4,4,1,"Counts and packet spans match. Mode bit 0 selects the stream layout; per-vertex suppression lives in Y or the color/alpha byte, not in this mode field.");}
                previousGroupEnd=end;first=next;
            }
        }
        if(chunks.size()==sectionStart){modelTopology=false;modelGif=false;modelV1=false;modelKaido=false;if(reason.isEmpty())reason="No supported position packet was validated in this model section.";}
        if(modelV1)for(int k=sectionStart;k<chunks.size();++k)if(!v1Covered.contains(chunks[k].toObject()["dmaOffset"].toString().toLongLong()))modelV1=false;
        // Shared control/position/color packets are also valid C1 syntax. Admit Kaido only
        // inside a fully walked hierarchy with at least one distinguishing attribute stream.
        for(int k=sectionStart;k<chunks.size();++k){const auto c=chunks[k].toObject();kaidoDistinct|=c["uvType"]=="Float32"||(c.contains("normal16Offset")&&c["normal16Offset"].toString().toLongLong()>=0);}
        modelKaido&=kaidoDistinct;
        if(chunks.size()==sectionStart)modelKaido=false;
        QString profile=modelTopology?QString(GenkiTopology::C1Profile):modelGif?QString(GenkiTopology::GifStripProfile):modelV1?QString(GenkiTopology::Txrd2Profile):modelKaido?QString(GenkiTopology::KaidoProfile):QString();if(!profile.isEmpty()){admittedProfiles.insert(profile);reason.clear();for(int i=sectionStart;i<chunks.size();++i){auto c=chunks[i].toObject();c["topologyProfile"]=profile;chunks[i]=c;auto pos=c["positionsOffset"].toString().toLongLong();int n=c["vertexCount"].toInt(),parity=c["tagParity"].toInt();modelTriangles+=GenkiTopology::chunkFaces(bytes,c).size()/3;}}
        allTopology&=!profile.isEmpty();totalTriangles+=modelTriangles;rejectedPackets+=sectionRejected;
        sections.append(QJsonObject{{"offset",hex(base)},{"end",hex(modelEnd)},{"ldmxVersion",ldmx},{"mronVersion",mron},{"containerBoundaryKnown",boundaryKnown},{"positionPackets",chunks.size()-sectionStart},{"vertices",modelVertices},{"matchingC1Packets",matchingPackets},{"rejectedPackets",sectionRejected},{"topologyValidated",!profile.isEmpty()},{"topologyProfile",profile},{"reason",reason},{"triangles",modelTriangles},{"hierarchyValidated",hierarchy.valid},{"hierarchyError",hierarchy.error},{"hierarchyNodes",hierarchy.nodes.size()},{"hierarchyEnd",hex(hierarchy.end)},
            {"locators",[&]{QJsonArray out;for(const auto &node:hierarchy.nodes)if(!node.geometry()&&!node.identity()&&out.size()<256)out.append(QJsonObject{{"nodeId",qint64(node.id)},{"offset",hex(node.offset)},{"translation",GenkiHierarchy::json(node.translation)},{"rotation",GenkiHierarchy::json(node.rotation)}});return out;}()}});
        if(capped)break;
    }
    // Mixed/partial models are never quietly exported using the C1 rule for only
    // their matching packets. A separate explicit per-model loader can be added later.
    const bool supported=allTopology&&admittedProfiles.size()==1&&!capped&&bytes.size()==sourceSize&&!chunks.isEmpty()&&totalTriangles>0;
    const bool sourceYDown=supported&&admittedProfiles.contains(GenkiTopology::GifStripProfile)&&GenkiHierarchy::rawTrackYDown(bytes);
    for(int i=0;i<v1FlagGroups.size();++i){auto group=v1FlagGroups[i].toObject();group["topologyValidated"]=supported&&admittedProfiles.contains(GenkiTopology::Txrd2Profile);v1FlagGroups[i]=group;}
    if(!supported){totalTriangles=0;for(int i=0;i<chunks.size();++i){auto c=chunks[i].toObject();c.remove("topologyProfile");chunks[i]=c;}}
    if(!directory.isEmpty())region("Genki container directory",0,4+qint64(directory.size())*4,4+qint64(directory.size())*4,1,"Ordered bounded section offsets; not a topology selector");
    auto diagnostic=supported?QString("Genki validated topology profile: %1 vertices, %2 triangles in %3 packets across %4 model section(s). Every packet passed its explicit control/layout checks; hierarchy and material bindings remain unresolved.").arg(totalVertices).arg(totalTriangles).arg(chunks.size()).arg(sections.size()):QString("Genki 0IVX recognized: %1 position records in %2 packets across %3 model section(s). Face topology is NOT validated for this variant; C1 and TXRD2 rules are not interchangeable. %4").arg(totalVertices).arg(chunks.size()).arg(sections.size()).arg(capped?"Scan limits reached; partial data must not be treated as a complete model.":"Raw point preview is available when position packets were found; it does not reconstruct a model.");
    return {{"coordinateConvention",sourceYDown?"raw-track-y-down":"source-native"},{"sourceYDown",sourceYDown},{"family","Genki PS2 0IVX/XMDL"},{"modelStart",hex(models.first())},{"modelEnd",sections.isEmpty()?QString():sections.first().toObject()["end"].toString()},{"modelSections",sections},{"v1MeshFlagGroups",v1FlagGroups},{"v1FlagStatistics",QJsonObject{{"validatedGroupHeaders",v1Groups},{"flagBit0SetGroups",v1FlagSet},{"streamLayoutAgreementGroups",v1FlagAgreements}}},{"samples",samples},{"previewChunks",chunks},{"controlStatistics",QJsonObject{{"v3_8AttributePackets",withV3_8},{"packetsWithoutV3_8Attribute",withoutV3_8},{"twoYLowBitsSetWithV3_8",resetWithV3_8},{"twoYLowBitsSetWithoutV3_8",resetWithoutV3_8}}},{"validatedPackets",chunks.size()},{"validatedVertices",totalVertices},{"validatedTriangles",totalTriangles},{"rejectedPackets",rejectedPackets},{"topologyProfile",supported?*admittedProfiles.cbegin():QString()},{"topologyValidated",supported},{"previewSupported",!chunks.isEmpty()&&!capped},{"scanIncomplete",capped||bytes.size()<sourceSize},{"scope","Bounded 0IVX sections; position evidence is distinct from validated connectivity."},{"renderSupported",supported},{"requiredDecoder",supported?"Isolated C1, GIF-tag or TXRD2 mode/ADC strip decoder; not a universal Genki importer.":"A complete validated topology profile for these model sections; unsupported packet modes or broken group/boundary checks are not reconstructed."},{"layoutNotes","Draw-bit storage and parity require full admission to their own profile. C1 and TXRD2 are validated separately. Explicit four-vector GIF-strip packets retain coordinate bits; TXRD2 v1 uses its validated group mode to choose Y or color/alpha suppression. Unknown or mixed variants retain raw coordinates and no faces."},{"diagnostic",diagnostic},{"regions",regions}};
}
