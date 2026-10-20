#pragma once
#include <QColor>
#include <QString>
// Portable surface description. Custom GPU programs are intentionally separate.
struct MeshMaterial {
    QString name="Default";
    QColor baseColor=Qt::white;
    bool unlit=false;
};
