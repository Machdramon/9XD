#pragma once
#include <QWidget>
#include "mesh_scene.h"
class HexView;
struct MeshVisualStyle {QColor face,vertex,text;bool faceGradient,vertexGradient;QVector<QColor> stops;};
class VulkanPanel final:public QWidget {
    Q_OBJECT
public:
    explicit VulkanPanel(QWidget *parent=nullptr);
    std::shared_ptr<const MeshData> meshSnapshot()const;
    // Scripting replaces only the active entry; null clears the scene.
    void setMesh(std::shared_ptr<MeshData> mesh);
    MeshScene *scene()const{return scene_;}
    MeshVisualStyle visualStyle()const;
    QWidget *createMeshControls(HexView *view);
    QWidget *createMeshCollection();
signals:
    void visualStyleChanged();
private:
    QWidget *surface_=nullptr;
    MeshScene *scene_=nullptr;
};
