#include "uv_panel.h"
#include "theme.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QSignalBlocker>
#include <algorithm>
#include <cmath>
namespace {
QPointF uv(const MeshVertex &v){return {v.uv[0],v.uv[1]};}
double cross(QPointF a,QPointF b){return a.x()*b.y()-a.y()*b.x();}
bool inside(QPointF p,const QPolygonF &t){auto a=t[1]-t[0],b=t[2]-t[0],r=p-t[0];double det=cross(a,b);if(std::abs(det)<1e-9)return false;double u=cross(r,b)/det,v=cross(a,r)/det;return u>=0&&v>=0&&u+v<=1;}
bool drawable(const QPolygonF &p){for(auto v:p)if(!std::isfinite(v.x())||!std::isfinite(v.y())||std::abs(v.x())>1e7||std::abs(v.y())>1e7)return false;return true;}
}
UvCanvas::UvCanvas(VulkanPanel *renderer,QWidget *parent):QWidget(parent),renderer_(renderer),scene_(renderer->scene()){
    setObjectName("uvCanvas");setMinimumSize(300,220);setMouseTracking(true);
    connect(scene_,&MeshScene::geometryChanged,this,[this]{update();});connect(scene_,&MeshScene::selectionChanged,this,[this]{update();});connect(renderer_,&VulkanPanel::visualStyleChanged,this,[this]{update();});
}
QPointF UvCanvas::uvToScreen(QPointF p)const{return {width()*.5+(p.x()-center_.x())*scale_,height()*.5-(p.y()-center_.y())*scale_};}
void UvCanvas::fit(){
    QPointF low(0,0),high(1,1);bool found=false;
    for(const auto &e:scene_->entries())if(e.visible&&e.data->hasUvs&&(!activeOnly||e.id==scene_->active()))for(auto v:e.data->vertices){auto p=uv(v);if(!found){low=high=p;found=true;}low.setX(qMin(low.x(),p.x()));low.setY(qMin(low.y(),p.y()));high.setX(qMax(high.x(),p.x()));high.setY(qMax(high.y(),p.y()));}
    center_=low*.5+high*.5;scale_=qMin(qMax(1,width()-60)/qMax(1e-6,high.x()-low.x()),qMax(1,height()-60)/qMax(1e-6,high.y()-low.y()));scale_=qBound(1e-38,scale_,1e8);update();
}
void UvCanvas::paintEvent(QPaintEvent *){
    QPainter p(this);{QColor top,bottom;if(graphicsGradient(top,bottom)){QLinearGradient fade(0,0,0,height());fade.setColorAt(0,top);fade.setColorAt(1,bottom);p.fillRect(rect(),fade);}else p.fillRect(rect(),renderer_->visualStyle().background);}p.setRenderHint(QPainter::Antialiasing);p.setClipRect(rect());auto style=renderer_->visualStyle();auto selection=scene_->selection();
    // The 0..1 tile remains a reference; UVs outside it are not wrapped or discarded.
    for(int y=0;y<8;++y)for(int x=0;x<8;++x){QRectF cell(uvToScreen({x/8.,(y+1)/8.}),uvToScreen({(x+1)/8.,y/8.}));if(cell.intersects(rect()))p.fillRect(cell.intersected(QRectF(rect())),(x+y)%2?style.background.lighter(workbenchDark()?140:105):style.background.darker(workbenchDark()?100:108));}
    if(auto *active=scene_->entry(scene_->active());active&&!active->texture.isNull()&&active->data->hasUvs){auto target=QRectF(uvToScreen({0,1}),uvToScreen({1,0}));if(drawable(QPolygonF(target)))p.drawImage(target,active->texture);}
    p.setPen(QPen(style.grid,1));p.setBrush(Qt::NoBrush);QRectF tile(uvToScreen({0,1}),uvToScreen({1,0}));if(tile.width()<1e7)p.drawRect(tile);p.drawText(10,20,"UV · U → · V ↑");
    QVector<QRectF> occupied;bool any=false;
    for(const auto &e:scene_->entries())if(e.visible&&e.data->hasUvs&&(!activeOnly||e.id==scene_->active())){
        any=true;const auto &mesh=*e.data;bool active=e.id==scene_->active();QVector<QPointF> points;points.reserve(mesh.vertices.size());for(auto v:mesh.vertices)points.append(uvToScreen(uv(v)));
        auto label=[&](QPointF at,QString text){QRectF box(at+QPointF(5,-18),QSizeF(p.fontMetrics().horizontalAdvance(text)+4,18));if(!rect().contains(box.toAlignedRect()))return;for(auto r:occupied)if(r.intersects(box))return;occupied.append(box);p.setPen(style.text);p.drawText(box,Qt::AlignCenter,text);};
        for(int f=0;f<mesh.triangles.size()/3;++f){QPolygonF triangle;for(int j=0;j<3;++j)triangle.append(points[mesh.triangles[f*3+j]]);if(!drawable(triangle)||!triangle.boundingRect().intersects(rect()))continue;
            bool chosen=selection.mesh==e.id&&selection.index>=0&&(selection.kind==MeshElement::Mesh||(selection.kind==MeshElement::Face&&selection.index==f));QColor color=style.faceGradient?indexColor(f,mesh.triangles.size()/3,style.stops):style.face;QColor fill=color;fill.setAlpha(chosen?100:active?22:10);p.setBrush(fill);p.setPen(QPen(chosen?style.selection:color,chosen?2:active?1:.6));p.drawPolygon(triangle);
            if(active&&selection.kind==MeshElement::Face&&((labels&&f<200)||chosen))label((triangle[0]+triangle[1]+triangle[2])/3,QString("F%1").arg(f));
        }
        p.setBrush(Qt::NoBrush);
        for(int i=0;i<points.size();++i){auto at=points[i];if(!QRectF(rect()).contains(at))continue;bool chosen=selection.mesh==e.id&&selection.kind==MeshElement::Vertex&&selection.index==i;QColor color=style.vertexGradient?indexColor(i,points.size(),style.stops):style.vertex;p.setPen(QPen(chosen?style.selection:color,chosen?2:1));p.setBrush(color);p.drawEllipse(at,chosen?4.:2.,chosen?4.:2.);if(active&&selection.kind==MeshElement::Vertex&&((labels&&i<200)||chosen))label(at,QString("V%1").arg(i));}
    }
    // Paint the selected face last so coincident UV islands cannot cover its outline.
    const auto *chosen=scene_->entry(selection.mesh);
    if(chosen&&chosen->visible&&chosen->data->hasUvs&&selection.kind==MeshElement::Face&&selection.index>=0&&(!activeOnly||chosen->id==scene_->active())){QPolygonF triangle;for(int j=0;j<3;++j)triangle.append(uvToScreen(uv(chosen->data->vertices[chosen->data->triangles[selection.index*3+j]])));if(drawable(triangle)){p.setBrush(Qt::NoBrush);p.setPen(QPen(style.selection,2));p.drawPolygon(triangle);}}
    if(chosen&&chosen->visible&&chosen->data->hasUvs&&selection.kind==MeshElement::Vertex&&selection.index>=0&&(!activeOnly||chosen->id==scene_->active())){auto at=uvToScreen(uv(chosen->data->vertices[selection.index]));if(QRectF(rect()).contains(at)){p.setBrush(Qt::NoBrush);p.setPen(QPen(style.selection,2));p.drawEllipse(at,5,5);}}
    if(!any){p.setPen(style.text);p.drawText(rect().adjusted(20,35,-20,-20),Qt::AlignCenter|Qt::TextWordWrap,"No visible UVs. Select a mesh with UVs and enable its checkbox in the collection.");}
}
void UvCanvas::pick(QPointF pixel,bool cycle){
    struct Hit{int mesh,index;double distance;};QVector<Hit> hits;auto kind=scene_->selection().kind;
    for(const auto &e:scene_->entries())if(e.visible&&e.data->hasUvs&&(!activeOnly||e.id==scene_->active())){
        const auto &mesh=*e.data;
        if(kind==MeshElement::Vertex){for(int i=0;i<mesh.vertices.size();++i){auto d=uvToScreen(uv(mesh.vertices[i]))-pixel;double distance=d.x()*d.x()+d.y()*d.y();if(distance<=64)hits.append({e.id,i,distance});}}
        else for(int f=mesh.triangles.size()/3-1;f>=0;--f){QPolygonF triangle;for(int j=0;j<3;++j)triangle.append(uvToScreen(uv(mesh.vertices[mesh.triangles[f*3+j]])));if(inside(pixel,triangle)){hits.append({e.id,kind==MeshElement::Mesh?0:f,0});if(kind==MeshElement::Mesh)break;}}
    }
    std::stable_sort(hits.begin(),hits.end(),[&](auto a,auto b){if(a.distance!=b.distance)return a.distance<b.distance;return a.mesh>b.mesh;});
    if(hits.isEmpty()){scene_->select(scene_->active(),kind,-1);cycle_=0;return;}cycle_=cycle?(cycle_+1)%hits.size():0;auto hit=hits[cycle_];scene_->select(hit.mesh,kind,hit.index);
}
void UvCanvas::mousePressEvent(QMouseEvent *e){press_=last_=e->position();}
void UvCanvas::mouseMoveEvent(QMouseEvent *e){if(e->buttons()&(Qt::MiddleButton|Qt::RightButton)){auto delta=e->position()-last_;center_+=QPointF(-delta.x()/scale_,delta.y()/scale_);update();}last_=e->position();}
void UvCanvas::mouseReleaseEvent(QMouseEvent *e){if(e->button()==Qt::LeftButton&&(e->position()-press_).manhattanLength()<4)pick(e->position(),e->modifiers()&Qt::AltModifier);}
void UvCanvas::wheelEvent(QWheelEvent *e){auto pixel=e->position();QPointF before(center_.x()+(pixel.x()-width()*.5)/scale_,center_.y()-(pixel.y()-height()*.5)/scale_);scale_=qBound(1e-38,scale_*std::exp(e->angleDelta().y()*.001),1e8);center_={before.x()-(pixel.x()-width()*.5)/scale_,before.y()+(pixel.y()-height()*.5)/scale_};update();e->accept();}
UvPanel::UvPanel(VulkanPanel *renderer,QWidget *parent):QWidget(parent){
    auto *scene=renderer->scene();auto *layout=new QVBoxLayout(this);auto *bar=new QHBoxLayout;layout->addLayout(bar);auto *mode=new QComboBox;mode->setObjectName("uvSelectionMode");mode->addItems({"Vertices","Faces","Meshes"});bar->addWidget(mode);auto *only=new QCheckBox("Active only");only->setObjectName("uvActiveOnly");only->setChecked(true);bar->addWidget(only);auto *numbers=new QCheckBox("Indices");bar->addWidget(numbers);auto *home=new QPushButton("Frame UVs");bar->addWidget(home);auto *canvas=new UvCanvas(renderer);layout->addWidget(canvas,1);auto *info=new QLabel;info->setObjectName("uvSelectionInfo");info->setWordWrap(true);layout->addWidget(info);auto *help=new QLabel("Click: select · Alt+click: next match · Wheel: zoom · Right/middle drag: pan");help->setWordWrap(true);layout->addWidget(help);
    auto refresh=[scene,mode,info]{auto selection=scene->selection();{QSignalBlocker block(mode);mode->setCurrentIndex(int(selection.kind));}auto *entry=scene->entry(scene->active());if(!entry){info->setText("Empty collection.");return;}QString text=QString("%1 · ID %2").arg(entry->name).arg(entry->id);if(!entry->data->hasUvs)text+=" · No UV coordinates";else if(selection.index>=0){if(selection.kind==MeshElement::Vertex){auto v=entry->data->vertices[selection.index];text+=QString(" · Vertex %1 · U %2 · V %3").arg(selection.index).arg(v.uv[0],0,'g',8).arg(v.uv[1],0,'g',8);}else if(selection.kind==MeshElement::Face)text+=QString(" · Face %1 · Vertices %2, %3, %4").arg(selection.index).arg(entry->data->triangles[selection.index*3]).arg(entry->data->triangles[selection.index*3+1]).arg(entry->data->triangles[selection.index*3+2]);}if(!entry->visible)text+=" · Hidden";info->setText(text);};
    connect(scene,&MeshScene::selectionChanged,this,refresh);connect(scene,&MeshScene::collectionChanged,this,refresh);connect(mode,&QComboBox::currentIndexChanged,this,[scene](int index){scene->select(scene->active(),MeshElement(index),-1);});connect(only,&QCheckBox::toggled,this,[canvas](bool v){canvas->activeOnly=v;canvas->update();});connect(numbers,&QCheckBox::toggled,this,[canvas](bool v){canvas->labels=v;canvas->update();});connect(home,&QPushButton::clicked,canvas,&UvCanvas::fit);refresh();
}
