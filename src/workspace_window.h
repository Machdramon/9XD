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
#include "interface_modes.h"
#include "default_workspace_layout.h"
#include "module_menus.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QSet>
#include "module_windows.h"
#include <QSignalBlocker>
#include "document_tabs.h"
#include <QPainter>
#include <QTimer>
#include <QProgressBar>
#include <QStatusBar>
#include <QTime>
#include <QMoveEvent>
#include <QDateTime>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QMessageBox>
#include <QDir>
#include <QSaveFile>
#include "release_defaults.h"
#include <functional>
#include <QCursor>
#include <QEnterEvent>
#include <QFrame>
#include <QJsonDocument>

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

// Floating HUD mode dock: a rounded, translucent tool window at the bottom of the work area with the modules, search
// and settings. It is a top-level window because the native 3D viewport would cover a child overlay.
class HudModeDock final : public QWidget {
public:
    explicit HudModeDock(QWidget *owner):QWidget(owner,Qt::Tool|Qt::FramelessWindowHint){
        setObjectName("hudModeDock");setAttribute(Qt::WA_TranslucentBackground);setAttribute(Qt::WA_ShowWithoutActivating);
        row=new QHBoxLayout(this);row->setContentsMargins(18,9,18,9);row->setSpacing(8);
        // Dragging any free part of the background (around and between the icons) moves the dock; double-click resets it.
        setCursor(Qt::SizeAllCursor);setToolTip("Drag the background to move the dock; double-click to put it back");
    }
    QHBoxLayout *row;
    std::function<void()> moved,reset;std::function<void(bool)> hovered;
protected:
    void mousePressEvent(QMouseEvent *event)override{if(event->button()==Qt::LeftButton){grab_=event->globalPosition().toPoint()-pos();dragging_=true;event->accept();}}
    void mouseMoveEvent(QMouseEvent *event)override{if(dragging_)move(event->globalPosition().toPoint()-grab_);}
    void mouseReleaseEvent(QMouseEvent *event)override{if(dragging_&&event->button()==Qt::LeftButton){dragging_=false;if(moved)moved();}}
    void mouseDoubleClickEvent(QMouseEvent *)override{dragging_=false;if(reset)reset();}
    void enterEvent(QEnterEvent *)override{if(hovered)hovered(true);}
    void leaveEvent(QEvent *)override{if(hovered)hovered(false);}
    void paintEvent(QPaintEvent *)override{
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);QColor fill=palette().color(QPalette::Window);fill.setAlpha(238);
        painter.setPen(QPen(palette().color(QPalette::Mid),1));painter.setBrush(fill);painter.drawRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),18,18);
    }
private:
    QPoint grab_;bool dragging_=false;
};

