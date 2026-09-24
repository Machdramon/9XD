#pragma once
#include <QObject>
#include "mesh_debug.h"
struct MeshEntry {
    int id=-1;
    QString name,source;
    bool visible=true,stale=false;
    std::shared_ptr<MeshData> data;
};
struct MeshSelection {int mesh=-1;MeshElement kind=MeshElement::Vertex;int index=-1;};
struct MeshSpan {int id,vertexStart,vertexCount,faceStart,faceCount;};
struct MeshAssembly {std::shared_ptr<MeshData> data;QVector<MeshSpan> spans;};
class MeshScene:public QObject {
    Q_OBJECT
public:
    explicit MeshScene(QObject *parent=nullptr):QObject(parent){}
    const QVector<MeshEntry> &entries()const{return entries_;}
    const MeshEntry *entry(int id)const;
    int active()const{return active_;}
    MeshSelection selection()const{return selection_;}
    int add(std::shared_ptr<MeshData>,QString name={},QString source={});
    bool replace(int id,std::shared_ptr<MeshData>,QString source={});
    void remove(int id);void clear();void rename(int id,QString name);void setVisible(int id,bool visible);
    void setActive(int id);void select(int mesh,MeshElement kind,int index);void markStale();
    MeshAssembly assemble(bool includeHidden=false)const;
signals:
    void geometryChanged();void collectionChanged();void selectionChanged();
private:
    QVector<MeshEntry> entries_;int active_=-1,nextId_=0;MeshSelection selection_;
};
