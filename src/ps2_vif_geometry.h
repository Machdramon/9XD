#pragma once
#include "ps2_vif_core.h"
#include "binary_document.h"
#include "mesh_data.h"
#include "memory_budget.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QSet>
// Qt adapter for the generic PS2 VIF analysis: scan candidates and decode them into MeshData.
namespace Ps2Vif {
inline constexpr auto ProfileId="native-ps2-vif";
inline QString basis(){return "Generic PS2 VIF analysis: UNPACK streams grouped per microprogram call; attribute roles and the strip restart rule were chosen by measured mesh quality. Parts are split at DMA RET tags. No game-specific header, transform or texture link is assumed.";}
// Coverage is over every classified batch, including completely rejected groups.
inline double batchCoverage(const Ps2VifCore::Result &r){return r.classified>0?double(r.batches.size())/r.classified:0.;}
inline QJsonObject scan(const QByteArray &bytes,const std::atomic_bool &cancel,bool nativeCoverage=false){
    if(bytes.size()<64)return {};
    auto r=Ps2VifCore::analyse(reinterpret_cast<const uint8_t *>(bytes.constData()),bytes.size(),&cancel);
    if(r.canceled||r.batches.empty())return {};
    QMap<int,std::array<qint64,4>> parts; // part -> vertices, triangles, first offset, batches
    for(auto &b:r.batches){auto &p=parts[b.part];if(!p[3])p[2]=b.positionOffset;p[0]+=b.positions.size();p[1]+=b.triangles.size();++p[3];}
    QJsonArray groups,rules;QSet<QString> seen;
    for(auto &g:r.groups){groups.append(QJsonObject{{"batches",g.batches},{"kept",g.kept},{"rule",QString::fromStdString(g.rule)},{"position",QString::fromStdString(g.position)},{"triangles",qint64(g.triangles)},{"accepted",g.accepted},{"reason",QString::fromStdString(g.reason)}});if(g.accepted&&!seen.contains(QString::fromStdString(g.rule))){seen.insert(QString::fromStdString(g.rule));rules.append(QString::fromStdString(g.rule));}}
    // Low batch acceptance means part of the file needs a rule the generic search did not find.
    const bool confident=!nativeCoverage&&batchCoverage(r)>=0.8;
    const qint64 vertices=r.vertexCount(),triangles=r.triangleCount();
    auto candidate=[&](QString name,int part,qint64 v,qint64 t,qint64 offset){
        return QJsonObject{{"name",name},{"profileId",ProfileId},{"profileName","Generic PS2 VIF"},{"nativeDecoder",true},{"heuristic",!confident},{"part",part},{"objectOffset",QString::number(offset)},{"triangleCount",t},{"vertexCount",v},{"rules",rules},{"basis",basis()},
            {"settings",QJsonObject{{"vertexOffset",QString::number(offset)},{"vertexCount",v},{"indexCount",t*3}}}};
    };
    QJsonArray candidates;
    candidates.append(candidate(QString("PS2 VIF geometry (generic, %1 parts)").arg(parts.size()),-1,vertices,triangles,r.batches.front().positionOffset));
    if(parts.size()>1){int n=0;for(auto it=parts.cbegin();it!=parts.cend()&&n<128;++it,++n){auto c=candidate(QString("PS2 VIF part %1").arg(it.key()),it.key(),it.value()[0],it.value()[1],it.value()[2]);c["partOfCandidate"]=true;candidates.append(c);}}
    QJsonArray notes;notes.append(QString("Generic PS2 VIF: %1 of %2 batches accepted, %3 triangles in %4 parts (rules: %5).%6").arg(r.batches.size()).arg(r.classified).arg(triangles).arg(parts.size()).arg([&]{QStringList l;for(auto v:rules)l<<v.toString();return l.join(", ");}()).arg(nativeCoverage?" A native decoder also recognizes this file; prefer its results.":confident?"":" Some batches need a rule the generic search did not find; verify the result."));
    return {{"candidates",candidates},{"notes",notes},{"groups",groups},{"streams",r.streams},{"classified",r.classified},{"batchCoverage",batchCoverage(r)},{"rets",qint64(r.rets.size())}};
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel){
    MeshResult result;auto fail=[&](QString error){result.error=error;result.mesh.reset();return result;};
    QByteArray bytes;const auto limit=qMin(source.expectedSize,AssetMemory::bytes()/4);
    for(qint64 offset=0;offset<limit;){if(cancel.load())return fail("PS2 VIF decoding canceled.");auto request=source;request.offset=offset;request.length=qMin(qint64(1048576),limit-offset);auto page=readPage(request);if(!page.error.isEmpty())return fail(page.error);if(page.bytes.size()!=request.length)return fail("Incomplete PS2 VIF read.");applyPatches(page.bytes,offset,patches);bytes+=page.bytes;offset+=page.bytes.size();}
    auto r=Ps2VifCore::analyse(reinterpret_cast<const uint8_t *>(bytes.constData()),bytes.size(),&cancel);
    if(r.canceled)return fail("PS2 VIF decoding canceled.");
    const int part=candidate["part"].toInt(-1);
    auto mesh=std::make_shared<MeshData>();mesh->hasUvs=true;int strip=0;
    for(auto &b:r.batches){
        if(part>=0&&b.part!=part)continue;
        if(!AssetMemory::fits(mesh->vertices.size()+b.positions.size(),mesh->triangles.size()+b.triangles.size()*3))return fail("PS2 VIF result exceeds the configured memory budget. Raise Settings > Performance or select a single part.");
        const quint32 base=mesh->vertices.size();if(b.uvs.empty())mesh->hasUvs=false;
        for(size_t i=0;i<b.positions.size();++i){
            MeshVertex v{};for(int k=0;k<3;++k){v.position[k]=float(b.positions[i][k]);if(!b.normals.empty())v.normal[k]=float(b.normals[i][k]);}
            if(!b.uvs.empty()){v.uv[0]=float(b.uvs[i][0]);v.uv[1]=float(b.uvs[i][1]);}
            mesh->vertices.append(v);mesh->vertexOffsets.append(b.positionOffset+qint64(i)*b.positionStride);
        }
        for(auto &t:b.triangles){MeshFaceSource face;face.strip=strip;face.indexSlots={-1,-1,-1};for(int k=0;k<3;++k){mesh->triangles.append(base+t[k]);face.offsets[k]=b.positionOffset+qint64(t[k])*b.positionStride;}mesh->faceSources.append(face);}
        ++strip;
    }
    if(mesh->triangles.isEmpty())return fail("No generic PS2 VIF geometry matches this candidate. Scan again.");
    // Fill missing or zero normals from the recovered faces.
    QVector<std::array<double,3>> sums(mesh->vertices.size());
    for(int i=0;i<mesh->triangles.size();i+=3){auto &a=mesh->vertices[mesh->triangles[i]],&b=mesh->vertices[mesh->triangles[i+1]],&c=mesh->vertices[mesh->triangles[i+2]];double u[3],w[3];for(int k=0;k<3;++k){u[k]=double(b.position[k])-a.position[k];w[k]=double(c.position[k])-a.position[k];}std::array<double,3> n{u[1]*w[2]-u[2]*w[1],u[2]*w[0]-u[0]*w[2],u[0]*w[1]-u[1]*w[0]};for(int j=0;j<3;++j)for(int k=0;k<3;++k)sums[mesh->triangles[i+j]][k]+=n[k];}
    for(int i=0;i<mesh->vertices.size();++i){auto &v=mesh->vertices[i];double length=std::hypot(v.normal[0],v.normal[1],v.normal[2]);if(length<1e-6){auto n=sums[i];length=std::hypot(n[0],n[1],n[2]);if(length<1e-12)v.normal[1]=1;else for(int k=0;k<3;++k)v.normal[k]=float(n[k]/length);}}
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return fail(final.error);
    result.mesh=mesh;return result;
}
}
