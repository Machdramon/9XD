#pragma once
#include "mesh_data.h"
#include "memory_budget.h"
#include "native_resources.h"
#include <QMap>
#include <algorithm>
#include <cmath>

// FM1 Xbox CARBIN only. Parse bounded named chunks; never scan arbitrary strings as geometry.
namespace Fm1Geometry {
struct Draw {qint64 offset=0,count=0;int set=0,material=0;};
struct Part {QString name;qint64 offset=0,vertices=0,vertexOffset=0;QVector<Draw> draws;qint64 triangles=0;};
inline QVector<Part> parse(const QByteArray &b,const std::atomic_bool &cancel){
    auto fits=[&](qint64 p,qint64 n,qint64 end){return p>=0&&n>=0&&end<=b.size()&&p<=end&&n<=end-p;};
    auto u=[&](qint64 p){return qFromLittleEndian<quint32>(b.constData()+p);};
    auto chunk=[&](qint64 at,qint64 parent,const char *name,qint64 &end,qint64 &data){if(!fits(at,9,parent))return false;qint64 size=u(at);if(size<9||!fits(at,size,parent))return false;end=at+size;auto zero=b.indexOf('\0',at+8);if(zero<0||zero>=end||b.mid(at+8,zero-at-8)!=name)return false;data=zero+1;return true;};
    if(b.size()<12||u(0)!=b.size()||u(4)!=3||b.mid(8,4)!=QByteArray("Car\0",4))return {};
    qint64 p=12,end=0,data=0;if(!chunk(p,b.size(),"Attribs",end,data))return {};p=end;if(!chunk(p,b.size(),"Driver",end,data))return {};p=end;if(!fits(p,4,b.size()))return {};auto count=u(p);p+=4;if(count<1||count>4096)return {};QVector<Part> parts;
    for(quint32 i=0;i<count;++i){if(cancel.load())return {};Part part;part.offset=p;qint64 partEnd=0;if(!chunk(p,b.size(),"Part",partEnd,data)||!fits(data,8,partEnd))return {};qint64 nameSize=qFromLittleEndian<quint16>(b.constData()+data+4);p=data+8;if(nameSize<1||nameSize>1024||!fits(p,nameSize,partEnd))return {};part.name=QString::fromUtf8(b.mid(p,nameSize));p+=nameSize;
        if(!chunk(p,partEnd,"Verts",end,data)||!fits(data,4,end))return {};part.vertices=u(data);part.vertexOffset=data+4;if(part.vertices<3||!AssetMemory::fits(part.vertices,3)||!fits(part.vertexOffset,part.vertices*20,end)||part.vertexOffset+part.vertices*20!=end)return {};p=end;
        if(!chunk(p,partEnd,"Damage",end,data)||!fits(data,4,end))return {};qint64 damage=u(data);if((damage!=0&&damage!=part.vertices)||!fits(data+4,damage*12,end)||data+4+damage*12!=end)return {};p=end;
        if(!chunk(p,partEnd,"Pivots",end,data))return {};p=end;if(!chunk(p,partEnd,"Attributes",end,data))return {};p=end;if(!fits(p,4,partEnd))return {};auto draws=u(p);p+=4;if(draws<1||draws>65536)return {};
        for(quint32 j=0;j<draws;++j){if(cancel.load())return {};if(!chunk(p,partEnd,"Subpart",end,data)||!fits(data,32,end))return {};Draw draw;draw.set=u(data);draw.material=u(data+4);draw.count=u(data+12);draw.offset=data+32;if(draw.set<1||draw.set>32||u(data+8)!=6||draw.count<3||!AssetMemory::fits(part.vertices,draw.count)||!fits(draw.offset,draw.count*2,end)||draw.offset+draw.count*2!=end)return {};qint64 run=0;quint16 a=0,c=0;for(qint64 k=0;k<draw.count;++k){if((k&4095)==0&&cancel.load())return {};auto v=qFromLittleEndian<quint16>(b.constData()+draw.offset+k*2);if(v>=part.vertices)return {};if(draw.set==1&&run>=2&&a!=c&&a!=v&&c!=v)++part.triangles;a=c;c=v;++run;}part.draws.append(draw);p=end;}
        if(p!=partEnd||!part.triangles||!AssetMemory::fits(part.vertices,part.triangles*3))return {};parts.append(part);p=partEnd;
    }
    // Non-mesh flare and line records follow the part array in the observed car variant.
    int extras=0;while(p<b.size()){if(cancel.load()||++extras>4096)return {};if(!chunk(p,b.size(),"Flare",end,data)&&!chunk(p,b.size(),"Line",end,data))return {};p=end;}return parts;
}
inline QJsonObject scan(const QByteArray &b,const std::atomic_bool &cancel){
    auto parts=parse(b,cancel);if(parts.isEmpty())return {};QJsonArray candidates,regions;
    std::stable_sort(parts.begin(),parts.end(),[](const Part &a,const Part &b){return a.vertices>b.vertices;});
    for(const auto &p:parts){QJsonObject settings{{"vertexOffset",QString::number(p.vertexOffset)},{"vertexCount",p.vertices},{"indexCount",p.triangles*3}};candidates.append(QJsonObject{{"name","FM1 / "+p.name+" / draw set 1"},{"profileId","native-fm1-carbin"},{"profileName","Forza Motorsport 1 CARBIN"},{"nativeDecoder",true},{"objectOffset",QString::number(p.offset)},{"triangleCount",p.triangles},{"settings",settings},{"basis","Bounded CARBIN parts and independent Xbox triangle strips (primitive 6). Int16 XYZ / 256 and source SNorm16 normals; clockwise faces converted. Draw set 1 only; alternative draw sets and damage buffers are not merged. UV encoding, materials, textures and pivot animation are not applied."}});if(regions.size()<32)regions.append(QJsonObject{{"name",p.name+" vertices"},{"offset",QString::number(p.vertexOffset)},{"length",QString::number(p.vertices*20)},{"evidence","CARBIN Verts chunk: 20 bytes per vertex"}});}
    return {{"candidates",candidates},{"regions",regions},{"notes",QJsonArray{"FM1 CARBIN: select the desired parts (Ctrl/Shift or Select All) and Preview. Only draw set 1 is loaded to avoid overlapping alternatives. Textures, damage and pivots are not applied."}}};
}
inline QJsonObject scanArchive(ReadRequest source,const PatchList &patches,const QJsonObject &resources,const std::atomic_bool &cancel){
    if(resources["format"]!="Forza Motorsport CAB")return {};QJsonArray candidates,notes;qint64 used=0;
    for(auto value:resources["entries"].toArray()){auto row=value.toObject();if(!row["name"].toString().endsWith(".carbin",Qt::CaseInsensitive)||!row["supported"].toBool())continue;qint64 size=row["decodedBytes"].toString().toLongLong();if(size<12||size>NativeResources::MaxOutput||size>AssetMemory::bytes()/4-used){notes.append("A CARBIN resource exceeds the scan budget; open it separately.");continue;}used+=size;auto data=NativeResources::extract(source,patches,row,cancel);if(!data.error.isEmpty()){notes.append(data.error);continue;}auto result=scan(data.bytes,cancel);for(auto v:result["candidates"].toArray()){auto c=v.toObject();c["fm1Resource"]=row;c["name"]=row["name"].toString()+" / "+c["name"].toString();candidates.append(c);}for(auto n:result["notes"].toArray())notes.append(n);}
    return {{"candidates",candidates},{"notes",notes}};
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel){
    MeshResult out;bool compressed=candidate.contains("fm1Resource");QByteArray bytes;QString error;
    if(compressed){auto data=NativeResources::extract(source,patches,candidate["fm1Resource"].toObject(),cancel);bytes=data.bytes;error=data.error;}else bytes=NativeResources::read(source,patches,0,source.expectedSize,cancel,error);
    if(!error.isEmpty()){out.error=error;return out;}auto parts=parse(bytes,cancel);auto at=candidate["objectOffset"].toString().toLongLong();auto found=std::find_if(parts.begin(),parts.end(),[&](const Part &p){return p.offset==at;});if(found==parts.end()){out.error="CARBIN part no longer matches its scan, or its layout is unsupported.";return out;}
    auto memory=std::make_shared<MemoryData>();memory->size=bytes.size();for(qint64 p=0;p<bytes.size();p+=MemoryData::BlockSize)memory->blocks.append(bytes.mid(p,MemoryData::BlockSize));ReadRequest child;child.memory=memory;child.expectedSize=bytes.size();out.mesh=std::make_shared<MeshData>();out.mesh->hasUvs=false;
    for(const auto &draw:found->draws){if(draw.set!=1)continue;if(cancel.load()){out.mesh.reset();out.error="CARBIN preview canceled.";return out;}MeshSpec s;s.vertexOffset=found->vertexOffset;s.vertexCount=found->vertices;s.vertexStride=20;s.positionType="Int16";s.positionScale=1./256;s.normals=true;s.normalOffset=found->vertexOffset+6;s.normalStride=20;s.normalType="SNorm16";s.uv=false;s.indexOffset=draw.offset;s.indexCount=draw.count;s.indexStride=2;s.indexType="UInt16";auto result=readMesh(child,{},s);if(!result.mesh){out.mesh.reset();out.error=result.error;return out;}if(out.mesh->vertices.isEmpty()){out.mesh->vertices=result.mesh->vertices;if(!compressed)out.mesh->vertexOffsets=result.mesh->vertexOffsets;}
        // Xbox source winding is clockwise relative to its outward source normals.
        for(qsizetype i=0;i<result.mesh->triangles.size();i+=3)out.mesh->triangles<<result.mesh->triangles[i]<<result.mesh->triangles[i+2]<<result.mesh->triangles[i+1];
        if(!compressed)for(auto face:result.mesh->faceSources){std::swap(face.offsets[1],face.offsets[2]);std::swap(face.indexSlots[1],face.indexSlots[2]);out.mesh->faceSources.append(face);}
    }
    // Some stored vertices have no normal (including unused vertices). Repair only those.
    QVector<bool> missing;for(const auto &v:out.mesh->vertices)missing.append(v.normal[0]==0&&v.normal[1]==0&&v.normal[2]==0);
    for(qsizetype i=0;i<out.mesh->triangles.size();i+=3){auto ia=out.mesh->triangles[i],ib=out.mesh->triangles[i+1],ic=out.mesh->triangles[i+2];const auto &a=out.mesh->vertices[ia],&b=out.mesh->vertices[ib],&c=out.mesh->vertices[ic];double u[3],v[3];for(int k=0;k<3;++k){u[k]=b.position[k]-a.position[k];v[k]=c.position[k]-a.position[k];}double n[]{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};for(auto id:{ia,ib,ic})if(missing[id])for(int k=0;k<3;++k)out.mesh->vertices[id].normal[k]+=float(n[k]);}
    for(qsizetype i=0;i<missing.size();++i)if(missing[i]){auto &n=out.mesh->vertices[i].normal;double length=std::hypot(double(n[0]),double(n[1]),double(n[2]));if(length>0)for(auto &v:n)v=float(v/length);else n[1]=1;}
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty()){out.mesh.reset();out.error=final.error;}return out;
}
}
