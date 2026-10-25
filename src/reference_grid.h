#pragma once
#include <QVector>
#include <QVector3D>
#include <QString>
#include <array>
struct ReferenceGridLine {QVector3D a,b;bool major=false;int axis=-1;};
inline std::array<int,2> referenceGridAxes(int view){return view==1||view==4?std::array<int,2>{0,1}:view==2||view==5?std::array<int,2>{2,1}:std::array<int,2>{0,2};}
inline QString referenceGridPlane(int view){auto axes=referenceGridAxes(view);return QString("XYZ")[axes[0]]+QString(QString("XYZ")[axes[1]]);}
// Scene/model units, fixed origin and extent. No camera or mesh-normalization values enter here.
inline QVector<ReferenceGridLine> referenceGrid(int view){
    QVector<ReferenceGridLine> lines;auto axes=referenceGridAxes(view);
    for(int i=-10;i<=10;++i)for(int direction=0;direction<2;++direction){QVector3D a,b;a[axes[direction]]=b[axes[direction]]=i;a[axes[1-direction]]=-10;b[axes[1-direction]]=10;lines.append({a,b,i%5==0,i==0?axes[1-direction]:-1});}
    return lines;
}
