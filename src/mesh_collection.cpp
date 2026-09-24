#include "mesh_scene.h"
#include "vulkan_panel.h"
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QLabel>
#include <QSignalBlocker>
QWidget *VulkanPanel::createMeshCollection(){
    auto *panel=new QWidget;auto *layout=new QVBoxLayout(panel);auto *tree=new QTreeWidget;tree->setObjectName("meshCollection");tree->setHeaderLabels({"Malla / visible","Vértices","Caras"});tree->setRootIsDecorated(false);tree->header()->setSectionResizeMode(0,QHeaderView::Stretch);layout->addWidget(tree);
    auto *hint=new QLabel("Añade mallas desde Configuración 3D. Doble clic: renombrar. La casilla controla ambos visores.");hint->setWordWrap(true);layout->addWidget(hint);auto *remove=new QPushButton("Eliminar malla activa");remove->setObjectName("removeMesh");layout->addWidget(remove);
    auto refresh=[this,tree]{QSignalBlocker block(tree);tree->clear();for(const auto &e:scene_->entries()){auto *item=new QTreeWidgetItem(tree,{e.name,QString::number(e.data->vertices.size()),QString::number(e.data->triangles.size()/3)});item->setData(0,Qt::UserRole,e.id);item->setFlags(item->flags()|Qt::ItemIsEditable|Qt::ItemIsUserCheckable);item->setCheckState(0,e.visible?Qt::Checked:Qt::Unchecked);item->setToolTip(0,QString("ID %1 · %2%3").arg(e.id).arg(e.source.isEmpty()?"Scripting":e.source).arg(e.stale?"\nCaptura anterior: el documento ha cambiado.":""));if(e.stale)item->setForeground(0,QColor("#C98A28"));if(e.id==scene_->active())tree->setCurrentItem(item);}};
    connect(scene_,&MeshScene::collectionChanged,panel,refresh);connect(scene_,&MeshScene::selectionChanged,panel,[this,tree]{QSignalBlocker block(tree);for(int i=0;i<tree->topLevelItemCount();++i)if(tree->topLevelItem(i)->data(0,Qt::UserRole).toInt()==scene_->active())tree->setCurrentItem(tree->topLevelItem(i));});
    connect(tree,&QTreeWidget::currentItemChanged,panel,[this](QTreeWidgetItem *item){if(item)scene_->setActive(item->data(0,Qt::UserRole).toInt());});
    connect(tree,&QTreeWidget::itemChanged,panel,[this](QTreeWidgetItem *item,int column){if(column!=0){QSignalBlocker block(item->treeWidget());auto *e=scene_->entry(item->data(0,Qt::UserRole).toInt());if(e){item->setText(1,QString::number(e->data->vertices.size()));item->setText(2,QString::number(e->data->triangles.size()/3));}return;}int id=item->data(0,Qt::UserRole).toInt();QString name=item->text(0);bool visible=item->checkState(0)==Qt::Checked;const auto *e=scene_->entry(id);if(!e)return;if(e->visible!=visible)scene_->setVisible(id,visible);else if(e->name!=name)scene_->rename(id,name);});
    connect(remove,&QPushButton::clicked,panel,[this]{scene_->remove(scene_->active());});refresh();return panel;
}
