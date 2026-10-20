#pragma once
#include "arcp_evidence.h"
#include "mesh_data.h"
inline MeshResult readArcpGeometry(ReadRequest source,const PatchList &patches,const std::atomic_bool &cancel,bool faces){
    MeshResult result;auto fail=[&](QString error){result.error=error;return result;};
    auto limit=qMin(source.expectedSize,qint64(32*1024*1024));QByteArray bytes;
    for(qint64 offset=0;offset<limit;){if(cancel.load())return fail("Point preview canceled.");auto request=source;request.offset=offset;request.length=qMin(qint64(1048576),limit-offset);auto part=readPage(request);if(!part.error.isEmpty())return fail(part.error);if(part.bytes.size()!=request.length)return fail("Incomplete point preview read.");applyPatches(part.bytes,offset,patches);bytes+=part.bytes;offset+=part.bytes.size();}
    auto evidence=arcpEvidence(bytes,source.expectedSize,cancel);
    if(cancel.load())return fail("Point preview canceled.");if(!evidence["previewSupported"].toBool())return fail(evidence["diagnostic"].toString("No supported ARCP VIF position streams found."));
    if(faces&&!evidence["renderSupported"].toBool())return fail("ARCP triangle-strip controls could not be fully validated.");
    auto mesh=std::make_shared<MeshData>();auto f32=[&](qint64 p){auto bits=qFromLittleEndian<quint32>(bytes.constData()+p);float value;std::memcpy(&value,&bits,4);return value;};
    for(auto value:evidence["previewChunks"].toArray()){auto chunk=value.toObject();auto position=chunk["positionsOffset"].toString().toLongLong(),uv=chunk["uvOffset"].toString().toLongLong(),normal=chunk["normalOffset"].toString().toLongLong();int count=chunk["vertexCount"].toInt();int base=mesh->vertices.size();
        for(int i=0;i<count;++i){if(cancel.load())return fail("Point preview canceled.");MeshVertex v{};for(int a=0;a<3;++a){v.position[a]=f32(position+i*12+a*4);v.normal[a]=qMax(-1.f,float(qint8(bytes[normal+i*4+a]))/127.f);}v.uv[0]=f32(uv+i*8);v.uv[1]=f32(uv+i*8+4);mesh->vertices.append(v);mesh->vertexOffsets.append(position+i*12);}
        if(faces)for(auto lengthValue:chunk["stripLengths"].toArray()){int length=lengthValue.toInt();for(int t=0;t<length;++t){int a=base+t,b=a+1,c=a+2;if(t&1)std::swap(a,b);mesh->triangles.append(quint32(a));mesh->triangles.append(quint32(b));mesh->triangles.append(quint32(c));}base+=length+2;}}

    auto check=source;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return fail(final.error);result.mesh=mesh;return result;
}

inline MeshResult readArcpPointPreview(ReadRequest source,const PatchList &patches,const std::atomic_bool &cancel){return readArcpGeometry(source,patches,cancel,false);}
