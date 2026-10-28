#pragma once
#include <QVector>
#include <QVector3D>
#include <QString>
#include <array>
#include <cmath>
#include <QtGlobal>
struct ReferenceGridLine {QVector3D a,b;bool major=false;int axis=-1;};
inline std::array<int,2> referenceGridAxes(int view){return view==1||view==4?std::array<int,2>{0,1}:view==2||view==5?std::array<int,2>{2,1}:std::array<int,2>{0,2};}
inline QString referenceGridPlane(int view){auto axes=referenceGridAxes(view);return QString("XYZ")[axes[0]]+QString(QString("XYZ")[axes[1]]);}
// Scene/model units, fixed origin. Spacing, major interval and extent are user settings (defaults 1 / 5 / 10);
// the line count is capped so a tiny spacing over a large extent cannot flood the GPU buffer.
struct ReferenceGridSettings {double spacing=1;int majorEvery=5;double extent=10;bool labels=true;};
inline int referenceGridSteps(const ReferenceGridSettings &s){return qBound(1,int(std::ceil(s.extent/s.spacing-1e-9)),400);}
inline QVector<ReferenceGridLine> referenceGrid(int view,const ReferenceGridSettings &settings={}){
    QVector<ReferenceGridLine> lines;auto axes=referenceGridAxes(view);const int n=referenceGridSteps(settings);const double half=n*settings.spacing;const int major=qMax(1,settings.majorEvery);
    for(int i=-n;i<=n;++i)for(int direction=0;direction<2;++direction){QVector3D a,b;a[axes[direction]]=b[axes[direction]]=float(i*settings.spacing);a[axes[1-direction]]=float(-half);b[axes[1-direction]]=float(half);lines.append({a,b,i%major==0,i==0?axes[1-direction]:-1});}
    return lines;
}
// Values that get a number label: major lines, thinned so each axis shows at most about 11 labels.
inline QVector<double> referenceGridLabelValues(const ReferenceGridSettings &settings){
    QVector<double> values;const int n=referenceGridSteps(settings),major=qMax(1,settings.majorEvery);int step=major;while((2*n)/step+1>11)step+=major;
    for(int i=-(n/step)*step;i<=n;i+=step)values.append(i*settings.spacing);return values;
}
