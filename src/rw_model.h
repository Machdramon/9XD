#pragma once
#include "rw_core.h"
#include "rw_txd.h"
#include "selene_mix.h"
#include "binary_document.h"
#include "mesh_data.h"
#include "mesh_surface.h"
#include "memory_budget.h"
#include <QCache>
#include <QMutex>
#include <QMutexLocker>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <cmath>
// RenderWare primary surfaces are partitioned by material texture name. Each
// recovered triangle belongs to exactly one default surface; extra TXD liveries
// are optional alternatives. No arbitrary image is assigned to unresolved names.
namespace RenderWare {
inline constexpr auto ProfileId="native-renderware";
inline qint64 sceneEnd(const QByteArray &bytes,qint64 at){auto next=bytes.indexOf("Selene Scene File",at+17);return next<0?bytes.size():next;}
inline QString materialName(const RwCore::Part &part,size_t triangle){int m=triangle<part.triangleMaterial.size()?part.triangleMaterial[triangle]:-1;return m>=0&&m<int(part.textures.size())?QString::fromStdString(part.textures[m]):QString();}
inline QMap<QString,qint64> batches(const RwCore::File &file,int group){QMap<QString,qint64> out;for(int gi=0;gi<int(file.groups.size());++gi)if(group<0||gi==group)for(const auto &p:file.groups[gi].parts)for(size_t i=0;i<p.triangles.size();++i)++out[materialName(p,i)];return out;}
inline QString hash(const QByteArray &bytes){return QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());}
inline QList<RwTxd::Entry> matches(const QList<RwTxd::Entry> &entries,const QString &name,qint64 start,qint64 end){QList<RwTxd::Entry> out;QSet<QByteArray> seen;for(const auto &e:entries)if(!name.isEmpty()&&e.name.compare(name,Qt::CaseInsensitive)==0&&e.dictionary>=start&&e.dictionaryEnd<=end&&!seen.contains(e.identity)){seen.insert(e.identity);out.append(e);}return out;}
inline QJsonObject scan(const QByteArray &bytes,const std::atomic_bool &cancel){
    if(bytes.size()<12||cancel.load())return {};auto archive=SeleneMix::prepare(bytes,cancel);const auto *d=reinterpret_cast<const uint8_t *>(archive.bytes.constData());auto textures=archive.textures;QJsonArray meshes,notes;
    auto add=[&](const RwCore::File &file,int group,qint64 start,qint64 end,const SeleneMix::Resource *resource=nullptr){
        const auto groups=batches(file,group);auto sourceHash=hash(bytes.mid(start,end-start));qint64 verts=0;for(int g=0;g<int(file.groups.size());++g)if(group<0||g==group)for(const auto &p:file.groups[g].parts)verts+=p.vertices.size();
        for(auto it=groups.begin();it!=groups.end();++it){if(cancel.load())return;auto options=resource?SeleneMix::linked(archive,it.key(),start,end):matches(textures,it.key(),start,end);bool hasUvs=true;for(int gi=0;gi<int(file.groups.size());++gi)if(group<0||gi==group)for(const auto &part:file.groups[gi].parts)for(size_t ti=0;ti<part.triangles.size();++ti)if(materialName(part,ti)==it.key())hasUvs&=part.hasUv;const int count=qMax(1,int(options.size()));
            for(int v=0;v<count;++v){const bool bound=hasUvs&&!options.isEmpty()&&options[v].supported();auto label=it.key().isEmpty()?QString("Untextured"):it.key();QJsonObject c{{"name",QString("RW %1 @0x%2 / %3%4").arg(group>=0?"object":"scene").arg(group>=0?file.groups[group].at:start,0,16).arg(label).arg(count>1?QString(" / TXD variant %1").arg(v):QString())},{"profileId",ProfileId},{"profileName","RenderWare PS2 material"},{"nativeDecoder",true},
                    {"rangeStart",QString::number(start)},{"rangeEnd",QString::number(end)},{"group",group},{"objectOffset",QString::number(group>=0?file.groups[group].at:start)},{"rwMaterialKey",it.key()},{"rwScopeHash",sourceHash},{"rwTextureVariant",v},{"rwTextureVariantCount",count},{"optionalVariant",v>0},{"triangleCount",it.value()},{"vertexCount",verts},{"textureExpected",!it.key().isEmpty()},{"textureAssigned",bound},
                    {"textureNames",QJsonArray{it.key()}},{"basis",bound?"Material-name match inside this source dictionary/scene; primary base-mip texture. TXD variant 0 is the default preview; other liveries are optional.":it.key().isEmpty()?"Geometry without a named texture.":"Named material has no supported TXD image inside this source scope; geometry retained."},{"settings",QJsonObject{{"vertexOffset",QString::number(group>=0?file.groups[group].at:start)},{"vertexCount",verts},{"indexCount",it.value()*3}}}};
                if(resource){c["rwArchiveResource"]=resource->name;c["name"]=resource->name+" / "+label+(count>1?QString(" / texture set %1").arg(v+1):QString());bool alternate=resource->name.endsWith(".BSP")||SeleneMix::base(resource->name).endsWith("_RAIN");if(resource->name.endsWith(".SLF")){auto components=resource->name.split('/');auto owner=components.value(0)+"/"+components.value(1)+"/";for(const auto &other:archive.resources)if(other.name.startsWith(owner)&&other.name.contains("/PS2/")&&other.name.endsWith(".SLI")&&SeleneMix::base(other.name)==SeleneMix::base(resource->name))alternate=true;}c["rwGeometryAlternative"]=alternate;c["optionalVariant"]=alternate||v>0;}
                if(!options.isEmpty()){c["rwTextureOffset"]=QString::number(options[v].at);c["rwTextureIdentity"]=QString::fromLatin1(options[v].identity.toHex());}meshes.append(c);
            }
        }
    };
    if(!archive.resources.isEmpty()){
        for(const auto &r:archive.resources){if(cancel.load())return {};if(!r.name.endsWith(".SLI")&&!r.name.endsWith(".SLF")&&!r.name.endsWith(".BSP"))continue;bool selene=archive.bytes.mid(r.at,17)=="Selene Scene File";auto file=selene?RwCore::scanRange(d,archive.bytes.size(),r.at,r.end()):RwCore::readAt(d,archive.bytes.size(),r.at,r.end());if(file.groups.empty())continue;if(selene)add(file,-1,r.at,r.end(),&r);else for(int i=0;i<int(file.groups.size());++i)add(file,i,r.at,r.end(),&r);}
        for(const auto &warning:archive.warnings)notes.append(warning);
        notes.append("MIX original paths recovered. Texture banks are linked within the same named vehicle/track resource; weather and alternative platform/BSP geometry are optional.");
    }else{auto file=RwCore::read(d,bytes.size());if(!file.groups.empty()){for(int i=0;i<int(file.groups.size());++i){if(cancel.load())return {};add(file,i,0,bytes.size());}}
        else for(qint64 at=bytes.indexOf("Selene Scene File");at>=0;at=bytes.indexOf("Selene Scene File",at+17)){if(cancel.load())return {};auto end=sceneEnd(bytes,at);auto scene=RwCore::scanRange(d,bytes.size(),at,end);if(!scene.groups.empty())add(scene,-1,at,end);}}
    if(meshes.isEmpty()&&textures.isEmpty())return {};if(cancel.load())return {};
    int defaults=0,textured=0;for(auto v:meshes){auto c=v.toObject();if(!c["optionalVariant"].toBool()){++defaults;textured+=c["textureAssigned"].toBool();}}
    notes.append(QString("RenderWare: %1 primary material surfaces (%2 with embedded TXD images), %3 PS2 raster records. Additional dictionary variants are optional; Render All loads the default set without duplicate triangles.").arg(defaults).arg(textured).arg(textures.size()));
    notes.append("Base mip and primary texture only. Game shaders and Selene car-part placement are not applied. A missing or unsupported image is reported without borrowing another scene's texture.");
    QJsonArray images;const auto archiveHash=archive.resources.isEmpty()?QString():hash(bytes);for(const auto &e:textures)if(e.supported()){
        if(e.at<bytes.size()){images.append(RwTxd::images({e}).first());continue;}
        const auto path=archive.texturePaths.value(e.dictionary);const SeleneMix::Resource *resource=nullptr;for(const auto &r:archive.resources)if(r.name==path)resource=&r;if(!resource)continue;
        images.append(QJsonObject{{"name",path+" / "+e.name},{"profileId","native-rw-mix-texture"},{"width",e.width},{"height",e.height},{"offset",QString::number(resource->at)},{"rwTextureOffset",QString::number(e.at)},{"rwTextureIdentity",QString::fromLatin1(e.identity.toHex())},{"rwArchiveTexture",path},{"rwArchiveHash",archiveHash},{"basis","Internally LZW-compressed TXD inside the named MIX resource; preview extracts in RAM."}});
    }
    return {{"candidates",meshes},{"images",images},{"notes",notes},{"recognized",true}};
}
struct Session {QMutex mutex;ReadRequest source;QByteArray patchHash,bytes;SeleneMix::Assets archive;bool ready=false;QMap<qint64,RwCore::File> scenes;QList<RwTxd::Entry> textures;QMap<qint64,QString> scopeHashes;QCache<QString,QImage> images;};
inline QByteArray patchHash(const PatchList &patches){QCryptographicHash hash(QCryptographicHash::Sha256);for(const auto &p:patches){hash.addData(QByteArray::number(p.offset));hash.addData(":");hash.addData(QByteArray::number(p.bytes.size()));hash.addData(":");hash.addData(p.bytes);}return hash.result();}
inline bool load(Session &s,ReadRequest source,const PatchList &patches,const std::atomic_bool &cancel,QString &error){auto reject=[&](QString text){error=text;return false;};const auto patchesHash=patchHash(patches);
    if(!s.ready||s.source.path!=source.path||s.source.memory!=source.memory||s.source.expectedSize!=source.expectedSize||s.source.expectedModified!=source.expectedModified||s.patchHash!=patchesHash){
        s.ready=false;s.bytes.clear();s.scenes.clear();s.textures.clear();s.scopeHashes.clear();s.images.clear();s.archive={};
        if(source.expectedSize<=0||source.expectedSize>AssetMemory::bytes()/4)return reject("RenderWare source exceeds the configured read budget.");
        for(qint64 offset=0;offset<source.expectedSize;){if(cancel.load())return reject("RenderWare decoding canceled.");auto r=source;r.offset=offset;r.length=qMin(qint64(1048576),source.expectedSize-offset);auto page=readPage(r);if(!page.error.isEmpty())return reject(page.error);if(page.bytes.size()!=r.length)return reject("Incomplete RenderWare read.");applyPatches(page.bytes,offset,patches);s.bytes+=page.bytes;offset+=page.bytes.size();}
        s.archive=SeleneMix::prepare(s.bytes,cancel);s.textures=s.archive.textures;if(cancel.load())return reject("RenderWare decoding canceled.");s.source=source;s.patchHash=patchesHash;s.ready=true;
    }
    return true;
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel,Session *shared=nullptr){
    MeshResult result;auto fail=[&](QString e){result.error=e;result.mesh.reset();return result;};Session local;auto &s=shared?*shared:local;QMutexLocker lock(&s.mutex);QString error;if(!load(s,source,patches,cancel,error))return fail(error);
    const auto &bytes=s.archive.bytes;const auto *d=reinterpret_cast<const uint8_t *>(bytes.constData());bool a,b;const auto start=candidate["rangeStart"].toString().toLongLong(&a),end=candidate["rangeEnd"].toString().toLongLong(&b);const int group=candidate["group"].toInt(-2);
    if(!a||!b||start<0||start>=end||end>s.bytes.size()||group<-1)return fail("RenderWare source scope is no longer valid. Scan again.");
    const auto *resource=SeleneMix::resourceAt(s.archive,start,end);bool archived=candidate.contains("rwArchiveResource");
    if(archived){if(!resource||resource->name!=candidate["rwArchiveResource"].toString())return fail("MIX resource scope changed. Scan again.");}
    else if((group>=0&&(start!=0||end!=s.bytes.size()))||(group<0&&(bytes.mid(start,17)!="Selene Scene File"||end!=sceneEnd(s.bytes,start))))return fail("RenderWare scope boundaries changed. Scan again.");
    if(!s.scopeHashes.contains(start))s.scopeHashes[start]=hash(s.bytes.mid(start,end-start));if(candidate.contains("rwScopeHash")&&candidate["rwScopeHash"]!=s.scopeHashes[start])return fail("RenderWare source changed. Scan again.");
    if(!s.scenes.contains(start))s.scenes[start]=archived?(bytes.mid(start,17)=="Selene Scene File"?RwCore::scanRange(d,bytes.size(),start,end):RwCore::readAt(d,bytes.size(),start,end)):(group>=0?RwCore::read(d,bytes.size()):RwCore::scanRange(d,bytes.size(),start,end));const auto &file=s.scenes[start];
    if(group>=int(file.groups.size())||(group>=0&&QString::number(file.groups[group].at)!=candidate["objectOffset"].toString()))return fail("RenderWare object is no longer valid. Scan again.");
    const QString material=candidate["rwMaterialKey"].toString();const bool partition=candidate.contains("rwMaterialKey");auto mesh=std::make_shared<MeshData>();mesh->hasUvs=true;int strip=0,repaired=0;
    for(int gi=0;gi<int(file.groups.size());++gi)if(group<0||gi==group)for(const auto &p:file.groups[gi].parts){if(cancel.load())return fail("RenderWare decoding canceled.");QMap<quint32,quint32> remap;bool used=false;
        for(size_t ti=0;ti<p.triangles.size();++ti){if(partition&&materialName(p,ti)!=material)continue;used=true;MeshFaceSource face;face.strip=strip;face.indexSlots={-1,-1,-1};for(int k=0;k<3;++k){auto idx=p.triangles[ti][k];if(idx>=p.vertices.size())return fail("RenderWare index outside its vertex array.");if(!remap.contains(idx)){
                    if(!AssetMemory::fits(mesh->vertices.size()+1,mesh->triangles.size()+3))return fail("RenderWare material exceeds the configured memory budget.");MeshVertex x{};const auto &v=p.vertices[idx];for(int n=0;n<3;++n){x.position[n]=v.position[n];x.normal[n]=v.normal[n];if(!std::isfinite(x.position[n]))return fail("Nonfinite RenderWare position data.");}
                    if(!std::isfinite(x.normal[0])||!std::isfinite(x.normal[1])||!std::isfinite(x.normal[2])){for(auto &n:x.normal)n=0;++repaired;}for(int n=0;n<2;++n){x.uv[n]=v.uv[n];if(!std::isfinite(x.uv[n]))return fail("Nonfinite RenderWare UV data.");}
                    remap[idx]=mesh->vertices.size();mesh->vertices.append(x);mesh->vertexOffsets.append(p.source);
                }mesh->triangles.append(remap[idx]);face.offsets[k]=p.source;}mesh->faceSources.append(face);
        }if(used)mesh->hasUvs&=p.hasUv;++strip;
    }
    if(mesh->triangles.isEmpty())return fail("No triangles match this RenderWare material. Scan again.");
    QVector<std::array<double,3>> sums(mesh->vertices.size());for(int i=0;i<mesh->triangles.size();i+=3){auto &a=mesh->vertices[mesh->triangles[i]],&b=mesh->vertices[mesh->triangles[i+1]],&c=mesh->vertices[mesh->triangles[i+2]];double u[3],v[3];for(int k=0;k<3;++k){u[k]=double(b.position[k])-a.position[k];v[k]=double(c.position[k])-a.position[k];}std::array<double,3> n{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};for(int j=0;j<3;++j)for(int k=0;k<3;++k)sums[mesh->triangles[i+j]][k]+=n[k];}
    for(int i=0;i<mesh->vertices.size();++i){auto &v=mesh->vertices[i];double length=std::hypot(v.normal[0],v.normal[1],v.normal[2]);auto n=std::array<double,3>{v.normal[0],v.normal[1],v.normal[2]};if(length<1e-6){n=sums[i];length=std::hypot(n[0],n[1],n[2]);}if(length<1e-12){v.normal[0]=0;v.normal[1]=1;v.normal[2]=0;}else for(int k=0;k<3;++k)v.normal[k]=float(n[k]/length);}
    result.surface=std::make_shared<MeshSurface>();auto &surface=*result.surface;surface.material.name=candidate["name"].toString();QJsonObject bindings=candidate;bindings["format"]="RenderWare PS2";bindings["sourceUvTopLeft"]=true;bindings["gameShaderApplied"]=false;bindings["repairedNormalCount"]=repaired;
    if(repaired)bindings["normalWarning"]=QString("Regenerated %1 nonfinite source normals from decoded faces.").arg(repaired);
    bindings["textureExpected"]=!material.isEmpty();auto options=archived?SeleneMix::linked(s.archive,material,start,end):matches(s.textures,material,start,end);const RwTxd::Entry *selected=nullptr;for(const auto &e:options)if(QString::number(e.at)==candidate["rwTextureOffset"].toString()&&QString::fromLatin1(e.identity.toHex())==candidate["rwTextureIdentity"].toString())selected=&e;
    if(candidate.contains("rwTextureOffset")&&!selected)return fail("TXD binding changed or belongs to another source scope. Scan again.");
    if(selected&&mesh->hasUvs){const auto key=QString::fromLatin1(selected->identity.toHex());s.images.setMaxCost(int(qMax<qint64>(1,AssetMemory::textureBytes()/1024)));if(auto image=s.images.object(key))surface.texture=*image;else{QString error;surface.texture=RwTxd::decode(bytes,*selected,error,cancel);if(!surface.texture.isNull())s.images.insert(key,new QImage(surface.texture),qMax(1,int(surface.texture.sizeInBytes()/1024)));else bindings["textureStatus"]="TXD decoding failed: "+error;}
        if(!surface.texture.isNull()){surface.textureName=QString("TXD / %1 / dictionary 0x%2").arg(selected->name).arg(selected->dictionary,0,16);surface.textureLibrary[surface.textureName]=surface.texture;auto wrap=[](int code){return code==3?"Clamp":code==2?"Mirror":"Repeat";};bindings["textureWrapU"]=wrap((selected->addressing>>8)&15);bindings["textureWrapV"]=wrap((selected->addressing>>12)&15);surface.material.alphaMask=true;}
    }else bindings["textureStatus"]=material.isEmpty()?"Material has no named texture.":"No supported primary TXD image in this source dictionary/scene, or no UV stream.";
    bindings["textureAssigned"]=!surface.texture.isNull();surface.material.sourceBindings=bindings;
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return fail(final.error);if(cancel.load())return fail("RenderWare decoding canceled.");result.mesh=mesh;return result;
}
inline QImage decodeTexture(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel,QString &error,Session *shared=nullptr){Session local;auto &s=shared?*shared:local;QMutexLocker lock(&s.mutex);if(!load(s,source,patches,cancel,error))return {};if(candidate["rwArchiveHash"]!=hash(s.bytes)){error="MIX texture source changed. Scan again.";return {};}const RwTxd::Entry *selected=nullptr;for(const auto &e:s.textures)if(QString::number(e.at)==candidate["rwTextureOffset"].toString()&&QString::fromLatin1(e.identity.toHex())==candidate["rwTextureIdentity"].toString()&&s.archive.texturePaths.value(e.dictionary)==candidate["rwArchiveTexture"].toString())selected=&e;if(!selected){error="MIX texture binding no longer exists.";return {};}auto image=RwTxd::decode(s.archive.bytes,*selected,error,cancel);auto check=source;check.offset=0;check.length=0;auto verified=readPage(check);if(!verified.error.isEmpty()){error=verified.error;return {};}return image;}

}
