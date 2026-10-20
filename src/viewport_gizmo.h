#pragma once
#include "mesh_scene.h"
#include <QMatrix4x4>
#include <QVector2D>
#include <QtMath>
#include <cmath>
#include <limits>
#include <numbers>
#include <QRectF>

namespace ViewportGizmo {
struct Segment {QPointF a,b;int axis;};
struct Triangle {QPointF a,b,c;QColor color;int axis=-1;};
struct Shape {
    QPointF pivot;
    QVector<Segment> segments;
    QVector<Triangle> triangles;
    std::array<QPointF,3> tips{};
    std::array<QPointF,3> vectors{};
    double worldLength=0;
    bool valid=false;
};
inline QPointF project(QVector3D position,const QMatrix4x4 &matrix,QSize size,bool *visible=nullptr){
    auto clip=matrix*QVector4D(position,1);bool valid=clip.w()>1e-6&&std::isfinite(clip.w());auto p=valid?clip.toVector3DAffine():QVector3D();valid=valid&&p.z()>=0&&p.z()<=1;
    if(visible)*visible=valid;return {(p.x()+1)*size.width()*.5,(p.y()+1)*size.height()*.5};
}
inline QMatrix4x4 rotation(const std::array<double,3> &degrees){QMatrix4x4 r;r.rotate(degrees[2],0,0,1);r.rotate(degrees[1],0,1,0);r.rotate(degrees[0],1,0,0);return r;}
inline std::array<double,3> euler(const QMatrix4x4 &r){
    double y=std::asin(qBound(-1.,-double(r(2,0)),1.)),x=0,z=0;
    if(std::abs(std::cos(y))>1e-5){x=std::atan2(r(2,1),r(2,2));z=std::atan2(r(1,0),r(0,0));}else x=std::atan2(-r(1,2),r(1,1));
    return {qRadiansToDegrees(x),qRadiansToDegrees(y),qRadiansToDegrees(z)};
}
inline Shape shape(const MeshEntry *entry,const MeshDebug *frame,const QMatrix4x4 &matrix,QSize size,int tool,float distance){
    Shape s;if(!entry||!entry->visible||!frame||tool==0||size.isEmpty())return s;
    QVector3D pivot(float(entry->positionDelta[0]),float(entry->positionDelta[1]),float(entry->positionDelta[2]));pivot=(pivot-frame->center)/float(frame->radius);
    bool visible=false;s.pivot=project(pivot,matrix,size,&visible);if(!visible||!QRectF(QPointF(),size).adjusted(-100,-100,100,100).contains(s.pivot))return s;
    s.valid=true;auto clip=matrix*QVector4D(pivot,1);double extent=std::abs(matrix(3,0))+std::abs(matrix(3,1))+std::abs(matrix(3,2))>1e-6?clip.w():distance;double length=qMax(.00001,extent*.41421356*160/qMax(1,size.height()));s.worldLength=length*frame->radius;
    auto line=[&](QPointF a,QPointF b,QColor color,int axis,double width=2.){auto delta=b-a;double len=std::hypot(delta.x(),delta.y());if(len<.1)return;QPointF n(-delta.y()/len*width*.5,delta.x()/len*width*.5);s.triangles.append({a+n,b+n,b-n,color,axis});s.triangles.append({a+n,b-n,a-n,color,axis});s.segments.append({a,b,axis});};
    auto square=[&](QPointF p,QColor color,int axis=-1){s.triangles.append({p+QPointF(-4,-4),p+QPointF(4,-4),p+QPointF(4,4),color,axis});s.triangles.append({p+QPointF(-4,-4),p+QPointF(4,4),p+QPointF(-4,4),color,axis});};
    for(int axis=0;axis<3;++axis){QColor color=axis==0?QColor("#f06060"):axis==1?QColor("#65d080"):QColor("#609eff");QVector3D direction;direction[axis]=1;
        if(tool==3)direction=rotation(entry->rotationDegrees).mapVector(direction);
        auto tip=project(pivot+direction*length,matrix,size);s.tips[axis]=tip;s.vectors[axis]=tip-s.pivot;
        if(tool==2){QVector3D a,b;a[(axis+1)%3]=1;b[(axis+2)%3]=1;auto ap=project(pivot+a*length,matrix,size)-s.pivot,bp=project(pivot+b*length,matrix,size)-s.pivot;if(std::abs(ap.x()*bp.y()-ap.y()*bp.x())<80)continue;QPointF previous;for(int i=0;i<=64;++i){double angle=i*2*std::numbers::pi/64;auto p=project(pivot+(a*std::cos(angle)+b*std::sin(angle))*length*.8,matrix,size);if(i)line(previous,p,color,axis,2.5);previous=p;if(i==8)s.tips[axis]=p;}}
        else {auto vector=tip-s.pivot;double pixels=std::hypot(vector.x(),vector.y());if(pixels<12)continue;auto unit=vector/pixels;line(s.pivot+unit*11,tip-unit*5,color,axis,3);if(tool==3)square(tip,color,axis);else {QPointF n(-unit.y(),unit.x());s.triangles.append({tip,tip-unit*12+n*5,tip-unit*12-n*5,color,axis});}}
    }
    if(tool!=2)square(s.pivot,QColor("#eeeeee"));return s;
}
inline double segmentDistance(QPointF p,QPointF a,QPointF b){auto d=b-a;double den=d.x()*d.x()+d.y()*d.y();double t=den>1e-8?qBound(0.,((p-a).x()*d.x()+(p-a).y()*d.y())/den,1.):0;auto v=p-(a+d*t);return std::hypot(v.x(),v.y());}
inline int hit(const Shape &shape,QPointF pixel,int tool){
    if(!shape.valid)return -2;if(tool!=2&&std::hypot(pixel.x()-shape.pivot.x(),pixel.y()-shape.pivot.y())<=8)return -1;
    double best=8;int axis=-2;for(const auto &segment:shape.segments){double distance=segmentDistance(pixel,segment.a,segment.b);if(distance<best){best=distance;axis=segment.axis;}}return axis;
}
struct Drag {
    int id=-1,axis=-2,tool=0;QPointF start;Shape handles;QMatrix4x4 inverse;
    QVector3D pivot;QSize viewport;double depth=0,radius=1;std::array<double,3> position,angles,scale;
    std::shared_ptr<MeshData> source;
    void begin(const MeshEntry &entry,const MeshDebug &frame,const QMatrix4x4 &matrix,QSize size,const Shape &shape,QPointF pixel,int selectedAxis,int selectedTool){
        id=entry.id;axis=selectedAxis;tool=selectedTool;start=pixel;handles=shape;inverse=matrix.inverted();viewport=size;radius=frame.radius;position=entry.positionDelta;angles=entry.rotationDegrees;scale=entry.scale;source=entry.data;
        pivot=(QVector3D(float(position[0]),float(position[1]),float(position[2]))-frame.center)/float(radius);auto clip=matrix*QVector4D(pivot,1);depth=clip.z()/clip.w();
    }
    bool update(MeshScene &scene,QPointF pixel){
        auto *entry=scene.entry(id);if(!entry||entry->data!=source||!entry->visible)return false;auto p=position,r=angles,s=scale;auto delta=pixel-start;
        if(tool==1){if(axis<0){auto unproject=[&](QPointF point){auto clip=inverse*QVector4D(float(point.x()*2/viewport.width()-1),float(point.y()*2/viewport.height()-1),depth,1);return clip.toVector3DAffine();};auto moved=(unproject(pixel)-unproject(start))*radius;for(int i=0;i<3;++i)p[i]+=moved[i];}
            else {auto v=handles.vectors[axis];double den=v.x()*v.x()+v.y()*v.y();if(den<1)return false;p[axis]+=(delta.x()*v.x()+delta.y()*v.y())/den*handles.worldLength;}}
        else if(tool==2){QVector3D direction;direction[axis]=1;auto intersection=[&](QPointF point,bool &ok){auto v=QVector4D(float(point.x()*2/viewport.width()-1),float(point.y()*2/viewport.height()-1),0,1);auto near=(inverse*v).toVector3DAffine();v.setZ(1);auto ray=(inverse*v).toVector3DAffine()-near;double denominator=QVector3D::dotProduct(direction,ray);ok=std::abs(denominator)>1e-7;return ok?near+ray*float(QVector3D::dotProduct(direction,pivot-near)/denominator):pivot;};bool aOk,bOk;auto a=intersection(start,aOk)-pivot,b=intersection(pixel,bOk)-pivot;if(!aOk||!bOk||a.lengthSquared()<1e-12||b.lengthSquared()<1e-12)return false;double angle=qRadiansToDegrees(std::atan2(QVector3D::dotProduct(direction,QVector3D::crossProduct(a,b)),QVector3D::dotProduct(a,b)));QMatrix4x4 turn;turn.rotate(angle,direction);r=euler(turn*rotation(angles));}
        else if(tool==3){double factor;if(axis<0)factor=std::exp(qBound(-5.,(delta.x()-delta.y())*.01,5.));else {auto v=handles.vectors[axis];double den=v.x()*v.x()+v.y()*v.y();if(den<1)return false;factor=qMax(.01,1+(delta.x()*v.x()+delta.y()*v.y())/den);}for(int i=0;i<3;++i)if(axis<0||axis==i)s[i]=std::copysign(qBound(1e-6,std::abs(scale[i])*factor,1e6),scale[i]);}
        return scene.setTransform(id,p,r,s);
    }
    void cancel(MeshScene &scene){auto *entry=scene.entry(id);if(entry&&entry->data==source)scene.setTransform(id,position,angles,scale);id=-1;}
};
}
