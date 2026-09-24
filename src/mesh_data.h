#pragma once
#include "binary_document.h"
#include <QVector>
#include <memory>
#include <array>
struct MeshVertex { float position[3],normal[3],uv[2]; };
struct MeshFaceSource { std::array<qint64,3> offsets; std::array<qint64,3> indexSlots; int strip=-1; };
struct MeshData { bool hasUvs=true; QVector<MeshVertex> vertices;QVector<quint32> triangles;QVector<qint64> vertexOffsets;QVector<MeshFaceSource> faceSources; };
struct MeshSpec {
    qint64 vertexOffset=128,vertexCount=128,vertexStride=12,indexOffset=1664,indexCount=402,indexStride=2,uvOffset=2480,uvStride=4,normalOffset=2992,normalStride=12;
    QString positionType="Int32",indexType="UInt16",uvType="Float16",normalType="Float32";
    bool bigEndian=false,strip=true,uv=true,normals=true;
    double positionScale=.0001;
};
struct MeshResult { std::shared_ptr<MeshData> mesh;QString error; };
MeshResult readMesh(const ReadRequest &,const PatchList &,const MeshSpec &);
