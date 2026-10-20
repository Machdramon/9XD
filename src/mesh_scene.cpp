#include "mesh_scene.h"
#include <algorithm>
#include <cmath>
#include <QMatrix4x4>
const MeshEntry *MeshScene::entry(int id)const{for(const auto &e:entries_)if(e.id==id)return &e;return nullptr;}
int MeshScene::add(std::shared_ptr<MeshData> data,QString name,QString source){if(!data)return -1;int id=nextId_++;if(name.trimmed().isEmpty())name=QString("Mesh %1").arg(id);entries_.append({id,name,source,true,false,std::move(data)});active_=id;selectedMeshes_={id};if(isolated()){isolation_.insert(id,true);entries_.last().visible=false;}selection_={id,selection_.kind,-1};emit geometryChanged();emit collectionChanged();emit selectionChanged();return id;}
bool MeshScene::replace(int id,std::shared_ptr<MeshData> data,QString source){if(!data)return false;for(auto &e:entries_)if(e.id==id){e.data=std::move(data);e.source=source;e.stale=false;selection_={id,selection_.kind,-1};active_=id;emit geometryChanged();emit collectionChanged();emit selectionChanged();return true;}return false;}
void MeshScene::remove(int id){auto it=std::find_if(entries_.begin(),entries_.end(),[&](const auto &e){return e.id==id;});if(it==entries_.end())return;entries_.erase(it);selectedMeshes_.remove(id);isolation_.remove(id);if(active_==id)active_=entries_.isEmpty()?-1:entries_.first().id;if(selection_.mesh==id)selection_={active_,selection_.kind,-1};emit geometryChanged();emit collectionChanged();emit selectionChanged();}
void MeshScene::clear(){selectedMeshes_.clear();isolation_.clear();entries_.clear();active_=-1;selection_={-1,selection_.kind,-1};emit geometryChanged();emit collectionChanged();emit selectionChanged();}
void MeshScene::rename(int id,QString name){name=name.trimmed();if(name.isEmpty())return;for(auto &e:entries_)if(e.id==id){e.name=name.left(120);emit collectionChanged();return;}}
void MeshScene::setVisible(int id,bool visible){for(auto &e:entries_)if(e.id==id&&e.visible!=visible){e.visible=visible;if(!visible&&selection_.mesh==id)selection_.index=-1;emit geometryChanged();emit collectionChanged();emit selectionChanged();return;}}
void MeshScene::setActive(int id){if(!entry(id))return;active_=id;selectedMeshes_={id};selection_={id,selection_.kind,-1};emit selectionChanged();}
void MeshScene::select(int mesh,MeshElement kind,int index){const auto *e=entry(mesh);if(!e)return;int count=kind==MeshElement::Vertex?e->data->vertices.size():kind==MeshElement::Face?e->data->triangles.size()/3:1;if(index < -1||index>=count||(!e->visible&&index>=0))return;active_=mesh;selectedMeshes_={mesh};selection_={mesh,kind,index};emit selectionChanged();}
void MeshScene::markStale(){for(auto &e:entries_)if(!e.source.isEmpty())e.stale=true;emit collectionChanged();}
MeshAssembly MeshScene::assemble(bool includeHidden)const{MeshAssembly result{std::make_shared<MeshData>(),{}};for(auto &e:entries_)if(e.visible||includeHidden){int v=result.data->vertices.size(),f=result.data->triangles.size()/3;result.spans.append({e.id,v,int(e.data->vertices.size()),f,int(e.data->triangles.size()/3)});auto placed=e.placedData();result.data->vertices+=placed->vertices;for(auto i:placed->triangles)result.data->triangles.append(v+i);}return result;}

bool MeshScene::setTexture(int id,const QImage &image,const QString &name){
    auto *target=entry(id);if(!target||(!image.isNull()&&!target->data->hasUvs))return false;
    if(!image.isNull()&&(image.width()>8192||image.height()>8192||qint64(image.width())*image.height()>4*1024*1024))return false;
    qint64 retained=qint64(image.width())*image.height()*4;for(const auto &e:entries_)if(e.id!=id)retained+=e.texture.sizeInBytes();if(retained>64*1024*1024)return false;
    for(auto &e:entries_)if(e.id==id){e.texture=image.convertToFormat(QImage::Format_RGBA8888);e.textureName=image.isNull()?QString():name;emit geometryChanged();emit collectionChanged();return true;}return false;
}

