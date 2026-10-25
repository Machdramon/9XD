#pragma once
#include "mesh_material.h"
#include <QImage>
#include <QMap>
struct MeshSurface {QImage texture;QString textureName;MeshMaterial material;QMap<QString,QImage> textureLibrary;};
