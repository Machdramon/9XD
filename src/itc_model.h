#pragma once
#include "itc_core.h"
#include "xpr2_textures.h"
#include "binary_document.h"
#include "mesh_data.h"
#include "mesh_surface.h"
#include "memory_budget.h"
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>
// Native Import Tuner Challenge (Xbox 360) adapter: one candidate per material element, textured from the
// section's own XPR2 package when the element's texture index is local; shared/common textures are reported.
namespace Itc {
inline constexpr auto ProfileId="native-itc-x360";
inline QJsonObject scan(const QByteArray &bytes,const std::atomic_bool &cancel){
    const auto *d=reinterpret_cast<const uint8_t *>(bytes.constData());auto file=ItcCore::read(d,bytes.size());
    if(cancel.load()||file.meshes.empty())return {};
    QList<Xpr2::Entry> textures;if(file.xpr2>=0)textures=Xpr2::entries(QByteArray::fromRawData(bytes.constData()+file.xpr2,bytes.size()-file.xpr2));
    QJsonArray meshes;int textured=0,shared=0,total=0;
    for(int mi=0;mi<int(file.meshes.size());++mi){const auto &m=file.meshes[mi];
        for(int ei=0;ei<int(m.elements.size());++ei){const auto &e=m.elements[ei];const auto tris=qint64(ItcCore::faces(d,e).size());if(!tris)continue;++total;
            const bool local=e.texture>=0&&e.texture<textures.size()&&!Xpr2::formatName(textures[e.texture].format).isEmpty();textured+=local;shared+=e.texture>=0&&!local;
            const QString texture=local?textures[e.texture].name:e.texture>=0?QString("shared texture %1").arg(e.texture):QString("untextured");
            meshes.append(QJsonObject{{"name",QString("ITC / %1 / %2").arg(QString::fromStdString(m.name)).arg(texture)},{"profileId",ProfileId},{"profileName","Import Tuner Challenge X360"},{"nativeDecoder",true},
                {"mesh",mi},{"element",ei},{"objectOffset",QString::number(e.indexData)},{"triangleCount",tris},{"vertexCount",m.vertexCount},{"textureAssigned",local},{"textureIndex",e.texture},
                {"basis",local?"ITC element: u16 strips (0xFFFF restart) over float3 positions; texture is a local XPR2 entry.":e.texture>=0?"ITC element; its texture index points past this section's XPR2 package (shared course/common textures, not in this file).":"ITC element without a texture layer."},
                {"settings",QJsonObject{{"vertexOffset",QString::number(m.position.offset)},{"vertexCount",m.vertexCount},{"indexCount",tris*3}}}});}}
    QJsonArray notes;notes.append(QString("Import Tuner Challenge model: %1 elements in %2 mesh(es), %3 textured from the local XPR2 package%4.").arg(total).arg(file.meshes.size()).arg(textured).arg(shared?QString(", %1 use shared textures not in this file").arg(shared):QString()));
    QJsonArray images;if(file.xpr2>=0)images=Xpr2::images(bytes,{file.xpr2},128);
    return {{"candidates",meshes},{"images",images},{"notes",notes},{"recognized",true}};
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel){
    MeshResult result;auto fail=[&](QString e){result.error=e;result.mesh.reset();return result;};
    QByteArray bytes;const auto limit=qMin(source.expectedSize,AssetMemory::bytes()/4);
    for(qint64 offset=0;offset<limit;){if(cancel.load())return fail("ITC decoding canceled.");auto r=source;r.offset=offset;r.length=qMin(qint64(1048576),limit-offset);auto page=readPage(r);if(!page.error.isEmpty())return fail(page.error);if(page.bytes.size()!=r.length)return fail("Incomplete ITC read.");applyPatches(page.bytes,offset,patches);bytes+=page.bytes;offset+=page.bytes.size();}
    const auto *d=reinterpret_cast<const uint8_t *>(bytes.constData());auto file=ItcCore::read(d,bytes.size());
    const int mi=candidate["mesh"].toInt(-1),ei=candidate["element"].toInt(-1);
    if(mi<0||mi>=int(file.meshes.size())||ei<0||ei>=int(file.meshes[mi].elements.size())||file.meshes[mi].elements[ei].indexData!=candidate["objectOffset"].toString().toLongLong())return fail("ITC element is no longer valid. Scan again.");
    const auto &m=file.meshes[mi];const auto &e=m.elements[ei];const auto tris=ItcCore::faces(d,e);
    if(!AssetMemory::fits(m.vertexCount,qint64(tris.size())*3))return fail("ITC element exceeds the configured memory budget.");
    // Compact to the vertices this element uses.
    QVector<int> remap(m.vertexCount,-1);auto mesh=std::make_shared<MeshData>();mesh->hasUvs=m.uvSets>0;
    for(const auto &t:tris){MeshFaceSource face;face.strip=0;face.indexSlots={-1,-1,-1};
        for(int k=0;k<3;++k){const int v=int(t[k]);if(remap[v]<0){remap[v]=mesh->vertices.size();MeshVertex x{};const qint64 p=m.position.offset+qint64(v)*12;
                for(int j=0;j<3;++j)x.position[j]=ItcCore::bef(d,p+4*j);
                if(m.normal.offset>=0)for(int j=0;j<3;++j)x.normal[j]=ItcCore::half(ItcCore::be16(d,m.normal.offset+qint64(v)*16+2*j));
                if(m.uvSets){const qint64 q=m.colourUv.offset+qint64(v)*m.colourUv.stride+4;x.uv[0]=ItcCore::half(ItcCore::be16(d,q));x.uv[1]=ItcCore::half(ItcCore::be16(d,q+2));}
                for(int j=0;j<3;++j)if(!std::isfinite(x.position[j])||!std::isfinite(x.normal[j]))return fail("Nonfinite ITC vertex data.");
                mesh->vertices.append(x);mesh->vertexOffsets.append(p);}
            mesh->triangles.append(quint32(remap[v]));face.offsets[k]=m.position.offset+qint64(v)*12;}
        mesh->faceSources.append(face);}
    if(mesh->triangles.isEmpty())return fail("ITC element has no triangles.");
    QVector<std::array<double,3>> sums(mesh->vertices.size());
    for(int i=0;i<mesh->triangles.size();i+=3){auto &a=mesh->vertices[mesh->triangles[i]],&b=mesh->vertices[mesh->triangles[i+1]],&c=mesh->vertices[mesh->triangles[i+2]];double u[3],w[3];for(int k=0;k<3;++k){u[k]=double(b.position[k])-a.position[k];w[k]=double(c.position[k])-a.position[k];}std::array<double,3> n{u[1]*w[2]-u[2]*w[1],u[2]*w[0]-u[0]*w[2],u[0]*w[1]-u[1]*w[0]};for(int j=0;j<3;++j)for(int k=0;k<3;++k)sums[mesh->triangles[i+j]][k]+=n[k];}
    for(int i=0;i<mesh->vertices.size();++i){auto &v=mesh->vertices[i];double l=std::hypot(v.normal[0],v.normal[1],v.normal[2]);if(l>1e-6){for(auto &x:v.normal)x=float(x/l);continue;}auto n=sums[i];l=std::hypot(n[0],n[1],n[2]);if(l<1e-12)v.normal[1]=1;else for(int k=0;k<3;++k)v.normal[k]=float(n[k]/l);}
    result.surface=std::make_shared<MeshSurface>();auto &surface=*result.surface;surface.material.name=candidate["name"].toString();surface.material.alphaMask=true;
    QJsonObject bindings{{"format","Import Tuner Challenge X360"},{"sourceUvTopLeft",true},{"textureWrapU","Repeat"},{"textureWrapV","Repeat"},{"textureIndex",e.texture},{"gameShaderApplied",false}};
    if(file.xpr2>=0&&e.texture>=0&&m.uvSets){const auto package=bytes.mid(file.xpr2);const auto list=Xpr2::entries(package);
        if(e.texture<list.size()){ImageRecipe recipe;recipe.surface=e.texture;QString error;surface.texture=Xpr2::decode(package,recipe,error,cancel);
            if(!surface.texture.isNull()){surface.textureName=QString("ITC / %1 (%2, %3x%4)").arg(list[e.texture].name,Xpr2::formatName(list[e.texture].format)).arg(list[e.texture].width).arg(list[e.texture].height);surface.textureLibrary[surface.textureName]=surface.texture;}
            else bindings["textureStatus"]="XPR2 decoding failed: "+error;}
        else bindings["textureStatus"]="Texture index points to shared course/common textures that are not in this file.";}
    else if(e.texture<0){bindings["textureExpected"]=false;}
    bindings["textureAssigned"]=!surface.texture.isNull();surface.material.sourceBindings=bindings;
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return fail(final.error);
    result.mesh=mesh;return result;
}
}