class WorkspaceWindow final : public QMainWindow {
public:
    explicit WorkspaceWindow(QString statePath={}):statePath_(std::move(statePath)){
        setWindowTitle("9XD");resize(1500,900);
        // A new installation starts from the embedded release defaults (tests use their own settings file).
        if(!statePath_.isEmpty()&&qApp->property("workspaceSettingsFile").toString().isEmpty()){auto settings=workspaceSettings();ReleaseDefaults::seed(settings);}
        mdi_=new QMdiArea(this);mdi_->setObjectName("documentWorkspace");
        mdi_->setViewMode(QMdiArea::TabbedView);mdi_->setTabsClosable(true);mdi_->setTabsMovable(true);
        mdi_->setDocumentMode(true);mdi_->setActivationOrder(QMdiArea::ActivationHistoryOrder);
        auto *body=new QWidget(this);auto *layout=new QHBoxLayout(body);layout->setContentsMargins(0,0,0,0);layout->setSpacing(0);
        moduleTabs_=new ModuleTabBar;moduleTabs_->setObjectName("moduleTabs");moduleTabs_->setDocumentMode(true);moduleTabs_->setExpanding(false);
        modulePages_=new QStackedWidget;modulePages_->setObjectName("modulePages");
        // Module rail: logo, module icons, then command search and settings at the bottom.
        auto *rail=new QWidget;rail_=rail;rail->setObjectName("moduleRail");rail->setAttribute(Qt::WA_StyledBackground);auto *railLayout=new QVBoxLayout(rail);railLayout->setContentsMargins(0,12,0,12);railLayout->setSpacing(6);
        // The project logo, scaled for the rail at the screen's pixel density.
        auto *logo=new QLabel;logo->setObjectName("railLogo");logo->setToolTip("9XD");logo->setAlignment(Qt::AlignCenter);logo->setFixedSize(52,34);
        {const qreal ratio=qMax<qreal>(2,qApp->devicePixelRatio());QPixmap image(":/branding/logo-sticker.png");if(!image.isNull()){image=image.scaled(QSize(52,34)*ratio,Qt::KeepAspectRatio,Qt::SmoothTransformation);image.setDevicePixelRatio(ratio);logo->setPixmap(image);}else logo->setText("9XD");}
        railLayout->addWidget(logo,0,Qt::AlignHCenter);railLayout->addSpacing(8);
        railLayout->addWidget(moduleTabs_,0,Qt::AlignHCenter);railLayout->addStretch();
        auto railButton=[&](const QString &name,const QString &icon,const QString &tip){auto *button=new QToolButton;button->setObjectName(name);button->setIcon(workbenchIcon(icon));button->setIconSize({20,20});button->setFixedSize(44,44);button->setToolTip(tip);railLayout->addWidget(button,0,Qt::AlignHCenter);return button;};
        connect(railButton("railSearch","search","Search commands (Ctrl+K)"),&QToolButton::clicked,this,[this]{showCommandPalette();});
        connect(railButton("railSettings","gear","Settings"),&QToolButton::clicked,this,[this]{showSettings();});
        connect(new QShortcut(QKeySequence("Ctrl+K"),this),&QShortcut::activated,this,[this]{showCommandPalette();});
        // Open files stay reachable from every module: the Hex Viewer shows the document tabs, other modules show this strip.
        auto *pages=new QWidget;auto *pagesLayout=new QVBoxLayout(pages);pagesLayout->setContentsMargins(0,0,0,0);pagesLayout->setSpacing(0);
        openDocumentsBar_=new QWidget;openDocumentsBar_->setObjectName("openDocumentsBar");openDocumentsBar_->setAttribute(Qt::WA_StyledBackground);
        auto *stripLayout=new QHBoxLayout(openDocumentsBar_);stripLayout->setContentsMargins(10,0,4,0);stripLayout->setSpacing(6);
        auto *stripCaption=new QLabel("Open files");stripCaption->setObjectName("openDocumentsCaption");
        openDocuments_=new QTabBar;openDocuments_->setObjectName("openDocumentsStrip");openDocuments_->setDocumentMode(true);openDocuments_->setDrawBase(false);
        openDocuments_->setExpanding(false);openDocuments_->setTabsClosable(true);openDocuments_->setElideMode(Qt::ElideMiddle);openDocuments_->setUsesScrollButtons(true);
        openDocuments_->setAccessibleName("Open files");
        auto *stripHex=new QToolButton;stripHex->setObjectName("openDocumentsShowHex");stripHex->setIcon(workbenchIcon("hex"));stripHex->setIconSize({16,16});stripHex->setToolTip("Show the active file in the Hex Viewer");
        stripLayout->addWidget(stripCaption);stripLayout->addWidget(openDocuments_,1);stripLayout->addWidget(stripHex);
        statusLine_=new QWidget;statusLine_->setObjectName("statusLine");statusLine_->setAttribute(Qt::WA_StyledBackground);statusLine_->hide();pagesLayout->addWidget(statusLine_);
        deckHeader_=new QWidget;deckHeader_->setObjectName("deckHeader");deckHeader_->setAttribute(Qt::WA_StyledBackground);deckHeader_->hide();pagesLayout->addWidget(deckHeader_);
        deckFooter_=new QWidget;deckFooter_->setObjectName("deckFooter");deckFooter_->setAttribute(Qt::WA_StyledBackground);deckFooter_->hide();
        pagesLayout->addWidget(openDocumentsBar_);pagesLayout->addWidget(modulePages_,1);pagesLayout->addWidget(deckFooter_);openDocumentsBar_->hide();
        connect(openDocuments_,&QTabBar::currentChanged,this,[this](int index){showOpenDocument(index);});
        connect(openDocuments_,&QTabBar::tabCloseRequested,this,[this](int index){if(auto window=openDocumentWindows_.value(index))window->close();});
        connect(openDocuments_,&QTabBar::tabBarDoubleClicked,this,[this](int index){if(index>=0)selectModule("hex");});
        connect(stripHex,&QToolButton::clicked,this,[this]{selectModule("hex");});
        layout->addWidget(rail);layout->addWidget(pages,1);setCentralWidget(body);
        // An explicit minimum keeps a panel's size hint from enlarging (and un-maximizing) the window.
        setMinimumSize(720,480);
        const QStringList ids{"hex","3d","uv","reconstruction","scripting","tools"},names{"Hex Viewer","3D","UV / 2D","Reconstruction","Scripting","Other Tools"};
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
            moduleHosts_.insert(ids[i],host);modulePages_->addWidget(slot);moduleTabs_->addTab(workbenchIcon(QHash<QString,QString>{{"hex","hex"},{"3d","cube"},{"uv","image"},{"reconstruction","film"},{"scripting","code"},{"tools","tools"}}.value(ids[i])),names[i]);moduleTabs_->setTabData(i,ids[i]);moduleTabs_->setTabToolTip(i,names[i]);
        }
        connect(moduleTabs_,&QTabBar::currentChanged,this,[this](int index){selectModule(moduleTabs_->tabData(index).toString());});
        {// Terminal status line (interface modes, e.g. Hackerman CRT): "[9XD] 1:hex 2:3d ..." replaces the module rail.
            auto *line=new QHBoxLayout(statusLine_);line->setContentsMargins(6,0,6,0);line->setSpacing(0);auto *brand=new QLabel("[9XD]");brand->setObjectName("statusLineBrand");line->addWidget(brand);
            const QStringList shortNames{"hex","3d","2d","recon","script","tools"};
            for(int i=0;i<ids.size();++i){auto *button=new QToolButton;button->setObjectName("statusLineModule_"+ids[i]);button->setText(QString("%1:%2").arg(i+1).arg(shortNames.value(i,ids[i])));button->setCheckable(true);button->setProperty("moduleId",ids[i]);line->addWidget(button);connect(button,&QToolButton::clicked,this,[this,id=ids[i]]{selectModule(id);refreshStatusLine();});
                auto *shortcut=new QShortcut(QKeySequence(Qt::ALT|Qt::Key(Qt::Key_1+i)),this);shortcut->setEnabled(false);connect(shortcut,&QShortcut::activated,this,[this,id=ids[i]]{selectModule(id);});statusShortcuts_.append(shortcut);}
            line->addStretch();statusInfo_=new QLabel;statusInfo_->setObjectName("statusLineInfo");line->addWidget(statusInfo_);
            auto *search=new QToolButton;search->setObjectName("statusLineSearch");search->setText("ctrl+k");connect(search,&QToolButton::clicked,this,[this]{showCommandPalette();});line->addWidget(search);
            auto *gear=new QToolButton;gear->setObjectName("statusLineSettings");gear->setText("settings");connect(gear,&QToolButton::clicked,this,[this]{showSettings();});line->addWidget(gear);
        }
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
        themeMenu_=new QMenu("Theme",this);rebuildThemeMenu();connect(themeMenu_,&QMenu::aboutToShow,this,[this]{rebuildThemeMenu();});
        connect(mdi_,&QMdiArea::subWindowActivated,this,[this](QMdiSubWindow *window){
            // Hiding the Hex Viewer page deactivates MDI focus, not its document.
            if(!window&&!mdi_->subWindowList().isEmpty())return;
            activate(dynamic_cast<DocumentSubWindow *>(window));refreshStatusLine();
        });
        buildInterfaceShell(ids);
        auto settings=workspaceSettings();loadWorkbenchPreferences(settings);applyInterfaceMode();
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
        // Picking a theme while a style is active means "colour this style with my theme".
        applyWorkbenchTheme(name);if(!statePath_.isEmpty())workspaceSettings().setValue("theme",name);
        checkThemeMenu();if(modeDock_)modeDock_->update();
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
            checkThemeMenu();applyInterfaceMode();
        },this);
        dialog.exec();
    }
    void selectModule(const QString &id,bool focusWindow=true){
        if(switchingDocuments_||!moduleHosts_.contains(id))return;
        const int index=modulePages_->indexOf(moduleSlots_[id]);
        modulePages_->setCurrentIndex(index);{QSignalBlocker blocker(moduleTabs_);moduleTabs_->setCurrentIndex(index);}
        if(focusWindow&&isModuleDetached(id)){auto *window=detachedWindows_[id];window->show();window->raise();window->activateWindow();}
        refreshOpenDocumentsVisibility();
        if(auto *doc=activeDocument())for(auto *dock:doc->sessionDocks())if(dock->isFloating()){
            if(dock->property("moduleId").toString()!=id&&!isModuleDetached(dock->property("moduleId").toString())){if(!dock->isHidden()){dock->setProperty("floatingVisible",true);suspendedFloating_.insert(dock->objectName());}dock->hide();}
            else if(dock->property("floatingVisible").toBool()){dock->setProperty("floatingVisible",false);suspendedFloating_.remove(dock->objectName());dock->show();}
        }
        refreshStatusLine();
    }
    // Interface modes (interface_modes.h) swap shell parts; Classic shows the module rail and no status line.
    void applyInterfaceMode(){
        const auto chrome=InterfaceModes::chrome();const auto layout=chrome.value("layout").toString();const bool line=chrome.value("statusLine").toBool();
        const bool deck=layout=="command-deck",hud=layout=="floating-hud",dock=hud&&chrome.value("modeDock",true).toBool();
        if(statusLine_)statusLine_->setVisible(line);if(rail_)rail_->setVisible(!line&&!deck&&!dock);for(auto *shortcut:statusShortcuts_)shortcut->setEnabled(line);
        if(deckHeader_)deckHeader_->setVisible(deck);if(deckFooter_)deckFooter_->setVisible(deck&&chrome.value("timeline",true).toBool());
        if(deckTimer_){if(deck)deckTimer_->start();else deckTimer_->stop();}
        refreshStatusLine();applyPanelLayout();placeModeDock();if(deck)refreshDeck();
    }
    // Panel layouts (interface/layout). Each layout keeps its own dock arrangement per module: Classic in
    // moduleLayouts/v1/<module> as before, others in interface/layouts/<layout>/<module>. Switching saves the outgoing
    // arrangement and restores (or builds) the incoming one, so Classic always comes back as it was.
    QString panelLayout()const{return panelLayout_;}
    static QString layoutKey(const QString &layout,const QString &module){return layout=="classic"?"moduleLayouts/v1/"+module:"interface/layouts/"+layout+"/"+module;}
    void applyPanelLayout(){
        QString wanted=qApp->property("interfaceLayout").toString();if(wanted.isEmpty())wanted="classic";
        if(!active_||switchingDocuments_)return;// No panels yet: the first activation applies it.
        if(wanted=="floating-hud"&&wanted!=panelLayout_&&!isVisible())return;// Cards are placed over the shown window (showEvent).
        if(wanted==panelLayout_){if(wanted!="classic"){arrangePanels({});for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it)sharedLayouts_[it.key()]=it.value()->saveState();}return;}
        for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it)sharedLayouts_[it.key()]=it.value()->saveState();
        if(!statePath_.isEmpty()){auto settings=workspaceSettings();for(auto it=sharedLayouts_.cbegin();it!=sharedLayouts_.cend();++it)settings.setValue(layoutKey(panelLayout_,it.key()),it.value());}
        layoutStates_[panelLayout_]=sharedLayouts_;
        if(panelLayout_=="classic")for(auto *host:moduleHosts_)InterfaceModes::DockStack::instance()->captureOrder(host);
        clearPanels();
        panelLayout_=wanted;auto next=layoutStates_.value(wanted);
        if(!statePath_.isEmpty()){auto settings=workspaceSettings();for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it)if(!next.contains(it.key())){const auto saved=settings.value(layoutKey(wanted,it.key())).toByteArray();if(!saved.isEmpty())next[it.key()]=saved;}}
        QSet<QString> build;// Modules without a saved arrangement for this layout are laid out from the Classic one.
        for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it){
            const auto state=next.value(it.key(),wanted=="classic"?standardModuleLayout(it.key()):QByteArray());
            if(!state.isEmpty())it.value()->restoreState(state);else build.insert(it.key());
        }
        if(wanted!="classic")arrangePanels(build);
        for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it)sharedLayouts_[it.key()]=it.value()->saveState();
        selectModule(moduleTabs_->tabData(moduleTabs_->currentIndex()).toString(),false);
    }
    QRect workArea()const{return QRect(modulePages_->mapToGlobal(QPoint(0,0)),modulePages_->size());}
    // build: the modules whose panels are laid out afresh; the others keep the user's arrangement.
    void arrangePanels(const QSet<QString> &build){
        for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it){
            if(InterfaceModes::stackLayout(panelLayout_))InterfaceModes::DockStack::instance()->arrange(it.value(),build.contains(it.key()));
            else if(panelLayout_=="floating-hud"){InterfaceModes::HudCards::instance()->setFrame(workArea());InterfaceModes::HudCards::instance()->arrange(it.value(),build.contains(it.key()));}
        }
    }
    void clearPanels(){for(auto *host:moduleHosts_){InterfaceModes::DockStack::instance()->clear(host);InterfaceModes::HudCards::instance()->clear(host);}}
    void refreshStatusLine(){
        {// Command Deck tabs, HUD mode dock and the open-file chip follow the current module and document.
            const auto current=moduleTabs_->tabData(moduleTabs_->currentIndex()).toString();
            for(auto *owner:{static_cast<QWidget *>(deckHeader_),static_cast<QWidget *>(modeDock_)})if(owner)for(auto *button:owner->findChildren<QToolButton *>())if(button->property("moduleId").isValid())button->setChecked(button->property("moduleId").toString()==current);
            if(deckDocument_){QString name=active_?active_->windowTitle():QString("No file");deckDocument_->setText(name+"  ");}
        }
        if(!statusLine_||statusLine_->isHidden())return;const auto current=moduleTabs_->tabData(moduleTabs_->currentIndex()).toString();
        for(auto *button:statusLine_->findChildren<QToolButton *>())if(button->property("moduleId").isValid())button->setChecked(button->property("moduleId").toString()==current);
        QString info;if(auto *window=mdi_->activeSubWindow())info=window->windowTitle();if(auto *doc=activeDocument();doc&&!info.isEmpty()&&doc->documentView()->fileSize()>=0)info+=QString(" \u00b7 %L1 B").arg(doc->documentView()->fileSize());statusInfo_->setText(info);
    }
    // Writes the current configuration (theme, interface mode, panel arrangements, window, toolbar) to
    // release-defaults.json next to the executable, for embedding in release_defaults.h.
    void exportReleaseDefaults(){
        if(statePath_.isEmpty())return;
        for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it)sharedLayouts_[it.key()]=it.value()->saveState();
        auto settings=workspaceSettings();for(auto it=sharedLayouts_.cbegin();it!=sharedLayouts_.cend();++it)settings.setValue(layoutKey(panelLayout_,it.key()),it.value());
        settings.setValue("geometryV2",saveGeometry());settings.setValue("toolbars/mainToolbar",!bar_->isHidden());settings.sync();
        const QString path=QDir(QCoreApplication::applicationDirPath()).filePath("release-defaults.json");QSaveFile file(path);const auto bytes=QJsonDocument(ReleaseDefaults::capture(settings)).toJson(QJsonDocument::Indented);
        if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){QMessageBox::warning(this,"Release defaults","Could not write "+path);return;}
        QMessageBox::information(this,"Release defaults","The current layout and settings were saved to:\n"+path+"\n\nEmbed them in src/release_defaults.h to make them the defaults of a release build.");
    }
    void resetPanelLayout(){
        const auto detached=detachedWindows_.keys();for(const auto &id:detached)attachModule(id);
        clearPanels();
        suspendedFloating_.clear();for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it){auto state=standardModuleLayout(it.key());if(!state.isEmpty()){sharedLayouts_[it.key()]=state;it.value()->restoreState(state);}}
        if(panelLayout_!="classic"){QSet<QString> all;for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it)all.insert(it.key());arrangePanels(all);for(auto it=moduleHosts_.cbegin();it!=moduleHosts_.cend();++it)sharedLayouts_[it.key()]=it.value()->saveState();selectModule(moduleTabs_->tabData(moduleTabs_->currentIndex()).toString(),false);}
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
        auto *window=new ModuleWindow;window->setObjectName("moduleWindow_"+id);window->setMinimumSize(560,380);window->resize(1100,750);
        window->returnRequested=[this,id]{attachModule(id);};detachedWindows_.insert(id,window);
        auto *host=moduleHosts_[id];moduleSlots_[id]->layout()->removeWidget(host);window->setCentralWidget(host);host->setWindowFlags(Qt::Widget);host->show();moduleNotices_[id]->show();
        if(!detachedGeometry_.value(id).isEmpty())window->restoreGeometry(detachedGeometry_[id]);
        if(!position.isNull())window->move(position-QPoint(30,15));
        keepModuleWindowOnScreen(window);moduleWindowActions_[id]->setText("Return "+moduleNames_[id]+" to Main Window");
        refreshDetachedWindows();window->show();window->raise();refreshOpenDocumentsVisibility();
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
    // Floating HUD: cards and the mode dock follow the window.
    void moveEvent(QMoveEvent *event)override{QMainWindow::moveEvent(event);if(panelLayout_=="floating-hud"&&!event->oldPos().isNull())InterfaceModes::HudCards::instance()->follow(event->pos()-event->oldPos());placeModeDock();}
    void resizeEvent(QResizeEvent *event)override{QMainWindow::resizeEvent(event);if(panelLayout_=="floating-hud")InterfaceModes::HudCards::instance()->setFrame(workArea());placeModeDock();}
    void changeEvent(QEvent *event)override{QMainWindow::changeEvent(event);if(event->type()==QEvent::WindowStateChange)QTimer::singleShot(0,this,[this]{placeModeDock();});}
    void showEvent(QShowEvent *event)override{QMainWindow::showEvent(event);QTimer::singleShot(0,this,[this]{if(panelLayout_=="floating-hud")InterfaceModes::HudCards::instance()->setFrame(workArea());else applyPanelLayout();placeModeDock();});}
    void hideEvent(QHideEvent *event)override{QMainWindow::hideEvent(event);if(modeDock_)modeDock_->hide();}
    void closeEvent(QCloseEvent *event)override{
        if(modeDock_)modeDock_->hide();
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
            for(auto it=sharedLayouts_.cbegin();it!=sharedLayouts_.cend();++it)settings.setValue(layoutKey(panelLayout_,it.key()),it.value());
            settings.setValue("moduleLayouts/v1/suspendedFloating",QStringList(suspendedFloating_.cbegin(),suspendedFloating_.cend()));
            settings.setValue("moduleWindows/v1/detached",detachedWindows_.keys());
            for(auto it=detachedWindows_.cbegin();it!=detachedWindows_.cend();++it)detachedGeometry_[it.key()]=it.value()->saveGeometry();
            for(auto it=detachedGeometry_.cbegin();it!=detachedGeometry_.cend();++it)settings.setValue("moduleWindows/v1/"+it.key()+"/geometry",it.value());
        }for(auto *window:detachedWindows_)window->hide();event->accept();
    }