namespace {
QMatrix4x4 placement(const std::array<double,3> &position,const std::array<double,3> &rotation,const std::array<double,3> &scale){
    QMatrix4x4 matrix;matrix.translate(position[0],position[1],position[2]);
    matrix.rotate(rotation[2],0,0,1);matrix.rotate(rotation[1],0,1,0);matrix.rotate(rotation[0],1,0,0);matrix.scale(scale[0],scale[1],scale[2]);return matrix;
}
}
std::shared_ptr<MeshData> MeshEntry::placedData()const{
    if(positionDelta==std::array<double,3>{}&&rotationDegrees==std::array<double,3>{}&&scale==std::array<double,3>{1,1,1})return data;
    auto placed=std::make_shared<MeshData>(*data);auto matrix=placement(positionDelta,rotationDegrees,scale);auto normal=matrix.normalMatrix();
    for(auto &v:placed->vertices){auto point=matrix.map(QVector3D(v.position[0],v.position[1],v.position[2]));auto n=QVector3D(normal(0,0)*v.normal[0]+normal(0,1)*v.normal[1]+normal(0,2)*v.normal[2],normal(1,0)*v.normal[0]+normal(1,1)*v.normal[1]+normal(1,2)*v.normal[2],normal(2,0)*v.normal[0]+normal(2,1)*v.normal[1]+normal(2,2)*v.normal[2]).normalized();
        for(int axis=0;axis<3;++axis){v.position[axis]=point[axis];v.normal[axis]=n[axis];}}
    if((scale[0]<0)^(scale[1]<0)^(scale[2]<0))for(qsizetype i=0;i+2<placed->triangles.size();i+=3)std::swap(placed->triangles[i+1],placed->triangles[i+2]);
    return placed;
}
bool MeshScene::setPositionDelta(int id,const std::array<double,3> &delta){auto *e=entry(id);return e&&setTransform(id,delta,e->rotationDegrees,e->scale);}
bool MeshScene::setTransform(int id,const std::array<double,3> &position,const std::array<double,3> &rotation,const std::array<double,3> &scale){
    for(int axis=0;axis<3;++axis)if(!std::isfinite(position[axis])||!std::isfinite(rotation[axis])||std::abs(rotation[axis])>1e6||!std::isfinite(scale[axis])||std::abs(scale[axis])<1e-6||std::abs(scale[axis])>1e6)return false;
    for(auto &e:entries_)if(e.id==id){auto matrix=placement(position,rotation,scale);for(const auto &v:e.data->vertices){auto p=matrix.map(QVector3D(v.position[0],v.position[1],v.position[2]));for(int axis=0;axis<3;++axis)if(!std::isfinite(p[axis]))return false;}
        e.positionDelta=position;e.rotationDegrees=rotation;e.scale=scale;emit geometryChanged();emit collectionChanged();emit selectionChanged();return true;}return false;
}
bool MeshScene::setMaterial(int id,const MeshMaterial &material){if(!material.baseColor.isValid())return false;for(auto &e:entries_)if(e.id==id){e.material=material;e.material.name=e.material.name.left(120);emit geometryChanged();emit collectionChanged();return true;}return false;}

void MeshScene::setSelectedMeshes(QSet<int> ids,int active){
    for(auto it=ids.begin();it!=ids.end();)if(!entry(*it))it=ids.erase(it);else ++it;
    selectedMeshes_=ids;if(ids.contains(active))active_=active;else if(!ids.contains(active_))active_=ids.isEmpty()?-1:*ids.begin();
    selection_={active_,selection_.kind,-1};emit selectionChanged();
}
void MeshScene::setMeshesVisible(bool visible,bool selectedOnly){
    bool changed=false;for(auto &e:entries_)if((!selectedOnly||selectedMeshes_.contains(e.id))&&e.visible!=visible){e.visible=visible;changed=true;}
    if(!changed)return;if(auto *e=entry(selection_.mesh);e&&!e->visible)selection_.index=-1;
    emit geometryChanged();emit collectionChanged();emit selectionChanged();
}
void MeshScene::isolateSelected(){
    if(selectedMeshes_.isEmpty())return;if(isolation_.isEmpty())for(const auto &e:entries_)isolation_.insert(e.id,e.visible);
    for(auto &e:entries_)e.visible=selectedMeshes_.contains(e.id);selection_.index=-1;
    emit geometryChanged();emit collectionChanged();emit selectionChanged();
}
void MeshScene::restoreVisibility(){
    if(isolation_.isEmpty())return;for(auto &e:entries_)e.visible=isolation_.value(e.id,e.visible);isolation_.clear();selection_.index=-1;
    emit geometryChanged();emit collectionChanged();emit selectionChanged();
}
MeshBounds MeshScene::bounds(bool selectedOnly)const{
    MeshBounds result;for(const auto &e:entries_)if(e.visible&&(!selectedOnly||selectedMeshes_.contains(e.id))){auto placed=e.placedData();for(const auto &v:placed->vertices){QVector3D p(v.position[0],v.position[1],v.position[2]);if(!result.valid){result.minimum=result.maximum=p;result.valid=true;}else for(int a=0;a<3;++a){result.minimum[a]=qMin(result.minimum[a],p[a]);result.maximum[a]=qMax(result.maximum[a],p[a]);}}}return result;
}
