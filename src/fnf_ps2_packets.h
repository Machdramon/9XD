#pragma once
#include "faf_geometry.h"
// Measured retail PS2 ARCP packet layouts. This is deliberately separate from
// the Xbox ARCX reader and from the legacy whole-document ARCP evidence scan.
namespace FnfPs2Packets {
using FafGeometry::u32;using FafGeometry::f32;using FafGeometry::range;
struct Unpack {qint64 at;int command,count,address,cl,wl;quint32 mask;};
inline QVector3D position(const QByteArray &b,const QJsonObject &chunk,int vertex){
    auto p=chunk["positionsOffset"].toString().toLongLong();
    if(chunk["positionLayout"]=="paired-xz-uvy"){
        auto attributes=chunk["uvOffset"].toString().toLongLong();
        return {f32(b,p+vertex*8),f32(b,attributes+vertex*16+8),f32(b,p+vertex*8+4)};
    }
    return {f32(b,p+vertex*12),f32(b,p+vertex*12+4),f32(b,p+vertex*12+8)};
}
inline QJsonObject inspect(const QByteArray &b,qint64 at,qint64 size,const std::atomic_bool &cancel){
    auto fail=[](QString error){return QJsonObject{{"renderSupported",false},{"diagnostic",error}};};
    if(!range(at,size,b.size())||size<80||size%16||u32(b,at)>1||u32(b,at+4)||quint64(u32(b,at+8))*4!=quint64(size)||u32(b,at+12))return fail("Unsupported PS2 geometry resource prefix.");
    const int mode=u32(b,at);const qint64 end=at+size;auto tag=u32(b,at+16);int kind=tag>>28,qwc=tag&65535;
    if((kind!=6&&kind!=7&&kind!=1)||tag!=((quint32(kind)<<28)|quint32(qwc))||!qwc||u32(b,at+20)||at+32+qint64(qwc)*16!=end)return fail("Unsupported or incomplete inline DMA geometry tag.");
    // The tag itself transfers STCYCL and the first GIF-tag UNPACK.
    qint64 p=at+24;int cl=1,wl=1;quint32 mask=0;QVector<Unpack> streams;
    while(p<end){
        if(cancel.load())return fail("PS2 packet inspection canceled.");if(!range(p,4,end))return fail("Truncated VIF command.");auto word=u32(b,p);int cmd=(word>>24)&127,num=(word>>16)&255,imm=word&65535;p+=4;qint64 extra=0;
        if(cmd==1){cl=imm&255;wl=imm>>8;if(!cl)cl=256;if(!wl)wl=256;}
        else if(cmd==0x20){extra=4;if(range(p,4,end))mask=u32(b,p);}
        else if(cmd==0x30||cmd==0x31)extra=16;
        else if(cmd==0x4a)extra=qint64(num?num:256)*8;
        else if(cmd==0x50||cmd==0x51)extra=qint64(imm)*16;
        else if(cmd>=0x60){int n=num?num:256,components=((cmd>>2)&3)+1,bits=32>>(cmd&3);if(bits==4&&components!=4)return fail("Unsupported VIF unpack mode.");int inputs=wl<=cl?n:(n/wl)*cl+qMin(n%wl,cl);extra=((qint64(inputs)*(bits==4?16:components*bits)+31)/32)*4;streams.append({p,cmd,n,imm,cl,wl,mask});}
        else if(cmd!=0&&cmd!=2&&cmd!=3&&cmd!=4&&cmd!=5&&cmd!=6&&cmd!=7&&cmd!=0x10&&cmd!=0x11&&cmd!=0x13&&cmd!=0x14&&cmd!=0x15&&cmd!=0x17)return fail("Unsupported VIF command.");
        if(!range(p,extra,end))return fail("VIF payload exceeds its DMA resource.");p+=extra;
    }
    QJsonArray chunks;int vertices=0,triangles=0,strips=0;Unpack positions{};bool pending=false;int loops=-1;qint64 gif=-1;
    for(int i=0;i<streams.size();++i){const auto &s=streams[i];if(s.cl!=1||s.wl!=1)return fail("Unsupported VIF cycle layout.");
        if(s.command==0x6c&&s.address==0xc001){
            if(pending||s.count!=2)return fail("Unexpected GIF header boundary.");auto hi=u32(b,s.at+4);if(((hi>>15)&7)!=4||(hi>>28)!=3||((hi>>26)&3)!=0||(u32(b,s.at+8)&0xfff)!=0x512)return fail("Unsupported GIF primitive/register list.");loops=u32(b,s.at)&0x7fff;gif=s.at;continue;
        }
        if(mode==1){
            if(s.command==0x6c&&s.address==0x8003&&!pending){positions=s;pending=true;continue;}
            if(s.command!=0x6c||s.address!=0x802f||!pending||loops<3||s.count!=positions.count*2||(loops!=s.count&&loops!=s.count-1))return fail("Unsupported packed track position/attribute pairing.");
            QJsonArray starts;int faces=0,j=0;
            for(int n=0;n<s.count;++n){auto w=u32(b,s.at+n*16+12);if((w&~quint32(0x80ff))!=0x3f800000||(w&255)>128)return fail("Unsupported packed track draw/alpha word.");if(n>=loops&&(!(w&0x8000)||(w&255)!=0))return fail("Invalid packed track alignment vertex.");}
            while(j<loops){int begin=j;while(j<loops&&(u32(b,s.at+j*16+12)&0x8000))++j;if(j-begin!=2||j>=loops)return fail("Packed track strip must begin with exactly two suppressed kicks.");starts.append(begin);while(j<loops&&!(u32(b,s.at+j*16+12)&0x8000)){++faces;++j;}}
            QJsonObject chunk{{"positionsOffset",QString::number(positions.at)},{"uvOffset",QString::number(s.at)},{"normalOffset",QString::number(-1)},{"colorOffset",QString::number(s.at)},{"vertexCount",loops},{"triangleCount",faces},{"positionLayout","paired-xz-uvy"},{"uvStride",16},{"gifOffset",QString::number(gif)},{"topologyKind","packed-adc15"},{"stripStarts",starts}};
            chunks.append(chunk);vertices+=loops;triangles+=faces;strips+=starts.size();pending=false;loops=-1;continue;
        }
        if(s.command!=0x68||s.address!=0x8003)continue;
        if(pending||loops!=s.count||s.count<3)return fail("Invalid explicit-strip position/GIF count.");
        qint64 uv=-1,normal=-1,color=-1,topology=-1;int topologyCount=0;
        for(int k=i+1;k<streams.size();++k){const auto &u=streams[k];if((u.command==0x68&&u.address==0x8003)||(u.command==0x6c&&u.address==0xc001))break;
            if(u.command==0x72&&u.address==0xc067&&u.mask==0x3f3f3f3f){topology=u.at;topologyCount=u.count;}
            if(u.count!=s.count)continue;if(u.command==0x64&&u.address==0x8067)uv=u.at;if(u.command==0x6e&&u.address==0x8099)normal=u.at;if(u.command==0x6e&&u.address==0xc035)color=u.at;
        }
        if(uv<0||color<0||topology<0||topologyCount<8)return fail("Incomplete explicit-strip attributes or topology.");int header=quint8(b[topology]),n=header&127,consumed=0,faces=0;QJsonArray lengths;
        if(!(header&128)||!n||topologyCount!=n+7)return fail("Invalid explicit strip control header.");
        for(int k=0;k<n;++k){int length=quint8(b[topology+1+k]);if(!length)return fail("Empty explicit strip.");lengths.append(length);consumed+=length+2;faces+=length;}
        if(consumed!=s.count)return fail("Explicit strip lengths disagree with vertex count.");
        chunks.append(QJsonObject{{"positionsOffset",QString::number(s.at)},{"uvOffset",QString::number(uv)},{"normalOffset",QString::number(normal)},{"colorOffset",QString::number(color)},{"vertexCount",s.count},{"triangleCount",faces},{"positionLayout","xyz"},{"uvStride",8},{"gifOffset",QString::number(gif)},{"topologyKind","explicit-lengths"},{"topologyOffset",QString::number(topology)},{"stripLengths",lengths}});vertices+=s.count;triangles+=faces;strips+=n;loops=-1;
    }
    if(pending||chunks.isEmpty())return fail("No complete supported static PS2 geometry chunks.");
    int invalidUvs=0;
    for(int i=0;i<chunks.size();++i){auto chunk=chunks[i].toObject();auto uv=chunk["uvOffset"].toString().toLongLong();int stride=chunk["uvStride"].toInt();bool uvValid=true;for(int n=0;n<chunk["vertexCount"].toInt();++n){auto point=position(b,chunk,n);for(int k=0;k<3;++k)if(!std::isfinite(point[k])||std::abs(point[k])>1e6)return fail("Invalid PS2 vertex position.");for(int k=0;k<2;++k){auto value=f32(b,uv+n*stride+k*4);if(!std::isfinite(value)||std::abs(value)>1e6)uvValid=false;}}chunk["hasUvs"]=uvValid;if(!uvValid)++invalidUvs;chunks[i]=chunk;}
    return {{"renderSupported",true},{"previewChunks",chunks},{"validatedVertices",vertices},{"validatedTriangles",triangles},{"validatedStrips",strips},{"invalidUvChunks",invalidUvs}};
}
}
