#pragma once
#include <QScrollArea>
#include <QSpinBox>

// Menus reuse each document's existing actions, so edits and analyses retain their source.
inline void configureModuleMenus(MainWindow *document,const std::function<void(const QString &)> &selectModule){
    auto old=document->menuBar()->actions();
    auto *file=old[0]->menu(),*edit=old[1]->menu(),*search=old[2]->menu(),*hex=old[3]->menu();
    auto *analyse=old[4]->menu(),*extras=old[5]->menu(),*windows=old[6]->menu(),*help=old[7]->menu();
    auto findAction=[document](const QString &text)->QAction *{
        for(auto *action:document->findChildren<QAction *>())if(QString(action->text()).remove('&')==text)return action;
        return nullptr;
    };
    auto pending=[](QMenu *menu,const QString &name){auto *a=menu->addAction(name);a->setEnabled(false);a->setToolTip("Not implemented yet.");return a;};
    auto *searchDock=new QDockWidget("Find / Coincidences",document);searchDock->setObjectName("searchDock");
    auto *searchPanel=document->findChild<QWidget *>("patternSearchPanel");
    auto *scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);scroll->setWidget(searchPanel);searchDock->setWidget(scroll);
    document->registerSessionDock(searchDock,Qt::RightDockWidgetArea);
    auto *consoleDock=new QDockWidget("Console",document);consoleDock->setObjectName("scriptConsoleDock");
    consoleDock->setWidget(document->findChild<QPlainTextEdit *>("scriptConsole"));document->registerSessionDock(consoleDock,Qt::BottomDockWidgetArea);
    for(auto *dock:document->sessionDocks()){
        const auto name=dock->objectName();QString module="hex";
        if(name=="vulkanDock"||name=="meshControlsDock"||name=="meshCollectionDock")module="3d";
        else if(name=="uvDock"||name=="imageLibraryDock"||name=="imageCalculatorDock")module="uv";
        else if(name=="scriptDock"||name=="scriptConsoleDock"||name=="scriptLibraryDock")module="scripting";
        else if(name=="threadActivityDock")module="tools";
        dock->setProperty("moduleId",module);dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    }
    auto dockAction=[document,selectModule](QMenu *menu,const QString &name,const QString &dockName){
        auto *dock=document->sessionDock(dockName);auto *action=dock->toggleViewAction();action->setText(name);menu->addAction(action);
        QObject::connect(action,&QAction::triggered,document,[dock,selectModule](bool shown){
            const bool moduleWasVisible=dock->parentWidget()->isVisible();
            selectModule(dock->property("moduleId").toString());
            if(shown||!moduleWasVisible){dock->show();dock->raise();}
        });
        return action;
    };
    auto *hexModule=new QMenu("Hex Viewer",document);hexModule->setObjectName("moduleHexMenu");
    for(auto *action:hex->actions())if(action==document->sessionDock("inspectorDock")->toggleViewAction()||action->text()=="Found Coincidences")hex->removeAction(action);
    hex->setTitle("Main Viewer");auto *showViewer=new QAction("Show Main Viewer",hex);
    hex->insertAction(hex->actions().first(),showViewer);QObject::connect(showViewer,&QAction::triggered,document,[selectModule]{selectModule("hex");});
    hexModule->addMenu(hex);dockAction(hexModule,"Inspector","inspectorDock");
    search->setTitle("Find / Coincidences");hexModule->addMenu(search);
    const auto searchActions=search->actions();
    for(auto *action:searchActions)QObject::disconnect(action,nullptr,document,nullptr);
    auto showSearch=[searchDock,searchPanel,selectModule]{selectModule("hex");searchDock->show();searchDock->raise();auto *field=searchPanel->findChild<QLineEdit *>("patternExpression");field->setFocus();field->selectAll();};
    QObject::connect(searchActions[0],&QAction::triggered,document,showSearch);
    QObject::connect(searchActions[2],&QAction::triggered,document,[showSearch,searchPanel]{showSearch();searchPanel->findChild<QSpinBox *>("matchIndex")->stepUp();});
    QObject::connect(searchActions[3],&QAction::triggered,document,[showSearch,searchPanel]{showSearch();searchPanel->findChild<QSpinBox *>("matchIndex")->stepDown();});
    auto *structure=hexModule->addMenu("Structure / Formatting Rules");dockAction(structure,"Formatting Rules","formatRulesDock");dockAction(structure,"Magic Bytes Library","magicDock");
    dockAction(hexModule,"Statistics","entropyDock");
    for(auto *action:analyse->actions())if(action->menu()&&action->menu()->title()=="Compare Data"){hexModule->addMenu(action->menu());for(auto *entry:action->menu()->actions())QObject::connect(entry,&QAction::triggered,document,[selectModule]{selectModule("hex");});}
    dockAction(hexModule,"Checksum","checksumDock");
    auto *three=new QMenu("3D",document);three->setObjectName("module3dMenu");
    dockAction(three,"3D View","vulkanDock");dockAction(three,"Mesh Collection","meshCollectionDock");dockAction(three,"3D Settings / Geometry Calculator","meshControlsDock");
    three->addSeparator();if(auto *exportAction=document->findChild<QAction *>("exportModelAction"))three->addAction(exportAction);
    auto *uv=new QMenu("UV / 2D",document);uv->setObjectName("moduleUvMenu");dockAction(uv,"2D View","uvDock");dockAction(uv,"Texture / Image Collection","imageLibraryDock");dockAction(uv,"2D Settings / Image Calculator","imageCalculatorDock");
    auto *scripting=new QMenu("Scripting",document);scripting->setObjectName("moduleScriptingMenu");dockAction(scripting,"Scripting Window","scriptDock");dockAction(scripting,"Console","scriptConsoleDock");dockAction(scripting,"Library Manager","scriptLibraryDock");
    auto *other=new QMenu("Other Tools",document);other->setObjectName("moduleToolsMenu");
    if(auto *calculator=findAction("Calculator…"))other->addAction(calculator);
    dockAction(other,"Thread Activity","threadActivityDock");
    if(auto *settings=findAction("Settings")){edit->removeAction(settings);other->addAction(settings);}
    if(auto *button=document->findChild<QToolButton *>("threadActivityToggle"))QObject::connect(button,&QToolButton::toggled,document,[selectModule](bool shown){if(shown)selectModule("tools");});
    // Move existing editing commands out of the legacy Extras menu without losing shortcuts.
    edit->insertAction(edit->actions().first(),findAction("Redo"));
    if(auto *overwrite=findAction("Overwrite bytes…"))edit->addAction(overwrite);
    windows->clear();windows->setObjectName("workspaceWindowsMenu");
    for(auto *action:document->menuBar()->actions())document->menuBar()->removeAction(action);
    for(auto *menu:{file,edit,hexModule,three,uv,scripting,other,windows,help})document->menuBar()->addMenu(menu);
}
