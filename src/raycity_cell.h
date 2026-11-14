#pragma once
#include "mesh_surface.h"
#include "mesh_data.h"
#include "memory_budget.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QtEndian>
#include <QVector3D>
#include <cmath>
#include <cstring>
#include "image_containers.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <zlib.h>

// Independently measured SCRIPTCE v1 cells and named texture banks.
// v3 vehicle serialization is separate and remains unsupported.
namespace RayCity {
inline constexpr auto ProfileId="native-raycity-cell";
inline QByteArray signature(int version){return QString("SCRIPTCE Data Format v%1.0s").arg(version).toUtf8();}
inline QByteArray magic(int version){auto text=signature(version);QByteArray b;for(char c:text){b.append(c);b.append(char(0));}return b;}
inline bool recognized(const QByteArray &b){return b.startsWith(magic(1))||b.startsWith(magic(3));}
inline quint32 u32(const QByteArray &b,qint64 p){return qFromLittleEndian<quint32>(b.constData()+p);}
inline quint16 u16(const QByteArray &b,qint64 p){return qFromLittleEndian<quint16>(b.constData()+p);}
inline double real(const QByteArray &b,qint64 p){auto bits=u32(b,p);float f;std::memcpy(&f,&bits,4);return f;}
struct Part {qint64 descriptor=0;quint32 vertexStart=0,indexStart=0;quint16 vertices=0,indices=0;bool uv=true;};
struct Layout {bool valid=false;QString error;quint32 vertices=0,indices=0;qint64 vertexBuffer=0,indexBuffer=0;QVector<Part> parts;};
inline std::array<double,3> world(const QByteArray &b,const Part &part,qint64 at){std::array<double,3> p{};for(int a=0;a<3;++a){p[a]=real(b,part.descriptor+a*16+12);for(int k=0;k<3;++k)p[a]+=real(b,part.descriptor+a*16+k*4)*real(b,at+k*4);}return p;}
inline Layout inspect(const QByteArray &b,const std::atomic_bool &cancel){
    Layout r;auto fail=[&](QString e){r.error=e;r.parts.clear();return r;};
    if(cancel.load())return fail("RayCity scan canceled.");
    if(!b.startsWith(magic(1)))return fail("RayCity SCRIPTCE v3 vehicle/object serialization is not decoded yet.");
    if(b.size()<0x325||b.mid(0x300,8)!=QByteArray::fromHex("aa47a002220a0000")||b.mid(0x317,8)!=QByteArray::fromHex("aa471504f1170200"))return fail("Unsupported RayCity v1 object graph.");
    const quint32 count=u16(b,0x31f);const qint64 tableEnd=0x321+qint64(count)*112;
    if(!count||count>4096||tableEnd>b.size()-4)return fail("Truncated RayCity part table.");
    r.vertices=u32(b,tableEnd);r.vertexBuffer=tableEnd+4;
    if(!r.vertices||r.vertices>2000000||qint64(r.vertices)*40>b.size()-r.vertexBuffer-4)return fail("Invalid RayCity vertex buffer.");
    const auto end=r.vertexBuffer+qint64(r.vertices)*40;r.indices=u32(b,end);r.indexBuffer=end+4;
    if(!r.indices||r.indices>12000000||r.indices%3||qint64(r.indices)*2>b.size()-r.indexBuffer)return fail("Invalid RayCity index buffer.");
    quint32 nextVertex=0,nextIndex=0;
    for(quint32 i=0;i<count;++i){
        if(cancel.load())return fail("RayCity scan canceled.");
        Part p;p.descriptor=0x321+qint64(i)*112;p.vertices=u16(b,p.descriptor+98);p.vertexStart=u32(b,p.descriptor+100);p.indices=u16(b,p.descriptor+104);p.indexStart=u32(b,p.descriptor+108);
        if(p.vertices<3||p.indices<3||p.indices%3||p.vertexStart!=nextVertex||p.indexStart!=nextIndex||p.vertices>r.vertices-p.vertexStart||p.indices>r.indices-p.indexStart)return fail("Invalid RayCity part ranges.");
        for(int k=0;k<18;++k)if(!std::isfinite(real(b,p.descriptor+k*4)))return fail("Nonfinite RayCity transform or bounds.");
        for(int k=0;k<3;++k)if(real(b,p.descriptor+48+k*4)>real(b,p.descriptor+60+k*4))return fail("Invalid RayCity part bounds.");
        for(quint32 j=0;j<p.vertices;++j){
            if((j&255)==0&&cancel.load())return fail("RayCity scan canceled.");const auto at=r.vertexBuffer+qint64(p.vertexStart+j)*40;
            auto xyz=world(b,p,at);for(int k=0;k<3;++k)if(!std::isfinite(xyz[k])||std::abs(xyz[k])>1e8||xyz[k]<real(b,p.descriptor+48+k*4)-.05||xyz[k]>real(b,p.descriptor+60+k*4)+.05)return fail("RayCity positions exceed their declared world bounds.");
            for(int k=0;k<2;++k){auto uv=real(b,at+24+k*4);p.uv&=std::isfinite(uv)&&std::abs(uv)<1e8;}
        }
        for(quint32 j=0;j<p.indices;++j)if(u16(b,r.indexBuffer+qint64(p.indexStart+j)*2)>=p.vertices)return fail("RayCity local face index exceeds its part.");
        nextVertex+=p.vertices;nextIndex+=p.indices;r.parts.append(p);
    }
    if(nextVertex!=r.vertices||nextIndex!=r.indices)return fail("RayCity parts do not cover the shared buffers.");r.valid=true;return r;
}

struct TextureEntry {qint64 offset=0,size=0;};
struct TextureBank {QByteArray bytes;QMap<QString,TextureEntry> entries;QString path,error;};
// Only the measured flat v1 directory variant is accepted. Never guess an image
// from its ordinal or a nearby DDS signature when directory validation fails.
inline TextureBank textureBank(const QByteArray &b,const std::atomic_bool &cancel){
    TextureBank r;auto fail=[&](QString e){r.entries.clear();r.error=e;return r;};
    if(b.size()<0x220||!b.startsWith(magic(1)))return fail("Unsupported RayCity texture bank.");
    auto descriptor=[&](qint64 at){std::array<quint32,8> d{};for(int i=0;i<8;++i)d[i]=u32(b,at+i*4)^(quint32(0xdfa47b56u)*quint32(i+1));return d;};
    auto root=descriptor(0x100);qint64 at=qint64(root[1])*256,size=root[2];
    if(root[0]!=0xffffffffu||root[4]!=5||size<8||size>1024*1024||root[3]!=size||at<0x120||at>b.size()-size||(at-0x100)/32>8192)return fail("Invalid RayCity bank directory range.");
    QMap<quint32,TextureEntry> blocks;
    for(qint64 pos=0x120;pos+32<=at;pos+=32){
        if(cancel.load())return fail("RayCity bank canceled.");if(b.mid(pos,32)==QByteArray(32,0))break;auto d=descriptor(pos);qint64 off=qint64(d[1])*256,n=d[2];
        if(d[0]==0xffffffffu||blocks.contains(d[0])||d[4]||d[2]!=d[3]||!n||off<at+size||off>b.size()-n)return fail("Invalid RayCity bank entry.");blocks[d[0]]={off,n};
    }
    auto directory=b.mid(at,size);const quint32 seed=u32(directory,0);
    for(qint64 j=0;j<size;++j){quint32 word=seed+quint32((j%64)/4)*0x8473fbc1u;directory[j]=char(uchar(directory[j])^uchar(word>>(8*(j%4))));}
    if(quint32(adler32(0,reinterpret_cast<const Bytef *>(directory.constData()),directory.size()))!=root[5]||u32(directory,0)!=0)return fail("RayCity directory checksum or folder variant is unsupported.");
    quint32 count=u32(directory,4);if(!count||count>8192||count!=blocks.size())return fail("Invalid RayCity bank file count.");qint64 pos=8;
    for(quint32 i=0;i<count;++i){
        if(cancel.load())return fail("RayCity bank canceled.");QString name;bool ended=false;
        for(int n=0;n<1024&&pos+2<=directory.size();++n){auto ch=u16(directory,pos);pos+=2;if(!ch){ended=true;break;}name+=QChar(ch);}
        if(!ended||name.isEmpty()||name.contains('/')||name.contains('\\')||name.contains("..")||pos>directory.size()-16)return fail("Invalid RayCity bank filename.");
        auto extension=u32(directory,pos),id=u32(directory,pos+8),bytes=u32(directory,pos+12);pos+=16;
        if(extension!=0x00736464||!blocks.contains(id)||blocks[id].size!=bytes||r.entries.contains(name.toCaseFolded()))return fail("Unsupported or duplicate RayCity bank filename.");
        r.entries[name.toCaseFolded()]=blocks[id];
    }
    if(pos!=directory.size())return fail("RayCity directory has unexplained trailing data.");r.bytes=b;return r;
}
inline TextureBank textureBankFile(const QString &path,const std::atomic_bool &cancel){
    TextureBank r;r.path=path;QFile f(path);QFileInfo before(f);const qint64 limit=qMin<qint64>(AssetMemory::bytes()/8,64*1024*1024);
    if(!before.isFile()||before.size()<0x220||before.size()>limit||!f.open(QIODevice::ReadOnly)){r.error="RayCity texture bank is missing or exceeds the input budget.";return r;}
    auto bytes=f.read(limit+1);QFileInfo after(path);if(bytes.size()!=before.size()||after.size()!=before.size()||after.lastModified()!=before.lastModified()){r.error="RayCity texture bank changed during reading.";return r;}
    r=textureBank(bytes,cancel);r.path=path;return r;
}
struct MaterialSlot {QString primary,baked;};
inline QVector<MaterialSlot> materialSlots(const QByteArray &b,const Layout &layout){
    QVector<MaterialSlot> materials(1);const qint64 end=layout.indexBuffer+qint64(layout.indices)*2;
    const auto marker=QByteArray::fromHex("aa479604791a");qint64 pos=b.indexOf(marker,end);
    if(pos<end+2||pos>end+1024)return {};const int count=u16(b,pos-2);if(count<2||count>4096)return {};
    QMap<quint16,QString> strings;QMap<quint16,MaterialSlot> objects;
    for(int i=1;i<count;++i){
        if(pos>b.size()-4)return {};auto token=u16(b,pos);
        if(token==0x47bb){auto id=u16(b,pos+2);pos+=4;if(!objects.contains(id))return {};materials.append(objects[id]);continue;}
        if(pos>b.size()-8||b.mid(pos,6)!=marker)return {};auto id=u16(b,pos+6);pos+=8;MaterialSlot slot;
        for(int layer=0;layer<2;++layer){
            if(pos>=b.size())return {};auto present=uchar(b[pos++]);if(present>1)return {};if(!present)continue;
            if(pos>b.size()-4)return {};auto tag=u16(b,pos),sid=u16(b,pos+2);pos+=4;
            if(tag==0x27aa){if(pos>b.size()-4)return {};auto n=u32(b,pos);pos+=4;if(n>1024||qint64(n)*2>b.size()-pos||strings.contains(sid))return {};QString name;for(quint32 j=0;j<n;++j)name+=QChar(u16(b,pos+j*2));pos+=qint64(n)*2;strings[sid]=name;}
            else if(tag!=0x27bb||!strings.contains(sid))return {};
            (layer?slot.baked:slot.primary)=strings[sid];
        }
        if(pos>b.size()-53||objects.contains(id))return {};pos+=53;objects[id]=slot;materials.append(slot);
    }return materials;
}
inline void assignTexture(MeshResult &out,const QByteArray &b,const Layout &layout,const Part &part,const ReadRequest &source,const std::atomic_bool &cancel){
    auto &surface=*out.surface;auto &bindings=surface.material.sourceBindings;auto materials=materialSlots(b,layout);quint32 index=u32(b,part.descriptor+80);
    bindings["materialSlot"]=int(index);if(index>=quint32(materials.size())){bindings["textureStatus"]="Material graph unavailable.";return;}
    auto slot=materials[index];bindings["primaryTexture"]=slot.primary;bindings["bakedTexture"]=slot.baked;surface.material.name=QString("RayCity material %1 / %2").arg(index).arg(slot.primary);
    // RAM-only sources have no trustworthy external directory. A disk-backed
    // source may resolve only its sibling bank and the game's common_<field>.
    if(source.path.isEmpty()||source.memory){bindings["textureStatus"]="No disk texture context for this RAM document.";return;}
    QFileInfo info(source.path);auto dir=info.dir();QStringList paths{dir.filePath(info.completeBaseName()+"_tex.sce")};
    const auto field=info.completeBaseName().section('_',0,0);if(!field.isEmpty())paths.append(dir.filePath("../common_"+field+"/tex.sce"));
    QStringList issues;int selected=-1;QImage primary,baked;
    for(const auto &path:paths){if(cancel.load())return;auto bank=textureBankFile(QDir::cleanPath(path),cancel);if(!bank.error.isEmpty()){issues.append(bank.error);continue;}
        for(int layer=0;layer<2;++layer){auto name=layer?slot.baked:slot.primary;auto &image=layer?baked:primary;if(name.isEmpty()||!image.isNull())continue;auto found=bank.entries.constFind(name.toCaseFolded());if(found==bank.entries.cend())continue;
            QString error;image=decodeDds(bank.bytes.mid(found->offset,found->size),ImageRecipe{},error,cancel);if(image.isNull()){issues.append(error);continue;}surface.textureLibrary[name]=image;bindings[layer?"bakedBank":"primaryBank"]=bank.path;}
    }
    // Prefer the actual primary texture and UV0. CompleteMap is a fallback baked
    // preview, not a recreation of the game's
    // two-layer shader. Keep the source names and both decoded images available.
    if(!primary.isNull()&&part.uv)selected=0;else if(!baked.isNull()&&slot.baked.endsWith("_CompleteMap",Qt::CaseInsensitive))selected=1;
    if(selected<0){bindings["textureStatus"]="Named textures unavailable or unsupported.";bindings["textureDiagnostics"]=issues.join(" ");return;}
    for(quint32 j=0;j<part.vertices;++j){auto at=layout.vertexBuffer+qint64(part.vertexStart+j)*40+24+selected*8;for(int k=0;k<2;++k){auto value=real(b,at+k*4);if(!std::isfinite(value)||std::abs(value)>1e8){bindings["textureStatus"]="Selected texture UV channel is invalid.";return;}}}
    for(quint32 j=0;j<part.vertices;++j){auto at=layout.vertexBuffer+qint64(part.vertexStart+j)*40+24+selected*8;for(int k=0;k<2;++k)out.mesh->vertices[j].uv[k]=k==1?1.0-real(b,at+k*4):real(b,at+k*4);}
    out.mesh->hasUvs=true;surface.texture=selected?baked:primary;surface.textureName=selected?slot.baked:slot.primary;surface.material.unlit=selected==1;bindings["textureAssigned"]=true;bindings["previewUvChannel"]=selected;bindings["textureStatus"]=selected?"Named CompleteMap baked preview; original two-layer shader not reproduced.":"Named primary texture preview; original shader not reproduced.";
}

inline QJsonObject scan(const QByteArray &b,const std::atomic_bool &cancel,const QString &name={}){
    QJsonArray candidates,notes,evidence;if(!recognized(b))return {};auto r=inspect(b,cancel);
    if(!r.valid){notes.append(r.error);if(b.startsWith(magic(3))&&!cancel.load())evidence.append(QJsonObject{{"family","RayCity SCRIPTCE v3"},{"diagnostic","Vehicle/object file recognized. The v3 3D importer is not implemented yet; no renderable model is available. Use Scan for Textures/Images to inspect embedded images; these may include shader masks."},{"renderSupported",false},{"previewSupported",false}});}else{
        for(int i=0;i<r.parts.size();++i){const auto &p=r.parts[i];candidates.append(QJsonObject{{"name",QString("RayCity / %1part %2").arg(name.isEmpty()?QString():name+" / ").arg(i+1)},{"profileId",ProfileId},{"profileName","RayCity SCRIPTCE v1 cell"},{"nativeDecoder",true},{"raycityPart",i},{"objectOffset",QString::number(p.descriptor)},{"vertexCount",p.vertices},{"triangleCount",p.indices/3},{"basis","Declared part ranges cover shared Float32 stride40 vertices and local UInt16 triangle lists. Source 3x4 placement applied; Z-up converted to Y-up. Named material slots resolved during decode; sibling/shared DDS banks and baked UV1 preview supported."},{"settings",QJsonObject{{"vertexOffset",QString::number(r.vertexBuffer+qint64(p.vertexStart)*40)},{"vertexCount",p.vertices},{"indexOffset",QString::number(r.indexBuffer+qint64(p.indexStart)*2)},{"indexCount",p.indices}}}});}
        notes.append(QString("RayCity cell: %1 independently placed parts, %2 vertices, %3 triangles. Texture banks use the sibling _tex.sce file; named textures are resolved from sibling and common_<field> banks during preview/export.").arg(r.parts.size()).arg(r.vertices).arg(r.indices/3));
    }return {{"candidates",candidates},{"notes",notes},{"evidence",evidence}};
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel){
    MeshResult out;auto fail=[&](QString e){out.mesh.reset();out.error=e;return out;};
    if(source.expectedSize<=0||source.expectedSize>qMin<qint64>(AssetMemory::bytes()/4,128*1024*1024))return fail("RayCity cell exceeds the supported input budget.");
    QByteArray b;for(qint64 at=0;at<source.expectedSize;){if(cancel.load())return fail("RayCity decoding canceled.");auto q=source;q.offset=at;q.length=qMin<qint64>(1048576,source.expectedSize-at);auto page=readPage(q);if(!page.error.isEmpty()||page.bytes.size()!=q.length)return fail(page.error.isEmpty()?"Incomplete RayCity source.":page.error);applyPatches(page.bytes,at,patches);b+=page.bytes;at+=page.bytes.size();}
    auto layout=inspect(b,cancel);int i=candidate["raycityPart"].toInt(-1);if(!layout.valid)return fail(layout.error);if(i<0||i>=layout.parts.size())return fail("RayCity part changed. Scan again.");const auto &p=layout.parts[i];
    if(candidate["objectOffset"].toString()!=QString::number(p.descriptor)||candidate["vertexCount"].toInt()!=p.vertices||candidate["triangleCount"].toInt()!=p.indices/3)return fail("RayCity candidate changed. Scan again.");
    auto mesh=std::make_shared<MeshData>();mesh->hasUvs=p.uv;mesh->vertices.reserve(p.vertices);
    for(quint32 j=0;j<p.vertices;++j){if(cancel.load())return fail("RayCity decoding canceled.");auto at=layout.vertexBuffer+qint64(p.vertexStart+j)*40;auto xyz=world(b,p,at);MeshVertex v{};v.position[0]=xyz[0];v.position[1]=xyz[2];v.position[2]=-xyz[1];if(p.uv)for(int k=0;k<2;++k)v.uv[k]=k==1?1.0-real(b,at+24+k*4):real(b,at+24+k*4);mesh->vertices.append(v);mesh->vertexOffsets.append(at);}
    QVector<QVector3D> normals(p.vertices);for(quint32 j=0;j<p.indices;j+=3){if(cancel.load())return fail("RayCity decoding canceled.");MeshFaceSource provenance;quint32 indices[3];for(int k=0;k<3;++k){auto at=layout.indexBuffer+qint64(p.indexStart+j+k)*2;indices[k]=u16(b,at);mesh->triangles.append(indices[k]);provenance.indexSlots[k]=at;provenance.offsets[k]=mesh->vertexOffsets[indices[k]];}provenance.strip=i;mesh->faceSources.append(provenance);auto point=[&](int k){auto &v=mesh->vertices[indices[k]];return QVector3D(v.position[0],v.position[1],v.position[2]);};auto n=QVector3D::crossProduct(point(1)-point(0),point(2)-point(0));for(auto ix:indices)normals[ix]+=n;}
    for(int j=0;j<mesh->vertices.size();++j){auto n=normals[j];if(!std::isfinite(n.lengthSquared())||n.lengthSquared()<1e-20)n=QVector3D(0,1,0);else n.normalize();for(int k=0;k<3;++k)mesh->vertices[j].normal[k]=n[k];}
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return fail(final.error);if(cancel.load())return fail("RayCity decoding canceled.");
    out.mesh=mesh;out.surface=std::make_shared<MeshSurface>();out.surface->material.name=QString("RayCity part %1").arg(i+1);out.surface->material.sourceBindings={{"format","RayCity SCRIPTCE v1"},{"part",i},{"sourcePartOffset",QString::number(p.descriptor)},{"placementsApplied",true},{"coordinateConversion","Z-up -> Y-up (x,z,-y); source winding retained"},{"uv0Decoded",p.uv},{"uv1Present",true},{"flipV",true},{"normalsGenerated",true},{"textureAssigned",false},{"gameShaderApplied",false}};assignTexture(out,b,layout,p,source,cancel);if(cancel.load())return fail("RayCity decoding canceled.");return out;
}
}
