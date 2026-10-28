#pragma once
#include "gmo_core.h"
#include "binary_document.h"
#include "mesh_data.h"
#include "mesh_surface.h"
#include "memory_budget.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
#include <cmath>
// Native Sony PSP GMO adapter (Initial D Street Stage, Street Supremacy and other GMO users).
// Geometry only: GIM textures are referenced by name from separate files and are reported, not guessed.
namespace PspGmo {
inline constexpr auto ProfileId="native-psp-gmo";
inline QJsonObject scan(const QByteArray &bytes,const std::atomic_bool &cancel){
    if(bytes.indexOf("OMG.00.1PSP")<0||bytes.indexOf("OMG.00.1PSP")>4096)return {};
    const auto *d=reinterpret_cast<const uint8_t *>(bytes.constData());auto file=GmoCore::read(d,bytes.size());
    if(cancel.load())return {};
    if(!file.error.empty())return {{"notes",QJsonArray{QString("PSP GMO header recognized but not decoded: %1").arg(QString::fromStdString(file.error))}}};
    QJsonArray meshes;QStringList textures;for(const auto &t:file.textures)if(!t.empty()&&textures.size()<12&&!textures.contains(QString::fromStdString(t)))textures<<QString::fromStdString(t);
    for(int mi=0;mi<int(file.models.size());++mi){const auto &m=file.models[mi];qint64 vertices=0,triangles=0;
        for(const auto &a:m.arrays)vertices+=a.count;for(const auto &w:m.draws)triangles+=qint64(GmoCore::faces(m.arrays[w.arrays],w).size());
        if(!triangles)continue;
        meshes.append(QJsonObject{{"name",QString("PSP GMO / %1 (%2 draws)").arg(m.name.empty()?QString("model %1").arg(mi):QString::fromStdString(m.name)).arg(m.draws.size())},{"profileId",ProfileId},{"profileName","Sony PSP GMO"},{"nativeDecoder",true},{"model",mi},
            {"objectOffset",QString::number(m.arrays.empty()?0:m.arrays.front().data)},{"triangleCount",triangles},{"vertexCount",vertices},{"textureAssigned",false},{"textureNames",QJsonArray::fromStringList(textures)},
            {"basis",QString("GMO model: arrays/draw-arrays chunks with GU vertex types and %1. Textures are external GIM files (%2), not decoded here.").arg(m.scaled?"the model's fixed-point offset/scale":"float positions").arg(textures.isEmpty()?QString("none named"):textures.join(", "))},
            {"settings",QJsonObject{{"vertexOffset",QString::number(m.arrays.empty()?0:m.arrays.front().data)},{"vertexCount",vertices},{"indexCount",triangles*3}}}});
    }
    QJsonArray notes;notes.append(QString("PSP GMO (%1 header variant): %2 model(s), %3 texture references to external GIM files.").arg(file.variantB?"Genki":"standard").arg(file.models.size()).arg(file.textures.size()));
    return {{"candidates",meshes},{"notes",notes},{"recognized",true}};
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel){
    MeshResult result;auto fail=[&](QString e){result.error=e;result.mesh.reset();return result;};
    QByteArray bytes;const auto limit=qMin(source.expectedSize,AssetMemory::bytes()/4);
    for(qint64 offset=0;offset<limit;){if(cancel.load())return fail("GMO decoding canceled.");auto r=source;r.offset=offset;r.length=qMin(qint64(1048576),limit-offset);auto page=readPage(r);if(!page.error.isEmpty())return fail(page.error);if(page.bytes.size()!=r.length)return fail("Incomplete GMO read.");applyPatches(page.bytes,offset,patches);bytes+=page.bytes;offset+=page.bytes.size();}
    const auto *d=reinterpret_cast<const uint8_t *>(bytes.constData());auto file=GmoCore::read(d,bytes.size());
    if(!file.error.empty())return fail(QString::fromStdString(file.error));
    const int mi=candidate["model"].toInt(-1);if(mi<0||mi>=int(file.models.size()))return fail("GMO model is no longer valid. Scan again.");
    const auto &m=file.models[mi];auto mesh=std::make_shared<MeshData>();mesh->hasUvs=false;
    QVector<int> base(int(m.arrays.size()),-1);qint64 vertices=0;for(const auto &a:m.arrays)vertices+=a.count;
    if(!AssetMemory::fits(vertices,qint64(candidate["triangleCount"].toInteger())*3))return fail("GMO model exceeds the configured memory budget.");
    for(int strip=0;strip<int(m.draws.size());++strip){const auto &w=m.draws[strip];const auto &a=m.arrays[w.arrays];
        if(base[w.arrays]<0){base[w.arrays]=mesh->vertices.size();for(int i=0;i<a.count;++i){
            MeshVertex v{};const auto p=GmoCore::position(d,m,a,i),n=GmoCore::normal(d,a,i);
            for(int k=0;k<3;++k){v.position[k]=float(p[k]);v.normal[k]=float(n[k]);if(!std::isfinite(v.position[k]))return fail("Nonfinite GMO vertex.");}
            mesh->vertices.append(v);mesh->vertexOffsets.append(a.data+qint64(i)*a.layout.stride+a.layout.position);}}
        for(const auto &t:GmoCore::faces(a,w)){MeshFaceSource face;face.strip=strip;face.indexSlots={-1,-1,-1};for(int k=0;k<3;++k){mesh->triangles.append(quint32(base[w.arrays])+t[k]);face.offsets[k]=a.data+qint64(t[k])*a.layout.stride+a.layout.position;}mesh->faceSources.append(face);}
    }
    if(mesh->triangles.isEmpty())return fail("GMO model has no supported draws.");
    QVector<std::array<double,3>> sums(mesh->vertices.size());
    for(int i=0;i<mesh->triangles.size();i+=3){auto &a=mesh->vertices[mesh->triangles[i]],&b=mesh->vertices[mesh->triangles[i+1]],&c=mesh->vertices[mesh->triangles[i+2]];double u[3],w[3];for(int k=0;k<3;++k){u[k]=double(b.position[k])-a.position[k];w[k]=double(c.position[k])-a.position[k];}std::array<double,3> n{u[1]*w[2]-u[2]*w[1],u[2]*w[0]-u[0]*w[2],u[0]*w[1]-u[1]*w[0]};for(int j=0;j<3;++j)for(int k=0;k<3;++k)sums[mesh->triangles[i+j]][k]+=n[k];}
    for(int i=0;i<mesh->vertices.size();++i){auto &v=mesh->vertices[i];double l=std::hypot(v.normal[0],v.normal[1],v.normal[2]);if(l>1e-6){for(auto &x:v.normal)x=float(x/l);continue;}auto n=sums[i];l=std::hypot(n[0],n[1],n[2]);if(l<1e-12)v.normal[1]=1;else for(int k=0;k<3;++k)v.normal[k]=float(n[k]/l);}
    result.surface=std::make_shared<MeshSurface>();result.surface->material.name=candidate["name"].toString();
    result.surface->material.sourceBindings=QJsonObject{{"format","Sony PSP GMO"},{"textureAssigned",false},{"textureStatus","GMO textures are external GIM files; not loaded."},{"textureNames",candidate["textureNames"]}};
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return fail(final.error);
    result.mesh=mesh;return result;
}
}
