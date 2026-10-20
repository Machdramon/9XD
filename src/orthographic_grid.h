#pragma once
#include <QVector>
#include <QLineF>
#include <QSize>
#include <QVector3D>
#include <cmath>
struct OrthographicGrid { QVector<QLineF> lines; double spacing=1; };
// World-origin anchored, constant-size cells; geometry covers only the viewport.
inline OrthographicGrid orthographicGrid(QSize size,QVector3D center,QVector3D right,QVector3D up,double halfHeight){
    OrthographicGrid result;if(size.isEmpty()||!std::isfinite(halfHeight)||halfHeight<=0)return result;
    double pixels=size.height()/(2*halfHeight),raw=64/pixels,decade=std::pow(10.,std::floor(std::log10(raw))),fraction=raw/decade;
    result.spacing=decade*(fraction<=1?1:fraction<=2?2:fraction<=5?5:10);
    double cell=result.spacing*pixels;
    double x=size.width()*.5-std::remainder(double(QVector3D::dotProduct(center,right)),result.spacing)*pixels;
    double y=size.height()*.5+std::remainder(double(QVector3D::dotProduct(center,up)),result.spacing)*pixels;
    x=std::fmod(std::fmod(x,cell)+cell,cell);y=std::fmod(std::fmod(y,cell)+cell,cell);
    for(double p=x;p<=size.width();p+=cell)result.lines.append(QLineF(p,0,p,size.height()));
    for(double p=y;p<=size.height();p+=cell)result.lines.append(QLineF(0,p,size.width(),p));return result;
}
