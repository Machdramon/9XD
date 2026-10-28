#pragma once
#include "idss_core.h"
#include "binary_document.h"
#include "mesh_data.h"
#include "mesh_surface.h"
#include "memory_budget.h"
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <cmath>
// Initial D Special Stage PAC adapter: SMD geometry is in world space, so records are merged per texture across
// the whole PAC (one textured candidate per GIM name); untextured records form their own candidate.
namespace InitialDSS {
inline constexpr auto ProfileId="native-idss-pac";
struct Parsed { QMap<QString,QVector<IdssCore::Record>> byTexture,lodByTexture; QMap<QString,IdssCore::Entry> gims; int models=0,failed=0; };
inline Parsed parse(const QByteArray &bytes,const std::atomic_bool &cancel){
    Parsed out;const auto *d=reinterpret_cast<const uint8_t *>(bytes.constData());
    for(const auto &e:IdssCore::pac(d,bytes.size())){
        if(cancel.load())return {};
        if(e.type==1){out.gims.insert(QString::fromStdString(e.name),e);continue;}
        if(e.type!=3)continue;std::vector<uint8_t> raw;
        if(e.flags&1){if(!IdssCore::decompress(d+e.offset,e.size,raw)){++out.failed;continue;}}else raw.assign(d+e.offset,d+e.offset+e.size);
        auto m=IdssCore::smd(raw.data(),raw.size());if(!m.error.empty()){++out.failed;continue;}++out.models;
        for(auto &r:m.records){if(r.triangles.empty())continue;const QString key=r.texture>=0&&r.texture<int(m.textures.size())?QString::fromStdString(m.textures[r.texture]):QString();auto &groups=QString::fromStdString(e.name).contains("lod",Qt::CaseInsensitive)?out.lodByTexture:out.byTexture;groups[key].append(std::move(r));}
    }
    return out;
}
inline QJsonObject scan(const QByteArray &bytes,const std::atomic_bool &cancel){
    if(!bytes.startsWith(QByteArray("PAC\0",4)))return {};
    auto parsed=parse(bytes,cancel);if(parsed.byTexture.isEmpty()&&parsed.lodByTexture.isEmpty())return {};
    QJsonArray meshes;int textured=0;
    for(bool lod:{false,true}){const auto &groups=lod?parsed.lodByTexture:parsed.byTexture;for(auto it=groups.cbegin();it!=groups.cend();++it){qint64 t=0,v=0;for(const auto &r:it.value()){t+=r.triangles.size();v+=r.vertices.size();}
        const bool has=parsed.gims.contains(it.key());textured+=has;
        meshes.append(QJsonObject{{"name",QString("Initial D SS / %1%2").arg(it.key().isEmpty()?QString("untextured"):it.key()).arg(lod?" / LOD alternative":"")},{"profileId",ProfileId},{"profileName","Initial D Special Stage SMD"},{"nativeDecoder",true},
            {"texture",it.key()},{"idssLod",lod},{"optionalVariant",lod},{"objectOffset","0"},{"triangleCount",t},{"vertexCount",v},{"textureAssigned",has},
            {"basis","PAC SMD models (LZ-compressed, decoded natively) merged per texture; VIF strips in world space. Texture: GIM from the same PAC. Named LOD resources are optional and separate from the main set."},
            {"settings",QJsonObject{{"vertexOffset","0"},{"vertexCount",v},{"indexCount",t*3}}}});}}
    QJsonArray notes;notes.append(QString("Initial D Special Stage PAC: %1 SMD models, %2 texture groups (%3 with a GIM in this PAC)%4.").arg(parsed.models).arg(meshes.size()).arg(textured).arg(parsed.failed?QString(", %1 entries failed").arg(parsed.failed):QString()));
    return {{"candidates",meshes},{"notes",notes},{"recognized",true}};
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel){
    MeshResult result;auto fail=[&](QString e){result.error=e;result.mesh.reset();return result;};
    QByteArray bytes;const auto limit=qMin(source.expectedSize,AssetMemory::bytes()/4);
    for(qint64 offset=0;offset<limit;){if(cancel.load())return fail("Initial D SS decoding canceled.");auto r=source;r.offset=offset;r.length=qMin(qint64(1048576),limit-offset);auto page=readPage(r);if(!page.error.isEmpty())return fail(page.error);if(page.bytes.size()!=r.length)return fail("Incomplete PAC read.");applyPatches(page.bytes,offset,patches);bytes+=page.bytes;offset+=page.bytes.size();}
    auto parsed=parse(bytes,cancel);const QString key=candidate["texture"].toString();
    const auto &groups=candidate["idssLod"].toBool()?parsed.lodByTexture:parsed.byTexture;
    if(!groups.contains(key))return fail("Initial D SS texture group is no longer valid. Scan again.");
    if(!AssetMemory::fits(candidate["vertexCount"].toInteger(),candidate["triangleCount"].toInteger()*3))return fail("Initial D SS group exceeds the configured memory budget.");
    auto mesh=std::make_shared<MeshData>();mesh->hasUvs=true;int strip=0;
    for(const auto &r:groups[key]){const quint32 base=mesh->vertices.size();
        for(const auto &v:r.vertices){MeshVertex x{};for(int k=0;k<3;++k){x.position[k]=v.position[k];if(!std::isfinite(x.position[k]))return fail("Nonfinite SMD vertex.");}x.uv[0]=v.uv[0];x.uv[1]=v.uv[1];mesh->vertices.append(x);mesh->vertexOffsets.append(0);}
        for(const auto &t:r.triangles){MeshFaceSource face;face.strip=strip;face.indexSlots={-1,-1,-1};for(int k=0;k<3;++k){mesh->triangles.append(base+t[k]);face.offsets[k]=0;}mesh->faceSources.append(face);}++strip;}
    QVector<std::array<double,3>> sums(mesh->vertices.size());
    for(int i=0;i<mesh->triangles.size();i+=3){auto &a=mesh->vertices[mesh->triangles[i]],&b=mesh->vertices[mesh->triangles[i+1]],&c=mesh->vertices[mesh->triangles[i+2]];double u[3],w[3];for(int k=0;k<3;++k){u[k]=double(b.position[k])-a.position[k];w[k]=double(c.position[k])-a.position[k];}std::array<double,3> n{u[1]*w[2]-u[2]*w[1],u[2]*w[0]-u[0]*w[2],u[0]*w[1]-u[1]*w[0]};for(int j=0;j<3;++j)for(int k=0;k<3;++k)sums[mesh->triangles[i+j]][k]+=n[k];}
    for(int i=0;i<mesh->vertices.size();++i){auto n=sums[i];double l=std::hypot(n[0],n[1],n[2]);auto &v=mesh->vertices[i];if(l<1e-12)v.normal[1]=1;else for(int k=0;k<3;++k)v.normal[k]=float(n[k]/l);}
    result.surface=std::make_shared<MeshSurface>();auto &s=*result.surface;s.material.name=key.isEmpty()?QString("untextured"):key;s.material.alphaMask=true;s.material.doubleSided=true;s.material.unlit=true;
    QJsonObject bindings{{"format","Initial D Special Stage SMD"},{"sourceVertexColorsApplied",false},{"previewLighting","Unlit base texture; original vertex-color modulation and game effects pending"},{"topology","GIF triangle strip; ADC skips kick without resetting parity"},{"sourceUvTopLeft",true},{"textureWrapU","Repeat"},{"textureWrapV","Repeat"}};
    if(parsed.gims.contains(key)){const auto &e=parsed.gims[key];auto im=IdssCore::gim(reinterpret_cast<const uint8_t *>(bytes.constData())+e.offset,e.size);
        if(im.error.empty()){QImage image(im.width,im.height,QImage::Format_ARGB32);for(int y=0;y<im.height;++y)std::memcpy(image.scanLine(y),im.rgba.data()+size_t(y)*im.width,size_t(im.width)*4);s.texture=image;s.textureName=QString("Initial D SS / %1 (%2x%3)").arg(key).arg(im.width).arg(im.height);s.textureLibrary[s.textureName]=image;}
        else bindings["textureStatus"]=QString("GIM decoding failed: %1").arg(QString::fromStdString(im.error));}
    else bindings["textureStatus"]=key.isEmpty()?QString("No texture."):QString("Texture %1 is not in this PAC.").arg(key);
    bindings["textureAssigned"]=!s.texture.isNull();bindings["textureExpected"]=!key.isEmpty();s.material.sourceBindings=bindings;
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return fail(final.error);
    result.mesh=mesh;return result;
}
}
