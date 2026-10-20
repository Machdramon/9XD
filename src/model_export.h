#pragma once
#include "mesh_data.h"
#include <QColor>
#include <QStringList>
#include <atomic>
#include <QImage>
#include "mesh_material.h"
enum class ModelFormat { Obj, Glb, Stl, Fbx };
enum class ExportNormals { Preserve, Recalculate, Omit };
struct ExportMesh {QString name,source;std::shared_ptr<const MeshData> data;QImage texture;MeshMaterial surface;};
bool fbxExportAvailable();
struct ModelExportOptions {
    ModelFormat format=ModelFormat::Glb;
    double scale=1;
    std::array<int,3> axes{0,1,2};
    std::array<bool,3> negate{false,false,false};
    bool uvs=true,flipV=false,reverseFaces=false;
    ExportNormals normals=ExportNormals::Preserve;
    bool material=false,doubleSided=false;
    bool sceneMaterials=true,textures=true;
    QColor color{"#78B4E8"};
    QStringList protectedPaths;
};
struct ExportProgress {std::atomic_bool cancel{false};std::atomic_int percent{0};};
struct ModelExportResult {bool ok=false,cancelled=false;QString error;quint64 vertices=0,faces=0;};
ModelExportResult exportModel(const QString &path,const QVector<ExportMesh> &,const ModelExportOptions &,const std::shared_ptr<ExportProgress> &progress={});
