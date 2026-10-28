#pragma once
#include "nfs_mw_core.h"
#include "binary_document.h"
#include "mesh_data.h"
#include "mesh_surface.h"
#include "memory_budget.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
#include <cmath>
// Native NFS Most Wanted (PC) GEOMETRY.BIN adapter: one stock-car assembly (LOD A, stock kit, no damage,
// decals or alternate hoods), plus every solid on its own. Textures live in TEXTURES.BIN and are not linked.
namespace NfsMw {
inline constexpr auto ProfileId="native-nfs-mw-solid";
inline QString prefix(const std::vector<NfsMwCore::Solid> &solids){
    for(const auto &s:solids){const auto at=s.name.find('_');if(at!=std::string::npos)return QString::fromStdString(s.name.substr(0,at));}
    return "car";
}
inline QJsonObject scan(const QByteArray &bytes,const std::atomic_bool &cancel){
    if(bytes.size()<16)return {};
    const auto *d=reinterpret_cast<const uint8_t *>(bytes.constData());auto file=NfsMwCore::read(d,bytes.size());
    if(cancel.load()||!file.container||file.solids.empty())return {};
    QJsonArray meshes;qint64 assemblyVertices=0,assemblyTriangles=0;int assemblyParts=0;QStringList wheels;
    for(int i=0;i<int(file.solids.size());++i){const auto &s=file.solids[i];const auto name=QString::fromStdString(s.name);
        QStringList labels;for(const auto &m:s.submeshes)if(!m.label.empty()&&!labels.contains(QString::fromStdString(m.label)))labels<<QString::fromStdString(m.label);
        QJsonObject c{{"name",QString("NFS MW / %1").arg(name)},{"profileId",ProfileId},{"profileName","NFS Most Wanted solid"},{"nativeDecoder",true},{"solid",i},{"solidName",name},
            {"objectOffset",QString::number(s.at)},{"triangleCount",s.triangles()},{"vertexCount",s.vertexCount},{"textureAssigned",false},{"materials",QJsonArray::fromStringList(labels)},
            {"basis",QString("NFS MW solid: %1 submeshes, 36-byte vertices, solid-global u16 triangle lists. Materials: %2.").arg(s.submeshes.size()).arg(labels.join(", "))},
            {"settings",QJsonObject{{"vertexOffset",QString::number(s.vertexData)},{"vertexCount",s.vertexCount},{"indexCount",s.triangles()*3}}}};
        if(NfsMwCore::assembled(s.name)){c["partOfCandidate"]=true;assemblyVertices+=s.vertexCount;assemblyTriangles+=s.triangles();++assemblyParts;}
        else if(NfsMwCore::variant(s.name))c["optionalVariant"]=true;
        else wheels<<name;
        meshes.append(c);
    }
    if(assemblyParts)meshes.prepend(QJsonObject{{"name",QString("NFS MW / %1 stock car (%2 parts, LOD A)").arg(prefix(file.solids)).arg(assemblyParts)},{"profileId",ProfileId},{"profileName","NFS Most Wanted solid"},{"nativeDecoder",true},{"solid",-1},
        {"objectOffset",QString::number(file.solids.front().at)},{"triangleCount",assemblyTriangles},{"vertexCount",assemblyVertices},{"textureAssigned",false},
        {"basis","Stock assembly: highest-LOD stock-kit solids in car space. Damage, decal, aftermarket kit and hood variants are separate; wheels and brakes are modelled at the origin and placed at runtime, so they are listed on their own."},
        {"settings",QJsonObject{{"vertexOffset",QString::number(file.solids.front().vertexData)},{"vertexCount",assemblyVertices},{"indexCount",assemblyTriangles*3}}}});
    QJsonArray notes;notes.append(QString("NFS Most Wanted GEOMETRY.BIN: %1 solids decoded%2. Textures are in TEXTURES.BIN and are not linked.%3")
        .arg(file.solids.size()).arg(file.rejected?QString(", %1 solids with an unrecognized layout skipped").arg(file.rejected):QString())
        .arg(wheels.isEmpty()?QString():QString(" Runtime-placed parts: %1.").arg(wheels.join(", "))));
    return {{"candidates",meshes},{"notes",notes},{"recognized",true}};
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel){
    MeshResult result;auto fail=[&](QString e){result.error=e;result.mesh.reset();return result;};
    QByteArray bytes;const auto limit=qMin(source.expectedSize,AssetMemory::bytes()/4);
    for(qint64 offset=0;offset<limit;){if(cancel.load())return fail("NFS MW decoding canceled.");auto r=source;r.offset=offset;r.length=qMin(qint64(1048576),limit-offset);auto page=readPage(r);if(!page.error.isEmpty())return fail(page.error);if(page.bytes.size()!=r.length)return fail("Incomplete NFS MW read.");applyPatches(page.bytes,offset,patches);bytes+=page.bytes;offset+=page.bytes.size();}
    const auto *d=reinterpret_cast<const uint8_t *>(bytes.constData());auto file=NfsMwCore::read(d,bytes.size());
    const int wanted=candidate["solid"].toInt(-2);
    if(wanted<-1||wanted>=int(file.solids.size())||(wanted>=0&&QString::fromStdString(file.solids[wanted].name)!=candidate["solidName"].toString()))return fail("NFS MW solid is no longer valid. Scan again.");
    if(!AssetMemory::fits(candidate["vertexCount"].toInteger(),candidate["triangleCount"].toInteger()*3))return fail("NFS MW geometry exceeds the configured memory budget.");
    auto mesh=std::make_shared<MeshData>();mesh->hasUvs=true;int strip=0;QStringList materials;
    for(int i=0;i<int(file.solids.size());++i){
        const auto &s=file.solids[i];if(wanted>=0?i!=wanted:!NfsMwCore::assembled(s.name))continue;
        if(cancel.load())return fail("NFS MW decoding canceled.");
        const quint32 base=mesh->vertices.size();
        for(int v=0;v<s.vertexCount;++v){const qint64 p=s.vertexData+qint64(v)*NfsMwCore::VertexStride;MeshVertex x{};
            for(int k=0;k<3;++k){x.position[k]=NfsMwCore::lef(d,p+4*k);x.normal[k]=NfsMwCore::lef(d,p+12+4*k);}
            x.uv[0]=NfsMwCore::lef(d,p+28);x.uv[1]=NfsMwCore::lef(d,p+32);
            for(int k=0;k<3;++k)if(!std::isfinite(x.position[k])||!std::isfinite(x.normal[k]))return fail("Nonfinite NFS MW vertex data.");
            mesh->vertices.append(x);mesh->vertexOffsets.append(p);}
        for(const auto &m:s.submeshes){
            if(!m.label.empty()&&!materials.contains(QString::fromStdString(m.label)))materials<<QString::fromStdString(m.label);
            for(qint64 k=0;k+2<m.indexCount;k+=3){MeshFaceSource face;face.strip=strip;face.indexSlots={-1,-1,-1};
                for(int j=0;j<3;++j){const auto at=NfsMwCore::index(d,s,m.indexStart+k+j);mesh->triangles.append(base+at);face.offsets[j]=s.vertexData+qint64(at)*NfsMwCore::VertexStride;}
                mesh->faceSources.append(face);}
            ++strip;}
    }
    if(mesh->triangles.isEmpty())return fail("No NFS MW geometry matches this candidate. Scan again.");
    for(auto &v:mesh->vertices){const double l=std::hypot(v.normal[0],v.normal[1],v.normal[2]);if(l>1e-6)for(auto &x:v.normal)x=float(x/l);else v.normal[2]=1;}
    result.surface=std::make_shared<MeshSurface>();result.surface->material.name=candidate["name"].toString();
    result.surface->material.sourceBindings=QJsonObject{{"format","NFS Most Wanted solid"},{"sourceUvTopLeft",true},{"textureWrapU","Repeat"},{"textureWrapV","Repeat"},{"textureAssigned",false},
        {"textureStatus","Textures are in TEXTURES.BIN (separate file); not linked."},{"materials",QJsonArray::fromStringList(materials)}};
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return fail(final.error);
    result.mesh=mesh;return result;
}
}
