#include "mesh_debug.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <QRectF>
#include <QHash>
namespace {
QVector3D point(const MeshVertex &v){return {v.position[0],v.position[1],v.position[2]};}
float triangle(QVector3D o,QVector3D d,QVector3D a,QVector3D b,QVector3D c){
    auto e=b-a,f=c-a,p=QVector3D::crossProduct(d,f);float det=QVector3D::dotProduct(e,p);if(std::abs(det)<1e-10f)return -1;
    float inv=1/det;auto s=o-a;float u=QVector3D::dotProduct(s,p)*inv;if(u < -1e-6f || u > 1.000001f)return -1;
    auto q=QVector3D::crossProduct(s,e);float v=QVector3D::dotProduct(d,q)*inv;if(v < -1e-6f || u+v > 1.000001f)return -1;
    return QVector3D::dotProduct(f,q)*inv;
}
bool box(QVector3D o,QVector3D d,QVector3D low,QVector3D high,float limit){float near=0,far=limit;for(int j=0;j<3;++j){if(std::abs(d[j])<1e-12f){if(o[j]<low[j]||o[j]>high[j])return false;}else{float a=(low[j]-o[j])/d[j],b=(high[j]-o[j])/d[j];if(a>b)std::swap(a,b);near=qMax(near,a);far=qMin(far,b);if(near>far)return false;}}return true;}
}
MeshDebug::MeshDebug(const MeshData &mesh,const MeshData *bounds,const MeshDebug *framing):indices(mesh.triangles){
    if(mesh.vertices.isEmpty()&&(!bounds||bounds->vertices.isEmpty()))return;const auto &basis=bounds&&!bounds->vertices.isEmpty()?bounds->vertices:mesh.vertices;auto low=point(basis.first()),high=low;
    for(auto v:basis)for(int j=0;j<3;++j){low[j]=qMin(low[j],v.position[j]);high[j]=qMax(high[j],v.position[j]);}
    center=low*.5f+high*.5f;radius=0;for(auto v:basis)radius=qMax(radius,std::hypot(double(v.position[0])-center.x(),double(v.position[1])-center.y(),double(v.position[2])-center.z()));if(radius<1e-20)radius=1;
    if(framing){center=framing->center;radius=framing->radius;}
    for(auto v:mesh.vertices)positions.append({float((double(v.position[0])-center.x())/radius),float((double(v.position[1])-center.y())/radius),float((double(v.position[2])-center.z())/radius)});
    for(int i=0;i<indices.size()/3;++i)order_.append(i);if(!order_.isEmpty())build(0,order_.size());
}
int MeshDebug::build(int begin,int end){
    int index=nodes_.size();Node n;n.begin=begin;n.end=end;n.low=QVector3D(1e30f,1e30f,1e30f);n.high=-n.low;
    for(int i=begin;i<end;++i)for(int k=0;k<3;++k){auto p=positions[indices[order_[i]*3+k]];for(int j=0;j<3;++j){n.low[j]=qMin(n.low[j],p[j]);n.high[j]=qMax(n.high[j],p[j]);}}
    nodes_.append(n);if(end-begin>8){auto span=n.high-n.low;int axis=span.y()>span.x()?1:0;if(span.z()>span[axis])axis=2;int middle=(begin+end)/2;
        std::nth_element(order_.begin()+begin,order_.begin()+middle,order_.begin()+end,[&](int a,int b){float x=0,y=0;for(int k=0;k<3;++k){x+=positions[indices[a*3+k]][axis];y+=positions[indices[b*3+k]][axis];}return x<y;});
        int left=build(begin,middle),right=build(middle,end);nodes_[index].left=left;nodes_[index].right=right;
    }return index;
}
int MeshDebug::hit(QVector3D origin,QVector3D direction,float maxDistance)const{
    if(nodes_.isEmpty())return -1;int result=-1;QVector<int> stack{0};while(!stack.isEmpty()){const auto &n=nodes_[stack.takeLast()];if(!box(origin,direction,n.low,n.high,maxDistance))continue;
        if(n.left>=0){stack.append(n.left);stack.append(n.right);continue;}
        for(int i=n.begin;i<n.end;++i){int face=order_[i];float t=triangle(origin,direction,positions[indices[face*3]],positions[indices[face*3+1]],positions[indices[face*3+2]]);if(t>=0&&t<maxDistance){maxDistance=t;result=face;}}
    }return result;
}
bool MeshDebug::visible(QVector3D eye,QVector3D anchor)const{auto ray=anchor-eye;float distance=ray.length();if(distance<1e-8f)return false;return hit(eye,ray/distance,qMax(0.f,distance-2e-5f))<0;}
QVector<MeshLabel> MeshDebug::labels(const QMatrix4x4 &mvp,QVector3D eye,QSize viewport,MeshElement kind,int first,int count)const{
    QVector<MeshLabel> result;if(viewport.isEmpty())return result;
    auto append=[&](int id,QVector3D anchor){auto clip=mvp*QVector4D(anchor,1);if(clip.w()<=0)return;auto p=clip.toVector3DAffine();if(p.x() < -1||p.x()>1||p.y() < -1||p.y()>1||p.z()<0||p.z()>1||!visible(eye,anchor))return;result.append({id,anchor,{(p.x()+1)*viewport.width()*.5,(p.y()+1)*viewport.height()*.5},p.z()});};
    first=qMax(0,first);count=qMax(0,count);
    if(kind==MeshElement::Vertex){for(int i=first;i<qMin(positions.size(),qsizetype(first)+count);++i)append(i,positions[i]);}
    else if(kind==MeshElement::Face){for(int i=first;i<qMin(indices.size()/3,qsizetype(first)+count);++i)append(i,(positions[indices[i*3]]+positions[indices[i*3+1]]+positions[indices[i*3+2]])/3);}
    else {for(int i=0;i<positions.size();++i){append(0,positions[i]);if(!result.isEmpty())break;}}
    return result;
}
int MeshDebug::pick(const QMatrix4x4 &mvp,QVector3D eye,QSize viewport,QPointF pixel,MeshElement kind)const{
    if(viewport.isEmpty())return -1;
    if(kind==MeshElement::Vertex){int picked=-1;double best=100;float depth=1e30f;for(int i=0;i<positions.size();++i){auto clip=mvp*QVector4D(positions[i],1);if(clip.w()<=0)continue;auto p=clip.toVector3DAffine();if(p.z()<0||p.z()>1||std::abs(p.x())>1||std::abs(p.y())>1)continue;auto delta=QPointF((p.x()+1)*viewport.width()*.5,(p.y()+1)*viewport.height()*.5)-pixel;double d=delta.x()*delta.x()+delta.y()*delta.y();if(d<=best&&visible(eye,positions[i])&&(d<best-1e-5||p.z()<depth)){best=d;depth=p.z();picked=i;}}return picked;}
    bool ok;auto inverse=mvp.inverted(&ok);if(!ok)return -1;if(viewport.isEmpty())return -1;float x=float(pixel.x()*2/viewport.width()-1),y=float(pixel.y()*2/viewport.height()-1);auto near=(inverse*QVector4D(x,y,0,1)).toVector3DAffine(),far=(inverse*QVector4D(x,y,1,1)).toVector3DAffine();int face=hit(near,(far-near).normalized(),(far-near).length());return face<0?-1:kind==MeshElement::Mesh?0:face;
}
QColor indexColor(int id,int count,const QVector<QColor> &stops){if(stops.isEmpty())return Qt::white;if(stops.size()==1)return stops.first();float x=qBound(0.f,float(id)/qMax(1,count-1),1.f)*(stops.size()-1);int i=qMin(int(x),int(stops.size()-2));float t=x-i;return QColor::fromRgbF(stops[i].redF()*(1-t)+stops[i+1].redF()*t,stops[i].greenF()*(1-t)+stops[i+1].greenF()*t,stops[i].blueF()*(1-t)+stops[i+1].blueF()*t);}
QVector<MeshVertex> labelGeometry(const QVector<MeshLabel> &labels,QSize viewport,MeshElement kind,float scale,int selected,bool selectedOnly,bool drawText){
    // Compact bitmap font, rendered as screen-space triangles; no native-widget stacking.
    static const unsigned char digits[13][7]={{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14},{17,17,17,17,17,10,4},{31,16,16,30,16,16,16},{17,27,21,21,17,17,17}};
    QVector<MeshVertex> out;if(viewport.isEmpty())return out;QHash<quint64,QVector<QRectF>> occupied;
    auto rect=[&](float x,float y,float w,float h){for(auto p:{QPointF(x,y),QPointF(x+w,y),QPointF(x+w,y+h),QPointF(x,y),QPointF(x+w,y+h),QPointF(x,y+h)}){MeshVertex v{};v.position[0]=float(p.x()*2/viewport.width()-1);v.position[1]=float(p.y()*2/viewport.height()-1);out.append(v);}};
    auto sorted=labels;std::stable_sort(sorted.begin(),sorted.end(),[&](auto a,auto b){if(a.id==selected)return b.id!=selected;if(b.id==selected)return false;return a.depth<b.depth;});
    for(auto label:sorted){if(selectedOnly&&label.id!=selected)continue;if(label.id==selected){rect(label.pixel.x()-5,label.pixel.y()-1,10,2);rect(label.pixel.x()-1,label.pixel.y()-5,2,10);}if(!drawText)continue;QString number=QString::number(label.id);float x=label.pixel.x()+5,y=label.pixel.y()-9*scale;QRectF bounds(x,y,(number.size()+1)*6*scale,7*scale);if(!QRectF(QPointF(0,0),viewport).contains(bounds))continue;bool overlap=false;int left=int(bounds.left()/32),right=int(bounds.right()/32),top=int(bounds.top()/16),bottom=int(bounds.bottom()/16);auto key=[](int x,int y){return (quint64(quint32(x))<<32)|quint32(y);};for(int y=top;y<=bottom&&!overlap;++y)for(int x=left;x<=right&&!overlap;++x){auto it=occupied.constFind(key(x,y));if(it!=occupied.cend())for(const auto &r:*it)if(r.intersects(bounds)){overlap=true;break;}}if(overlap)continue;for(int y=top;y<=bottom;++y)for(int x=left;x<=right;++x)occupied[key(x,y)].append(bounds);
        if(label.id==selected){rect(x-2,y-2,bounds.width()+4,1);rect(x-2,y+7*scale+1,bounds.width()+4,1);rect(x-2,y-2,1,7*scale+4);rect(x+bounds.width()+1,y-2,1,7*scale+4);}
        int prefix=kind==MeshElement::Vertex?10:kind==MeshElement::Face?11:12;
        for(int c=-1;c<number.size();++c){int glyph=c<0?prefix:number[c].digitValue();for(int row=0;row<7;++row){int col=0;while(col<5){while(col<5&&!(digits[glyph][row]&(1<<(4-col))))++col;int start=col;while(col<5&&(digits[glyph][row]&(1<<(4-col))))++col;if(col>start)rect(x+(c+1)*6*scale+start*scale,y+row*scale,(col-start)*scale,scale);}}}
    }return out;
}
