#pragma once
#include <QObject>
#include <QImage>
#include <QSet>
#include <QHash>
#include "mesh_debug.h"
#include "mesh_material.h"
struct MeshEntry {
    int id=-1;
    QString name,source;
    bool visible=true,stale=false;
    std::shared_ptr<MeshData> data;
    QImage texture;QString textureName;
    std::array<double,3> positionDelta{};
    std::array<double,3> rotationDegrees{},scale{1,1,1};
    MeshMaterial material;
    std::shared_ptr<MeshData> placedData()const;
};
struct MeshSelection {int mesh=-1;MeshElement kind=MeshElement::Mesh;int index=-1;};
struct MeshSpan {int id,vertexStart,vertexCount,faceStart,faceCount;};
struct MeshAssembly {std::shared_ptr<MeshData> data;QVector<MeshSpan> spans;};
struct MeshBounds { bool valid=false; QVector3D minimum,maximum; QVector3D dimensions()const{return maximum-minimum;} };
MeshAssembly assembleMeshEntries(const QVector<MeshEntry> &,bool includeHidden=false);
class MeshScene:public QObject {
    Q_OBJECT
public:
    explicit MeshScene(QObject *parent=nullptr):QObject(parent){}
    void beginBatch();void endBatch();bool batchLoading()const{return batchDepth_>0;}
    const QVector<MeshEntry> &entries()const{return entries_;}
    const MeshEntry *entry(int id)const;
    qint64 retainedTextureBytes()const{return textureBytes_;}
    int active()const{return active_;}
    MeshSelection selection()const{return selection_;}
    int add(std::shared_ptr<MeshData>,QString name={},QString source={},const std::shared_ptr<MeshSurface> &surface={});
    bool replace(int id,std::shared_ptr<MeshData>,QString source={});
    bool setPositionDelta(int id,const std::array<double,3> &delta);
    bool setTransform(int id,const std::array<double,3> &position,const std::array<double,3> &rotation,const std::array<double,3> &scale);
    bool setMaterial(int id,const MeshMaterial &material);
    bool setTexture(int id,const QImage &image,const QString &name={});
    void remove(int id);void clear();void rename(int id,QString name);void setVisible(int id,bool visible);
    void setActive(int id);void select(int mesh,MeshElement kind,int index);void markStale();
    MeshAssembly assemble(bool includeHidden=false)const;
    QSet<int> selectedMeshes()const{return selectedMeshes_;}
    void setSelectedMeshes(QSet<int> ids,int active=-1);
    void setMeshesVisible(bool visible,bool selectedOnly=false);
    void isolateSelected();void restoreVisibility();
    bool isolated()const{return !isolation_.isEmpty();}
    MeshBounds bounds(bool selectedOnly=false)const;

signals:
    void geometryChanged();void collectionChanged();void selectionChanged();
private:
    struct BoundsCache {std::weak_ptr<MeshData> data;std::array<double,3> position,rotation,scale;MeshBounds bounds;};
    mutable QHash<int,BoundsCache> boundsCache_;
    struct TextureUse {qint64 bytes=0;int references=0;};
    QHash<qint64,TextureUse> textureUses_;qint64 textureBytes_=0;
    void retainTexture(const QImage &image);void releaseTexture(const QImage &image);
    bool fitsTexture(const QImage &image,const QImage &replaced={})const;
    int batchDepth_=0;bool previousBlocked_=false;
    QSet<int> selectedMeshes_;QHash<int,bool> isolation_;
    QVector<MeshEntry> entries_;int active_=-1,nextId_=0;MeshSelection selection_;
};
