#pragma once
#include "mesh_profiles.h"
#include "genki_geometry.h"
#include "faf_geometry.h"
#include "linked_scene_surfaces.h"
#include "ai_decoder_setup.h"
#include "rtuned_collision.h"
#include "rtuned_visual_scene.h"
#include "fm1_geometry.h"
#include "fm1_track.h"
#include "msr_alpha_geometry.h"
#include "pgr2_geometry.h"
#include "pgr2_track.h"
#include "pgr1_track.h"
#include "pgr1_car.h"
#include "pgr34_geometry.h"
#include "pgr34_track.h"
#include "ps2_vif_geometry.h"
#include "wm_ps3_xmd.h"
#include "gmo_model.h"
#include "nfs_mw_solid.h"
#include "itc_model.h"
#include "blur_model.h"
#include "rw_model.h"
#include "idss_model.h"
#include <QMatrix4x4>
#include <QVector3D>
namespace MeshProfiles {
inline QJsonObject inspect(ReadRequest source,const PatchList &patches,const QJsonObject &profile,const std::atomic_bool &cancel){
    QByteArray bytes;auto limit=qMin(source.expectedSize,AssetMemory::bytes()/4);for(qint64 p=0;p<limit;){if(cancel.load())return {{"error","Profile test canceled."}};auto r=source;r.offset=p;r.length=qMin(qint64(1048576),limit-p);auto page=readPage(r);if(!page.error.isEmpty()||page.bytes.size()!=r.length)return {{"error",page.error.isEmpty()?"Incomplete profile read.":page.error}};applyPatches(page.bytes,p,patches);bytes+=page.bytes;p+=page.bytes.size();}auto result=scan(bytes,{profile},cancel);if(cancel.load())return {{"error","Profile test canceled."}};auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return {{"error",final.error}};return result;
}
// Bizarre Creations titles (MSR, PGR1-4) store Direct3D left-handed coordinates; the viewer and exports are
// right-handed. Convert once here: negate Z for positions and normals and reverse triangle order so front faces
// stay front faces. Copies the mesh so cached/shared decoder data is never mirrored twice.
inline MeshResult mirrorLeftHanded(MeshResult r){
    if(!r.mesh)return r;auto m=std::make_shared<MeshData>(*r.mesh);
    for(auto &v:m->vertices){v.position[2]=-v.position[2];v.normal[2]=-v.normal[2];}
    for(qsizetype i=0;i+2<m->triangles.size();i+=3)std::swap(m->triangles[i+1],m->triangles[i+2]);
    for(auto &f:m->faceSources){std::swap(f.offsets[1],f.offsets[2]);std::swap(f.indexSlots[1],f.indexSlots[2]);}
    r.positionDelta[2]=-r.positionDelta[2];r.mesh=m;
    if(r.surface)r.surface->material.sourceBindings["handednessConverted"]="Direct3D left-handed -> right-handed (Z negated)";
    return r;
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel,QMap<QByteArray,std::shared_ptr<MeshData>> *cache=nullptr,Fm1Track::Session *trackSession=nullptr,Pgr1Track::Session *pgrSession=nullptr,MsrAlpha::Session *msrSession=nullptr,Pgr2Geometry::Session *pgr2Session=nullptr,Pgr2Track::Session *pgr2TrackSession=nullptr,Pgr34Geometry::Session *pgr34Session=nullptr,Pgr34Track::Session *pgr34TrackSession=nullptr,FafGeometry::Session *fafSession=nullptr,LinkedSurfaces::Session *linkedSession=nullptr,RenderWare::Session *rwSession=nullptr){
    if(candidate["profileId"]=="native-linked-surface"){QMap<QByteArray,std::shared_ptr<MeshData>> solids;return LinkedSurfaces::decode(source,patches,candidate,cancel,linkedSession,[&](QJsonObject c){return decode(source,patches,c,cancel,&solids);});}
    if(candidate["profileId"]==PspGmo::ProfileId)return PspGmo::decode(source,patches,candidate,cancel);
    if(candidate["profileId"]==NfsMw::ProfileId)return NfsMw::decode(source,patches,candidate,cancel);
    if(candidate["profileId"]==Blur::ProfileId)return Blur::decode(source,patches,candidate,cancel);
    if(candidate["profileId"]==Itc::ProfileId)return Itc::decode(source,patches,candidate,cancel);
    if(candidate["profileId"]==InitialDSS::ProfileId)return InitialDSS::decode(source,patches,candidate,cancel);
    if(candidate["profileId"]==RenderWare::ProfileId)return RenderWare::decode(source,patches,candidate,cancel,rwSession);
    if(candidate["profileId"]==WmPs3::ProfileId)return WmPs3::decode(source,patches,candidate,cancel);
    if(candidate["profileId"]==Ps2Vif::ProfileId)return Ps2Vif::decode(source,patches,candidate,cancel);
    if(candidate["profileId"]=="native-faf-arcx")return FafGeometry::decode(source,patches,candidate,cancel,fafSession);
    if(candidate["profileId"]=="native-genki-part")return readGenkiGeometry(source,patches,cancel,true,candidate);
    if(candidate["profileId"]=="native-pgr34-track")return mirrorLeftHanded(Pgr34Track::decode(source,patches,candidate,cancel,pgr34TrackSession));
    if(candidate["profileId"]=="native-pgr34-car")return mirrorLeftHanded(Pgr34Geometry::decode(source,patches,candidate,cancel,pgr34Session));
    if(candidate["profileId"]=="native-pgr1-car")return mirrorLeftHanded(Pgr1Car::decode(source,patches,candidate,cancel,pgrSession));
    if(candidate["profileId"]=="native-pgr1-track")return mirrorLeftHanded(Pgr1Track::decode(source,patches,candidate,cancel,pgrSession));
    if((candidate["profileId"]=="native-pgr2-track"||candidate["profileId"]=="native-pgr2-track-instances"))return mirrorLeftHanded(Pgr2Track::decode(source,patches,candidate,cancel,pgr2TrackSession));
    if(candidate["profileId"]=="native-pgr2-car")return mirrorLeftHanded(Pgr2Geometry::decode(source,patches,candidate,cancel,pgr2Session));
    if(candidate["profileId"]=="native-msr-alpha-car"||candidate["profileId"]=="native-msr-alpha-track"||candidate["profileId"]=="native-msr-retail-track")return mirrorLeftHanded(MsrAlpha::decode(source,patches,candidate,cancel,msrSession));
    if(candidate["profileId"]=="native-fm1-track")return Fm1Track::decode(source,patches,candidate,cancel,trackSession);
    if(candidate["profileId"]=="native-fm1-carbin")return Fm1Geometry::decode(source,patches,candidate,cancel);
    if(candidate["profileId"]=="native-rtuned-visual-scene")return RTunedVisualScene::decode(source,patches,candidate,cancel);
    if(candidate["profileId"]=="native-rtuned-collision"||candidate["profileId"]=="native-rtuned-collision-resource")return RTunedCollision::decode(source,patches,candidate,cancel);
    if(candidate["profileId"]=="native-nfsu2-scene"){
        MeshResult combined;auto fail=[&](QString error){combined.mesh.reset();combined.error=error;return combined;};
        auto entries=candidate["nfsu2Instances"].toArray();if(entries.isEmpty()||entries.size()>AssetMemory::bytes()/16384)return fail("Invalid NFSU2 scene instance count.");
        combined.mesh=std::make_shared<MeshData>();QMap<QByteArray,std::shared_ptr<MeshData>> decodedSolids;
        for(auto value:entries){if(cancel.load())return fail("NFSU2 scene preview canceled.");auto entry=value.toObject();if(entry["profileId"]=="native-nfsu2-scene")return fail("Nested NFSU2 scene is invalid.");auto settings=entry["settings"].toObject();if(!AssetMemory::fits(settings["vertexCount"].toInteger()+combined.mesh->vertices.size(),settings["indexCount"].toInteger()+combined.mesh->triangles.size()))return fail("Scene exceeds the configured memory budget. Raise Settings > Performance or select individual sectors.");auto part=decode(source,patches,entry,cancel,&decodedSolids);if(!part.error.isEmpty()||!part.mesh)return fail(part.error.isEmpty()?"Missing NFSU2 instance mesh.":part.error);auto base=quint32(combined.mesh->vertices.size());combined.mesh->vertices+=part.mesh->vertices;combined.mesh->vertexOffsets+=part.mesh->vertexOffsets;combined.mesh->faceSources+=part.mesh->faceSources;for(auto index:part.mesh->triangles)combined.mesh->triangles.append(base+index);}
        auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return fail(final.error);return combined;
    }
    MeshResult result;QString error;MeshSpec spec;if(cancel.load()){result.error="Profile preview canceled.";return result;}if(!AiDecoderSetup::mesh(candidate["settings"].toObject(),source.expectedSize,spec,error)){result.error=error;return result;}auto key=QJsonDocument(candidate["settings"].toObject()).toJson(QJsonDocument::Compact);if(cache&&cache->contains(key))result.mesh=std::make_shared<MeshData>(*cache->value(key));else {result=readMesh(source,patches,spec);if(!result.error.isEmpty()||!result.mesh)return result;if(cache){cache->insert(key,result.mesh);result.mesh=std::make_shared<MeshData>(*result.mesh);}}
    auto values=candidate["sourceTransform"].toArray();auto rotation=candidate["profileRotation"].toArray();if(!values.isEmpty()||!rotation.isEmpty()){if(values.isEmpty())values=QJsonArray{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};if(values.size()!=16){result.mesh.reset();result.error="Invalid profile matrix.";return result;}float raw[16];for(int i=0;i<16;++i){raw[i]=values[i].toDouble(std::numeric_limits<double>::quiet_NaN());if(!std::isfinite(raw[i])){result.mesh.reset();result.error="Nonfinite profile matrix.";return result;}}QMatrix4x4 matrix=QMatrix4x4(raw).transposed();if(!rotation.isEmpty()){if(rotation.size()!=3){result.mesh.reset();result.error="Invalid profile rotation.";return result;}for(auto value:rotation)if(!value.isDouble()||!std::isfinite(value.toDouble())||std::abs(value.toDouble())>360){result.mesh.reset();result.error="Invalid profile rotation.";return result;}QMatrix4x4 orientation;orientation.rotate(rotation[2].toDouble(),0,0,1);orientation.rotate(rotation[1].toDouble(),0,1,0);orientation.rotate(rotation[0].toDouble(),1,0,0);matrix=orientation*matrix;}double determinant=matrix.determinant();if(!std::isfinite(determinant)||std::abs(determinant)<1e-12){result.mesh.reset();result.error="Singular profile matrix.";return result;}auto normals=matrix.normalMatrix();
        for(auto &v:result.mesh->vertices){if(cancel.load()){result.mesh.reset();result.error="Profile preview canceled.";return result;}auto p=matrix.map(QVector3D(v.position[0],v.position[1],v.position[2]));QVector3D n;for(int a=0;a<3;++a)n[a]=normals(a,0)*v.normal[0]+normals(a,1)*v.normal[1]+normals(a,2)*v.normal[2];if(!std::isfinite(p.x())||!std::isfinite(p.y())||!std::isfinite(p.z())||!std::isfinite(n.lengthSquared())){result.mesh.reset();result.error="Profile transform exceeds numeric limits.";return result;}if(n.lengthSquared()>1e-20)n.normalize();for(int a=0;a<3;++a){v.position[a]=p[a];v.normal[a]=n[a];}}
        if(determinant<0){for(int i=0;i<result.mesh->triangles.size();i+=3)std::swap(result.mesh->triangles[i+1],result.mesh->triangles[i+2]);for(auto &f:result.mesh->faceSources){std::swap(f.offsets[1],f.offsets[2]);std::swap(f.indexSlots[1],f.indexSlots[2]);}}
    }
    if(cancel.load()){result.mesh.reset();result.error="Profile preview canceled.";}return result;
}
}
