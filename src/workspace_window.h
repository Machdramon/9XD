#pragma once
#include "workbench_icons.h"
#include "command_palette.h"
#include <QShortcut>
#include <QToolButton>
#include "file_dialogs.h"
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPointer>
#include <QSettings>
#include <QFileInfo>
#include <QTabBar>
#include <QScreen>
#include "theme.h"
#include "default_workspace_layout.h"
#include "module_menus.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QSet>
#include "module_windows.h"
#include <QSignalBlocker>
#include "document_tabs.h"

// Each session owns its widgets and asynchronous work. Switching documents never reuses a live worker.
class DocumentSubWindow final : public QMdiSubWindow {
public:
    MainWindow *document=nullptr;
    bool waiting=false;
protected:
    void closeEvent(QCloseEvent *event)override{
        if(!document || document->close()){waiting=false;event->accept();return;}
        waiting=document->documentView()->busy();event->ignore();
        if(waiting)QTimer::singleShot(50,this,[this]{close();});
    }
};

class WorkspaceWindow final : public QMainWindow {
public:
    explicit WorkspaceWindow(QString statePath={}):statePath_(std::move(statePath)){
        setWindowTitle("9XD");resize(1500,900);
        mdi_=new QMdiArea(this);mdi_->setObjectName("documentWorkspace");
        mdi_->setViewMode(QMdiArea::TabbedView);mdi_->setTabsClosable(true);mdi_->setTabsMovable(true);
        mdi_->setDocumentMode(true);mdi_->setActivationOrder(QMdiArea::ActivationHistoryOrder);
        auto *body=new QWidget(this);auto *layout=new QHBoxLayout(body);layout->setContentsMargins(0,0,0,0);layout->setSpacing(0);
        moduleTabs_=new ModuleTabBar;moduleTabs_->setObjectName("moduleTabs");moduleTabs_->setDocumentMode(true);moduleTabs_->setExpanding(false);
        modulePages_=new QStackedWidget;modulePages_->setObjectName("modulePages");
        // Module rail: logo, module icons, then command search and settings at the bottom.
        auto *rail=new QWidget;rail->setObjectName("moduleRail");rail->setAttribute(Qt::WA_StyledBackground);auto *railLayout=new QVBoxLayout(rail);railLayout->setContentsMargins(0,12,0,12);railLayout->setSpacing(6);
        // The project logo, scaled for the rail at the screen's pixel density.
        auto *logo=new QLabel;logo->setObjectName("railLogo");logo->setToolTip("9XD");logo->setAlignment(Qt::AlignCenter);logo->setFixedSize(52,34);
        {const qreal ratio=qMax<qreal>(2,qApp->devicePixelRatio());QPixmap image(":/branding/logo-sticker.png");if(!image.isNull()){image=image.scaled(QSize(52,34)*ratio,Qt::KeepAspectRatio,Qt::SmoothTransformation);image.setDevicePixelRatio(ratio);logo->setPixmap(image);}else logo->setText("9XD");}
        railLayout->addWidget(logo,0,Qt::AlignHCenter);railLayout->addSpacing(8);
        railLayout->addWidget(moduleTabs_,0,Qt::AlignHCenter);railLayout->addStretch();
        auto railButton=[&](const QString &name,const QString &icon,const QString &tip){auto *button=new QToolButton;button->setObjectName(name);button->setIcon(workbenchIcon(icon));button->setIconSize({20,20});button->setFixedSize(44,44);button->setToolTip(tip);railLayout->addWidget(button,0,Qt::AlignHCenter);return button;};
        connect(railButton("railSearch","search","Search commands (Ctrl+K)"),&QToolButton::clicked,this,[this]{showCommandPalette();});
        connect(railButton("railSettings","gear","Settings"),&QToolButton::clicked,this,[this]{showSettings();});
        connect(new QShortcut(QKeySequence("Ctrl+K"),this),&QShortcut::activated,this,[this]{showCommandPalette();});
        layout->addWidget(rail);layout->addWidget(modulePages_,1);setCentralWidget(body);
        const QStringList ids{"hex","3d","uv","scripting","tools"},names{"Hex Viewer","3D","UV / 2D","Scripting","Other Tools"};
        for(int i=0;i<ids.size();++i){
            auto *slot=new QWidget(modulePages_);auto *slotLayout=new QVBoxLayout(slot);slotLayout->setContentsMargins(0,0,0,0);
            moduleSlots_.insert(ids[i],slot);moduleNames_.insert(ids[i],names[i]);
            auto *host=new QMainWindow(slot);host->setWindowFlags(Qt::Widget);host->setObjectName("moduleHost_"+ids[i]);host->setDockNestingEnabled(true);
            host->setDockOptions(QMainWindow::AllowNestedDocks|QMainWindow::AllowTabbedDocks);
            if(i==0){host->setCentralWidget(mdi_);host->statusBar()->setObjectName("hexViewerStatusBar");host->statusBar()->setSizeGripEnabled(false);}
            if(ids[i]=="tools"){
                auto *hint=new QLabel("Open Calculator, Thread Activity or Settings from the Other Tools menu.",host);
                hint->setAlignment(Qt::AlignCenter);hint->setWordWrap(true);host->setCentralWidget(hint);
            }
            auto *notice=new QWidget(slot);auto *noticeLayout=new QVBoxLayout(notice);
            auto *noticeText=new QLabel(names[i]+" is open in a separate window.");noticeText->setAlignment(Qt::AlignCenter);
            noticeLayout->addStretch();noticeLayout->addWidget(noticeText);
            auto *show=new QPushButton("Show Window"),*bringBack=new QPushButton("Return to Main Window");
            noticeLayout->addWidget(show,0,Qt::AlignHCenter);noticeLayout->addWidget(bringBack,0,Qt::AlignHCenter);noticeLayout->addStretch();
            connect(show,&QPushButton::clicked,this,[this,id=ids[i]]{selectModule(id);});connect(bringBack,&QPushButton::clicked,this,[this,id=ids[i]]{attachModule(id);});
            moduleNotices_.insert(ids[i],notice);slotLayout->addWidget(host);slotLayout->addWidget(notice);notice->hide();
            moduleHosts_.insert(ids[i],host);modulePages_->addWidget(slot);moduleTabs_->addTab(workbenchIcon(QHash<QString,QString>{{"hex","hex"},{"3d","cube"},{"uv","image"},{"scripting","code"},{"tools","tools"}}.value(ids[i])),names[i]);moduleTabs_->setTabData(i,ids[i]);moduleTabs_->setTabToolTip(i,names[i]);
        }
        connect(moduleTabs_,&QTabBar::currentChanged,this,[this](int index){selectModule(moduleTabs_->tabData(index).toString());});
        moduleTabs_->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(moduleTabs_,&QTabBar::customContextMenuRequested,this,[this](QPoint position){
            const int index=moduleTabs_->tabAt(position);if(index<0)return;const auto id=moduleTabs_->tabData(index).toString();
            QMenu menu(this);auto *toggle=menu.addAction(isModuleDetached(id)?"Return to Main Window":"Detach Window");
            connect(toggle,&QAction::triggered,this,[this,id]{if(isModuleDetached(id))attachModule(id);else detachModule(id);});menu.exec(moduleTabs_->mapToGlobal(position));
        });
        connect(moduleTabs_,&QTabBar::tabBarDoubleClicked,this,[this](int index){if(index<0)return;const auto id=moduleTabs_->tabData(index).toString();if(isModuleDetached(id))attachModule(id);else detachModule(id);});
        moduleTabs_->detachRequested=[this](int index,QPoint position){detachModule(moduleTabs_->tabData(index).toString(),position);};
        moduleWindowsMenu_=new QMenu("Module Windows",this);moduleWindowsMenu_->setObjectName("moduleWindowsMenu");
        for(const auto &id:ids){auto *action=moduleWindowsMenu_->addAction("Detach "+moduleNames_[id]);moduleWindowActions_.insert(id,action);connect(action,&QAction::triggered,this,[this,id]{if(isModuleDetached(id))attachModule(id);else detachModule(id);});}
        bar_=addToolBar("Common Actions");bar_->setObjectName("mainToolbar");setDockNestingEnabled(true);
        toolbarsMenu_=new QMenu("Toolbars",this);toolbarsMenu_->setObjectName("toolbarsMenu");
        connect(toolbarsMenu_,&QMenu::aboutToShow,this,[this]{refreshToolbarsMenu();});
        if(!statePath_.isEmpty())bar_->setVisible(workspaceSettings().value("toolbars/mainToolbar",standardWorkspaceLayout()["toolbarVisible"].toBool(true)).toBool());
        connect(bar_->toggleViewAction(),&QAction::triggered,this,[this](bool visible){if(!statePath_.isEmpty())workspaceSettings().setValue("toolbars/mainToolbar",visible);});
        documentsMenu_=new QMenu("Documents",this);
        auto *next=documentsMenu_->addAction("Next Document");next->setShortcut(QKeySequence("Ctrl+Tab"));connect(next,&QAction::triggered,mdi_,&QMdiArea::activateNextSubWindow);
        auto *previous=documentsMenu_->addAction("Previous Document");previous->setShortcut(QKeySequence("Ctrl+Shift+Tab"));connect(previous,&QAction::triggered,mdi_,&QMdiArea::activatePreviousSubWindow);
        documentsMenu_->addSeparator();
        auto *tabs=documentsMenu_->addAction("Tabbed View");connect(tabs,&QAction::triggered,this,[this]{mdi_->setViewMode(QMdiArea::TabbedView);refreshDocumentTabs();});
        auto *vertical=documentsMenu_->addAction("Tile Vertically");connect(vertical,&QAction::triggered,this,[this]{tile(true);});
        auto *horizontal=documentsMenu_->addAction("Tile Horizontally");connect(horizontal,&QAction::triggered,this,[this]{tile(false);});
        auto *cascade=documentsMenu_->addAction("Cascade");connect(cascade,&QAction::triggered,this,[this]{mdi_->setViewMode(QMdiArea::SubWindowView);mdi_->cascadeSubWindows();});
        documentsMenu_->addSeparator();
        auto *saveAll=documentsMenu_->addAction("Save All");saveAll->setObjectName("saveAllDocuments");saveAll->setShortcut(QKeySequence("Ctrl+Shift+S"));connect(saveAll,&QAction::triggered,this,[this]{saveNext(0);});
        // Ctrl+Shift+S remains Save As; Save All has no conflicting shortcut.
        saveAll->setShortcut({});
        auto *closeAll=documentsMenu_->addAction("Close All");connect(closeAll,&QAction::triggered,mdi_,&QMdiArea::closeAllSubWindows);
        listMenu_=documentsMenu_->addMenu("Open Documents");connect(listMenu_,&QMenu::aboutToShow,this,[this]{rebuildDocumentList();});
        themeMenu_=new QMenu("Theme",this);
        for(auto name:{QString("Default"),QString("Light"),QString("Dark"),QString("System")}){
            auto *action=themeMenu_->addAction(name);action->setObjectName(name.toLower()+"ThemeAction");action->setCheckable(true);
            connect(action,&QAction::triggered,this,[this,name]{setTheme(name.toLower());});
        }
        connect(mdi_,&QMdiArea::subWindowActivated,this,[this](QMdiSubWindow *window){
            // Hiding the Hex Viewer page deactivates MDI focus, not its document.
            if(!window&&!mdi_->subWindowList().isEmpty())return;
            activate(dynamic_cast<DocumentSubWindow *>(window));
        });
        auto settings=workspaceSettings();loadWorkbenchPreferences(settings);
        connect(qApp->styleHints(),&QStyleHints::colorSchemeChanged,this,[this]{if(qApp->property("workbenchThemeBase").toString()=="system")applyWorkbenchTheme(qApp->property("workbenchTheme").toString());});
        if(!statePath_.isEmpty()){
            auto geometry=settings.value("geometryV2").toByteArray();if(geometry.isEmpty())geometry=QByteArray::fromBase64(standardWorkspaceLayout()["windowGeometry"].toString().toLatin1());
            if(!geometry.isEmpty())restoreGeometry(geometry);
        }
        if(!statePath_.isEmpty()){
            for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it)sharedLayouts_[it.key()]=workspaceModuleLayout(settings,it.key());
            const auto suspended=settings.value("moduleLayouts/v1/suspendedFloating").toStringList();
            for(const auto &name:suspended)suspendedFloating_.insert(name);
        }
        newDocument();
        if(!statePath_.isEmpty()){
            QFileInfo path(statePath_);statePath_=path.dir().filePath(path.completeBaseName()+".modules.json");
            workspaceLayout_=new UiLayout(this,statePath_);
        }
        const auto available=screen()->availableGeometry();resize(size().boundedTo(available.size()));
        move(qBound(available.left(),x(),available.right()-width()+1),qBound(available.top(),y(),available.bottom()-height()+1));
        if(!statePath_.isEmpty()){
            for(const auto &id:ids)detachedGeometry_[id]=settings.value("moduleWindows/v1/"+id+"/geometry").toByteArray();
            const auto detached=settings.value("moduleWindows/v1/detached").toStringList();
            QTimer::singleShot(0,this,[this,detached]{for(const auto &id:detached)detachModule(id);});
        }
    }
    ~WorkspaceWindow()override{
        switchingDocuments_=true;
        if(workspaceLayout_)workspaceLayout_->saveAndStop();
        disconnect(mdi_,nullptr,this,nullptr);disconnect(moduleTabs_,nullptr,this,nullptr);
        for(auto *window:detachedWindows_)window->returnRequested={};
        // Destroy sessions while their module hosts and layout maps still exist.
        for(auto window:sessions_)if(window){disconnect(window,nullptr,this,nullptr);delete window;}
        qDeleteAll(detachedWindows_);detachedWindows_.clear();
        for(auto window:retiredWindows_)if(window)delete window;
    }
    MainWindow *activeDocument()const{auto *sub=dynamic_cast<DocumentSubWindow *>(mdi_->activeSubWindow());if(!sub)sub=active_;return sub?sub->document:nullptr;}
    int documentCount()const{return mdi_->subWindowList().size();}
    void newDocument(){auto *document=addDocument();document->documentView()->newMemoryDocument();document->setUntitledName(QString("Untitled %1").arg(++untitled_));refreshTitle(document);}
    void openFile(const QString &path){
        if(path.isEmpty())return;const QString canonical=QFileInfo(path).canonicalFilePath();
        for(auto *window:mdi_->subWindowList()){
            auto *sub=dynamic_cast<DocumentSubWindow *>(window);const auto existing=sub->document->documentView()->filePath();
            if(!canonical.isEmpty()&&!existing.isEmpty()&&QFileInfo(existing).canonicalFilePath()==canonical){mdi_->setActiveSubWindow(sub);return;}
        }
        auto *document=activeDocument();
        if(!document || document->documentView()->busy() || document->documentView()->dirty() || !document->documentView()->filePath().isEmpty() || document->documentView()->fileSize()>0)document=addDocument();
        document->openInPlace(path);refreshTitle(document);
    }
    void setTheme(const QString &name){
        applyWorkbenchTheme(name);if(!statePath_.isEmpty())workspaceSettings().setValue("theme",name);
        for(auto *action:themeMenu_->actions())action->setChecked(action->text().toLower()==name);
        for(auto *window:mdi_->subWindowList())window->update();
    }
    void showCommandPalette(){
        QList<CommandPalette::Entry> entries;
        for(int i=0;i<moduleTabs_->count();++i){const auto id=moduleTabs_->tabData(i).toString();entries.append({"Go to "+moduleNames_.value(id),[this,id]{selectModule(id);}});}
        entries.append({"Settings",[this]{showSettings();}});
        // Main panel actions: switch to their module, then press the button.
        QSet<QString> seen;
        for(const auto &name:workbenchPrimaryButtons())for(auto *button:findChildren<QPushButton *>(name)){
            QString module;for(QWidget *w=button;w;w=w->parentWidget())if(w->objectName().startsWith("moduleHost_")){module=w->objectName().mid(11);break;}
            const auto label=(module.isEmpty()?QString("Actions"):moduleNames_.value(module))+" \u203a "+(button->toolTip().isEmpty()||button->toolTip().size()>40?button->text():button->toolTip());
            if(seen.contains(label))continue;seen.insert(label);QPointer<QPushButton> target(button);
            entries.append({label,[this,target,module]{if(!module.isEmpty())selectModule(module);if(target&&target->isEnabled())target->click();}});
        }
        entries+=CommandPalette::menuEntries(menuBar());
        (new CommandPalette(this,entries))->popup();
    }
    void showSettings(){
        auto settings=workspaceSettings();
        SettingsDialog dialog(settings,[this]{
            for(auto *window:mdi_->subWindowList())dynamic_cast<DocumentSubWindow *>(window)->document->documentView()->applyDisplayPreferences();
            for(auto *action:themeMenu_->actions())action->setChecked(action->text().toLower()==qApp->property("workbenchTheme").toString());
        },this);
        dialog.exec();
    }
    void selectModule(const QString &id,bool focusWindow=true){
        if(switchingDocuments_||!moduleHosts_.contains(id))return;
        const int index=modulePages_->indexOf(moduleSlots_[id]);
        modulePages_->setCurrentIndex(index);{QSignalBlocker blocker(moduleTabs_);moduleTabs_->setCurrentIndex(index);}
        if(focusWindow&&isModuleDetached(id)){auto *window=detachedWindows_[id];window->show();window->raise();window->activateWindow();}
        if(auto *doc=activeDocument())for(auto *dock:doc->sessionDocks())if(dock->isFloating()){
            if(dock->property("moduleId").toString()!=id&&!isModuleDetached(dock->property("moduleId").toString())){if(!dock->isHidden()){dock->setProperty("floatingVisible",true);suspendedFloating_.insert(dock->objectName());}dock->hide();}
            else if(dock->property("floatingVisible").toBool()){dock->setProperty("floatingVisible",false);suspendedFloating_.remove(dock->objectName());dock->show();}
        }
    }
    void resetPanelLayout(){
        const auto detached=detachedWindows_.keys();for(const auto &id:detached)attachModule(id);
        suspendedFloating_.clear();for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it){auto state=standardModuleLayout(it.key());if(!state.isEmpty()){sharedLayouts_[it.key()]=state;it.value()->restoreState(state);}}
        bar_->setVisible(standardWorkspaceLayout()["toolbarVisible"].toBool(true));
        if(auto *doc=activeDocument())for(auto *dock:doc->sessionDocks())dock->setProperty("floatingVisible",false);
        if(!statePath_.isEmpty()){auto settings=workspaceSettings();settings.setValue("toolbars/mainToolbar",!bar_->isHidden());}
    }
    QMainWindow *moduleHost(const QString &id)const{return moduleHosts_.value(id);}
    bool isModuleDetached(const QString &id)const{return detachedWindows_.contains(id);}
    ModuleWindow *moduleWindow(const QString &id)const{return detachedWindows_.value(id);}
    void detachModule(const QString &id,QPoint position={}){
        if(!moduleHosts_.contains(id)||switchingDocuments_)return;
        if(isModuleDetached(id)){selectModule(id);return;}
        auto *window=new ModuleWindow;window->setObjectName("moduleWindow_"+id);window->resize(1100,750);
        window->returnRequested=[this,id]{attachModule(id);};detachedWindows_.insert(id,window);
        auto *host=moduleHosts_[id];moduleSlots_[id]->layout()->removeWidget(host);window->setCentralWidget(host);host->setWindowFlags(Qt::Widget);host->show();moduleNotices_[id]->show();
        if(!detachedGeometry_.value(id).isEmpty())window->restoreGeometry(detachedGeometry_[id]);
        if(!position.isNull())window->move(position-QPoint(30,15));
        keepModuleWindowOnScreen(window);moduleWindowActions_[id]->setText("Return "+moduleNames_[id]+" to Main Window");
        refreshDetachedWindows();window->show();window->raise();
        selectModule(moduleTabs_->tabData(moduleTabs_->currentIndex()).toString(),false);
    }
    void attachModule(const QString &id){
        auto *window=detachedWindows_.value(id);if(!window)return;
        detachedGeometry_[id]=window->saveGeometry();auto *host=window->takeCentralWidget();
        host->setParent(moduleSlots_[id]);host->setWindowFlags(Qt::Widget);moduleSlots_[id]->layout()->addWidget(host);moduleNotices_[id]->hide();host->show();
        detachedWindows_.remove(id);window->returnRequested={};window->hide();retiredWindows_.append(window);window->deleteLater();
        moduleWindowActions_[id]->setText("Detach "+moduleNames_[id]);selectModule(id,false);
    }
