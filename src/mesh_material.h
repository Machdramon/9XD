#pragma once
#include <QColor>
#include <QString>
#include <QJsonObject>
// Portable surface description. Custom GPU programs are intentionally separate.
struct MeshMaterial {
    QString name="Default";
    QColor baseColor=Qt::white;
    bool unlit=false;
    QJsonObject sourceBindings;
    bool doubleSided=false,alphaMask=false;
};
