#pragma once
#include "binary_document.h"
#include "genki_mesh_evidence.h"
#include "genki_materials.h"
#include "genki_track_context.h"
#include "faf_geometry.h"
#include "arcp_evidence.h"
#include "mesh_profiles.h"
#include "native_resources.h"
#include "geometry_candidates.h"
#include "encoded_image_evidence.h"
#include "nfsu2_scene.h"
#include "rtuned_geometry.h"
#include "rtuned_collision.h"
#include "sega_a3da_evidence.h"
#include "rtuned_visual_scene.h"
#include "linked_scene_surfaces.h"
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
#include <QJsonArray>
#include <QJsonDocument>
#include <QtEndian>
#include <QSet>
#include <algorithm>
#include <cmath>
// Bounded metadata discovery; pixel/geometry interpretation stays in production decoders.
inline QJsonObject discoverAssets(ReadRequest source,const PatchList &patches,const std::atomic_bool &cancel,bool findGeometry=false,const QJsonObject &trackContext={}){
    const qint64 limit=AssetMemory::bytes()/4;QByteArray bytes;auto length=qMin(source.expectedSize,limit);
    if(length<=0)return {{"error","Open a nonempty document first."}};
    for(qint64 offset=0;offset<length;){if(cancel.load())return {{"error","Discovery canceled."}};auto request=source;request.offset=offset;request.length=qMin(qint64(1048576),length-offset);auto read=readPage(request);if(!read.error.isEmpty())return {{"error",read.error}};applyPatches(read.bytes,offset,patches);if(read.bytes.size()!=request.length)return {{"error","Incomplete discovery read."}};bytes+=read.bytes;offset+=read.bytes.size();}
    auto u16=[&](qint64 at){return qFromLittleEndian<quint16>(bytes.constData()+at);};auto u32=[&](qint64 at){return qFromLittleEndian<quint32>(bytes.constData()+at);};QJsonArray images,meshes,notes,regions,imageEvidence;
    auto region=[&](QString name,qint64 offset,qint64 length,QString evidence,qint64 step=0,qint64 count=1){if(length>0&&regions.size()<32)regions.append(QJsonObject{{"name",name},{"offset",QString::number(offset)},{"length",QString::number(length)},{"step",QString::number(step?step:length)},{"repetitions",count},{"evidence",evidence}});};
    qsizetype at=0;for(int hit=0;hit<16&&images.size()<128;++hit){at=bytes.indexOf("TIM2",at);if(at<0)break;auto start=at++;if(start>bytes.size()-16)continue;auto align=quint8(bytes[start+5]);auto count=u16(start+6);if(align>1||count<1||count>128)continue;qint64 p=start+(align?128:16);
        for(int i=0;i<int(count)&&images.size()<128;++i){if(p>bytes.size()-48)break;auto total=u32(p),palette=u32(p+4),pixels=u32(p+8);auto header=u16(p+12),w=u16(p+20),h=u16(p+22);if(header<48||total<header||qint64(header)+palette+pixels>total||p+total>source.expectedSize||w<1||h<1||qint64(w)*h>4194304)break;
            region(QString("TIM2 picture %1 header").arg(i),p,header,"TIM2 picture header and validated sizes");region(QString("TIM2 picture %1 image data").arg(i),p+header,pixels,"TIM2 image byte count");region(QString("TIM2 picture %1 palette").arg(i),p+header+pixels,palette,"TIM2 palette byte count");if(quint8(bytes[p+16])!=0&&!tim2TransferBits(bytes,p)){if(imageEvidence.size()<128)imageEvidence.append(QJsonObject{{"name",QString("TIM2 picture %1: PictFormat 0x%2").arg(i).arg(quint8(bytes[p+16]),0,16)},{"offset",QString::number(p)},{"basis","Bounded TIM2 picture; nonstandard PictFormat storage is not decoded. No linear/swizzle layout was guessed."}});p+=total;continue;}images.append(QJsonObject{{"basis","Bounded TIM2 header; decoding not yet run"},{"width",w},{"height",h},{"settings",QJsonObject{{"offset",QString::number(start)},{"width",1},{"height",1},{"format","Encoded image"},{"surface",i}}}});p+=total;}}
    at=0;for(int hit=0;hit<4&&meshes.size()<4;++hit){at=bytes.indexOf("glTF",at);if(at<0)break;qint64 start=at++;if(start>bytes.size()-20||u32(start+4)!=2)continue;qint64 total=u32(start+8),jsonSize=u32(start+12);if(total<28||start+total>source.expectedSize||jsonSize>1024*1024||start+20+jsonSize>bytes.size()||bytes.mid(start+16,4)!="JSON")continue;
        auto text=bytes.mid(start+20,jsonSize);while(!text.isEmpty()&&(text.back()==0||text.back()==' '||text.back()=='\n'||text.back()=='\r'))text.chop(1);auto document=QJsonDocument::fromJson(text);auto json=document.object();if(json["asset"].toObject()["version"]!="2.0")continue;
        qint64 p=start+20+jsonSize;if(p>bytes.size()-8||bytes.mid(p+4,4)!=QByteArray("BIN\0",4))continue;qint64 binSize=u32(p),base=p+8;if(base+binSize>start+total)continue;auto buffers=json["buffers"].toArray();if(buffers.isEmpty()||buffers[0].toObject().contains("uri"))continue;auto bufferSize=buffers[0].toObject()["byteLength"].toDouble(-1);if(bufferSize<0||bufferSize>binSize)continue;
        region("GLB header",start,12,"GLB v2 magic and declared length");region("GLB JSON chunk",start+12,8+jsonSize,"GLB JSON chunk and parsed accessors");region("GLB BIN chunk header",p,8,"GLB BIN chunk length/type");auto accessors=json["accessors"].toArray(),views=json["bufferViews"].toArray();
        struct Access {qint64 offset=0,count=0,stride=0;int type=0;bool valid=false;};
        auto accessor=[&](QJsonValue index,bool position){Access a;auto integer=[](QJsonValue v,qint64 fallback=-1){double n=v.toDouble(double(fallback));return std::isfinite(n)&&n==std::floor(n)&&n>=0&&n<=268435456?qint64(n):fallback;};auto i=integer(index);if(i<0||i>=accessors.size())return a;auto item=accessors[i].toObject();if(item.contains("sparse")||item["type"]!=(position?"VEC3":"SCALAR"))return a;auto vi=integer(item["bufferView"]);if(vi<0||vi>=views.size())return a;auto view=views[vi].toObject();if(view["buffer"].toInt(-1)!=0||view.contains("extensions"))return a;a.type=item["componentType"].toInt();if(position?a.type!=5126:a.type!=5123&&a.type!=5125)return a;auto width=position?12:a.type==5123?2:4;a.count=integer(item["count"]);a.stride=integer(view.value("byteStride"),width);auto local=integer(item.value("byteOffset"),0),offset=integer(view.value("byteOffset"),0),size=integer(view["byteLength"]);if(a.count<1||a.stride<width||a.stride>65536||local<0||offset<0||size<0||offset+size>bufferSize||local+width>size||(a.count-1)>(size-local-width)/a.stride)return a;a.offset=base+offset+local;a.valid=true;return a;};
        QSet<QString> seen;for(auto mesh:json["meshes"].toArray()){for(auto value:mesh.toObject()["primitives"].toArray()){if(cancel.load())return {{"error","Discovery canceled."}};if(meshes.size()>=4)break;auto primitive=value.toObject();if(primitive["mode"].toInt(4)!=4||primitive.contains("extensions"))continue;auto pos=accessor(primitive["attributes"].toObject()["POSITION"],true),idx=accessor(primitive["indices"],false);if(!pos.valid||!idx.valid)continue;auto key=QString::number(pos.offset)+":"+QString::number(idx.offset);if(seen.contains(key))continue;seen.insert(key);region(QString("Positions at %1").arg(pos.offset),pos.offset,12,"GLB Float32 VEC3 accessor",pos.stride,pos.count);region(QString("Indices at %1").arg(idx.offset),idx.offset,idx.type==5123?2:4,"GLB triangle index accessor",idx.stride,idx.count);
            if(!AssetMemory::fits(pos.count,idx.count)){if(notes.size()<4)notes.append(QString("Candidate with %1 vertices / %2 indices exceeds the configured memory budget; adjust Settings > Performance.").arg(pos.count).arg(idx.count));continue;}
            meshes.append(QJsonObject{{"basis","GLB Float32 local positions and triangle indices; no transforms/skins/materials/packed UV import"},{"settings",QJsonObject{{"vertexOffset",QString::number(pos.offset)},{"vertexCount",pos.count},{"vertexStride",pos.stride},{"positionType","Float32"},{"positionScale",1},{"indexOffset",QString::number(idx.offset)},{"indexCount",idx.count},{"indexStride",idx.stride},{"indexType",idx.type==5123?"UInt16":"UInt32"},{"topology","Triangle list"},{"uv",false},{"normals",false}}}});}}
    }
    for(auto value:encodedImageEvidence(bytes,source.expectedSize,cancel))if(images.size()<128){auto item=value.toObject();auto offset=item["settings"].toObject()["offset"].toString().toLongLong();region("Encoded image header",offset,qMin(qint64(16),source.expectedSize-offset),"Recognized image signature and bounded dimensions");images.append(item);}
    QList<QJsonObject> ordered;for(auto value:meshes)ordered.append(value.toObject());std::stable_sort(ordered.begin(),ordered.end(),[](const auto &a,const auto &b){return a["settings"].toObject()["vertexCount"].toInteger()>b["settings"].toObject()["vertexCount"].toInteger();});meshes={};for(const auto &candidate:ordered)meshes.append(candidate);
    QJsonArray meshEvidence;auto genki=genkiMeshEvidence(bytes,source.expectedSize,&cancel);if(!genki.isEmpty()){QJsonArray combined;for(auto value:genki["regions"].toArray())if(combined.size()<32)combined.append(value);for(auto value:regions)if(combined.size()<32)combined.append(value);regions=combined;genki.remove("regions");auto materialScan=GenkiMaterials::inspect(bytes,genki,cancel);genki["materialReferences"]=materialScan["materials"];genki["textureBanks"]=materialScan["banks"];
        for(auto rawBank:materialScan["banks"].toArray()){auto bank=rawBank.toObject();if(!bank["headerless"].toBool())continue;for(auto rawImage:bank["images"].toArray()){auto image=rawImage.toObject();if(!image["decodeSupported"].toBool())continue;if(images.size()>=128)break;images.append(QJsonObject{{"name",QString("Genki image bank / slot %1").arg(image["index"].toInt())},{"basis","Bounded headerless Indexed8 image; empty external-reference slots are preserved but not decoded."},{"width",image["width"]},{"height",image["height"]},{"settings",QJsonObject{{"offset",bank["offset"]},{"format","Encoded image"},{"container","Genki image bank"},{"surface",image["index"]},{"width",1},{"height",1}}}});}}
        int materialPackets=0;for(auto value:materialScan["candidates"].toArray())materialPackets+=value.toObject()["triangleCount"].toInt();genki["coveredByMaterialParts"]=materialPackets>0&&materialPackets==genki["validatedTriangles"].toInt();for(auto candidate:materialScan["candidates"].toArray())meshes.append(candidate);if(!materialScan["materials"].toArray().isEmpty())notes.append(QString("Genki: %1 material parts, %2 bounded local TIM2 banks. Unresolved references are left untextured.").arg(materialScan["materials"].toArray().size()).arg(materialScan["banks"].toArray().size()));meshEvidence.append(genki);}
    auto arcp=arcpEvidence(bytes,source.expectedSize,cancel);if(!arcp.isEmpty()){for(auto value:arcp["regions"].toArray())if(regions.size()<32)regions.append(value);arcp.remove("regions");meshEvidence.append(arcp);}
    auto a3da=SegaA3da::inspect(bytes,cancel);if(!a3da.isEmpty()){for(auto value:a3da["regions"].toArray())if(regions.size()<32)regions.append(value);a3da.remove("regions");meshEvidence.append(a3da);notes.append(a3da["diagnostic"]);}
    QJsonObject scenery;
    if(bytes.size()>=8&&(u32(0)==0||u32(0)==0x80134000)){
        scenery=length==source.expectedSize?Nfsu2Scene::scan(bytes,cancel):Nfsu2Scene::inspect(source,patches,cancel);
        if(scenery.contains("error"))return {{"error",scenery["error"]}};
        for(auto value:scenery["candidates"].toArray())meshes.append(value);
        for(auto value:scenery["notes"].toArray())notes.append(value);
        for(auto value:scenery["regions"].toArray())if(regions.size()<32)regions.append(value);
    }
    bool fm1Header=bytes.size()>=12&&bytes.mid(8,4)==QByteArray("Car\0",4)&&u32(0)==source.expectedSize;
    auto fm1=Fm1Geometry::scan(bytes,cancel);
    if(fm1Header&&fm1.isEmpty())notes.append("FM1 CARBIN header found, but this part/buffer variant is not supported; no geometry was guessed.");
    for(auto value:fm1["candidates"].toArray())meshes.append(value);
    for(auto value:fm1["notes"].toArray())notes.append(value);
    for(auto value:fm1["regions"].toArray())if(regions.size()<32)regions.append(value);
    auto wmPs3=WmPs3::scan(bytes,cancel);for(auto value:wmPs3["candidates"].toArray())meshes.append(value);for(auto value:wmPs3["images"].toArray())if(images.size()<128)images.append(value);for(auto value:wmPs3["notes"].toArray())notes.append(value);
    auto gmo=PspGmo::scan(bytes,cancel);for(auto value:gmo["candidates"].toArray())meshes.append(value);for(auto value:gmo["notes"].toArray())notes.append(value);
    auto nfsMw=NfsMw::scan(bytes,cancel);for(auto value:nfsMw["candidates"].toArray())meshes.append(value);for(auto value:nfsMw["notes"].toArray())notes.append(value);
    auto idss=InitialDSS::scan(bytes,cancel);for(auto value:idss["candidates"].toArray())meshes.append(value);for(auto value:idss["notes"].toArray())notes.append(value);
    auto rw=RenderWare::scan(bytes,cancel);for(auto value:rw["candidates"].toArray())meshes.append(value);for(auto value:rw["images"].toArray())images.append(value);for(auto value:rw["notes"].toArray())notes.append(value);
    auto blur=Blur::scan(bytes,cancel);for(auto value:blur["images"].toArray())if(images.size()<128)images.append(value);for(auto value:blur["candidates"].toArray())meshes.append(value);for(auto value:blur["notes"].toArray())notes.append(value);
    auto itc=Itc::scan(bytes,cancel);for(auto value:itc["candidates"].toArray())meshes.append(value);for(auto value:itc["images"].toArray())if(images.size()<128)images.append(value);for(auto value:itc["notes"].toArray())notes.append(value);
    if(itc.isEmpty()){const auto packages=Xpr2::find(bytes);if(!packages.isEmpty()){for(auto value:Xpr2::images(bytes,packages,128))if(images.size()<128)images.append(value);notes.append(QString("Xbox 360 XPR2 texture package(s) at %1.").arg([&]{QStringList l;for(auto at:packages)l<<QString("0x%1").arg(at,0,16);return l.join(", ");}()));}}
    auto faf=FafGeometry::scan(bytes,cancel);for(auto value:faf["candidates"].toArray())meshes.append(value);for(auto value:faf["images"].toArray())images.append(value);for(auto value:faf["notes"].toArray())notes.append(value);for(auto value:faf["regions"].toArray())if(regions.size()<32)regions.append(value);
    auto rtuned=RTunedGeometry::scan(bytes,cancel);
    for(auto value:rtuned["candidates"].toArray())meshes.append(value);
    for(auto value:rtuned["notes"].toArray())notes.append(value);
    auto collision=RTunedCollision::scan(bytes,cancel);
    for(auto value:collision["candidates"].toArray())meshes.append(value);
    for(auto value:collision["notes"].toArray())notes.append(value);
    for(auto value:collision["regions"].toArray())if(regions.size()<32)regions.append(value);
    auto profileResults=scenery["recognized"].toBool()?QJsonObject():MeshProfiles::scan(bytes,MeshProfiles::active(),cancel);
    for(auto value:profileResults["candidates"].toArray())meshes.append(value);
    for(auto value:profileResults["notes"].toArray())notes.append(value);
    for(auto value:profileResults["regions"].toArray())if(regions.size()<32)regions.append(value);
    if(cancel.load())return {{"error","Discovery canceled."}};
    auto resources=NativeResources::inspect(source,patches,cancel);
    for(auto value:RTunedCollision::archiveCandidates(source,patches,resources,cancel))meshes.append(value);
    if(bytes.startsWith(QByteArray::fromHex("50616b1b")))notes.append("MSR retail Pak container recognized. Native decompression is supported: open its decoded resource. Supported retail track headers have a native strip adapter and can link matching texture companions. Vehicle/unknown wrappers remain separate.");
    if(bytes.startsWith(QByteArray::fromHex("474f5448504f4b1a")))notes.append("PGR1 GOTHPOK container recognized. Open its decoded resource to inspect the native LZ payload. Open the original .DyP/.NtP with matching T/V files to scan textured track material groups. Supported primary vehicle LODs now have a native material decoder with _v and _res companions.");
    if(resources["format"]=="PGR2 indexed resources")notes.append("PGR2 indexed resources: open/extract named sections using the dedicated bounded sync-flush decompressor. Validated MESH vehicle groups have a separate native decoder. Supported WMSH base tracks have a separate bounded native adapter. Supported compact INST objects use source placements; command-based INST objects and sibling stream assembly remain pending; a GPU buffer alone is not an assembled model.");
    if(resources["format"]=="PGR chunk resources")notes.append("PGR3/4 vehicles and supported static track draws have direct native model candidates. Track scenes require the matching PAK and STREAM2 together; instanced trees and original shaders remain separate. Archive resources belong to Scan File, not the 3D result list.");
    auto pgr34track=Pgr34Track::scan(source,patches,cancel);for(auto v:pgr34track["candidates"].toArray())meshes.append(v);for(auto v:pgr34track["notes"].toArray())notes.append(v);
    auto pgr34=Pgr34Geometry::scan(source,patches,cancel);for(auto v:pgr34["candidates"].toArray())meshes.append(v);for(auto v:pgr34["notes"].toArray())notes.append(v);
    auto pgrCar=Pgr1Car::scan(source,patches,cancel);for(auto v:pgrCar["candidates"].toArray())meshes.append(v);for(auto v:pgrCar["notes"].toArray())notes.append(v);
    auto pgrTrack=Pgr1Track::scan(source,patches,cancel);for(auto v:pgrTrack["candidates"].toArray())meshes.append(v);for(auto v:pgrTrack["notes"].toArray())notes.append(v);
    auto pgr2Track=Pgr2Track::scan(source,patches,resources,cancel);for(auto v:pgr2Track["candidates"].toArray())meshes.append(v);for(auto v:pgr2Track["notes"].toArray())notes.append(v);
    auto pgrVehicle=Pgr2Geometry::scan(source,patches,resources,cancel);for(auto v:pgrVehicle["candidates"].toArray())meshes.append(v);for(auto v:pgrVehicle["notes"].toArray())notes.append(v);
    auto msrBytes=bytes;if(bytes.startsWith(QByteArray::fromHex("50616b1b"))){QString error;msrBytes=MsrAlpha::decodedSource(source,patches,cancel,error);if(!error.isEmpty())notes.append(error);}
    auto msrAlpha=MsrAlpha::scan(msrBytes,cancel);for(auto v:msrAlpha["candidates"].toArray())meshes.append(v);for(auto v:msrAlpha["notes"].toArray())notes.append(v);
    auto fm1Track=Fm1Track::scan(source,patches,resources,cancel);
    for(auto v:fm1Track["candidates"].toArray())meshes.append(v);
    for(auto v:fm1Track["images"].toArray())images.append(v);
    for(auto v:fm1Track["notes"].toArray())notes.append(v);
    auto fm1Archive=Fm1Geometry::scanArchive(source,patches,resources,cancel);
    for(auto value:fm1Archive["candidates"].toArray())meshes.append(value);
    for(auto value:fm1Archive["notes"].toArray())notes.append(value);
    auto visualCourse=RTunedVisualScene::scanArchive(source,patches,resources,cancel);
    for(auto value:visualCourse["candidates"].toArray())meshes.append(value);
    for(auto value:visualCourse["notes"].toArray())notes.append(value);

    auto linkedNfs=LinkedSurfaces::nfsScan(bytes,meshes,cancel,source);if(linkedNfs.contains("candidates")){meshes=linkedNfs["candidates"].toArray();for(auto image:linkedNfs["images"].toArray())images.append(image);}
    auto linkedRt=LinkedSurfaces::rtunedScan(source,patches,resources,cancel);if(linkedRt.contains("candidates")){QJsonArray retained;for(auto value:meshes)if(value.toObject()["profileId"]!="native-rtuned-visual-scene")retained.append(value);for(auto value:linkedRt["candidates"].toArray())retained.append(value);meshes=retained;for(auto value:linkedRt["images"].toArray())images.append(value);}if(linkedRt.contains("error"))notes.append(linkedRt["error"]);
    // Generic PS2 VIF analysis is a fallback for unknown formats: it runs only when no native decoder produced a model.
    if(meshes.isEmpty()&&meshEvidence.isEmpty()){auto vif=Ps2Vif::scan(bytes,cancel);if(cancel.load())return {{"error","Discovery canceled."}};
        for(auto value:vif["candidates"].toArray())meshes.append(value);for(auto value:vif["notes"].toArray())notes.append(value);}
    QJsonObject geometrySearch;bool nativeMesh=faf["recognized"].toBool();for(auto evidence:meshEvidence)nativeMesh|=evidence.toObject()["renderSupported"].toBool()||evidence.toObject()["family"].toString().startsWith("Genki");if(findGeometry&&!fm1Header&&!a3da["metadataOnly"].toBool()&&meshes.isEmpty()&&!nativeMesh&&resources["entries"].toArray().isEmpty()&&!resources.contains("error")){geometrySearch=GeometryCandidates::scan(bytes,cancel);for(auto candidate:geometrySearch["candidates"].toArray())meshes.append(candidate);}
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return {{"error",final.error}};
    return GenkiTrack::decorate(QJsonObject{{"geometrySearch",geometrySearch},{"resources",resources},{"scannedBytes",QString::number(length)},{"scanTruncated",length<source.expectedSize},{"imageCandidates",images},{"imageEvidence",imageEvidence},{"meshCandidates",meshes},{"meshEvidence",meshEvidence},{"profileMatches",profileResults["matches"]},{"structureRegions",regions},{"notes",notes},{"status","Metadata only. Copy exact settings; run the decoder to confirm. Empty candidates do not prove absence of assets."}},bytes,source,trackContext,cancel);
}
