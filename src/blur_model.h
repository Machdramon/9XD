#pragma once
#include "binary_document.h"
#include "mesh_data.h"
#include "mesh_surface.h"
#include "memory_budget.h"
#include "texture_decode.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <cstring>

// Bounded Blur PC stride20 static buffers; per-draw strips retain their own stream offsets.
namespace Blur {
inline constexpr auto ProfileId="native-blur-static";
inline quint32 word(const QByteArray &b,qsizetype p){return qFromLittleEndian<quint32>(b.constData()+p);}
inline float real(const QByteArray &b,qsizetype p){auto w=word(b,p);float f;std::memcpy(&f,&w,4);return f;}
struct Draw {qsizetype descriptor=0;quint32 first=0,count=0,minimum=0,vertices=0,streamOffset=0;};
struct Layout {qsizetype scene=0,end=0,vertices=0,indices=0;quint32 vertexCount=0,indexCount=0;QVector<Draw> draws;};
inline bool marker(const QByteArray &b,qsizetype p){return p>=0&&p<=b.size()-4&&b.mid(p,4)==QByteArray("RA\0\0",4);}
inline bool children(const QByteArray &b,qsizetype p,qsizetype end,Layout &r,int depth=0){
    if(depth>8)return false;
    while(p<end){if(p>end-16)return false;auto tag=b.mid(p,8);auto size=word(b,p+8);if(size<16||size>quint64(end-p))return false;
        if(tag==QByteArray("Scene\0\0\0",8)){
            if(r.scene||size<40||b.mid(p+16,4)!="ARCH"||word(b,p+20)!=1||b.mid(p+28,4)!="ARCH")return false;
            r.scene=p+16;r.end=p+size;
        }else if(tag==QByteArray("Render\0\0",8)){if(!children(b,p+16,p+size,r,depth+1))return false;}
        p+=size;
    }return p==end;
}
inline Layout inspect(const QByteArray &b,const std::atomic_bool &cancel){
    Layout r;
    if(b.size()<36||b.left(12)!=QByteArray("  CPModel\0\0\0",12))return {};
    const quint64 rootEnd=quint64(word(b,12))+4;
    if(rootEnd>quint64(b.size())||(rootEnd!=quint64(b.size())&&b.mid(rootEnd,8)!=QByteArray("MtaDbg\0\0",8))||!children(b,20,b.size(),r)||!r.scene)return {};
    int vb=0,ib=0;
    for(qsizetype o=r.scene+24;o+12<r.end;++o){
        if((o&4095)==0&&cancel.load())return {};
        if(!marker(b,o)||word(b,o+8)!=16)continue;
        quint32 n=word(b,o+4);if(n<6||n>quint64(r.end-o-12))continue;
        const bool vertexBlob=o>=18&&marker(b,o-18)&&word(b,o-14)==3&&b.mid(o-10,6)==QByteArray::fromHex("00000c001000");
        qsizetype payload=-1;int matches=0;
        for(int pad=0;pad<32;++pad){auto s=o+12+pad;if(s>r.end-n-4)break;if(pad&&b[o+11+pad]!=0)break;
            if(marker(b,s+n)&&(!vertexBlob||(s+n+13<=r.end&&word(b,s+n+4)==1&&b[s+n+8]==2&&b.mid(s+n+9,4)==QByteArray("RA\1\0",4)))){payload=s;++matches;}}
        if(matches!=1)continue;
        if(o>=18&&marker(b,o-18)&&word(b,o-14)==3&&b.mid(o-10,6)==QByteArray::fromHex("00000c001000")){
            auto count=word(b,o-4);if(count&&quint64(count)*20==n){++vb;r.vertices=payload;r.vertexCount=count;}
        }
        if(o>=12&&b.mid(o-12,4)==QByteArray("RA\1\0",4)&&word(b,o-4)==0&&quint64(word(b,o-8))*2==n){++ib;r.indices=payload;r.indexCount=n/2;}
    }
    if(vb!=1||ib!=1||r.indexCount<3||r.vertexCount<3||!AssetMemory::fits(r.vertexCount,qint64(r.indexCount)*3))return {};
    // D3D triangle-strip descriptors. minimumVertex is a range constraint, NOT baseVertex.
    for(qsizetype p=r.indices+qint64(r.indexCount)*2+48;p+16<=r.end;++p){
        if((p&4095)==0&&cancel.load())return {};
        if(!marker(b,p-44)||word(b,p-40)!=1||word(b,p-36)!=5||word(b,p-32)||word(b,p-28)||word(b,p-24)!=1||word(b,p-20)||!marker(b,p-16)||word(b,p-12)!=1||!marker(b,p-8)||!marker(b,p+12))continue;
        Draw d{p-44,word(b,p-4),word(b,p),word(b,p+4),word(b,p+8),0};
        if(!d.count||d.first>r.indexCount||d.count>r.indexCount-d.first||!d.vertices)return {};
        const auto o=d.descriptor;
        if(o+84<=r.end&&marker(b,o+56)&&word(b,o+60)==1&&marker(b,o+64)&&word(b,o+68)==0&&word(b,o+72)==0&&word(b,o+80)==0)d.streamOffset=word(b,o+76);
        else if(d.first||d.count!=r.indexCount||d.minimum||d.vertices!=r.vertexCount)return {};
        if(d.streamOffset%20||quint64(d.streamOffset)/20+d.minimum+d.vertices>r.vertexCount)return {};
        quint32 lo=65535,hi=0;
        for(quint32 i=0;i<d.count;++i){auto idx=qFromLittleEndian<quint16>(b.constData()+r.indices+2*qint64(d.first+i));lo=qMin(lo,quint32(idx));hi=qMax(hi,quint32(idx));}
        if(lo!=d.minimum||quint64(hi)-lo+1!=d.vertices)return {};
        r.draws.append(d);if(r.draws.size()>4096)return {};
    }
    if(r.draws.isEmpty())return {};
    auto sorted=r.draws;std::sort(sorted.begin(),sorted.end(),[](const Draw&a,const Draw&c){return a.first<c.first;});
    quint32 end=0;for(const auto &d:sorted){if(d.first!=end)return {};end+=d.count;}if(end!=r.indexCount)return {};
    for(quint32 i=0;i<r.vertexCount;++i){if((i&4095)==0&&cancel.load())return {};for(int k=0;k<3;++k){auto f=real(b,r.vertices+qint64(i)*20+4*k);if(!std::isfinite(f)||std::abs(f)>1e7)return {};}}
    for(quint32 i=0;i<r.indexCount;++i)if(qFromLittleEndian<quint16>(b.constData()+r.indices+qint64(i)*2)>=r.vertexCount)return {};
    return r;
}
inline Layout drawLayout(const Layout &r,int index){auto selected=r;const auto &d=r.draws[index];selected.vertices+=d.streamOffset;selected.vertexCount=d.minimum+d.vertices;selected.indices+=qint64(d.first)*2;selected.indexCount=d.count;return selected;}
inline QString drawIdentity(const Layout&r,int index){return QString::number(r.draws.size()==1?r.vertices:r.draws[index].descriptor);}
inline QVector<quint32> faces(const QByteArray &b,const Layout &r){
    QVector<quint32> out;for(quint32 i=2;i<r.indexCount;++i){quint32 a=qFromLittleEndian<quint16>(b.constData()+r.indices+2*(i-2)),c=qFromLittleEndian<quint16>(b.constData()+r.indices+2*i),d=qFromLittleEndian<quint16>(b.constData()+r.indices+2*(i-1));if(a==d||d==c||a==c)continue;if(i&1)std::swap(a,d);out<<a<<d<<c;}return out;
}
inline float halfValue(quint16 v){const int e=(v>>10)&31,m=v&1023;double f=e==0?std::ldexp(double(m),-24):e==31?(m?NAN:INFINITY):std::ldexp(1.+double(m)/1024,e-15);return float(v&0x8000?-f:f);}
inline bool validUvs(const QByteArray &b,const Layout &r){for(quint32 i=0;i<r.vertexCount;++i)for(int k=0;k<2;++k)if(!std::isfinite(halfValue(qFromLittleEndian<quint16>(b.constData()+r.vertices+qint64(i)*20+16+2*k))))return false;return true;}
// Names belong to the exact serialized image object, not to a global hash/nearest-offset lookup.
inline QString imageName(const QByteArray &b,qsizetype f,qsizetype first){
    for(qsizetype p=qMax(first,f-212);p+12<f;++p){
        if(!marker(b,p)||word(b,p+4)!=2)continue;auto n=word(b,p+8);if(n<4||n>128||p+12+n+68!=f)continue;
        auto end=p+12+n;if(b.mid(end,4)!=QByteArray("RA\1\0",4))continue;
        bool ascii=true;for(auto ch:b.mid(p+12,n))if(uchar(ch)<32||uchar(ch)>126){ascii=false;break;}
        if(ascii)return QString::fromLatin1(b.constData()+p+12,n);
    }return {};
}
inline QJsonObject diffusePreview(const QJsonArray &list){
    QJsonObject found;for(auto value:list){auto row=value.toObject();auto name=row["blurSourceName"].toString().toLower();if(!name.endsWith("_diff")&&!name.endsWith("_diffuse")&&!name.endsWith("_albedo"))continue;if(!found.isEmpty())return {};found=row;}return found;
}

inline bool streamHeader(const QByteArray &b,qsizetype p){return p>=0&&p<=b.size()-24&&b.mid(p,4)=="ARCH"&&word(b,p+4)==1&&word(b,p+8)==0&&b.mid(p+12,4)=="ARCH"&&word(b,p+16)==0&&word(b,p+20)==10;}
inline QJsonArray streamImages(const QByteArray &b,const std::atomic_bool &cancel){
    QJsonArray out;if(!streamHeader(b,0)||cancel.load())return out;
    QVector<qsizetype> segments;for(qsizetype p=0;p+24<=b.size();p+=2048){if(cancel.load())return {};if(streamHeader(b,p))segments<<p;}segments<<b.size();
    for(int segment=0;segment+1<segments.size()&&out.size()<128;++segment){auto end=segments[segment+1];
        for(qsizetype f=segments[segment]+28;f+24<end&&out.size()<128;++f){
            if((f&4095)==0&&cancel.load())return {};auto format=b.mid(f,4);if(format!="DXT1"&&format!="DXT3"&&format!="DXT5")continue;
            auto size=word(b,f-20),w=word(b,f-16),h=word(b,f-12),depth=word(b,f-8),mips=word(b,f-4);
            if(size!=28||!w||!h||w>16384||h>16384||(w&(w-1))||(h&(h-1))||depth!=1||!mips||mips>15||word(b,f+4)!=0xffffffff||word(b,f+8)!=3||!marker(b,f+12)||word(b,f+20)!=16)continue;
            const quint64 block=format=="DXT1"?8:16;auto bytes=[&](quint32 x,quint32 y){return quint64(qMax(1u,(x+3)/4))*qMax(1u,(y+3)/4)*block;};
            quint64 full=0;for(quint32 i=0;i<mips;++i)full+=bytes(qMax(1u,w>>i),qMax(1u,h>>i));auto stored=word(b,f+16);
            if(stored!=bytes(w,h)&&stored!=full)continue;qsizetype payload=-1;int matches=0;
            for(int pad=0;pad<32;++pad){auto p=f+24+pad;if(p>end-qint64(stored)-4)break;if(pad&&b[p-1])break;auto next=p+stored;if(b.mid(next,2)=="RA"&&qFromLittleEndian<quint16>(b.constData()+next+2)<=4){payload=p;++matches;}}
            if(matches!=1)continue;
            out.append(QJsonObject{{"name",QString("Blur / ARCH %1 / texture at 0x%2").arg(segment).arg(f,0,16)},{"basis","Bounded split RA texture payload; source offset identity retained, original material binding unresolved. Image list limited to 128 records."},{"width",int(w)},{"height",int(h)},{"settings",QJsonObject{{"offset",QString::number(payload)},{"width",int(w)},{"height",int(h)},{"format",format=="DXT1"?"BC1":format=="DXT3"?"BC2":"BC3"},{"layout","Linear"},{"byteSwap","None"}}}});
            f=payload+stored-1;
        }
    }return out;
}

inline QJsonArray images(const QByteArray &b,const std::atomic_bool &cancel){
    QJsonArray out;Layout container;if(cancel.load())return out;if(streamHeader(b,0))return streamImages(b,cancel);
    if(b.size()<36||b.left(12)!=QByteArray("  CPModel\0\0\0",12)||!children(b,20,b.size(),container)||!container.scene)return out;
    const quint64 rootEnd=quint64(word(b,12))+4;if(rootEnd>quint64(b.size())||(rootEnd!=quint64(b.size())&&b.mid(rootEnd,8)!=QByteArray("MtaDbg\0\0",8)))return out;
    for(qsizetype f=container.scene+24;f+12<container.end&&out.size()<128;++f){
        if((f&4095)==0&&cancel.load())return {};
        if(b.mid(f,3)!="DXT")continue;auto format=b.mid(f,4);if(format!="DXT1"&&format!="DXT3"&&format!="DXT5")continue;
        auto size=word(b,f-20),w=word(b,f-16),h=word(b,f-12),depth=word(b,f-8),mips=word(b,f-4);
        if(!w||!h||w>16384||h>16384||(w&(w-1))||(h&(h-1))||depth!=1||!mips||mips>15||word(b,f+4)!=0||word(b,f+8)!=3)continue;
        quint64 payload=0;for(quint32 level=0;level<mips;++level)payload+=quint64(qMax(1u,(qMax(1u,w>>level)+3)/4))*qMax(1u,(qMax(1u,h>>level)+3)/4)*(format=="DXT1"?8:16);
        if(payload+28!=size||payload>quint64(container.end-f-12))continue;
        const auto sourceName=imageName(b,f,container.scene);
        out.append(QJsonObject{{"blurSourceName",sourceName},{"name",sourceName.isEmpty()?QString("Blur / inline texture %1 / 0x%2").arg(out.size()).arg(word(b,f-24),8,16,QChar('0')):QString("Blur / %1").arg(sourceName)},{"basis","Bounded inline Blur DXT record with exact mip-chain size; material association pending."},{"width",int(w)},{"height",int(h)},{"settings",QJsonObject{{"offset",QString::number(f+12)},{"width",int(w)},{"height",int(h)},{"format",format=="DXT1"?"BC1":format=="DXT3"?"BC2":"BC3"},{"layout","Linear"},{"byteSwap","None"}}}});
        f+=qsizetype(payload)+11;
    }return out;
}

inline QJsonObject scan(const QByteArray &b,const std::atomic_bool &cancel){
    if(streamHeader(b,0))return {{"candidates",QJsonArray()},{"images",streamImages(b,cancel)},{"notes",QJsonArray{"Blur ARCH stream: split image records supported (first 128 listed); scene geometry/material references are not yet reconstructed."}}};
    if(b.left(12)!=QByteArray("  CPModel\0\0\0",12))return {};
    auto r=inspect(b,cancel);auto imageRows=images(b,cancel);const auto named=diffusePreview(imageRows);QJsonArray candidates,notes;
    for(int i=0;i<r.draws.size();++i){auto selected=drawLayout(r,i);auto tris=faces(b,selected);if(tris.isEmpty())continue;
        const bool multi=r.draws.size()>1,preview=!multi&&validUvs(b,selected)&&!named.isEmpty();
        candidates.append(QJsonObject{{"name",multi?QString("Blur / local section %1 of %2").arg(i+1).arg(r.draws.size()):QString("Blur / static single draw")},{"profileId",ProfileId},{"profileName","Blur PC static ARCH"},{"nativeDecoder",true},{"objectOffset",drawIdentity(r,i)},{"blurDrawIndex",i},{"vertexCount",qint64(selected.vertexCount)},{"triangleCount",tris.size()/3},{"textureAssigned",preview},{"blurNamedAlbedoPreview",preview},{"basis",multi?"Independent triangle-strip section with validated index range and stream byte offset. Local coordinates; object transforms/states and per-draw materials unresolved.":"Single static draw with Float16 UV0; unique diffuse-named image is an approximate preview, not an original GPU binding."},{"settings",QJsonObject{{"vertexOffset",QString::number(selected.vertices)},{"vertexCount",qint64(selected.vertexCount)},{"indexCount",qint64(selected.indexCount)}}}});
    }
    notes.append(r.vertexCount?"Blur stride20 geometry recovered. Multi-draw results are independent local sections; Render All does not resolve original dynamic states/placements. Per-draw material links remain pending.":"Blur CPModel found; this ARCH/multiple-buffer or skinned layout is not supported yet. No geometry guessed.");
    return {{"candidates",candidates},{"notes",notes},{"images",imageRows}};
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &c,const std::atomic_bool &cancel){
    MeshResult result;auto fail=[&](QString e){result.error=e;result.mesh.reset();return result;};
    if(source.expectedSize<0||source.expectedSize>qMin<qint64>(AssetMemory::bytes()/4,128*1024*1024))return fail("Blur source exceeds the supported input budget.");
    QByteArray b;for(qint64 p=0;p<source.expectedSize;){if(cancel.load())return fail("Blur decoding canceled.");auto req=source;req.offset=p;req.length=qMin<qint64>(1048576,source.expectedSize-p);auto page=readPage(req);if(!page.error.isEmpty()||page.bytes.size()!=req.length)return fail(page.error.isEmpty()?"Incomplete Blur source.":page.error);applyPatches(page.bytes,p,patches);b+=page.bytes;p+=page.bytes.size();}
    auto full=inspect(b,cancel);int draw=c["blurDrawIndex"].toInt(0);if(!full.vertexCount||draw<0||draw>=full.draws.size()||c["objectOffset"].toString()!=drawIdentity(full,draw))return fail("Blur candidate changed or layout unsupported. Scan again.");auto r=drawLayout(full,draw);
    auto mesh=std::make_shared<MeshData>();mesh->hasUvs=validUvs(b,r);mesh->triangles=faces(b,r);if(mesh->triangles.isEmpty())return fail("Blur draw has no nondegenerate triangles.");
    for(quint32 i=0;i<r.vertexCount;++i){MeshVertex v{};auto p=r.vertices+qint64(i)*20;for(int k=0;k<3;++k)v.position[k]=real(b,p+4*k);if(mesh->hasUvs)for(int k=0;k<2;++k)v.uv[k]=halfValue(qFromLittleEndian<quint16>(b.constData()+p+16+2*k));mesh->vertices.append(v);mesh->vertexOffsets.append(p);}
    // Remove unused prefix/hole vertices so local sections frame and export only their own data.
    if(full.draws.size()>1){QVector<qint32> remap(mesh->vertices.size(),-1);QVector<MeshVertex> used;QVector<qint64> offsets;
        for(auto &index:mesh->triangles){if(remap[index]<0){remap[index]=used.size();used.append(mesh->vertices[index]);offsets.append(mesh->vertexOffsets[index]);}index=quint32(remap[index]);}
        mesh->vertices=std::move(used);mesh->vertexOffsets=std::move(offsets);
    }
    QVector<std::array<double,3>> sums(mesh->vertices.size());
    for(qsizetype i=0;i<mesh->triangles.size();i+=3){auto a=mesh->triangles[i],d=mesh->triangles[i+1],cidx=mesh->triangles[i+2];auto &av=mesh->vertices[a],&dv=mesh->vertices[d],&cv=mesh->vertices[cidx];double u[3],v[3];for(int k=0;k<3;++k){u[k]=double(dv.position[k])-av.position[k];v[k]=double(cv.position[k])-av.position[k];}std::array<double,3> n{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};for(auto idx:{a,d,cidx})for(int k=0;k<3;++k)sums[idx][k]+=n[k];MeshFaceSource f;f.strip=0;f.offsets={mesh->vertexOffsets[a],mesh->vertexOffsets[d],mesh->vertexOffsets[cidx]};f.indexSlots={-1,-1,-1};mesh->faceSources.append(f);}
    for(qsizetype i=0;i<mesh->vertices.size();++i){auto n=sums[i];double len=std::hypot(n[0],n[1],n[2]);for(int k=0;k<3;++k)mesh->vertices[i].normal[k]=len>1e-12?float(n[k]/len):float(k==1);}
    if(cancel.load())return fail("Blur decoding canceled.");auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return fail(final.error);
    result.mesh=mesh;result.surface=std::make_shared<MeshSurface>();result.surface->material.name="Blur static draw";result.surface->material.sourceBindings=QJsonObject{{"format","Blur PC CPModel"},{"textureAssigned",false},{"gameShaderApplied",false},{"normalsGenerated",true},
{"sourceBasisPreserved",true},{"uvDecoded",mesh->hasUvs},{"uvFormat","Float16x2, offset16 stride20; source coordinates retained"},{"textureWrapU","Repeat"},{"textureWrapV","Repeat"}};
    auto &sourceBindings=result.surface->material.sourceBindings;sourceBindings["drawIndex"]=draw;sourceBindings["drawCount"]=full.draws.size();sourceBindings["streamByteOffset"]=qint64(full.draws[draw].streamOffset);sourceBindings["firstIndex"]=qint64(full.draws[draw].first);sourceBindings["objectTransformsResolved"]=full.draws.size()==1;sourceBindings["localSectionPreview"]=full.draws.size()>1;
    const auto named=mesh->hasUvs&&full.draws.size()==1?diffusePreview(images(b,cancel)):QJsonObject();
    if(!named.isEmpty()){
        auto settings=named["settings"].toObject();ImageRecipe recipe;recipe.format=settings["format"].toString();recipe.width=settings["width"].toInt();recipe.height=settings["height"].toInt();recipe.offset=0;
        QString error;auto span=imageSpan(recipe,error);const auto at=settings["offset"].toString().toLongLong();
        if(error.isEmpty()&&at>=0&&at<=b.size()&&span<=b.size()-at)result.surface->texture=decodeTextureData(b.mid(at,span),{},recipe,error,cancel);
        if(!result.surface->texture.isNull()){
            result.surface->textureName=named["blurSourceName"].toString();result.surface->textureLibrary[result.surface->textureName]=result.surface->texture;
            result.surface->material.unlit=true;auto &bindings=result.surface->material.sourceBindings;bindings["textureAssigned"]=true;bindings["textureAssignmentVerified"]=false;bindings["previewLighting"]="Unlit named-albedo preview";bindings["textureAssignmentMethod"]="Unique diffuse-named serialized image in this single-draw file; original GPU binding unresolved";bindings["sourceImageOffset"]=QString::number(at);bindings["sourceImageName"]=result.surface->textureName;
        }
    }
    if(cancel.load())return fail("Blur decoding canceled.");return result;
}
}
