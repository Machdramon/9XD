#include "mesh_scene.h"
#include <algorithm>
#include <cmath>
const MeshEntry *MeshScene::entry(int id)const{for(const auto &e:entries_)if(e.id==id)return &e;return nullptr;}
int MeshScene::add(std::shared_ptr<MeshData> data,QString name,QString source){if(!data)return -1;int id=nextId_++;if(name.trimmed().isEmpty())name=QString("Mesh %1").arg(id);entries_.append({id,name,source,true,false,std::move(data)});active_=id;selection_={id,selection_.kind,-1};emit geometryChanged();emit collectionChanged();emit selectionChanged();return id;}
bool MeshScene::replace(int id,std::shared_ptr<MeshData> data,QString source){if(!data)return false;for(auto &e:entries_)if(e.id==id){e.data=std::move(data);e.source=source;e.stale=false;selection_={id,selection_.kind,-1};active_=id;emit geometryChanged();emit collectionChanged();emit selectionChanged();return true;}return false;}
void MeshScene::remove(int id){auto it=std::find_if(entries_.begin(),entries_.end(),[&](const auto &e){return e.id==id;});if(it==entries_.end())return;entries_.erase(it);if(active_==id)active_=entries_.isEmpty()?-1:entries_.first().id;if(selection_.mesh==id)selection_={active_,selection_.kind,-1};emit geometryChanged();emit collectionChanged();emit selectionChanged();}
void MeshScene::clear(){entries_.clear();active_=-1;selection_={-1,selection_.kind,-1};emit geometryChanged();emit collectionChanged();emit selectionChanged();}
void MeshScene::rename(int id,QString name){name=name.trimmed();if(name.isEmpty())return;for(auto &e:entries_)if(e.id==id){e.name=name.left(120);emit collectionChanged();return;}}
void MeshScene::setVisible(int id,bool visible){for(auto &e:entries_)if(e.id==id&&e.visible!=visible){e.visible=visible;if(!visible&&selection_.mesh==id)selection_.index=-1;emit geometryChanged();emit collectionChanged();emit selectionChanged();return;}}
void MeshScene::setActive(int id){if(!entry(id))return;active_=id;selection_={id,selection_.kind,-1};emit selectionChanged();}
void MeshScene::select(int mesh,MeshElement kind,int index){const auto *e=entry(mesh);if(!e)return;int count=kind==MeshElement::Vertex?e->data->vertices.size():kind==MeshElement::Face?e->data->triangles.size()/3:1;if(index < -1||index>=count||(!e->visible&&index>=0))return;active_=mesh;selection_={mesh,kind,index};emit selectionChanged();}
void MeshScene::markStale(){for(auto &e:entries_)if(!e.source.isEmpty())e.stale=true;emit collectionChanged();}
MeshAssembly MeshScene::assemble(bool includeHidden)const{MeshAssembly result{std::make_shared<MeshData>(),{}};for(auto &e:entries_)if(e.visible||includeHidden){int v=result.data->vertices.size(),f=result.data->triangles.size()/3;result.spans.append({e.id,v,int(e.data->vertices.size()),f,int(e.data->triangles.size()/3)});result.data->vertices+=e.placedData()->vertices;for(auto i:e.data->triangles)result.data->triangles.append(v+i);}return result;}

bool MeshScene::setTexture(int id,const QImage &image,const QString &name){
    auto *target=entry(id);if(!target||(!image.isNull()&&!target->data->hasUvs))return false;
    if(!image.isNull()&&(image.width()>8192||image.height()>8192||qint64(image.width())*image.height()>4*1024*1024))return false;
    qint64 retained=qint64(image.width())*image.height()*4;for(const auto &e:entries_)if(e.id!=id)retained+=e.texture.sizeInBytes();if(retained>64*1024*1024)return false;
    for(auto &e:entries_)if(e.id==id){e.texture=image.convertToFormat(QImage::Format_RGBA8888);e.textureName=image.isNull()?QString():name;emit geometryChanged();emit collectionChanged();return true;}return false;
}

std::shared_ptr<MeshData> MeshEntry::placedData()const{
    if(positionDelta==std::array<double,3>{})return data;
    auto placed=std::make_shared<MeshData>(*data);
    for(auto &vertex:placed->vertices)for(int axis=0;axis<3;++axis)vertex.position[axis]=float(double(vertex.position[axis])+positionDelta[axis]);
    return placed;
}
bool MeshScene::setPositionDelta(int id,const std::array<double,3> &delta){
    for(double value:delta)if(!std::isfinite(value))return false;
    for(auto &entry:entries_)if(entry.id==id){
        for(const auto &vertex:entry.data->vertices)for(int axis=0;axis<3;++axis)if(!std::isfinite(float(double(vertex.position[axis])+delta[axis])))return false;
        if(entry.positionDelta==delta)return true;
        entry.positionDelta=delta;emit geometryChanged();emit collectionChanged();emit selectionChanged();return true;
    }return false;
}
