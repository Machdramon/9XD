#pragma once
#include "genki_mesh_evidence.h"
#include "mesh_data.h"
inline MeshResult readGenkiGeometry(ReadRequest source,const PatchList &patches,const std::atomic_bool &cancel){
    MeshResult result;auto fail=[&](QString error){result.error=error;return result;};QByteArray bytes;auto limit=qMin(source.expectedSize,qint64(32*1024*1024));
    for(qint64 offset=0;offset<limit;){if(cancel.load())return fail("Genki decoding canceled.");auto request=source;request.offset=offset;request.length=qMin(qint64(1048576),limit-offset);auto part=readPage(request);if(!part.error.isEmpty())return fail(part.error);if(part.bytes.size()!=request.length)return fail("Incomplete Genki read.");applyPatches(part.bytes,offset,patches);bytes+=part.bytes;offset+=part.bytes.size();}
    auto evidence=genkiMeshEvidence(bytes,source.expectedSize,&cancel);if(cancel.load())return fail("Genki decoding canceled.");if(!evidence["renderSupported"].toBool())return fail(evidence["diagnostic"].toString("No complete supported Genki packet model found."));
    auto mesh=std::make_shared<MeshData>();mesh->hasUvs=true;int strip=0;
    for(auto value:evidence["previewChunks"].toArray()){if(cancel.load())return fail("Genki decoding canceled.");const auto chunk=value.toObject();auto position=chunk["positionsOffset"].toString().toLongLong(),uv=chunk["uvOffset"].toString().toLongLong(),normal=chunk["normalOffset"].toString().toLongLong();int count=chunk["vertexCount"].toInt(),base=mesh->vertices.size();
        for(int i=0;i<count;++i){MeshVertex vertex{};for(int a=0;a<3;++a){auto bits=qFromLittleEndian<quint32>(bytes.constData()+position+i*12+a*4);if(a==1)bits&=~quint32(1);std::memcpy(&vertex.position[a],&bits,4);if(normal>=0)vertex.normal[a]=qMax(-1.f,float(qint8(bytes[normal+i*3+a]))/127.f);}if(uv>=0)for(int a=0;a<2;++a)vertex.uv[a]=qFromLittleEndian<qint16>(bytes.constData()+uv+i*4+a*2)/4096.f;mesh->vertices.append(vertex);mesh->vertexOffsets.append(position+i*12);}
        auto indices=genkiPacketFaces(bytes,position,count,chunk["tagParity"].toInt());for(int i=0;i<indices.size();i+=3){MeshFaceSource face;face.strip=strip;face.indexSlots={-1,-1,-1};for(int a=0;a<3;++a){mesh->triangles.append(base+indices[i+a]);face.offsets[a]=position+indices[i+a]*12;}mesh->faceSources.append(face);}++strip;
    }
    // Preserve stored normals; generate missing normals from recovered faces. Isolated unused records get a neutral normal.
    QVector<std::array<double,3>> sums(mesh->vertices.size());for(int i=0;i<mesh->triangles.size();i+=3){auto a=mesh->vertices[mesh->triangles[i]],b=mesh->vertices[mesh->triangles[i+1]],c=mesh->vertices[mesh->triangles[i+2]];double u[3],v[3];for(int k=0;k<3;++k){u[k]=double(b.position[k])-a.position[k];v[k]=double(c.position[k])-a.position[k];}std::array<double,3> n{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};for(int j=0;j<3;++j)for(int k=0;k<3;++k)sums[mesh->triangles[i+j]][k]+=n[k];}
    for(int i=0;i<mesh->vertices.size();++i){auto &v=mesh->vertices[i];double length=std::hypot(v.normal[0],v.normal[1],v.normal[2]);if(length<1e-12){auto n=sums[i];length=std::hypot(n[0],n[1],n[2]);if(length<1e-12){v.normal[1]=1;}else for(int k=0;k<3;++k)v.normal[k]=n[k]/length;}}
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty())return fail(final.error);result.mesh=mesh;return result;
}
