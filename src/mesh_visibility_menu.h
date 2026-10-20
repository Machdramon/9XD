#pragma once
#include "mesh_scene.h"
#include <QMenu>
inline QMenu *meshVisibilityMenu(MeshScene *scene,QWidget *parent){
    auto *menu=new QMenu("Visibility",parent);
    auto action=[&](const char *label,const char *id,auto callback){auto *a=menu->addAction(label);a->setObjectName(id);QObject::connect(a,&QAction::triggered,menu,callback);return a;};
    action("Show all","meshShowAll",[scene]{scene->setMeshesVisible(true);});
    action("Hide all","meshHideAll",[scene]{scene->setMeshesVisible(false);});menu->addSeparator();
    auto *show=action("Show selected","meshShowSelected",[scene]{scene->setMeshesVisible(true,true);});
    auto *hide=action("Hide selected","meshHideSelected",[scene]{scene->setMeshesVisible(false,true);});
    auto *isolate=action("Isolate selected","meshIsolateSelected",[scene]{scene->isolateSelected();});
    auto *restore=action("Exit isolation","meshRestoreVisibility",[scene]{scene->restoreVisibility();});
    QObject::connect(menu,&QMenu::aboutToShow,menu,[scene,show,hide,isolate,restore]{bool any=!scene->selectedMeshes().isEmpty();show->setEnabled(any);hide->setEnabled(any);isolate->setEnabled(any);restore->setEnabled(scene->isolated());});return menu;
}
