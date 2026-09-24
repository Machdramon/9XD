#pragma once
#include "mesh_data.h"
#include <QJsonObject>
QJsonObject scriptMeshJson(const std::shared_ptr<const MeshData> &mesh);
std::shared_ptr<MeshData> scriptMeshFromJson(const QJsonObject &json,QString &error);
