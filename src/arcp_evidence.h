#pragma once
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>
#include <QtEndian>
#include <atomic>
#include <cmath>
#include <cstring>
// Original bounded analysis of the ARCP sample profile. Not a general archive or VU emulator.
inline QJsonObject arcpEvidence(const QByteArray &bytes,qint64 sourceSize,const std::atomic_bool &cancel){
    if(bytes.size()<4||bytes.left(4)!="ARCP")return {};
    QJsonObject out{{"family","ARCP / PS2 VIF"},{"signatureOffset","0x0"},{"renderSupported",false},{"previewSupported",false}};
    auto u32=[&](qint64 p){return qFromLittleEndian<quint32>(bytes.constData()+p);};
    if(bytes.size()<128){out["diagnostic"]="Truncated ARCP header.";return out;}
    auto count=u32(4);qint64 tableEnd=128+qint64(count)*16;
    if(count<1||count>4096||tableEnd>bytes.size()){out["diagnostic"]="Invalid or truncated ARCP resource directory.";return out;}
    for(quint32 i=0;i<count;++i){auto offset=u32(128+i*16+4);if(offset&&offset>=sourceSize){out["diagnostic"]="ARCP resource pointer is outside the document.";return out;}}
    QJsonArray regions,samples,chunks;int packets=0,vertices=0,triangles=0,strips=0,topologyChunks=0;bool capped=false;
    auto region=[&](QString name,qint64 offset,qint64 width,qint64 repetitions=1){if(regions.size()<24)regions.append(QJsonObject{{"name",name},{"offset",QString::number(offset)},{"length",QString::number(width)},{"step",QString::number(width)},{"repetitions",repetitions},{"evidence","ARCP signature, bounded directory and validated inline DMA/VIF stream"}});};
    region("ARCP header",0,128);region("ARCP resource directory",128,16,count);
    struct Unpack{qint64 offset;int command,count,address,cl,wl;quint32 mask;};
    auto hex=[](qint64 n){return QString("0x%1").arg(n,0,16);};
    for(qint64 a=(tableEnd+15)&~qint64(15);a+16<=bytes.size();a+=16){
        if(cancel.load())return {{"family","ARCP / PS2 VIF"},{"diagnostic","Inspection canceled."}};
        auto tag=u32(a);int kind=tag>>28,qwc=tag&65535;qint64 end=a+16+qint64(qwc)*16;
        if((kind!=1&&kind!=6&&kind!=7)||tag!=((quint32(kind)<<28)|quint32(qwc))||qwc<1||qwc>4096||u32(a+4)!=0||end>bytes.size())continue;
        int cl=1,wl=1;quint32 mask=0;bool valid=true;QVector<Unpack> unpacks;qint64 p=a+16;
        while(p<end){if(end-p<4){valid=false;break;}auto w=u32(p);int command=(w>>24)&127,num=(w>>16)&255,immediate=w&65535;p+=4;qint64 extra=0;
            if(command==1){cl=immediate&255;wl=immediate>>8;if(!cl)cl=256;if(!wl)wl=256;}
            else if(command==0x20){extra=4;if(end-p>=4)mask=u32(p);}else if(command==0x30||command==0x31)extra=16;
            else if(command==0x4a)extra=(num?num:256)*8;else if(command==0x50||command==0x51)extra=qint64(immediate)*16;
            else if(command>=0x60){int n=num?num:256,components=((command>>2)&3)+1,bits=32>>(command&3);if(bits==4&&components!=4){valid=false;break;}int inputs=wl<=cl?n:(n/wl)*cl+qMin(n%wl,cl);extra=((qint64(inputs)*(bits==4?16:components*bits)+31)/32)*4;unpacks.append({p,command,n,immediate,cl,wl,mask});}
            else if(command!=0&&command!=2&&command!=3&&command!=4&&command!=5&&command!=6&&command!=7&&command!=0x10&&command!=0x11&&command!=0x13&&command!=0x14&&command!=0x15&&command!=0x17){valid=false;break;}
            if(extra>end-p){valid=false;break;}p+=extra;
        }
        if(!valid||p!=end)continue;QJsonArray packetChunks;
        for(int i=0;i<unpacks.size();++i){const auto &v=unpacks[i];if(v.command!=0x68||v.address!=0x8003||v.cl!=1||v.wl!=1||v.count<3)continue;
            qint64 uv=-1,normal=-1,topology=-1;int topologyCount=0;
            for(int j=i+1;j<unpacks.size()&&unpacks[j].command!=0x68;++j){const auto &u=unpacks[j];if(u.cl!=1||u.wl!=1)continue;
                if(u.command==0x72&&u.address==0xc067&&u.mask==0x3f3f3f3f){topology=u.offset;topologyCount=u.count;}
                if(u.count!=v.count)continue;if(u.command==0x64&&u.address==0x8067)uv=u.offset;if(u.command==0x6e&&u.address==0x8099)normal=u.offset;
            }
            if(uv<0||normal<0)continue;bool finite=true;
            for(int n=0;n<v.count&&finite;++n){for(int axis=0;axis<3;++axis){auto bits=u32(v.offset+n*12+axis*4);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value)||std::abs(value)>1e6)finite=false;}for(int axis=0;axis<2;++axis){auto bits=u32(uv+n*8+axis*4);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value)||std::abs(value)>1e6)finite=false;}}
            if(finite){QJsonObject chunk{{"positionsOffset",QString::number(v.offset)},{"vertexCount",v.count},{"uvOffset",QString::number(uv)},{"normalOffset",QString::number(normal)}};
                if(topology>=0&&topologyCount>=8){int header=quint8(bytes[topology]),n=header&127,consumed=0,total=0;QJsonArray lengths;bool validLengths=(header&128)&&n>0&&topologyCount==n+7;
                    if(validLengths)for(int k=0;k<n;++k){int length=quint8(bytes[topology+1+k]);if(!length)validLengths=false;lengths.append(length);consumed+=length+2;total+=length;}
                    if(validLengths&&consumed==v.count){chunk["stripLengths"]=lengths;chunk["triangleCount"]=total;chunk["topologyOffset"]=QString::number(topology);}
                }packetChunks.append(chunk);}

        }
        if(packetChunks.isEmpty())continue;
        if(packets>=128||chunks.size()+packetChunks.size()>1024){capped=true;break;}
        ++packets;for(auto item:packetChunks){auto c=item.toObject();if(vertices+c["vertexCount"].toInt()>100000){capped=true;break;}vertices+=c["vertexCount"].toInt();chunks.append(c);if(c.contains("stripLengths")){++topologyChunks;triangles+=c["triangleCount"].toInt();strips+=c["stripLengths"].toArray().size();}
            if(samples.size()<4){auto start=c["positionsOffset"].toString().toLongLong();samples.append(QJsonObject{{"dmaOffset",hex(a)},{"dmaEnd",hex(end)},{"positionsOffset",hex(start)},{"vertexCount",c["vertexCount"]},{"positionStride",12},{"positionType","Float32 XYZ"}});region("ARCP sampled XYZ positions",start,12,c["vertexCount"].toInt());}}
        if(capped)break;a=end-16;
    }
    out["directoryEntries"]=int(count);out["validatedPackets"]=packets;out["validatedVertices"]=vertices;out["samples"]=samples;out["regions"]=regions;out["previewChunks"]=chunks;out["previewSupported"]=vertices>0;out["scanIncomplete"]=capped||bytes.size()<sourceSize;
    const bool supported=vertices>0&&topologyChunks==chunks.size()&&!out["scanIncomplete"].toBool();out["renderSupported"]=supported;out["validatedTriangles"]=triangles;out["validatedStrips"]=strips;
    QJsonArray references;for(auto extension:{QByteArray(".png"),QByteArray(".tga"),QByteArray(".dds"),QByteArray(".bmp")}){qsizetype search=0;while(references.size()<32){auto at=bytes.indexOf(extension,search);if(at<0)break;search=at+extension.size();if(search>=bytes.size()||bytes[search]!=0)continue;qsizetype begin=at;while(begin>0&&at-begin<80){auto c=quint8(bytes[begin-1]);if((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.')--begin;else break;}if(begin<at)references.append(QJsonObject{{"name",QString::fromLatin1(bytes.mid(begin,search-begin))},{"offset",QString::number(begin)},{"evidence","NUL-terminated texture filename reference; not embedded pixel data"}});}}
    out["textureReferences"]=references;
    out["requiredDecoder"]="ARCP PS2 VIF topology reconstruction; strip/control stream and hierarchy/material assignment are not yet validated. No raw index-buffer recipe is available.";
    out["diagnostic"]=vertices>0?QString("ARCP recognized: %1 validated vertices in %2 DMA/VIF packets. Triangle connectivity is not yet supported; raw mesh settings cannot decode this stream.").arg(vertices).arg(packets):QString("ARCP recognized, but no supported VIF position/UV/normal profile was validated. This is not proof that geometry is absent.");
    if(supported){out["requiredDecoder"]="Native ARCP strip importer. Hierarchy and material assignment remain unresolved.";out["diagnostic"]=QString("ARCP: %1 vertices, %2 reconstructed triangles in %3 independent strips (%4 DMA/VIF packets). Hierarchy and materials are not reconstructed.").arg(vertices).arg(triangles).arg(strips).arg(packets);}
    return out;
}
inline QString assetMeshDiagnostic(const QJsonObject &discovery){
    auto evidence=discovery["meshEvidence"].toArray();for(auto value:evidence){auto item=value.toObject();if(!item["diagnostic"].toString().isEmpty())return item["diagnostic"].toString();}
    if(!evidence.isEmpty())return "Geometry packets found. This format needs a packet importer and reconstructed triangle indices; raw index settings are insufficient.";
    return QString("No supported model recipe found after inspecting %1 bytes. This does not prove that the file contains no geometry. Inspect the header or load a known setup; existing settings were preserved.").arg(discovery["scannedBytes"].toString());
}
