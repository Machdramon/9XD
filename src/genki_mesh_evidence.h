#pragma once
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QtEndian>
#include <cmath>
#include <cstring>
#include <QVector>
#include <atomic>
#include <array>
// Sample-validated Genki packet profile; not a general VU emulator or scene importer.
inline QVector<quint32> genkiPacketFaces(const QByteArray &bytes,qint64 position,int count,int parity){
    QVector<quint32> faces;auto word=[&](qint64 p){return qFromLittleEndian<quint32>(bytes.constData()+p);};
    auto xyz=[&](int i){std::array<double,3> out;for(int a=0;a<3;++a){auto bits=word(position+i*12+a*4);if(a==1)bits&=~quint32(1);float f;std::memcpy(&f,&bits,4);out[a]=f;}return out;};
    for(int i=2;i<count;++i){if(word(position+i*12+4)&1)continue;int a=i-2,b=i-1,c=i;if(((i-2)&1)^parity)std::swap(a,b);auto x=xyz(a),y=xyz(b),z=xyz(c);double u[3],v[3];for(int k=0;k<3;++k){u[k]=y[k]-x[k];v[k]=z[k]-x[k];}double nx=u[1]*v[2]-u[2]*v[1],ny=u[2]*v[0]-u[0]*v[2],nz=u[0]*v[1]-u[1]*v[0];if(nx*nx+ny*ny+nz*nz<=1e-20)continue;faces+={quint32(a),quint32(b),quint32(c)};}return faces;
}
inline QJsonObject genkiMeshEvidence(const QByteArray &bytes,qint64 sourceSize,const std::atomic_bool *cancel=nullptr){
    auto hex=[](qint64 n){return QString("0x%1").arg(n,0,16);};
    auto u32=[&](qint64 p){return qFromLittleEndian<quint32>(bytes.constData()+p);};
    if(bytes.size()<64)return {};auto count=u32(0);if(count<2||count>256||4+qint64(count)*4>bytes.size())return {};
    qint64 previous=0;for(quint32 i=0;i<count;++i){auto at=u32(4+i*4);if(at<4+count*4||at<previous||at>=sourceSize)return {};previous=at;}
    qint64 base=u32(4),modelEnd=u32(8);if(base%16||base>bytes.size()-32||modelEnd<=base+32||modelEnd>bytes.size()||bytes.mid(base,8)!="0IVX00.1"||bytes.mid(base+16,8)!="LDMX00.2"||bytes.mid(base+24,8)!="MRON00.2")return {};
    QJsonArray samples,regions,chunks;int totalVertices=0,totalTriangles=0;bool capped=false;
    auto region=[&](QString name,qint64 start,qint64 width,qint64 stride,qint64 repetitions,QString evidence){if(regions.size()<24)regions.append(QJsonObject{{"name",name},{"offset",QString::number(start)},{"length",QString::number(width)},{"step",QString::number(stride)},{"repetitions",repetitions},{"evidence",evidence}});};
    for(qint64 a=base;a+16<=modelEnd;a+=16){
        if(cancel&&cancel->load())return {{"error","Genki scan canceled."}};
        auto tag=u32(a),kind=(tag>>28)&7,qwc=tag&65535,vertices=u32(a+12);qint64 end=a+16+qint64(qwc)*16;
        if((kind!=1&&kind!=7)||tag!=((kind<<28)|qwc)||qwc<1||qwc>2048||u32(a+4)!=0||u32(a+8)>1||vertices<3||vertices>256||end>modelEnd)continue;
        int cl=1,wl=1;bool valid=true;qint64 position=-1,uv=-1,normal=-1;int controls=0,unpacks=0;qint64 p=a+16;
        while(p<end){if(end-p<4){valid=false;break;}auto w=u32(p);int command=(w>>24)&127,num=(w>>16)&255,immediate=w&65535;p+=4;qint64 extra=0;
            if(command==1){cl=immediate&255;wl=immediate>>8;if(!cl||!wl){valid=false;break;}}
            else if(command==0x20)extra=4;else if(command==0x30||command==0x31)extra=16;
            else if(command==0x4a)extra=(num?num:256)*8;else if(command==0x50||command==0x51)extra=qint64(immediate)*16;
            else if(command>=0x60){int components=((command>>2)&3)+1,bits=32>>(command&3),n=num?num:256;if(bits==4&&components!=4){valid=false;break;}
                int inputs=wl<=cl?n:(n/wl)*cl+qMin(n%wl,cl);extra=bits==4?((qint64(inputs)*2+3)/4)*4:((qint64(inputs)*components*bits+31)/32)*4;++unpacks;
                if(command==0x68&&n==int(vertices)&&cl==4&&wl==1){if(position>=0){valid=false;break;}position=p;}
                if(command==0x65&&n==int(vertices)&&cl>=wl)uv=p;if(command==0x6a&&n==int(vertices)&&cl>=wl)normal=p;
            }else if(command!=0&&command!=2&&command!=3&&command!=4&&command!=5&&command!=6&&command!=7&&command!=0x10&&command!=0x11&&command!=0x13&&command!=0x14&&command!=0x15&&command!=0x17){valid=false;break;}
            if(extra>end-p){valid=false;break;}p+=extra;
        }
        if(!valid||p!=end||position<0||unpacks<2)continue;
        for(quint32 i=0;i<vertices&&valid;++i){for(int k=0;k<3;++k){auto bits=u32(position+i*12+k*4);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value)||std::abs(value)>100000)valid=false;}if(u32(position+i*12+4)&1)++controls;}
        if(!valid)continue;
        QJsonObject sample{{"dmaOffset",hex(a)},{"dmaEnd",hex(end)},{"positionsOffset",hex(position)},{"vertexCount",int(vertices)},{"positionStride",12},{"positionType","Float32 XYZ"},{"tagParity",int(u32(a+8))},{"yLowBitSetVertices",controls}};
        if(chunks.size()>=4096||totalVertices+int(vertices)>100000){capped=true;break;}
        auto faces=genkiPacketFaces(bytes,position,vertices,u32(a+8));totalVertices+=vertices;totalTriangles+=faces.size()/3;
        chunks.append(QJsonObject{{"positionsOffset",QString::number(position)},{"vertexCount",int(vertices)},{"uvOffset",QString::number(uv)},{"normalOffset",QString::number(normal)},{"tagParity",int(u32(a+8))},{"dmaOffset",QString::number(a)}});
        if(samples.size()<2){region("Genki sampled positions",position,12,12,vertices,"Bounded DMA/VIF V3-32 packet");
        if(uv>=0){sample["uvOffset"]=hex(uv);sample["uvType"]="VIF V2-16";region("Genki sampled UV pairs",uv,4,4,vertices,"Bounded DMA/VIF V2-16 packet");}
        if(normal>=0){sample["normalOffset"]=hex(normal);sample["normalType"]="VIF signed V3-8";region("Genki sampled normals",normal,3,3,vertices,"Bounded DMA/VIF V3-8 packet");}
        samples.append(sample);}a=end-16;
    }
    if(chunks.isEmpty())return {};
    region("Genki container directory",0,4+qint64(count)*4,4+qint64(count)*4,1,"Ordered little-endian section offsets");
    const bool supported=!capped&&totalTriangles>0;
    return {{"family","Genki PS2 0IVX/XMDL"},{"modelStart",hex(base)},{"modelEnd",hex(modelEnd)},{"samples",samples},{"previewChunks",chunks},{"validatedPackets",chunks.size()},{"validatedVertices",totalVertices},{"validatedTriangles",totalTriangles},{"previewSupported",supported},{"scanIncomplete",capped},{"scope","All validated geometry packets within the bounded model section; local geometry with variants retained."},{"renderSupported",supported},{"requiredDecoder",supported?"Native Genki packet importer. Part visibility, hierarchy and material bindings remain unresolved.":"No complete supported Genki triangle geometry found within decoder limits."},{"layoutNotes","Sample-validated Y float bit0 draw suppression and DMA-tag parity; not a generic VIF rule. UV normalization uses 4096; texture bindings remain unverified."},{"diagnostic",QString("Genki PS2: %1 validated vertices, %2 reconstructed triangles in %3 DMA/VIF packets. All local parts/variants retained; hierarchy and materials are not reconstructed.").arg(totalVertices).arg(totalTriangles).arg(chunks.size())},{"regions",regions}};
}