private:
    void rebuildThemeMenu(){
        themeMenu_->clear();
        auto add=[this](const QString &id,const QString &name){auto *action=themeMenu_->addAction(name);action->setObjectName(QString(id).remove(':')+"ThemeAction");action->setData(id);action->setCheckable(true);connect(action,&QAction::triggered,this,[this,id]{setTheme(id);});};
        // Dark is the product default appearance (id "default"); the older plain dark and System entries are not offered.
        add("default","Dark");add("light","Light");
        themeMenu_->addSection("Included");for(const auto &key:includedThemeOrder())add(key,includedThemes()[key].toObject()["name"].toString(key));
        const auto library=QJsonDocument::fromJson(qApp->property("workbenchThemeLibrary").toByteArray()).object();
        if(!library.isEmpty()){themeMenu_->addSection("My themes");for(auto it=library.begin();it!=library.end();++it)add(it.key(),it.value().toObject()["name"].toString(it.key()));}
        checkThemeMenu();
    }
    void checkThemeMenu(){auto current=qApp->property("workbenchTheme").toString();if(current=="dark"||current=="system")current="default";for(auto *action:themeMenu_->actions())if(action->isCheckable())action->setChecked(action->data().toString()==current);}
    // Command Deck header and activity strip, and the Floating HUD mode dock. Hidden unless their layout is active.
    void buildInterfaceShell(const QStringList &ids){
        static const QHash<QString,QString> icons{{"hex","hex"},{"3d","cube"},{"uv","image"},{"reconstruction","film"},{"scripting","code"},{"tools","tools"}};
        auto *row=new QHBoxLayout(deckHeader_);row->setContentsMargins(12,7,12,7);row->setSpacing(12);
        auto *logo=new QLabel;logo->setObjectName("deckLogo");
        {const qreal ratio=qMax<qreal>(2,qApp->devicePixelRatio());QPixmap image(":/branding/logo-sticker.png");if(!image.isNull()){image=image.scaled(QSize(44,28)*ratio,Qt::KeepAspectRatio,Qt::SmoothTransformation);image.setDevicePixelRatio(ratio);logo->setPixmap(image);}else logo->setText("9XD");}
        row->addWidget(logo);
        auto *modes=new QWidget;modes->setObjectName("deckModes");modes->setAttribute(Qt::WA_StyledBackground);auto *modesRow=new QHBoxLayout(modes);modesRow->setContentsMargins(3,3,3,3);modesRow->setSpacing(2);
        for(const auto &id:ids){
            auto *button=new QToolButton;button->setObjectName("deckModule_"+id);button->setProperty("moduleId",id);button->setProperty("baseText",moduleNames_.value(id));button->setCheckable(true);
            button->setIcon(workbenchIcon(icons.value(id)));button->setIconSize({16,16});button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);button->setText(moduleNames_.value(id));
            modesRow->addWidget(button);connect(button,&QToolButton::clicked,this,[this,id]{selectModule(id);refreshStatusLine();});
        }
        row->addWidget(modes);row->addStretch(1);
        auto *search=new QPushButton(workbenchIcon("search"),"Search or run a command\u2026          Ctrl K");search->setObjectName("deckSearch");search->setMinimumWidth(340);search->setCursor(Qt::PointingHandCursor);
        connect(search,&QPushButton::clicked,this,[this]{showCommandPalette();});row->addWidget(search);row->addStretch(1);
        deckDocument_=new QToolButton;deckDocument_->setObjectName("deckDocument");deckDocument_->setPopupMode(QToolButton::InstantPopup);deckDocument_->setMenu(listMenu_);deckDocument_->setToolButtonStyle(Qt::ToolButtonTextOnly);row->addWidget(deckDocument_);
        auto *gear=new QToolButton;gear->setObjectName("deckSettings");gear->setIcon(workbenchIcon("gear"));gear->setIconSize({18,18});gear->setToolTip("Settings");connect(gear,&QToolButton::clicked,this,[this]{showSettings();});row->addWidget(gear);
        auto *foot=new QHBoxLayout(deckFooter_);foot->setContentsMargins(12,5,12,5);foot->setSpacing(8);
        auto *caption=new QLabel("ACTIVITY");caption->setObjectName("deckFooterCaption");foot->addWidget(caption);
        deckChips_=new QHBoxLayout;deckChips_->setSpacing(8);foot->addLayout(deckChips_);
        deckProgress_=new QWidget;deckProgress_->setObjectName("deckChip");deckProgress_->setAttribute(Qt::WA_StyledBackground);auto *progressRow=new QHBoxLayout(deckProgress_);progressRow->setContentsMargins(10,3,10,3);progressRow->setSpacing(8);
        deckProgressBar_=new QProgressBar;deckProgressBar_->setObjectName("deckProgressBar");deckProgressBar_->setTextVisible(false);deckProgressBar_->setFixedSize(70,8);progressRow->addWidget(deckProgressBar_);
        deckProgressLabel_=new QLabel;progressRow->addWidget(deckProgressLabel_);foot->addWidget(deckProgress_);deckProgress_->hide();
        foot->addStretch(1);deckThreads_=new QLabel;deckThreads_->setObjectName("deckChip");foot->addWidget(deckThreads_);
        for(auto *host:moduleHosts_)if(auto *bar=host->findChild<QStatusBar *>(QString(),Qt::FindDirectChildrenOnly))connect(bar,&QStatusBar::messageChanged,this,[this](const QString &message){if(!message.trimmed().isEmpty())pushActivity(message.trimmed());});
        deckTimer_=new QTimer(this);deckTimer_->setInterval(800);connect(deckTimer_,&QTimer::timeout,this,[this]{refreshDeck();});
        modeDock_=new HudModeDock(this);
        const QStringList shortNames{"Hex","3D","2D","Recon","Script","Tools"};
        for(int i=0;i<ids.size();++i){
            auto *button=new QToolButton;button->setObjectName("hudDock_"+ids[i]);button->setProperty("moduleId",ids[i]);button->setCheckable(true);button->setIcon(workbenchIcon(icons.value(ids[i])));button->setIconSize({22,22});
            button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);button->setText(shortNames.value(i));button->setToolTip(moduleNames_.value(ids[i]));button->setMinimumWidth(54);
            modeDock_->row->addWidget(button);connect(button,&QToolButton::clicked,this,[this,id=ids[i]]{selectModule(id);refreshStatusLine();});
        }
        auto *divider=new QFrame;divider->setObjectName("hudDockDivider");divider->setFrameShape(QFrame::VLine);modeDock_->row->addWidget(divider);
        for(auto pair:{std::pair{QString("search"),QString("Search")},{QString("gear"),QString("Settings")}}){
            auto *button=new QToolButton;button->setObjectName("hudDock_"+pair.first);button->setIcon(workbenchIcon(pair.first));button->setIconSize({22,22});button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);button->setText(pair.second);button->setMinimumWidth(54);
            modeDock_->row->addWidget(button);connect(button,&QToolButton::clicked,this,[this,search=pair.first=="search"]{if(search)showCommandPalette();else showSettings();});
        }
        for(auto *button:modeDock_->findChildren<QToolButton *>())button->setCursor(Qt::ArrowCursor);
        // Moving the dock stores where it sits in the work area (centre and bottom, as fractions); double-click resets.
        modeDock_->moved=[this]{const QRect area=workArea();if(area.isEmpty())return;const QRect frame=modeDock_->frameGeometry();
            dockAnchor_=QPointF(double(frame.center().x()-area.left())/area.width(),double(frame.bottom()-area.top())/area.height());if(!statePath_.isEmpty())workspaceSettings().setValue("interface/floating-hud/dockAnchor",dockAnchor_);};
        modeDock_->reset=[this]{dockAnchor_=QPointF(-1,-1);if(!statePath_.isEmpty())workspaceSettings().remove("interface/floating-hud/dockAnchor");placeModeDock();};
        modeDock_->hovered=[this](bool inside){dockHovered_=inside;if(inside)dockLastSeen_.restart();QTimer::singleShot(inside?0:450,this,[this]{placeModeDock();});};
        if(!statePath_.isEmpty())dockAnchor_=workspaceSettings().value("interface/floating-hud/dockAnchor",QPointF(-1,-1)).toPointF();
        // Auto-hide: a hidden window gets no hover events, so the pointer is watched while the HUD is active.
        dockWatch_=new QTimer(this);dockWatch_->setInterval(150);connect(dockWatch_,&QTimer::timeout,this,[this]{watchModeDock();});dockLastSeen_.start();
        modeDock_->hide();
    }
    // Floating HUD mode dock: placed at its saved spot (or bottom centre); optionally folded to the active module until
    // the pointer is over it (dockCollapse) and hidden until the pointer comes near (dockAutoHide).
    bool modeDockActive()const{const auto chrome=InterfaceModes::chrome();return chrome.value("layout").toString()=="floating-hud"&&chrome.value("modeDock",true).toBool()&&isVisible()&&!isMinimized();}
    void placeModeDock(){
        if(!modeDock_)return;const auto chrome=InterfaceModes::chrome();
        if(!modeDockActive()){modeDock_->hide();if(dockWatch_)dockWatch_->stop();return;}
        if(dockWatch_&&!dockWatch_->isActive())dockWatch_->start();
        const bool collapsed=chrome.value("dockCollapse",false).toBool()&&!dockHovered_;
        for(auto *button:modeDock_->findChildren<QToolButton *>()){const bool module=button->property("moduleId").isValid();button->setVisible(!collapsed||(module&&button->isChecked()));}
        if(auto *divider=modeDock_->findChild<QFrame *>("hudDockDivider"))divider->setVisible(!collapsed);
        modeDock_->adjustSize();modeDock_->resize(modeDock_->sizeHint());const QRect area=workArea();
        const bool custom=dockAnchor_.x()>=0&&dockAnchor_.y()>=0;
        const int centre=custom?area.left()+int(dockAnchor_.x()*area.width()):area.center().x(),bottom=custom?area.top()+int(dockAnchor_.y()*area.height()):area.bottom()-14;
        const int x=qBound(area.left(),centre-modeDock_->width()/2,qMax(area.left(),area.right()-modeDock_->width())),y=qBound(area.top(),bottom-modeDock_->height(),qMax(area.top(),area.bottom()-modeDock_->height()));
        modeDock_->move(x,y);
        if(chrome.value("dockAutoHide",false).toBool()&&!dockHovered_&&dockLastSeen_.elapsed()>900){modeDock_->hide();return;}
        if(modeDock_->isHidden())modeDock_->show();modeDock_->raise();
    }
    void watchModeDock(){
        if(!modeDock_||!modeDockActive()){if(dockWatch_)dockWatch_->stop();return;}
        if(!InterfaceModes::chrome().value("dockAutoHide",false).toBool())return;
        const bool nearby=modeDock_->frameGeometry().adjusted(-40,-40,40,40).contains(QCursor::pos());
        if(nearby||dockHovered_){dockLastSeen_.restart();if(modeDock_->isHidden()){modeDock_->show();modeDock_->raise();}}
        else if(!modeDock_->isHidden()&&dockLastSeen_.elapsed()>900)modeDock_->hide();
    }
    void pushActivity(const QString &text,const QString &mark={}){
        if(!deckChips_)return;auto *chip=new QLabel;chip->setObjectName("deckChip");
        QString shown=text;if(shown.size()>64)shown=shown.left(61)+"\u2026";chip->setText((mark.isEmpty()?QString():mark+"  ")+shown+"  <span style='color:gray'>"+QTime::currentTime().toString("HH:mm")+"</span>");chip->setToolTip(text);
        deckChips_->addWidget(chip);while(deckChips_->count()>4){auto *item=deckChips_->takeAt(0);if(item->widget())item->widget()->deleteLater();delete item;}
    }
    static QString dockTitleOf(QWidget *widget){for(auto *w=widget;w;w=w->parentWidget())if(auto *dock=qobject_cast<QDockWidget *>(w))return InterfaceModes::Chrome::plainTitle(dock);return widget->objectName();}
    void refreshDeck(){
        if(panelLayout_!="command-deck")return;
        auto *doc=activeDocument();
        if(auto *host=moduleHosts_.value(moduleTabs_->tabData(moduleTabs_->currentIndex()).toString()))InterfaceModes::DockStack::instance()->refreshSummaries(host);
        // Running work: the first busy progress bar anywhere in the modules; when it finishes it becomes an activity chip.
        QProgressBar *busy=nullptr;QSet<QProgressBar *> running;
        for(auto *host:moduleHosts_)for(auto *bar:host->findChildren<QProgressBar *>()){if(!bar->isVisible())continue;const bool active=bar->maximum()==0||(bar->value()>bar->minimum()&&bar->value()<bar->maximum());if(!active)continue;running.insert(bar);if(!busy)busy=bar;if(!busySince_.contains(bar))busySince_.insert(bar,{QPointer<QProgressBar>(bar),QDateTime::currentMSecsSinceEpoch(),dockTitleOf(bar)});}
        for(auto it=busySince_.begin();it!=busySince_.end();){
            if(running.contains(it.key())){++it;continue;}
            const double seconds=(QDateTime::currentMSecsSinceEpoch()-it->started)/1000.0;pushActivity(it->title+QString("  %1 s").arg(seconds,0,'f',1),QString(QChar(0x2713)));it=busySince_.erase(it);
        }
        deckProgress_->setVisible(busy);
        if(busy){deckProgressBar_->setRange(busy->minimum(),busy->maximum());deckProgressBar_->setValue(busy->value());QString label=dockTitleOf(busy);const auto text=busy->text();if(!text.isEmpty()&&busy->isTextVisible())label+="  "+text;else if(busy->maximum()>0)label+=QString("  %1 / %2").arg(busy->value()).arg(busy->maximum());deckProgressLabel_->setText(label);}
        QString threads;if(auto *button=moduleHosts_.value("hex")->findChild<QToolButton *>("threadActivityToggle"))threads=button->text();deckThreads_->setText(threads);deckThreads_->setVisible(!threads.isEmpty()&&moduleTabs_->tabData(moduleTabs_->currentIndex()).toString()!="hex");
        refreshStatusLine();
    }
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
        auto *exportDefaults=windows->addAction("Export Layout as Release Default...");exportDefaults->setObjectName("exportReleaseDefaults");connect(exportDefaults,&QAction::triggered,this,[this]{exportReleaseDefaults();});
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
            else if(it.key()=="reconstruction"){
                auto *video=doc->sessionDock("videoReconstructionDock"),*spline=doc->sessionDock("splineModelingDock");
                video->show();spline->show();it.value()->tabifyDockWidget(video,spline);video->raise();
            }
        }
        for(auto *dock:doc->sessionDocks())dock->setProperty("floatingVisible",suspendedFloating_.contains(dock->objectName()));
        switchingDocuments_=false;if(panelLayout_!="classic")arrangePanels({});else applyPanelLayout();selectModule(moduleTabs_->tabData(moduleTabs_->currentIndex()).toString(),false);
        doc->menuBar()->hide();
        for(auto *action:doc->menuBar()->actions()){if(QString(action->text()).remove('&')=="Windows")menuBar()->addMenu(toolbarsMenu_);menuBar()->addAction(action);}
        if(auto *tools=doc->findChild<QToolBar *>("sessionToolbar")){for(auto *action:tools->actions())bar_->addAction(action);tools->hide();}
        refreshTitle(doc);refreshDetachedWindows();
        checkThemeMenu();
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
        refreshOpenDocuments();
    }
    // Mirrors the MDI document tabs (creation order follows tab moves) for modules other than the Hex Viewer.
    void refreshOpenDocuments(){
        if(!openDocuments_)return;
        const auto windows=mdi_->subWindowList(QMdiArea::CreationOrder);
        {
            QSignalBlocker blocker(openDocuments_);openDocumentWindows_.clear();
            while(openDocuments_->count()>windows.size())openDocuments_->removeTab(openDocuments_->count()-1);
            int current=-1;
            for(int i=0;i<windows.size();++i){
                if(i>=openDocuments_->count())openDocuments_->addTab(QString());
                openDocuments_->setTabText(i,windows[i]->windowTitle());openDocuments_->setTabToolTip(i,windows[i]->toolTip());
                openDocumentWindows_.append(windows[i]);if(windows[i]==active_.data())current=i;
            }
            if(current>=0)openDocuments_->setCurrentIndex(current);
        }
        refreshOpenDocumentsVisibility();
    }
    void refreshOpenDocumentsVisibility(){
        if(!openDocumentsBar_)return;
        const auto current=moduleTabs_->tabData(moduleTabs_->currentIndex()).toString();
        openDocumentsBar_->setVisible(openDocuments_->count()>0&&(current!="hex"||isModuleDetached("hex")));
    }
    void showOpenDocument(int index){
        QPointer<DocumentSubWindow> sub=dynamic_cast<DocumentSubWindow *>(openDocumentWindows_.value(index).data());if(!sub)return;
        // The MDI area is hidden here; when the click also activates the window, QMdiArea re-activates its
        // previous document afterwards. Re-apply once the activation events have been processed.
        auto apply=[this,sub]{if(!sub||switchingDocuments_||activeDocument()==sub->document)return;mdi_->setActiveSubWindow(sub);activate(sub);if(activeDocument()!=sub->document)refreshOpenDocuments();};
        apply();QTimer::singleShot(0,this,apply);QTimer::singleShot(120,this,apply);
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
    QWidget *openDocumentsBar_=nullptr;QTabBar *openDocuments_=nullptr;QList<QPointer<QMdiSubWindow>> openDocumentWindows_;
    QWidget *statusLine_=nullptr,*rail_=nullptr;QLabel *statusInfo_=nullptr;QList<QShortcut *> statusShortcuts_;
    QMdiArea *mdi_;QToolBar *bar_;QMenu *toolbarsMenu_,*documentsMenu_,*listMenu_,*themeMenu_,*fallback_=nullptr;
    QPointer<DocumentSubWindow> active_;QPointer<CompactDocumentTabs> documentTabs_;QList<QPointer<DocumentSubWindow>> sessions_;QHash<QString,QByteArray> sharedLayouts_;QString panelLayout_="classic";QHash<QString,QHash<QString,QByteArray>> layoutStates_;
    QWidget *deckHeader_=nullptr,*deckFooter_=nullptr,*deckProgress_=nullptr;QToolButton *deckDocument_=nullptr;QHBoxLayout *deckChips_=nullptr;QProgressBar *deckProgressBar_=nullptr;QLabel *deckProgressLabel_=nullptr,*deckThreads_=nullptr;
    QTimer *deckTimer_=nullptr,*dockWatch_=nullptr;HudModeDock *modeDock_=nullptr;QPointF dockAnchor_{-1,-1};bool dockHovered_=false;QElapsedTimer dockLastSeen_;struct BusyWork {QPointer<QProgressBar> bar;qint64 started;QString title;};QHash<QProgressBar *,BusyWork> busySince_;
    QHash<QString,QMainWindow *> moduleHosts_;QSet<QString> suspendedFloating_;ModuleTabBar *moduleTabs_;
    QHash<QString,QWidget *> moduleSlots_,moduleNotices_;QHash<QString,QString> moduleNames_;
    QHash<QString,ModuleWindow *> detachedWindows_;QList<QPointer<ModuleWindow>> retiredWindows_;QHash<QString,QByteArray> detachedGeometry_;
    QHash<QString,QAction *> moduleWindowActions_;QMenu *moduleWindowsMenu_;QStackedWidget *modulePages_;bool switchingDocuments_=false;QString statePath_;int untitled_=0;UiLayout *workspaceLayout_=nullptr;
    void rebuildDocumentList(){listMenu_->clear();for(auto *window:mdi_->subWindowList()){auto *action=listMenu_->addAction(window->windowTitle());action->setCheckable(true);action->setChecked(window==mdi_->activeSubWindow());connect(action,&QAction::triggered,this,[this,window]{mdi_->setActiveSubWindow(window);});}}
};
