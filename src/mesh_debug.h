#pragma once
#include "mesh_data.h"
#include <QMatrix4x4>
#include <QPointF>
#include <QSize>
#include <QVector3D>
#include <QColor>

// IDs refer to the original vertex array and the decoded triangle array, never GPU copies.
enum class MeshElement { Vertex, Face, Mesh };
struct MeshLabel { int id; QVector3D anchor; QPointF pixel; float depth; };
class MeshDebug {
public:
    explicit MeshDebug(const MeshData &mesh,const MeshData *bounds=nullptr,const MeshDebug *framing=nullptr);
    QVector3D center;
    double radius=1;
    QVector<QVector3D> positions;
    QVector<quint32> indices;
    bool visible(QVector3D eye,QVector3D anchor)const;
    int hit(QVector3D origin,QVector3D direction,float maxDistance=1e30f)const;
    QVector<MeshLabel> labels(const QMatrix4x4 &mvp,QVector3D eye,QSize viewport,MeshElement kind,int first,int count)const;
    int pick(const QMatrix4x4 &mvp,QVector3D eye,QSize viewport,QPointF pixel,MeshElement kind)const;
private:
    struct Node { QVector3D low,high;int begin=0,end=0,left=-1,right=-1; };
    QVector<Node> nodes_;QVector<int> order_;
    int build(int begin,int end);
};
QVector<MeshVertex> labelGeometry(const QVector<MeshLabel> &,QSize viewport,MeshElement kind,float scale,int selected,bool selectedOnly=false,bool drawText=true);
QColor indexColor(int id,int count,const QVector<QColor> &stops);