protected:
    void closeEvent(QCloseEvent *event)override{
        for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it)sharedLayouts_[it.key()]=it.value()->saveState();
        if(workspaceLayout_)workspaceLayout_->saveAndStop();
        for(auto *window:mdi_->subWindowList())if(!window->close()){
            event->ignore();auto *sub=dynamic_cast<DocumentSubWindow *>(window);
            if(sub&&sub->waiting)QTimer::singleShot(100,this,[this]{close();});
            else if(workspaceLayout_){delete workspaceLayout_;workspaceLayout_=new UiLayout(this,statePath_);}
            return;
        }
        if(!statePath_.isEmpty()){
            auto settings=workspaceSettings();settings.setValue("geometryV2",saveGeometry());
            for(auto it=sharedLayouts_.cbegin();it!=sharedLayouts_.cend();++it)settings.setValue("moduleLayouts/v1/"+it.key(),it.value());
            settings.setValue("moduleLayouts/v1/suspendedFloating",QStringList(suspendedFloating_.cbegin(),suspendedFloating_.cend()));
            settings.setValue("moduleWindows/v1/detached",detachedWindows_.keys());
            for(auto it=detachedWindows_.cbegin();it!=detachedWindows_.cend();++it)detachedGeometry_[it.key()]=it.value()->saveGeometry();
            for(auto it=detachedGeometry_.cbegin();it!=detachedGeometry_.cend();++it)settings.setValue("moduleWindows/v1/"+it.key()+"/geometry",it.value());
        }for(auto *window:detachedWindows_)window->hide();event->accept();
    }
