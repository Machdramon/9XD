#pragma once
#include <QWidget>
#include <QPointer>
#include <QVector3D>
#include "mesh_scene.h"
#include "reference_grid.h"
class HexView;
struct MeshVisualStyle {QColor face,vertex,text;bool faceGradient,vertexGradient;QVector<QColor> stops;QColor background,grid,selection;};
class VulkanPanel final:public QWidget {
    Q_OBJECT
public:
    explicit VulkanPanel(QWidget *parent=nullptr,MeshScene *sharedScene=nullptr,bool embedded=false);
    ~VulkanPanel()override;
    // 0 single, 1 two side by side, 2 two stacked, 3 four views.
    void setViewportLayout(int layout);
    // 0 perspective; 1 front, 2 right, 3 top, 4 back, 5 left, 6 bottom (orthographic).
    void setViewOrientation(int view);
    // 0 realistic, 1 cel, 2 simple shading, 3 wireframe.
    void setRenderStyle(int style);
    void setNormalMode(int mode); // 0 source vertex normals, 1 geometric face normals.
    // 0 select, 1 move, 2 rotate, 3 scale. Handles act on scene transforms.
    void setTransformTool(int tool);
    // Selection tool: clicks add or remove meshes from the selection. Off by default.
    void setSelectionTool(bool enabled);
    bool selectionTool()const;
    // Camera pose in model units (eye position, yaw/pitch in degrees, distance to the orbit pivot).
    struct CameraPose {QVector3D position;double yaw=0,pitch=0,distance=1;};
    CameraPose cameraPose()const;
    bool setCameraPose(const CameraPose &pose);
    // Moves a panel (Transform / Material) into a foldable card in the viewport's top-right corner.
    void hostInspector(QWidget *content,const QString &title,bool open=false);
    void setBackgroundGradient(bool enabled,QColor top,QColor bottom);
    // Home resets orientation; All/Selected preserve the current viewing direction.
    bool frameView(int mode); // 0 Home, 1 All visible, 2 Selected, 3 Reference grid
    // Manual clipping distances use original model units; automatic follows camera and scene bounds.
    bool setFieldOfView(double degrees); // Vertical perspective FOV, 10..120 degrees.
    bool setCameraClipping(bool automatic,double nearWorld=.01,double farWorld=100000);
    // Reference grid in model units (spacing, major interval, half size, number labels); also applied to tile views.
    void setIgnoreTextureAlpha(bool enabled);
    bool ignoreTextureAlpha()const;
    void setReferenceGrid(const ReferenceGridSettings &settings,bool visible=true);
    QPointF gizmoAxisPosition(int axis)const;
    std::shared_ptr<const MeshData> meshSnapshot()const;
    // Scripting replaces only the active entry; null clears the scene.
    void setMesh(std::shared_ptr<MeshData> mesh);
    MeshScene *scene()const{return scene_;}
    MeshVisualStyle visualStyle()const;
    QWidget *createMeshControls(HexView *view);
    QWidget *createMeshCollection();
signals:
    void scanRequested();
    void visualStyleChanged();
    void cameraChanged();
    void sourceMaterialRequested(const QJsonObject &candidate);
    void meshCalculationFinished(bool success,const QString &message,quint64 scanRequest=0);
private:
    void syncToolButtons();
    QWidget *surface_=nullptr;
    MeshScene *scene_=nullptr;
    QWidget *viewportHost_=nullptr,*primaryViewport_=nullptr,*viewGrid_=nullptr;
    QVector<QPointer<VulkanPanel>> tileViews_;
    bool embedded_=false;
};
