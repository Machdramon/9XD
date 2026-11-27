#pragma once
#include "blur_model.h"
#include <QFileInfo>
#include <QDir>
#include <QMap>
#include <QSet>
#include <QCryptographicHash>
#include <mutex>

// File decoding only. Resource identity is retained; full shader/streaming state is not emulated.
namespace BlurScene {
struct Pair { quint32 group=0,variant=0; };
struct Block { qint64 offset=0,size=0; QVector<Pair> pairs; };
struct Reader {
    const QByteArray &b; qint64 p=0,end=0; bool ok=true;
    bool take(qint64 n){if(!ok||n<0||p>end||n>end-p){ok=false;return false;}p+=n;return true;}
    quint32 u(){auto at=p;return take(4)?qFromLittleEndian<quint32>(b.constData()+at):0;}
    quint64 wide(){auto at=p;return take(8)?qFromLittleEndian<quint64>(b.constData()+at):0;}
    bool frame(quint16 version=0){auto x=u();return ok&&x==(quint32(version)<<16|0x4152);}
    bool array(QVector<Pair> *pairs,const std::atomic_bool &cancel){
        if(!frame())return false;auto n=u();if(!ok||n>100000)return false;
        for(quint32 i=0;i<n;++i){if((i&1023)==0&&cancel.load())return false;if(pairs){if(!frame())return false;Pair v{u(),u()};if(!ok)return false;pairs->append(v);}else if(!take(4))return false;}return ok;
    }
};
inline bool archive(const QByteArray &b,qint64 at){return at>=0&&at<=b.size()-24&&b.mid(at,4)=="ARCH"&&Blur::word(b,at+4)==1&&!Blur::word(b,at+8)&&b.mid(at+12,4)=="ARCH"&&!Blur::word(b,at+16);}
inline QVector<Block> directory(const QByteArray &render,const QByteArray &stream,const std::atomic_bool &cancel){
    if(cancel.load()||render.left(12)!=QByteArray("  CPRender\0\0",12)||!archive(stream,0))return {};
    QVector<Block> found;qsizetype search=8;int attempts=0;
    while((search=render.indexOf(QByteArray("RA\1\0RA\0\0",8),search))>=0){
        auto start=search++;if(cancel.load())return {};if(start<8||!Blur::marker(render,start-8))continue;
        const auto count=Blur::word(render,start-4);if(!count||count>4096)continue;if(++attempts>256)return {};
        Reader r{render,start,render.size()};QVector<Block> blocks;qint64 next=0;quint64 totalPairs=0;
        for(quint32 i=0;i<count;++i){
            Block v;if(!r.frame(1)||!r.frame()||!r.take(32)||!r.array(&v.pairs,cancel)||!r.array(nullptr,cancel)||!r.array(nullptr,cancel)||!r.array(nullptr,cancel))break;
            auto off=r.wide(),size=r.wide();r.wide();totalPairs+=v.pairs.size();
            if(!r.ok||totalPairs>1000000||off!=quint64(next)||!size||off%2048||size%2048||off>quint64(stream.size())||size>quint64(stream.size())-off||!archive(stream,qint64(off))||Blur::word(stream,off+20)!=quint32(v.pairs.size()))break;
            v.offset=off;v.size=size;next=qint64(off+size);blocks.append(v);
        }
        if(blocks.size()==count&&next==stream.size()){if(!found.isEmpty())return {};found=blocks;}
    }return found;
}
struct Image {quint32 group=0,variant=0,index=0,w=0,h=0,mips=0;QString name,format; qint64 offset=0,size=0;};
inline bool image(Reader &r,Pair key,quint32 index,Image &out,bool &inherited){
    out.group=key.group;out.variant=key.variant;out.index=index;inherited=false;
    if(!r.frame(2)||!r.frame()||r.u()!=2)return false;
    if(!key.variant){
        auto n=r.u();if(!r.ok||!n||n>1024)return false;auto at=r.p;if(!r.take(n))return false;
        auto text=r.b.mid(at,n);for(auto ch:text)if(uchar(ch)<32||uchar(ch)>126)return false;out.name=QString::fromLatin1(text);
        if(!r.frame(1)||!r.take(68))return false;
    }else{
        auto state=r.u();if(!r.ok||state>1)return false;if(!state){inherited=true;return true;}if(!r.take(20))return false;
    }
    auto f=r.p;if(f<20||f>r.end-24)return false;auto fmt=r.b.mid(f,4);
    if(fmt!="DXT1"&&fmt!="DXT3"&&fmt!="DXT5")return false;
    auto w=Blur::word(r.b,f-16),h=Blur::word(r.b,f-12),mips=Blur::word(r.b,f-4);
    if(Blur::word(r.b,f-20)!=28||!w||!h||w>16384||h>16384||(w&(w-1))||(h&(h-1))||Blur::word(r.b,f-8)!=1||!mips||mips>15||Blur::word(r.b,f+4)!=0xffffffff||Blur::word(r.b,f+8)!=3||!Blur::marker(r.b,f+12)||Blur::word(r.b,f+20)!=16)return false;
    auto bytes=[&](quint32 x,quint32 y){return quint64(qMax(1u,(x+3)/4))*qMax(1u,(y+3)/4)*(fmt=="DXT1"?8:16);};
    quint64 full=0;for(quint32 i=0;i<mips;++i)full+=bytes(qMax(1u,w>>i),qMax(1u,h>>i));auto size=Blur::word(r.b,f+16);qint64 payload=-1;int matches=0;
    if(size!=full&&size!=bytes(w,h))return false;
    for(int pad=0;pad<32;++pad){auto at=f+24+pad;if(at>r.end)break;if(pad&&r.b[at-1])break;if(at%16||size>quint64(r.end-at))continue;auto next=at+size;
        bool successor=next<=r.end-12&&(r.b.mid(next,12)==QByteArray("RA\0\0RA\0\0\2\0\0\0",12)||r.b.mid(next,12)==QByteArray("RA\2\0RA\0\0\2\0\0\0",12));
        if(!successor&&r.end-next<2048){successor=true;for(auto tail=next;tail<r.end;++tail)if(r.b[tail]){successor=false;break;}}
        if(successor){payload=at;++matches;}}
    if(matches!=1)return false;
    out.w=w;out.h=h;out.mips=mips;out.offset=payload;out.size=size;out.format=fmt=="DXT1"?"BC1":fmt=="DXT3"?"BC2":"BC3";r.p=payload+size;return true;
}
// readPage deliberately caps each request at1MiB; scene companions can be much larger.
inline QByteArray readComplete(ReadRequest source,const PatchList &patches,const std::atomic_bool &cancel,QString &error){
    QByteArray bytes;for(qint64 at=0;at<source.expectedSize;){if(cancel.load()){error="Blur scene read canceled.";return {};}auto request=source;request.offset=at;request.length=qMin<qint64>(1048576,source.expectedSize-at);auto page=readPage(request);if(!page.error.isEmpty()||page.bytes.size()!=request.length){error=page.error.isEmpty()?"Incomplete Blur scene read.":page.error;return {};}applyPatches(page.bytes,at,patches);bytes+=page.bytes;at+=page.bytes.size();}
    source.offset=0;source.length=0;auto final=readPage(source);if(!final.error.isEmpty()){error=final.error;return {};}return bytes;
}
#include "blur_scene_geometry.inc"
inline QJsonObject scan(const QByteArray &stream,const QByteArray &render,const std::atomic_bool &cancel){
    auto blocks=directory(render,stream,cancel);if(blocks.isEmpty())return {};
    QMap<QString,QString> names;QMap<QString,Image> best;QSet<QString> ambiguous;int complete=0,partial=0,decoded=0,inherited=0;
    QJsonArray regions;
    for(int bi=0;bi<blocks.size();++bi){
        if(cancel.load())return {};const auto &block=blocks[bi];Reader r{stream,block.offset+24,block.offset+block.size};int accepted=0;bool valid=true;
        regions.append(QJsonObject{{"name",QString("Blur streaming block %1").arg(bi)},{"offset",QString::number(block.offset)},{"length",QString::number(block.size)},{"evidence","Serialized directory; aligned span and ARCH record count validated."}});
        for(const auto &key:block.pairs){
            if(cancel.load())return {};if(!r.frame()||!r.frame()||r.u()!=2||r.u()!=key.variant){valid=false;break;}
            if(!key.variant){for(int i=0;i<3;++i)if(!r.frame()||r.u()!=0){valid=false;break;}if(!valid)break;}
            auto count=r.u();if(!r.ok||!count||count>1024){valid=false;break;}QVector<Image> staged;int refs=0;
            for(quint32 i=0;i<count;++i){Image v;bool inherit=false;if(!image(r,key,i,v,inherit)){valid=false;break;}if(inherit)++refs;else staged.append(v);}if(!valid)break;
            ++accepted;inherited+=refs;
            for(auto &v:staged){++decoded;auto identity=QString::number(v.group)+":"+QString::number(v.index);
                if(!v.name.isEmpty()){if(names.contains(identity)&&names[identity]!=v.name)ambiguous.insert(identity);else names[identity]=v.name;}
                if(!best.contains(identity)||quint64(v.w)*v.h>quint64(best[identity].w)*best[identity].h)best[identity]=v;
            }
        }
        if(valid){for(auto at=r.p;at<r.end;++at){if((at&4095)==0&&cancel.load())return {};if(stream[at]){valid=false;break;}}}
        if(valid)++complete;else if(accepted)++partial;
    }
    QJsonArray images;for(auto it=best.cbegin();it!=best.cend()&&images.size()<4096;++it){
        if(cancel.load())return {};if(!names.contains(it.key())||ambiguous.contains(it.key()))continue;const auto &v=it.value();
        images.append(QJsonObject{{"name",QString("Blur / %1 / group %2:%3 / variant %4").arg(names[it.key()]).arg(v.group).arg(v.index).arg(v.variant)},{"blurSourceName",names[it.key()]},{"blurResourceGroup",qint64(v.group)},{"blurTextureIndex",qint64(v.index)},{"blurVariant",qint64(v.variant)},{"textureAssignmentVerified",false},{"basis","Named resource identity from ordered streaming records. Largest decoded variant for image inspection only; original draw/variant selection is unresolved."},{"width",int(v.w)},{"height",int(v.h)},{"settings",QJsonObject{{"offset",QString::number(v.offset)},{"width",int(v.w)},{"height",int(v.h)},{"format",v.format},{"layout","Linear"},{"byteSwap","None"}}}});
    }
    QJsonObject geometryStats;auto meshes=Geometry::candidates(stream,render,cancel,&geometryStats);int textured=0;for(auto row:meshes)textured+=row.toObject()["textureAssigned"].toBool();
    QJsonObject summary{{"geometry",geometryStats},{"streamBlocks",blocks.size()},{"completeTextureBlocks",complete},{"partialTextureBlocks",partial},{"boundedImageRecords",decoded},{"inheritedRecords",inherited},{"namedImageCandidates",images.size()},{"sceneGeometrySupported",!meshes.isEmpty()},{"supportedGeometrySections",meshes.size()},{"texturedGeometrySections",textured},{"variantSelection","largest decoded variant for image inspection only"}};
    auto pending=geometryStats["skinnedInstancesPending"].toInt();
    return {{"candidates",meshes},{"images",images},{"regions",regions},{"summary",summary},{"notes",QJsonArray{QString("Blur: %1 streaming blocks; %2 complete texture-only blocks, %3 partial prefixes; %4 named images. Validated scene strip sections are available in 3D scan; base-colour textures follow original material resource pairs; rigid placements are recovered, skeletal poses and full game shading remain pending.").arg(blocks.size()).arg(complete).arg(partial).arg(images.size()),QString("Blur placements: %1 recovered; %2 skeletal poses pending; %3 unsupported; %4 exact placement duplicates skipped.").arg(geometryStats["placedInstances"].toInt()).arg(pending).arg(geometryStats["unsupportedInstances"].toInt()).arg(geometryStats["duplicateInstances"].toInt())}}};
}
// Current-document recipes must never point into a sibling or an unrelated memory document.
inline QJsonObject scan(const ReadRequest &source,const QByteArray &bytes,const std::atomic_bool &cancel){
    if(source.memory||source.path.isEmpty()||bytes.size()!=source.expectedSize||!archive(bytes,0)||QFileInfo(source.path).fileName().compare("level.levelstream",Qt::CaseInsensitive))return {};
    const auto path=QFileInfo(source.path).dir().filePath("level.levelrender");QFileInfo info(path);
    if(!info.isFile()||info.isSymLink()||info.size()<=0||info.size()>qMin<qint64>(AssetMemory::bytes()/16,64*1024*1024))return {};
    ReadRequest sibling;sibling.path=path;sibling.expectedSize=info.size();sibling.expectedModified=info.lastModified();sibling.offset=0;sibling.length=info.size();QString error;auto render=readComplete(sibling,{},cancel,error);
    if(!error.isEmpty()||render.size()!=info.size()||cancel.load())return {{"notes",QJsonArray{"Blur scene companion could not be read: "+error}},{"summary",QJsonObject{{"sceneGeometrySupported",false}}}};auto result=scan(bytes,render,cancel);
    sibling.length=0;auto check=readPage(sibling);if(!check.error.isEmpty()||cancel.load())return {};return result;
}
}