private:
    MainWindow *addDocument(){
        auto *document=new MainWindow({},statePath_.isEmpty()?QString():QFileInfo(statePath_).dir().filePath("recent-files.json"));
        document->openRequested=[this](const QString &path){openFile(path);};document->newRequested=[this]{newDocument();};document->exitRequested=[this]{close();};
        document->resourceRequested=[this](QByteArray bytes,QString name,QJsonObject origin,QString mode){auto *child=addDocument();child->openResourceData(std::move(bytes),name,origin,mode);refreshTitle(child);};
        document->settingsRequested=[this]{showSettings();};
        document->showModuleRequested=[this,document](const QString &module){for(auto session:sessions_)if(session&&session->document==document){mdi_->setActiveSubWindow(session);break;}selectModule(module);};
        document->aiWorkspaceDocuments=[this]{QJsonArray documents;for(auto session:sessions_)if(session&&documents.size()<16){auto *view=session->document->documentView();documents.append(QJsonObject{{"name",view->filePath().isEmpty()?session->document->untitledName():QFileInfo(view->filePath()).fileName()},{"size",QString::number(view->fileSize())},{"unsavedChanges",view->dirty()},{"active",session==active_}});}return documents;};
        connect(document->documentView(),&HexView::viewerFontSizeChanged,this,[this,document](int points){
            if(!statePath_.isEmpty())workspaceSettings().setValue("editor/fontSize",points);
            for(auto *window:mdi_->subWindowList()){
                auto *other=dynamic_cast<DocumentSubWindow *>(window)->document;
                if(other!=document)other->documentView()->applyDisplayPreferences();
            }
        });
        document->saveDestinationAllowed=[this,document](const QString &path){
            for(auto *window:mdi_->subWindowList()){
                auto *other=dynamic_cast<DocumentSubWindow *>(window)->document;
                if(other!=document&&!other->documentView()->filePath().isEmpty()&&QFileInfo(other->documentView()->filePath()).absoluteFilePath().compare(QFileInfo(path).absoluteFilePath(),Qt::CaseInsensitive)==0){QMessageBox::warning(this,"Save File","This path is already open in another document. Choose a different destination.");return false;}
            }return true;
        };
        configureModuleMenus(document,[this](const QString &id){selectModule(id);});
        auto *sub=new DocumentSubWindow;sub->document=document;sub->setAttribute(Qt::WA_DeleteOnClose);sub->setWidget(document);mdi_->addSubWindow(sub);
        sessions_.removeIf([](auto session){return session.isNull();});sessions_.append(sub);
        document->menuBar()->hide();for(auto *tool:document->findChildren<QToolBar *>(QString(),Qt::FindDirectChildrenOnly)){document->removeToolBar(tool);tool->setObjectName("sessionToolbar");tool->setFloatable(false);tool->hide();}
        for(auto *action:document->findChildren<QAction *>())if(!action->shortcut().isEmpty())action->setProperty("sessionShortcut",action->shortcut());
        auto *windows=document->findChild<QMenu *>("workspaceWindowsMenu");windows->addSeparator();windows->addMenu(documentsMenu_);windows->addMenu(moduleWindowsMenu_);windows->addMenu(themeMenu_);windows->addSeparator();auto *reset=windows->addAction("Reset Panel Layout");reset->setObjectName("resetWorkspaceLayout");connect(reset,&QAction::triggered,this,[this]{resetPanelLayout();});
        for(auto *action:document->menuBar()->actions()[0]->menu()->actions())if(action->text()=="Close Files"){
            action->setText("Close Document");action->setEnabled(true);action->setShortcut(QKeySequence("Ctrl+W"));action->setProperty("sessionShortcut",action->shortcut());connect(action,&QAction::triggered,sub,&QWidget::close);
        }
        connect(document->documentView(),&HexView::fileOpened,this,[this,document](const QString &){refreshTitle(document);});
        connect(document->documentView(),&HexView::editStateChanged,this,[this,document]{refreshTitle(document);});
        connect(sub,&QObject::destroyed,this,[this]{QTimer::singleShot(0,this,[this]{if(!activeDocument())emptyMenu();refreshDocumentTabs();});});
        if(mdi_->viewMode()==QMdiArea::TabbedView)sub->showMaximized();else sub->show();
        mdi_->setActiveSubWindow(sub);activate(sub);return document;
    }
    void refreshTitle(MainWindow *document){
        auto *sub=qobject_cast<QMdiSubWindow *>(document->parentWidget());if(!sub)return;
        auto *view=document->documentView();QString name=view->filePath().isEmpty()?document->untitledName():QFileInfo(view->filePath()).fileName();
        sub->setWindowTitle(name+(view->dirty()?" *":""));sub->setToolTip(view->filePath().isEmpty()?"Unsaved RAM document":view->filePath());
        if(document==activeDocument())setWindowTitle(name+(view->dirty()?" *":"")+" — 9XD");
        if(document==activeDocument())refreshDetachedWindows();
        if(auto *tabs=mdi_->findChild<QTabBar *>())for(int i=0;i<tabs->count();++i)if(tabs->tabText(i)==sub->windowTitle())tabs->setTabToolTip(i,sub->toolTip());
        refreshDocumentTabs();
    }
    void activate(DocumentSubWindow *sub){
        if(sub&&active_==sub){refreshTitle(sub->document);return;}
        switchingDocuments_=true;
        if(active_&&active_!=sub){
            active_->document->documentView()->restoreStatusFooter();
            for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it)sharedLayouts_[it.key()]=it.value()->saveState();
            for(auto *dock:active_->document->sessionDocks()){
                dock->setProperty("sessionVisible",!dock->isHidden()||dock->property("floatingVisible").toBool());
                if(auto *host=qobject_cast<QMainWindow *>(dock->parentWidget()))host->removeDockWidget(dock);
                dock->setFloating(false);
                dock->setParent(active_->document);dock->hide();
            }
            for(auto *action:active_->document->findChildren<QAction *>())if(action->property("sessionShortcut").isValid())action->setShortcut({});
        }
        active_=sub;for(auto *action:menuBar()->actions())menuBar()->removeAction(action);bar_->clear();
        if(!sub){switchingDocuments_=false;emptyMenu();return;}
        auto *doc=sub->document;
        moduleHosts_["hex"]->statusBar()->addPermanentWidget(doc->documentView()->statusFooter(),1);doc->documentView()->statusFooter()->show();
        for(auto *action:doc->findChildren<QAction *>())if(action->property("sessionShortcut").isValid())action->setShortcut(action->property("sessionShortcut").value<QKeySequence>());
        for(auto *dock:doc->sessionDocks()){
            doc->removeDockWidget(dock);auto *host=moduleHosts_.value(dock->property("moduleId").toString());
            dock->setParent(host);host->addDockWidget(Qt::DockWidgetArea(dock->property("initialArea").toInt()),dock);
            dock->setVisible(dock->property("sessionVisible").toBool());
        }
        for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it){
            const auto state=sharedLayouts_.value(it.key());
            if(!state.isEmpty()&&(it.value()->restoreState(state)||(!statePath_.isEmpty()&&it.value()->restoreState(standardModuleLayout(it.key()))))){}
            else if(it.key()=="hex"){
                auto *inspector=doc->sessionDock("inspectorDock"),*format=doc->sessionDock("formatRulesDock");
                it.value()->tabifyDockWidget(inspector,format);inspector->raise();doc->sessionDock("entropyDock")->hide();
            }else if(it.key()=="3d"){
                doc->sessionDock("vulkanDock")->show();doc->sessionDock("meshCollectionDock")->show();
            }else if(it.key()=="uv")doc->sessionDock("uvDock")->show();
            else if(it.key()=="scripting"){doc->sessionDock("scriptDock")->show();doc->sessionDock("scriptConsoleDock")->show();}
        }
        for(auto *dock:doc->sessionDocks())dock->setProperty("floatingVisible",suspendedFloating_.contains(dock->objectName()));
        switchingDocuments_=false;selectModule(moduleTabs_->tabData(moduleTabs_->currentIndex()).toString(),false);
        doc->menuBar()->hide();
        for(auto *action:doc->menuBar()->actions()){if(QString(action->text()).remove('&')=="Windows")menuBar()->addMenu(toolbarsMenu_);menuBar()->addAction(action);}
        if(auto *tools=doc->findChild<QToolBar *>("sessionToolbar")){for(auto *action:tools->actions())bar_->addAction(action);tools->hide();}
        refreshTitle(doc);refreshDetachedWindows();
        for(auto *action:themeMenu_->actions())action->setChecked(action->text().toLower()==qApp->property("workbenchTheme").toString());
    }
    void refreshToolbarsMenu(){
        toolbarsMenu_->clear();
        for(auto *toolbar:findChildren<QToolBar *>()){
            auto *host=qobject_cast<QMainWindow *>(toolbar->parentWidget());
            if(!host||host->toolBarArea(toolbar)==Qt::NoToolBarArea)continue;
            auto *action=toolbar->toggleViewAction();action->setText(toolbar->windowTitle());toolbarsMenu_->addAction(action);
        }
        if(toolbarsMenu_->isEmpty()){auto *empty=toolbarsMenu_->addAction("No toolbars available");empty->setEnabled(false);}
    }
    void emptyMenu(){
        for(auto *action:menuBar()->actions())menuBar()->removeAction(action);bar_->clear();
        if(!fallback_){fallback_=new QMenu("File",this);auto *create=fallback_->addAction("New File");create->setShortcut(QKeySequence::New);connect(create,&QAction::triggered,this,[this]{newDocument();});
            auto *open=fallback_->addAction("Open File…");open->setShortcut(QKeySequence::Open);connect(open,&QAction::triggered,this,[this]{auto paths=WorkbenchFiles::getOpenFileNames(this,"Open Files");for(const auto &path:paths)openFile(path);});fallback_->addSeparator();connect(fallback_->addAction("Settings"),&QAction::triggered,this,[this]{showSettings();});fallback_->addSeparator();connect(fallback_->addAction("Exit"),&QAction::triggered,this,&QWidget::close);}
        menuBar()->addMenu(fallback_);menuBar()->addMenu(toolbarsMenu_);menuBar()->addMenu(moduleWindowsMenu_);menuBar()->addMenu(themeMenu_);setWindowTitle("9XD");refreshDetachedWindows();
    }
    void keepModuleWindowOnScreen(QWidget *window){
        bool accessible=false;auto title=window->frameGeometry();title.setHeight(30);
        for(auto *screen:QGuiApplication::screens())if(screen->availableGeometry().intersected(title).width()>=80){accessible=true;break;}
        if(!accessible){const auto available=screen()->availableGeometry();window->resize(window->size().boundedTo(available.size()));window->move(available.topLeft()+QPoint(25,25));}
    }
    void refreshDocumentTabs(){
        if(auto *tabs=mdi_->findChild<QTabBar *>()){
            if(!documentTabs_||documentTabs_->parent()!=tabs)documentTabs_=new CompactDocumentTabs(tabs);
            documentTabs_->refresh();
        }
    }
    void refreshDetachedWindows(){
        auto *doc=activeDocument();const auto name=doc?(doc->documentView()->filePath().isEmpty()?doc->untitledName():QFileInfo(doc->documentView()->filePath()).fileName()):QString("No document");
        for(auto it=detachedWindows_.cbegin();it!=detachedWindows_.cend();++it){
            auto *window=it.value();window->setWindowTitle(moduleNames_[it.key()]+" — "+name+(doc&&doc->documentView()->dirty()?" *":"")+" — 9XD");
            for(auto *action:window->menuBar()->actions())window->menuBar()->removeAction(action);
            for(auto *action:menuBar()->actions())window->menuBar()->addAction(action);
            for(auto *action:window->actions())window->removeAction(action);
            if(doc)for(auto *action:doc->findChildren<QAction *>())if(action->property("sessionShortcut").isValid())window->addAction(action);
            if(doc)for(auto *dock:doc->sessionDocks())if(!dock->toggleViewAction()->shortcut().isEmpty())window->addAction(dock->toggleViewAction());
            for(auto *action:documentsMenu_->actions())if(!action->shortcut().isEmpty())window->addAction(action);
        }
    }
    void tile(bool vertical){
        mdi_->setViewMode(QMdiArea::SubWindowView);auto windows=mdi_->subWindowList();if(windows.isEmpty())return;auto rect=mdi_->viewport()->rect();
        for(int i=0;i<windows.size();++i){windows[i]->showNormal();const int begin=(vertical?rect.width():rect.height())*i/windows.size(),end=(vertical?rect.width():rect.height())*(i+1)/windows.size();windows[i]->setGeometry(vertical?QRect(begin,0,end-begin,rect.height()):QRect(0,begin,rect.width(),end-begin));}
    }
    void saveNext(int index){
        auto windows=mdi_->subWindowList();if(index>=windows.size())return;auto *doc=dynamic_cast<DocumentSubWindow *>(windows[index])->document;
        if(!doc->documentView()->dirty()&&!doc->documentView()->filePath().isEmpty()){saveNext(index+1);return;}
        mdi_->setActiveSubWindow(windows[index]);auto connection=std::make_shared<QMetaObject::Connection>();
        *connection=connect(doc->documentView(),&HexView::saveFinished,this,[this,index,connection](bool ok){disconnect(*connection);if(ok)QTimer::singleShot(0,this,[this,index]{saveNext(index+1);});});
        if(!doc->saveDocument(false))disconnect(*connection);
    }
    QMdiArea *mdi_;QToolBar *bar_;QMenu *toolbarsMenu_,*documentsMenu_,*listMenu_,*themeMenu_,*fallback_=nullptr;
    QPointer<DocumentSubWindow> active_;QPointer<CompactDocumentTabs> documentTabs_;QList<QPointer<DocumentSubWindow>> sessions_;QHash<QString,QByteArray> sharedLayouts_;
    QHash<QString,QMainWindow *> moduleHosts_;QSet<QString> suspendedFloating_;ModuleTabBar *moduleTabs_;
    QHash<QString,QWidget *> moduleSlots_,moduleNotices_;QHash<QString,QString> moduleNames_;
    QHash<QString,ModuleWindow *> detachedWindows_;QList<QPointer<ModuleWindow>> retiredWindows_;QHash<QString,QByteArray> detachedGeometry_;
    QHash<QString,QAction *> moduleWindowActions_;QMenu *moduleWindowsMenu_;QStackedWidget *modulePages_;bool switchingDocuments_=false;QString statePath_;int untitled_=0;UiLayout *workspaceLayout_=nullptr;
    void rebuildDocumentList(){listMenu_->clear();for(auto *window:mdi_->subWindowList()){auto *action=listMenu_->addAction(window->windowTitle());action->setCheckable(true);action->setChecked(window==mdi_->activeSubWindow());connect(action,&QAction::triggered,this,[this,window]{mdi_->setActiveSubWindow(window);});}}
};
