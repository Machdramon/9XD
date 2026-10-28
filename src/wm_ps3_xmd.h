#pragma once
#include "wm_ps3_core.h"
#include "binary_document.h"
#include "mesh_data.h"
#include "mesh_surface.h"
#include "memory_budget.h"
#include "image_containers.h"
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>
// Native Wangan Midnight PS3 (XMD/XTD/X3D) adapter: one candidate per geometry with its own
// material and resolved GTF texture; embedded GTFs are also listed as images.
namespace WmPs3 {
inline constexpr auto ProfileId="native-wm-ps3-xmd";
inline QString formatName(int f){const int b=f&~0x60;return b==0x86?"DXT1":b==0x87?"DXT3":b==0x88?"DXT5":b==0x85?"ARGB8":QString("0x%1").arg(f,0,16);}
inline bool lightingVariant(const std::string &shape){return shape.find("_neon")!=std::string::npos||shape.find("_led")!=std::string::npos||shape.find("_spot")!=std::string::npos;}
inline QJsonObject scan(const QByteArray &bytes,const std::atomic_bool &cancel){
    if(!bytes.startsWith(QByteArray("XMD\x01",4)))return {};
    const auto *d=reinterpret_cast<const uint8_t *>(bytes.constData());auto lib=WmPs3Core::read(d,bytes.size());
    if(cancel.load())return {};
    if(!lib.error.empty())return {{"notes",QJsonArray{QString("XMD container recognized but not decoded: %1").arg(QString::fromStdString(lib.error))}}};
    QJsonArray meshes,images,notes;int textured=0,external=0,total=0;
    for(int t=0;t<int(lib.textures.size());++t){const auto &x=lib.textures[t];if(!x.real())continue;
        images.append(QJsonObject{{"name",QString("WM PS3 texture bank %1 / image %2").arg(x.bank).arg(x.index)},{"basis","Embedded GTF inside an XTD texture container; decoded by the GTF reader."},{"width",x.width},{"height",x.height},
            {"settings",QJsonObject{{"offset",QString::number(x.gtf)},{"format","Encoded image"},{"container","GTF"},{"surface",0},{"width",1},{"height",1}}}});}
    for(int mi=0;mi<int(lib.models.size());++mi){const auto &m=lib.models[mi];
        for(const auto &g:m.geometries){
            if(g.position.count<3||g.indices.count<3)continue;
            const auto tris=qint64(WmPs3Core::faces(d,g).size());if(!tris)continue;++total;
            const int tex=WmPs3Core::resolveTexture(lib,m,g.material);const bool assigned=tex>=0;textured+=assigned;
            const bool wantsTexture=g.material>=0&&g.material<int(m.materials.size())&&m.materials[g.material].textureSlot>=0;external+=wantsTexture&&!assigned;
            const QString shape=g.shape>=0?QString::fromStdString(m.shapes[g.shape].name):QString("unbound"),material=g.material>=0&&g.material<int(m.materials.size())?QString::fromStdString(m.materials[g.material].name):QString("no material");
            QJsonObject c{{"name",QString("WM PS3 / %1 / %2").arg(shape,material)},{"profileId",ProfileId},{"profileName","Wangan Midnight PS3 X3D"},{"nativeDecoder",true},
                {"model",mi},{"objectOffset",QString::number(g.block)},{"geometryName",QString::fromStdString(g.name)},{"material",material},{"triangleCount",tris},{"vertexCount",g.position.count},{"textureAssigned",assigned},
                {"basis",assigned?"XMD geometry with strip indices; material texture slot resolved to an embedded GTF.":wantsTexture?"XMD geometry; its texture lives in an external XTD bank (course/common data) that is not in this file.":"XMD geometry; this material has no texture slot (paint/colour or shader-only)."},
                {"settings",QJsonObject{{"vertexOffset",QString::number(g.position.data)},{"vertexCount",g.position.count},{"indexCount",tris*3}}}};
            if(g.shape>=0&&lightingVariant(m.shapes[g.shape].name))c["optionalVariant"]=true;
            meshes.append(c);
        }
    }
    notes.append(QString("Wangan Midnight PS3 XMD: %1 geometries in %2 model(s), %3 textured from embedded GTFs%4. Neon/LED/spot tuning shapes are listed as variants.").arg(total).arg(lib.models.size()).arg(textured).arg(external?QString(", %1 reference external XTD banks not present in this file").arg(external):QString()));
    return {{"candidates",meshes},{"images",images},{"notes",notes},{"recognized",true}};
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel){
    MeshResult result;auto fail=[&](QString e){result.error=e;result.mesh.reset();return result;};
    QByteArray bytes;const auto limit=qMin(source.expectedSize,AssetMemory::bytes()/4);
    for(qint64 offset=0;offset<limit;){if(cancel.load())return fail("WM PS3 decoding canceled.");auto r=source;r.offset=offset;r.length=qMin(qint64(1048576),limit-offset);auto page=readPage(r);if(!page.error.isEmpty())return fail(page.error);if(page.bytes.size()!=r.length)return fail("Incomplete WM PS3 read.");applyPatches(page.bytes,offset,patches);bytes+=page.bytes;offset+=page.bytes.size();}
    const auto *d=reinterpret_cast<const uint8_t *>(bytes.constData());auto lib=WmPs3Core::read(d,bytes.size());
    if(!lib.error.empty())return fail(QString::fromStdString(lib.error));
    const int mi=candidate["model"].toInt(-1);const qint64 block=candidate["objectOffset"].toString().toLongLong();
    if(mi<0||mi>=int(lib.models.size()))return fail("WM PS3 model is no longer valid. Scan again.");
    const auto &m=lib.models[mi];const WmPs3Core::Geometry *g=nullptr;for(const auto &x:m.geometries)if(x.block==block)g=&x;
    if(!g)return fail("WM PS3 geometry is no longer valid. Scan again.");
    const auto tris=WmPs3Core::faces(d,*g);
    if(!AssetMemory::fits(g->position.count,qint64(tris.size())*3))return fail("WM PS3 geometry exceeds the configured memory budget.");
    auto mesh=std::make_shared<MeshData>();mesh->hasUvs=g->hasUv;
    for(int i=0;i<g->position.count;++i){
        MeshVertex v{};for(int k=0;k<3;++k){v.position[k]=WmPs3Core::bef(d,g->position.data+qint64(i)*12+k*4);if(g->hasNormal)v.normal[k]=WmPs3Core::bef(d,g->normal.data+qint64(i)*12+k*4);}
        if(g->hasUv)for(int k=0;k<2;++k)v.uv[k]=WmPs3Core::bef(d,g->uv.data+qint64(i)*8+k*4);
        for(int k=0;k<3;++k)if(!std::isfinite(v.position[k])||!std::isfinite(v.normal[k]))return fail("Nonfinite WM PS3 vertex data.");
        mesh->vertices.append(v);mesh->vertexOffsets.append(g->position.data+qint64(i)*12);
    }
    for(const auto &t:tris){MeshFaceSource face;face.strip=0;face.indexSlots={-1,-1,-1};for(int k=0;k<3;++k){mesh->triangles.append(t[k]);face.offsets[k]=g->position.data+qint64(t[k])*12;}mesh->faceSources.append(face);}
    // Normalize stored normals; fill missing ones from faces.
    QVector<std::array<double,3>> sums(mesh->vertices.size());
    for(int i=0;i<mesh->triangles.size();i+=3){auto &a=mesh->vertices[mesh->triangles[i]],&b=mesh->vertices[mesh->triangles[i+1]],&c=mesh->vertices[mesh->triangles[i+2]];double u[3],w[3];for(int k=0;k<3;++k){u[k]=double(b.position[k])-a.position[k];w[k]=double(c.position[k])-a.position[k];}std::array<double,3> n{u[1]*w[2]-u[2]*w[1],u[2]*w[0]-u[0]*w[2],u[0]*w[1]-u[1]*w[0]};for(int j=0;j<3;++j)for(int k=0;k<3;++k)sums[mesh->triangles[i+j]][k]+=n[k];}
    for(int i=0;i<mesh->vertices.size();++i){auto &v=mesh->vertices[i];double l=std::hypot(v.normal[0],v.normal[1],v.normal[2]);if(l>1e-6){for(auto &x:v.normal)x=float(x/l);continue;}auto n=sums[i];l=std::hypot(n[0],n[1],n[2]);if(l<1e-12)v.normal[1]=1;else for(int k=0;k<3;++k)v.normal[k]=float(n[k]/l);}
    result.surface=std::make_shared<MeshSurface>();auto &surface=*result.surface;
    surface.material.name=candidate["material"].toString();
    QJsonObject bindings{{"format","Wangan Midnight PS3 X3D"},{"sourceUvTopLeft",true},{"textureWrapU","Repeat"},{"textureWrapV","Repeat"},{"geometryName",candidate["geometryName"]},{"gameShaderApplied",false}};
    const int tex=WmPs3Core::resolveTexture(lib,m,g->material);
    if(tex>=0&&g->hasUv){const auto &t=lib.textures[tex];QString error;ImageRecipe recipe;surface.texture=decodeGtf(bytes.mid(t.gtf),recipe,error,cancel);
        if(!surface.texture.isNull()){surface.textureName=QString("WM PS3 bank %1 / image %2 (%3, %4x%5)").arg(t.bank).arg(t.index).arg(formatName(t.format)).arg(t.width).arg(t.height);surface.textureLibrary[surface.textureName]=surface.texture;}
        else bindings["textureStatus"]="GTF decoding failed: "+error;}
    else if(tex<0&&g->material>=0&&g->material<int(m.materials.size())&&m.materials[g->material].textureSlot>=0)bindings["textureStatus"]="Texture is in an external XTD bank (course/common data) that is not part of this file.";
    bindings["textureAssigned"]=!surface.texture.isNull();surface.material.sourceBindings=bindings;
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return fail(final.error);
    result.mesh=mesh;return result;
}
}
