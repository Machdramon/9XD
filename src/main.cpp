#include "about_dialog.h"
#include "file_dialogs.h"
#include "ai_chat_panel.h"
#include "asset_scan_panel.h"
#include "arcp_point_preview.h"
#include "genki_geometry.h"
#include "ai_structure.h"
#include "programmer_calculator.h"
#include "script_panel.h"
#include "recent_files.h"
#include "export_dialog.h"
#include <QDir>
#include <memory>
#include "layout_persistence.h"
#include "vulkan_panel.h"
#include "uv_panel.h"
#include "image_workbench.h"
#include "entropy_panel.h"
#include "checksum_panel.h"
#include "compare_panel.h"
#include "magic_panel.h"
#include "hex_view.h"
#include "format_panel.h"
#include "local_config.h"
#include <QApplication>
#include "theme.h"
#include "settings_dialog.h"
#include <QClipboard>
#include <QActionGroup>
#include <QSpinBox>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QTimer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QTextBrowser>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QToolButton>
#include <QThread>
#include <QScrollArea>

class MainWindow final : public QMainWindow {
public:
    std::function<void(const QString &)> openRequested;
    std::function<void()> newRequested,exitRequested,settingsRequested;
    std::function<void(const QString &)> showModuleRequested;
    std::function<bool(const QString &)> saveDestinationAllowed;
    std::function<QJsonArray()> aiWorkspaceDocuments;
    HexView *documentView()const{return view_;}
    QString untitledName()const{return untitledName_;}
    void setUntitledName(QString name){untitledName_=std::move(name);setWindowTitle(untitledName_+"[*]");}
    QList<QDockWidget *> sessionDocks()const{return sessionDocks_;}
    void registerSessionDock(QDockWidget *dock,Qt::DockWidgetArea area,bool visible=false){sessionDocks_.append(dock);dock->setProperty("initialArea",int(area));dock->setProperty("sessionVisible",visible);dock->hide();}
    QDockWidget *sessionDock(const QString &name)const{for(auto *dock:sessionDocks_)if(dock->objectName()==name)return dock;return nullptr;}
    bool saveDocument(bool saveAs=false){return saveCurrent(saveAs);}
    void openInPlace(const QString &path){if(view_->directEditing()){QTimer::singleShot(30,this,[this,path]{openInPlace(path);});return;}if(view_->saving())return;if(!resolvePending([this,path]{openInPlace(path);}))return;path_=path;view_->openFile(path);}
    ~MainWindow()override{
        view_->restoreStatusFooter();
        disconnect(view_,nullptr,this,nullptr);
        auto docks=sessionDocks_;
        if(auto *script=sessionDock("scriptDock")){docks.removeOne(script);docks.prepend(script);}
        // Delete panels in place: reparenting native graphics containers into
        // their own ancestor document can recurse through Qt's window hierarchy.
        for(auto *dock:docks){
            if(auto *host=qobject_cast<QMainWindow *>(dock->parentWidget()))host->removeDockWidget(dock);
            dock->hide();delete dock;
        }
        sessionDocks_.clear();
    }
    explicit MainWindow(QString uiStatePath={},QString recentPath={}) {
        recentPath_=recentPath.isEmpty()?(uiStatePath.isEmpty()?QString():QFileInfo(uiStatePath).dir().filePath("recent-files.json")):recentPath;
        recent_=std::make_unique<RecentFiles>(recentPath_);
        setWindowTitle("9XD");
        view_=new HexView(this);setCentralWidget(view_);
        menuBar()->setNativeMenuBar(false);
        auto *file=menuBar()->addMenu("File");
        auto *open=file->addAction("Open File…");open->setShortcut(QKeySequence::Open);
        connect(open,&QAction::triggered,this,[this]{const auto path=WorkbenchFiles::getOpenFileName(this,"Open local binary file");if(!path.isEmpty())openFile(path);});
        auto *recentMenu=file->addMenu("Recent Files");recentMenu->setObjectName("recentFilesMenu");
        connect(recentMenu,&QMenu::aboutToShow,this,[this,recentMenu]{
            if(!recentPath_.isEmpty())recent_=std::make_unique<RecentFiles>(recentPath_);
            recentMenu->clear();int index=0;for(const auto &path:recent_->paths()){QFileInfo info(path);QString label=QString("%1 · %2 — %3%4").arg(++index).arg(info.fileName(),info.absolutePath(),info.isFile()?QString():QString(" · unavailable"));label.replace("&","&&");auto *action=recentMenu->addAction(label);action->setData(path);action->setToolTip(path);action->setEnabled(info.isFile());connect(action,&QAction::triggered,this,[this,path]{openFile(path);});}
            if(recent_->paths().isEmpty()){auto *empty=recentMenu->addAction("No recent files");empty->setEnabled(false);}else{recentMenu->addSeparator();auto *clear=recentMenu->addAction("Clear list");clear->setObjectName("clearRecentFiles");connect(clear,&QAction::triggered,this,[this]{if(!recent_->clear())statusBar()->showMessage("Could not save the recent files list.",6000);});}
        });
        auto *reload=file->addAction("Reload");
        connect(reload,&QAction::triggered,this,[this]{if(!path_.isEmpty())openInPlace(path_);});
        auto *save=file->addAction("Save");save->setShortcut(QKeySequence::Save);save->setEnabled(false);
        connect(save,&QAction::triggered,this,[this]{saveCurrent(false);});
        auto *saveAs=file->addAction("Save As…");saveAs->setShortcut(QKeySequence::SaveAs);
        connect(saveAs,&QAction::triggered,this,[this]{saveCurrent(true);});
        file->addSeparator();auto *options=file->addAction("Options…");
        connect(options,&QAction::triggered,this,[this]{showOptions();});
        file->addSeparator();auto *exit=file->addAction("Exit");exit->setShortcut(QKeySequence::Quit);connect(exit,&QAction::triggered,this,[this]{if(exitRequested)exitRequested();else close();});
        auto *edit=menuBar()->addMenu("Edit");
        auto *undo=edit->addAction("Undo");undo->setShortcut(QKeySequence::Undo);
        connect(undo,&QAction::triggered,view_,&HexView::undo);
        auto *redo=edit->addAction("Redo");redo->setShortcut(QKeySequence::Redo);
        connect(redo,&QAction::triggered,view_,&HexView::redo);
        auto *replace=edit->addAction("Overwrite bytes…");replace->setShortcut(Qt::Key_F2);
        connect(replace,&QAction::triggered,view_,&HexView::editSelection);
        auto *cut=edit->addAction("Cut");cut->setEnabled(false);
        cut->setToolTip("Cut selected bytes from a RAM document.");connect(cut,&QAction::triggered,view_,&HexView::cutSelection);
        auto *copy=edit->addAction("Copy hexadecimal selection");copy->setShortcut(QKeySequence::Copy);copy->setEnabled(false);
        connect(copy,&QAction::triggered,view_,&HexView::copySelection);connect(view_,&HexView::selectionAvailable,copy,&QAction::setEnabled);
        auto *paste=edit->addAction("Paste");paste->setShortcut(QKeySequence::Paste);paste->setEnabled(false);paste->setToolTip("Overwrite with clipboard bytes or hexadecimal text.");
        connect(paste,&QAction::triggered,view_,&HexView::pasteSelection);
        auto updateActions=[this,save,saveAs,undo,redo,paste,replace,open,reload,cut]{
            const bool writable=view_->hasFile()&&!view_->saving();
            save->setEnabled(writable&&(view_->dirty()||view_->filePath().isEmpty()));saveAs->setEnabled(writable);
            undo->setEnabled(writable&&view_->dirty());redo->setEnabled(writable&&view_->canRedo());
            paste->setEnabled(writable&&view_->selection().anchor>=0);
            replace->setEnabled(writable&&view_->selection().anchor>=0);
            open->setEnabled(true);reload->setEnabled(writable&&!view_->filePath().isEmpty());cut->setEnabled(writable&&view_->isMemoryDocument()&&view_->selection().count(view_->fileSize())>0);
            setWindowModified(view_->dirty());
        };
        connect(view_,&HexView::editStateChanged,this,updateActions);
        connect(view_,&HexView::selectionAvailable,this,[updateActions](bool){updateActions();});
        updateActions();
        connect(view_,&HexView::saveFinished,this,[this](bool success){
            auto next=std::move(afterSave_);afterSave_={};
            if(success){if(next)QTimer::singleShot(0,this,std::move(next));}
            else QMessageBox::warning(this,"Save incomplete","Could not save. Check the status message below. Your changes are still pending.");
        });
        edit->addSeparator();auto *select=edit->addAction("Select visible page");select->setShortcut(QKeySequence::SelectAll);connect(select,&QAction::triggered,view_,&HexView::selectWholeDocument);
        auto *tools=menuBar()->addMenu("Tools");
        auto *calculator=tools->addAction("Calculator…");connect(calculator,&QAction::triggered,this,[this]{showCalculator();});
        auto *interpret=tools->addAction("Encoding and interpretation");connect(interpret,&QAction::triggered,view_,&HexView::focusInterpretation);
        auto *dock=new QDockWidget("Inspector",this);dock->setObjectName("inspectorDock");dock->setWidget(view_->inspector());view_->inspector()->show();addDockWidget(Qt::RightDockWidgetArea,dock);
        dock->setMinimumWidth(290);
        auto *viewMenu=menuBar()->addMenu("View");viewMenu->addAction(dock->toggleViewAction());
        auto *formatDock=new QDockWidget("Formatting Rules",this);formatDock->setObjectName("formatRulesDock");
        formatPanel_=new FormatPanel(view_,this);auto *formatScroll=new QScrollArea;formatScroll->setWidgetResizable(true);formatScroll->setFrameShape(QFrame::NoFrame);formatScroll->setWidget(formatPanel_);formatDock->setWidget(formatScroll);addDockWidget(Qt::RightDockWidgetArea,formatDock);
        viewMenu->addAction(formatDock->toggleViewAction());
        auto *magicDock=new QDockWidget("Magic Bytes Library",this);magicDock->setObjectName("magicDock");
        magicPanel_=new MagicPanel(view_,formatPanel_,magicDock);magicDock->setWidget(magicPanel_);addDockWidget(Qt::BottomDockWidgetArea,magicDock);magicDock->hide();
        viewMenu->addAction(magicDock->toggleViewAction());auto *magicAction=tools->addAction("Magic Bytes Library");connect(magicAction,&QAction::triggered,this,[magicDock]{magicDock->show();magicDock->raise();});
        auto *entropyDock=new QDockWidget("Entropy",this);entropyDock->setObjectName("entropyDock");entropyDock->setWidget(new EntropyPanel(view_,entropyDock));addDockWidget(Qt::RightDockWidgetArea,entropyDock);splitDockWidget(dock,entropyDock,Qt::Vertical);tabifyDockWidget(dock,formatDock);dock->raise();
        viewMenu->addAction(entropyDock->toggleViewAction());auto *entropyAction=tools->addAction("Entropy Analyzer");connect(entropyAction,&QAction::triggered,this,[entropyDock]{entropyDock->show();entropyDock->raise();});
        auto *checksumDock=new QDockWidget("Checksum",this);checksumDock->setObjectName("checksumDock");
        auto *checksumScroll=new QScrollArea;checksumScroll->setWidgetResizable(true);checksumScroll->setFrameShape(QFrame::NoFrame);
        checksumScroll->setWidget(new ChecksumPanel(view_,checksumScroll));checksumDock->setWidget(checksumScroll);addDockWidget(Qt::RightDockWidgetArea,checksumDock);checksumDock->hide();
        checksumDock->toggleViewAction()->setObjectName("checksumAction");viewMenu->addAction(checksumDock->toggleViewAction());
        auto *compareDock=new QDockWidget("Compare Data",this);compareDock->setObjectName("compareDock");
        auto *comparePanel=new ComparePanel(view_,compareDock);compareDock->setWidget(comparePanel);addDockWidget(Qt::RightDockWidgetArea,compareDock);compareDock->hide();
        auto *vulkanDock=new QDockWidget("3D View · Vulkan",this);vulkanDock->setObjectName("vulkanDock");auto *vulkanPanel=new VulkanPanel(vulkanDock);vulkanDock->setWidget(vulkanPanel);addDockWidget(Qt::BottomDockWidgetArea,vulkanDock);vulkanDock->hide();viewMenu->addAction(vulkanDock->toggleViewAction());
        auto *exportAction=new QAction("Export 3D Model…",this);exportAction->setObjectName("exportModelAction");file->insertAction(options,exportAction);file->insertSeparator(options);exportAction->setEnabled(false);
        connect(vulkanPanel->scene(),&MeshScene::collectionChanged,this,[vulkanPanel,exportAction]{exportAction->setEnabled(!vulkanPanel->scene()->entries().isEmpty());});
        connect(exportAction,&QAction::triggered,this,[this,vulkanPanel]{ExportDialog dialog(vulkanPanel->scene(),path_,vulkanPanel->visualStyle().face,this);dialog.exec();});
        auto *meshDock=new QDockWidget("3D Settings",this);meshDock->setObjectName("meshControlsDock");meshDock->setWidget(vulkanPanel->createMeshControls(view_));addDockWidget(Qt::LeftDockWidgetArea,meshDock);meshDock->hide();viewMenu->addAction(meshDock->toggleViewAction());auto *meshAction=tools->addAction("3D Mesh Settings");connect(meshAction,&QAction::triggered,this,[meshDock]{meshDock->show();meshDock->raise();});
        auto *vulkanAction=tools->addAction("3D View · Vulkan");connect(vulkanAction,&QAction::triggered,this,[vulkanDock]{vulkanDock->show();vulkanDock->raise();});
        auto *collectionDock=new QDockWidget("Mesh Collection",this);collectionDock->setObjectName("meshCollectionDock");collectionDock->setWidget(vulkanPanel->createMeshCollection());addDockWidget(Qt::LeftDockWidgetArea,collectionDock);collectionDock->hide();viewMenu->addAction(collectionDock->toggleViewAction());
        auto *uvDock=new QDockWidget("UV View",this);uvDock->setObjectName("uvDock");auto *imageTools=new ImageWorkbench(view_,uvDock);auto *views=new QTabWidget;views->setObjectName("uvImageTabs");views->addTab(imageTools->preview,"Image Preview");views->addTab(new UvPanel(vulkanPanel,views),"UV Layout");uvDock->setWidget(views);addDockWidget(Qt::BottomDockWidgetArea,uvDock);uvDock->hide();viewMenu->addAction(uvDock->toggleViewAction());auto *uvAction=tools->addAction("UV View");connect(uvAction,&QAction::triggered,this,[uvDock]{uvDock->show();uvDock->raise();});
        auto *imageLibraryDock=new QDockWidget("Texture / Image Collection",this);imageLibraryDock->setObjectName("imageLibraryDock");imageLibraryDock->setWidget(imageTools->collection);addDockWidget(Qt::LeftDockWidgetArea,imageLibraryDock);imageLibraryDock->hide();viewMenu->addAction(imageLibraryDock->toggleViewAction());
        auto *imageCalculatorDock=new QDockWidget("2D Settings / Image Calculator",this);imageCalculatorDock->setObjectName("imageCalculatorDock");auto *imageScroll=new QScrollArea;imageScroll->setWidgetResizable(true);imageScroll->setWidget(imageTools->settings);imageCalculatorDock->setWidget(imageScroll);addDockWidget(Qt::RightDockWidgetArea,imageCalculatorDock);imageCalculatorDock->hide();viewMenu->addAction(imageCalculatorDock->toggleViewAction());
        imageTools->assignTexture=[vulkanPanel,views,uvDock](const QImage &image,const QString &name){bool ok=vulkanPanel->scene()->setTexture(vulkanPanel->scene()->active(),image,name);if(ok&&!image.isNull()){views->setCurrentIndex(1);uvDock->show();uvDock->raise();}return ok;};
        imageTools->imageSelected=[views,uvDock](const QImage &){views->setCurrentIndex(0);uvDock->show();uvDock->raise();};
        auto *scriptDock=new QDockWidget("Scripting · Python / C++",this);scriptDock->setObjectName("scriptDock");scriptPanel_=new ScriptPanel(view_,vulkanPanel,scriptDock);scriptDock->setWidget(scriptPanel_);addDockWidget(Qt::BottomDockWidgetArea,scriptDock);scriptDock->hide();viewMenu->addAction(scriptDock->toggleViewAction());auto *scriptAction=tools->addAction("Scripting · Python / C++");connect(scriptAction,&QAction::triggered,this,[scriptDock]{scriptDock->show();scriptDock->raise();});
        auto *librariesDock=new QDockWidget("Library Manager",this);librariesDock->setObjectName("scriptLibraryDock");librariesDock->setWidget(scriptPanel_->libraryManager());librariesDock->widget()->show();addDockWidget(Qt::RightDockWidgetArea,librariesDock);librariesDock->hide();connect(librariesDock,&QDockWidget::visibilityChanged,this,[this](bool visible){if(visible)scriptPanel_->refreshLibraries();});
        auto *aiDock=new QDockWidget("AI Analysis · Ollama",this);aiDock->setObjectName("aiChatDock");aiPanel_=new AiChatPanel(view_,vulkanPanel->scene(),aiDock);aiDock->setWidget(aiPanel_);addDockWidget(Qt::RightDockWidgetArea,aiDock);aiDock->hide();viewMenu->addAction(aiDock->toggleViewAction());
        aiPanel_->fillImageSettings=[imageTools](const QJsonObject &args){return imageTools->applyAssistantSetup(args);};
        aiPanel_->annotateStructure=[this,formatDock](const QJsonObject &args,bool metadata){auto result=AiStructure::apply(view_,formatPanel_,args,metadata);if(!result.contains("error")){if(showModuleRequested)showModuleRequested("hex");formatDock->show();formatDock->raise();}return result;};
        aiPanel_->fillMeshSettings=[this,meshDock](const QJsonObject &args){if(view_->saving()||view_->directEditing())return QJsonObject{{"error","Wait for document editing to finish."}};return AiDecoderSetup::applyMesh(meshDock,args,view_->fileSize());};
        auto showDecoder=[this,views,uvDock,imageCalculatorDock,imageLibraryDock,vulkanDock,meshDock,collectionDock](bool mesh){
            if(showModuleRequested)showModuleRequested(mesh?"3d":"uv");
            if(mesh){for(auto *dock:{meshDock,collectionDock,vulkanDock})dock->show();vulkanDock->raise();}
            else{views->setCurrentIndex(0);for(auto *dock:{imageCalculatorDock,imageLibraryDock,uvDock})dock->show();uvDock->raise();}
        };
        aiPanel_->calculateImage=[imageTools,showDecoder]{auto result=imageTools->calculateActiveDocument();if(result["started"].toBool())showDecoder(false);return result;};
        aiPanel_->renderMesh=[this,meshDock,showDecoder,vulkanPanel]{
            auto *render=meshDock->findChild<QPushButton *>("renderMesh");if(!render||!render->isEnabled()||view_->saving()||view_->directEditing())return QJsonObject{{"error","Wait for mesh decoding or document editing to finish."}};
            if(auto *setup=meshSetupSession(meshDock);setup&&setup->empty()){
                if(!view_->hasFile()||view_->fileSize()==0)return QJsonObject{{"error","Open a nonempty binary before discovering a model."}};
                auto *watcher=new QFutureWatcher<QJsonObject>(this);auto cancel=std::make_shared<std::atomic_bool>(false);auto stale=std::make_shared<bool>(false);auto state=setup->state();auto source=view_->meshSource();auto patches=view_->pendingPatches();
                connect(view_,&HexView::documentChanged,watcher,[cancel,stale]{*stale=true;cancel->store(true);});connect(watcher,&QObject::destroyed,[cancel]{cancel->store(true);});
                render->setEnabled(false);meshDock->findChild<QLabel *>("meshStatus")->setText("Discovering supported models from this document...");showDecoder(true);
                connect(watcher,&QFutureWatcher<QJsonObject>::finished,this,[this,watcher,render,meshDock,vulkanPanel,state,stale]{
                    auto result=watcher->result();watcher->deleteLater();render->setEnabled(true);
                    auto fail=[vulkanPanel](QString message){emit vulkanPanel->meshCalculationFinished(false,message);};
                    auto *current=meshSetupSession(meshDock);if(*stale||!current||current->state()!=state){fail("Discovery canceled: document or 3D settings changed. Click Render 3D again.");return;}
                    if(result.contains("error")){fail(result["error"].toString());return;}auto candidates=result["meshCandidates"].toArray();QJsonObject selected;qint64 largest=-1;
                    for(auto value:candidates){auto candidate=value.toObject()["settings"].toObject();auto count=candidate["vertexCount"].toInteger();if(count>largest){largest=count;selected=candidate;}}
                    if(selected.isEmpty()){fail(assetMeshDiagnostic(result));return;}
                    auto applied=aiPanel_->fillMeshSettings(selected);if(applied.contains("error")){fail(applied["error"].toString());return;}
                    auto started=aiPanel_->renderMesh();if(!started["started"].toBool())fail(started["error"].toString("Could not start mesh decoding."));
                });
                watcher->setFuture(QtConcurrent::run([source,patches,cancel]{return discoverAssets(source,patches,*cancel);}));
                return QJsonObject{{"started",true},{"settingsSummary","Blank 3D setup: discovering the largest supported model locally."}};
            }
            auto summary=QString("%1 vertices / %2 indices · UVs %3 · normals %4.").arg(meshDock->findChild<QLineEdit *>("meshVertexCount")->text(),meshDock->findChild<QLineEdit *>("meshIndexCount")->text(),meshDock->findChild<QCheckBox *>("meshReadUvs")->isChecked()?"on":"off",meshDock->findChild<QCheckBox *>("meshReadNormals")->isChecked()?"read":"calculated");
            render->click();if(render->isEnabled())return QJsonObject{{"error",meshDock->findChild<QLabel *>("meshStatus")->text()}};
            showDecoder(true);return QJsonObject{{"started",true},{"settingsSummary",summary}};
        };
        auto lastScan=std::make_shared<QJsonObject>();
        QList<QPointer<AssetScanPanel>> scanPanels;QMap<QString,QDockWidget *> scanDocks;
        for(auto mode:QStringList{"all","mesh","image"}){auto *scanDock=new QDockWidget(mode=="all"?"File Scan":mode=="mesh"?"3D Data Scan":"Texture / Image Scan",this);scanDock->setObjectName(mode=="all"?"assetScanDock":mode=="mesh"?"meshScanDock":"imageScanDock");auto *panel=new AssetScanPanel(view_,mode,scanDock);scanDock->setWidget(panel);addDockWidget(Qt::BottomDockWidgetArea,scanDock);scanDock->hide();scanPanels.append(panel);scanDocks.insert(mode,scanDock);}
        auto publishScan=[lastScan,scanPanels](QJsonObject result){*lastScan=result;for(auto panel:scanPanels)if(panel)panel->showResults(result);};
        connect(view_,&HexView::documentChanged,this,[lastScan]{*lastScan={};});aiPanel_->cachedDiscovery=[lastScan]{return *lastScan;};aiPanel_->discoveryPublished=publishScan;
        auto pointPreview=[this,meshDock,vulkanPanel,showDecoder](bool faces=false,bool genki=false){
            auto *render=meshDock->findChild<QPushButton *>("renderMesh");if(!render||!render->isEnabled()||view_->saving()||view_->directEditing())return QJsonObject{{"error","Wait for decoding or editing to finish."}};
            auto *watcher=new QFutureWatcher<MeshResult>(this);auto cancel=std::make_shared<std::atomic_bool>(false);auto stale=std::make_shared<bool>(false);auto source=view_->meshSource();auto patches=view_->pendingPatches();auto name=QFileInfo(view_->filePath()).fileName();
            connect(view_,&HexView::documentChanged,watcher,[cancel,stale]{*stale=true;cancel->store(true);});connect(watcher,&QObject::destroyed,[cancel]{cancel->store(true);});render->setEnabled(false);showDecoder(true);
            connect(watcher,&QFutureWatcher<MeshResult>::finished,this,[watcher,render,vulkanPanel,stale,source,name,faces,genki]{auto result=watcher->result();watcher->deleteLater();render->setEnabled(true);if(*stale||!result.error.isEmpty()||!result.mesh){emit vulkanPanel->meshCalculationFinished(false,*stale?"Point preview canceled: document changed.":result.error);return;}int count=result.mesh->vertices.size();vulkanPanel->scene()->add(result.mesh,name+(genki?" / Genki all local parts (variants retained)":faces?" / ARCP recovered strips":" / ARCP point preview (NO FACES)"),source.path);vulkanPanel->frameView(1);emit vulkanPanel->meshCalculationFinished(true,faces?QString("%1: %2 vertices, %3 faces. Hierarchy and materials are not reconstructed; variants retained.").arg(genki?"Genki":"ARCP").arg(count).arg(result.mesh->triangles.size()/3):QString("ARCP point preview: %1 vertices, 0 faces. Triangle topology, hierarchy and materials are not reconstructed.").arg(count));});
            watcher->setFuture(QtConcurrent::run([source,patches,cancel,faces,genki]{return genki?readGenkiGeometry(source,patches,*cancel):readArcpGeometry(source,patches,*cancel,faces);}));return QJsonObject{{"started",true},{"status",faces?(genki?"Loading validated Genki triangle strips; hierarchy and materials remain unresolved.":"Loading validated ARCP triangle strips; hierarchy and materials remain unresolved."):"Loading validated ARCP positions as points. This is not a reconstructed triangle model."}};
        };
        aiPanel_->renderArcp=[pointPreview]{return pointPreview(true);};aiPanel_->renderGenki=[pointPreview]{return pointPreview(true,true);};
        for(auto panel:scanPanels){panel->showHex=[this]{if(showModuleRequested)showModuleRequested("hex");};panel->published=publishScan;panel->annotate=[this,formatDock](const QJsonArray &regions){auto result=AiStructure::apply(view_,formatPanel_,{{"regions",regions}},true,"Scan");if(!result.contains("error"))formatDock->show();return result;};
            panel->useCandidate=[this,showDecoder,pointPreview](const QString &kind,const QJsonObject &data,bool run){if(kind=="evidence"){if(!run||!data["previewSupported"].toBool())return QJsonObject{{"error","This format requires an importer."}};return pointPreview(data["renderSupported"].toBool(),data["family"].toString().startsWith("Genki"));}bool mesh=kind=="mesh";auto &fill=mesh?aiPanel_->fillMeshSettings:aiPanel_->fillImageSettings;auto result=fill(data["settings"].toObject());if(result.contains("error"))return result;showDecoder(mesh);if(!run)return QJsonObject{{"status","Settings loaded. Review the corresponding decoder controls; no data has been decoded yet."}};auto &decode=mesh?aiPanel_->renderMesh:aiPanel_->calculateImage;return decode();};}
        auto openScan=[this,scanPanels,scanDocks](QString mode){if(showModuleRequested)showModuleRequested(mode=="all"?"hex":mode=="mesh"?"3d":"uv");auto *dock=scanDocks.value(mode);dock->show();dock->raise();for(auto panel:scanPanels)if(panel&&panel->objectName()=="assetScanPanel_"+mode)panel->startScan();};
        connect(view_,&HexView::scanRequested,this,[openScan]{openScan("all");});connect(vulkanPanel,&VulkanPanel::scanRequested,this,[openScan]{openScan("mesh");});
        auto *uvContainer=new QWidget;auto *uvLayout=new QVBoxLayout(uvContainer);uvLayout->setContentsMargins(0,0,0,0);auto *scanImages=new QPushButton("Scan for Textures / Images");scanImages->setObjectName("scanForTexturesImages");uvLayout->addWidget(scanImages,0,Qt::AlignLeft);uvLayout->addWidget(views,1);uvDock->setWidget(uvContainer);connect(scanImages,&QPushButton::clicked,this,[openScan]{openScan("image");});
        imageTools->calculationFinished=[ai=QPointer<AiChatPanel>(aiPanel_),scanPanels](bool success,const QString &message){if(ai)ai->decoderFinished(false,success,message);for(auto panel:scanPanels)if(panel)panel->decodingFinished("image",success,message);};
        connect(vulkanPanel,&VulkanPanel::meshCalculationFinished,aiPanel_,[ai=aiPanel_,scanPanels](bool success,const QString &message){ai->decoderFinished(true,success,message);for(auto panel:scanPanels)if(panel)panel->decodingFinished("mesh",success,message);});
        aiPanel_->applicationContext=[this,imageTools,meshDock,lastScan]{QJsonArray images,panels;for(int i=0;i<qMin(imageTools->imageCount(),16);++i){const auto &entry=imageTools->entry(i);images.append(QJsonObject{{"name",entry.name.left(128)},{"width",entry.image.width()},{"height",entry.image.height()},{"format",entry.recipe.format},{"offset",QString::number(entry.recipe.offset)}});}for(auto *dock:sessionDocks_)panels.append(QJsonObject{{"name",dock->windowTitle()},{"shown",!dock->isHidden()}});QJsonObject meshFields;for(auto *field:meshDock->findChildren<QLineEdit *>())if(field->objectName().startsWith("mesh"))meshFields[field->objectName()]=field->text().left(128);for(auto *field:meshDock->findChildren<QComboBox *>())if(field->objectName().startsWith("mesh"))meshFields[field->objectName()]=field->currentText();for(auto *field:meshDock->findChildren<QDoubleSpinBox *>())if(field->objectName().startsWith("mesh"))meshFields[field->objectName()]=field->value();for(auto *field:meshDock->findChildren<QCheckBox *>())if(field->objectName().startsWith("mesh"))meshFields[field->objectName()]=field->isChecked();return QJsonObject{{"application","9XD"},{"latestNativeScanBytes",lastScan->value("scannedBytes")},{"panels",panels},{"imageCount",imageTools->imageCount()},{"images",images},{"meshSettings",meshFields},{"imageSettings",imageTools->setupSnapshot()},{"openDocuments",aiWorkspaceDocuments?aiWorkspaceDocuments():QJsonArray()},{"scriptIncluded",false}};};
        auto *about=menuBar()->addMenu("About");
        auto *docs=about->addAction("Documentation");connect(docs,&QAction::triggered,this,[this]{showDocs();});
        auto *disclaimer=about->addAction("About Binary Workbench…");disclaimer->setMenuRole(QAction::NoRole);
        disclaimer->setObjectName("showAbout9XD");connect(disclaimer,&QAction::triggered,this,[this]{showWorkbenchAbout(this);});
    #ifdef WORKBENCH_FBX_SDK
        auto *fbxNotice=about->addAction("Autodesk FBX SDK notices");connect(fbxNotice,&QAction::triggered,this,[this]{QMessageBox::about(this,"Autodesk FBX SDK","This software contains Autodesk\u00ae FBX\u00ae code developed by Autodesk, Inc. Copyright 2019 Autodesk, Inc. All rights, reserved. Such code is provided \"as is\" and Autodesk, Inc. disclaims any and all warranties, whether express or implied, including without limitation the implied warranties of merchantability, fitness for a particular purpose or non-infringement of third party rights. In no event shall Autodesk, Inc. be liable for any direct, indirect, incidental, special, exemplary, or consequential damages (including, but not limited to, procurement of substitute goods or services; loss of use, data, or profits; or business interruption) however caused and on any theory of liability, whether in contract, strict liability, or tort (including negligence or otherwise) arising in any way out of such code.");});
#endif
        auto *toolbar=addToolBar("Common Actions");toolbar->setObjectName("mainToolbar");toolbar->addAction(open);toolbar->addAction(reload);toolbar->addAction(save);toolbar->addAction(exportAction);toolbar->addSeparator();toolbar->addAction(copy);toolbar->addSeparator();toolbar->addAction(calculator);toolbar->addAction(options);
        connect(view_,&HexView::fileOpened,this,[this](const QString &path){path_=path;WorkbenchFiles::remember(path);if(path.isEmpty()){setWindowTitle(untitledName_+"[*]");return;}setWindowTitle(QFileInfo(path).fileName()+"[*] — 9XD");if(!recent_->add(path))statusBar()->showMessage("Could not save the recent files list.",6000);});
        connect(view_,&HexView::idle,this,[this]{if(closing_){closing_=false;QTimer::singleShot(0,this,&QWidget::close);}});
        auto *activityDock = new QDockWidget("Thread Activity", this);
        activityDock->setObjectName("threadActivityDock");
        auto *activityText = new QPlainTextEdit(activityDock);
        activityText->setObjectName("threadActivityDetails");
        activityText->setReadOnly(true);
        activityDock->setWidget(activityText);
        addDockWidget(Qt::BottomDockWidgetArea, activityDock);
        activityDock->hide();
        viewMenu->addAction(activityDock->toggleViewAction());
        auto *activityButton = new QToolButton(this);
        activityButton->setObjectName("threadActivityToggle");
        activityButton->setCheckable(true);
        activityButton->setToolTip("Show or hide background task details");
        view_->addStatusWidget(activityButton);
        connect(activityButton, &QToolButton::toggled, activityDock, &QWidget::setVisible);
        connect(activityDock, &QDockWidget::visibilityChanged, activityButton, &QToolButton::setChecked);
        auto refreshActivity = [this, activityButton, activityText, activityDock] {
            const auto channels = view_->workerActivity();
            int active = 0, pending = 0;
            for (const auto &channel : channels) {
                active += channel.running; pending += channel.pending;
            }
            activityButton->setText(QString("Threads · %1/%2 busy · %3 pending · %4")
                .arg(active).arg(channels.size()).arg(pending).arg(activityDock->isVisible() ? "Hide details" : "Show details"));
            if (!activityDock->isVisible()) return;
            QString details = QString("Task occupancy: %1 of %2 channels (%3 %). Pending: %4 of 3.\n")
                .arg(active).arg(channels.size()).arg(active * 100 / channels.size()).arg(pending);
            const int processors = QThread::idealThreadCount();
            details += QString("Logical processors detected: %1. UI: separate main thread.\n\n")
                .arg(processors > 0 ? QString::number(processors) : QString("unavailable"));
            const QStringList names{"Viewer", "Inspector", "Copy", "Save", "Search", "Signatures", "Entropy", "Direct Editing", "Checksum"};
            for (int i = 0; i < channels.size(); ++i) {
                const auto &c = channels[i];
                details += QString("%1 — %2%3 · pending: %4\n")
                    .arg(names[i], c.running ? "Busy" : "Idle", c.stopping ? " (closing)" : "")
                    .arg(c.pending ? 1 : 0);
                if (c.running) details += QString("  Request: offset 0x%1 · %2 bytes · elapsed: %3 ms\n")
                    .arg(QString::number(c.offset, 16).toUpper()).arg(c.length).arg(c.elapsedMs);
                details += QString("  Started: %1 · completed: %2 · errors: %3 · last duration: %4\n")
                    .arg(c.started).arg(c.completed).arg(c.errors)
                    .arg(c.completed ? QString::number(c.lastMs) + " ms" : "—");
                details += QString("  Discarded results: %1 · replaced pending requests: %2\n\n")
                    .arg(c.discarded).arg(c.replaced);
            }
            if(view_->saving())details+=QString("Saving: %1 / %2 MiB processed (not yet committed to disk).\n\n")
                .arg(double(view_->saveProgress())/(1024*1024),0,'f',2).arg(double(view_->fileSize())/(1024*1024),0,'f',2);
            details += "Channels share a bounded background pool. Reads keep only the latest pending request.\n"
                "Save, search, signatures, entropy and checksum allow one task per channel, without a queue. Saving pauses editing.\n"
                "Counters cover this session. Discarded: an obsolete result or one received while closing.\n"
                "Replaced: a pending request replaced by a newer one.\n"
                "Occupancy indicates running tasks, not CPU usage or the total number of process threads.\n"
                "Includes disk/cache waits and delivery to the UI; timings are end to end.\n"
                "Updated every 200 ms: very short tasks may not appear active, but are still counted.";
            if (activityText->toPlainText() != details) activityText->setPlainText(details);
        };
        auto *activityTimer = new QTimer(this);
        activityTimer->setInterval(200);
        connect(activityTimer, &QTimer::timeout, this, refreshActivity);
        connect(activityDock, &QDockWidget::visibilityChanged, this, [refreshActivity](bool){ refreshActivity(); });
        activityTimer->start();
        refreshActivity();
        resize(1450,850);
        if(!uiStatePath.isEmpty()){
            uiLayout_=new UiLayout(this,uiStatePath);
            viewMenu->addSeparator();auto *reset=viewMenu->addAction("Reset Panel Layout");reset->setObjectName("resetUiLayout");connect(reset,&QAction::triggered,this,[this]{uiLayout_->reset();});
        }
        // MenuBar1 from the Xojo UI prototype. Reuse actions to retain their state and handlers.
        for (auto *menu : {file, edit, tools, about})
            for (auto *action : menu->actions()) menu->removeAction(action);
        for (auto *action : menuBar()->actions()) menuBar()->removeAction(action);
        file->setTitle("&File"); edit->setTitle("&Edit");
        auto *searchMenu = new QMenu("Search", this);
        auto *hexMenu = new QMenu("&Hex Viewer", this);
        auto *analyseMenu = new QMenu("&Analyse", this);
        tools->setTitle("E&xtras"); viewMenu->setTitle("&Windows"); about->setTitle("&Help");
        for (auto *menu : {file, edit, searchMenu, hexMenu, analyseMenu, tools, viewMenu, about})
            menuBar()->addMenu(menu);
        auto pending = [](QMenu *menu, const QString &text, const QKeySequence &key = {}) {
            auto *action = menu->addAction(text);
            action->setShortcut(key); action->setEnabled(false);
            action->setToolTip("Not implemented yet.");
            return action;
        };
        open->setText("Open File"); recentMenu->setTitle("Open Recent");
        save->setText("Save"); saveAs->setText("Save as"); exit->setText("Exit Program");
        auto *newFile = file->addAction("New File");
        newFile->setObjectName("newFileAction"); newFile->setShortcut(QKeySequence::New);
        connect(newFile, &QAction::triggered, this, [this] { createNewFile(); });
        connect(view_, &HexView::editStateChanged, newFile, [this, newFile] { newFile->setEnabled(!view_->saving()); });
        file->addAction(open); file->addMenu(recentMenu); file->addSeparator();toolbar->insertAction(open,newFile);
        file->addAction(save); file->addAction(saveAs);
        pending(file, "Save selection", QKeySequence("Ctrl+Alt+S"));
        file->addSeparator(); pending(file, "Close Files"); file->addAction(exit);
        undo->setText("Undo"); cut->setText("Cut"); copy->setText("Copy"); paste->setText("Paste");
        cut->setShortcut(QKeySequence::Cut);
        edit->addAction(undo); edit->addSeparator(); edit->addAction(cut); edit->addAction(copy); edit->addAction(paste);
        auto *deleteBytes=edit->addAction("Delete");deleteBytes->setShortcut(QKeySequence::Delete);deleteBytes->setEnabled(false);
        connect(deleteBytes,&QAction::triggered,view_,&HexView::deleteSelection);edit->addSeparator();
        auto *insertBytes=edit->addAction("Insert Bytes");insertBytes->setEnabled(false);connect(insertBytes,&QAction::triggered,view_,&HexView::insertBytes);
        pending(edit, "Insert file"); pending(edit, "Fill selection");
        auto updateSizeActions=[this,deleteBytes,insertBytes]{const bool allowed=view_->isMemoryDocument()&&!view_->saving();insertBytes->setEnabled(allowed);deleteBytes->setEnabled(allowed&&view_->selection().count(view_->fileSize())>0);};
        connect(view_,&HexView::editStateChanged,this,updateSizeActions);connect(view_,&HexView::selectionAvailable,this,[updateSizeActions](bool){updateSizeActions();});updateSizeActions();
        edit->addSeparator(); select->setText("Select All");
        select->setToolTip("Select all bytes in this document."); edit->addAction(select);
        pending(edit, "Select chunk"); edit->addSeparator();
        auto *goOffset = edit->addAction("Go to Offset"); goOffset->setShortcut(QKeySequence("Ctrl+G"));
        connect(goOffset, &QAction::triggered, this, [this] {
            view_->focusOffsetHeader();
        });
        auto *copyOffset = edit->addAction("Copy offset");
        connect(copyOffset, &QAction::triggered, this, [this] {
            if (view_->selection().anchor >= 0)
                QApplication::clipboard()->setText("0x" + QString::number(view_->selection().anchor, 16).toUpper());
        });
        auto updateOffset = [this, copyOffset] { copyOffset->setEnabled(view_->hasFile() && view_->selection().anchor >= 0); };
        connect(view_, &HexView::selectionAvailable, this, [updateOffset](bool) { updateOffset(); });
        connect(view_, &HexView::documentChanged, this, updateOffset); updateOffset();
        edit->addSeparator(); options->setText("Settings"); edit->addAction(options);
        auto showSearch = [this, formatDock] {
            formatDock->show(); formatDock->raise();
            auto *field = formatPanel_->findChild<QLineEdit *>("patternExpression"); field->setFocus(); field->selectAll();
            if (auto *scroll = qobject_cast<QScrollArea *>(formatDock->widget())) scroll->ensureWidgetVisible(field);
        };
        auto *find = searchMenu->addAction("Find"); find->setShortcut(QKeySequence::Find);
        connect(find, &QAction::triggered, this, showSearch);
        pending(searchMenu, "Replace", QKeySequence("Ctrl+R"));
        auto *next = searchMenu->addAction("Find next"); next->setShortcut(QKeySequence("F3"));
        auto *previous = searchMenu->addAction("Find Previous"); previous->setShortcut(QKeySequence("Shift+F3"));
        connect(next, &QAction::triggered, this, [this, showSearch] {
            showSearch(); formatPanel_->findChild<QSpinBox *>("matchIndex")->stepUp();
        });
        connect(previous, &QAction::triggered, this, [this, showSearch] {
            showSearch(); formatPanel_->findChild<QSpinBox *>("matchIndex")->stepDown();
        });
        pending(hexMenu, "Fit to window");
        auto *bytesPerRow = hexMenu->addAction("Bytes per row");
        connect(bytesPerRow, &QAction::triggered, this, [this] {
            view_->chooseBytesPerRow();
        });
        hexMenu->addSeparator(); auto *encodingMenu = hexMenu->addMenu("Codification");
        auto *encodingBox = view_->findChild<QComboBox *>("encoding");
        auto *encodingGroup = new QActionGroup(this); encodingGroup->setExclusive(true);
        const QStringList encodings{"ASCII", "UTF-8", "UTF-16LE", "UTF-16BE", "Japanese CP932"};
        for (int i = 0; i < encodings.size(); ++i) {
            auto *action = encodingMenu->addAction(encodings[i]); action->setCheckable(true);
            encodingGroup->addAction(action); action->setChecked(encodingBox->currentIndex() == i);
            connect(action, &QAction::triggered, this, [encodingBox, i] { encodingBox->setCurrentIndex(i); });
            connect(encodingBox, &QComboBox::currentIndexChanged, action, [action, i](int index) { action->setChecked(index == i); });
        }
        encodingMenu->addSeparator();
        auto *endianBox = view_->findChild<QComboBox *>("endian");
        auto *endianGroup = new QActionGroup(this); endianGroup->setExclusive(true);
        for (int i = 0; i < 2; ++i) {
            auto *action = encodingMenu->addAction(i == 0 ? "Little Endian" : "Big Endian");
            action->setObjectName(i == 0 ? "littleEndianAction" : "bigEndianAction");
            action->setCheckable(true); endianGroup->addAction(action);
            action->setChecked(endianBox->currentIndex() == i);
            connect(action, &QAction::triggered, this, [endianBox, i] { endianBox->setCurrentIndex(i); });
            connect(endianBox, &QComboBox::currentIndexChanged, action, [action, i](int index) { action->setChecked(index == i); });
        }
        auto *offsetMenu = hexMenu->addMenu("Offset Format");
        auto *offsetGroup=new QActionGroup(this);
        for(auto entry:{qMakePair(QString("Hexadecimal"),16),qMakePair(QString("Decimal"),10),qMakePair(QString("Octal"),8)}){
            auto *action=offsetMenu->addAction(entry.first);action->setCheckable(true);offsetGroup->addAction(action);action->setChecked(view_->offsetBase()==entry.second);
            connect(action,&QAction::triggered,this,[this,base=entry.second]{view_->setOffsetBase(base);});
        }
        auto *columnsMenu = hexMenu->addMenu("Columns");auto *columnsGroup=new QActionGroup(this);
        const QStringList columnLabels{"Hexadecimal + Text","Hexadecimal","Text"};
        for(int i=0;i<columnLabels.size();++i){auto *action=columnsMenu->addAction(columnLabels[i]);action->setCheckable(true);columnsGroup->addAction(action);action->setChecked(view_->columnMode()==i);connect(action,&QAction::triggered,this,[this,i]{view_->setColumnMode(i);});}
        auto *groupMenu = hexMenu->addMenu("Byte Group");auto *byteGroup=new QActionGroup(this);
        for(int n:{1,2,4,8,16,32}){auto *action=groupMenu->addAction(QString::number(n));action->setCheckable(true);byteGroup->addAction(action);action->setChecked(view_->byteGroup()==n);connect(action,&QAction::triggered,this,[this,n]{view_->setByteGroup(n);});}
        groupMenu->addSeparator();auto *customGroup=groupMenu->addAction("...");customGroup->setCheckable(true);byteGroup->addAction(customGroup);
        connect(customGroup,&QAction::triggered,this,[this,byteGroup]{view_->chooseByteGroup();for(auto *action:byteGroup->actions())action->setChecked(action->text()==QString::number(view_->byteGroup())||(action->text()=="..."&&!QStringList{"1","2","4","8","16","32"}.contains(QString::number(view_->byteGroup()))));});
        hexMenu->addSeparator(); auto *inspectorAction = dock->toggleViewAction();
        inspectorAction->setText("Data Inspector"); inspectorAction->setShortcut(QKeySequence("Ctrl+Shift+D"));
        hexMenu->addAction(inspectorAction);
        auto *matchesAction = hexMenu->addAction("Found Coincidences");
        connect(matchesAction, &QAction::triggered, this, showSearch);
        hexMenu->addSeparator(); reload->setText("Update Viewer"); hexMenu->addAction(reload);
        entropyAction->setText("Statistics"); analyseMenu->addAction(entropyAction);analyseMenu->addAction(checksumDock->toggleViewAction());
        auto *compareMenu = analyseMenu->addMenu("Compare Data");
        auto showCompare=[compareDock]{compareDock->show();compareDock->raise();};
        connect(compareMenu->addAction("Compare"),&QAction::triggered,this,showCompare);
        connect(compareMenu->addAction("Next difference"),&QAction::triggered,this,[showCompare,comparePanel]{showCompare();comparePanel->step(1);});
        connect(compareMenu->addAction("Previous difference"),&QAction::triggered,this,[showCompare,comparePanel]{showCompare();comparePanel->step(-1);});
        pending(tools, "Open RAM"); tools->addSeparator(); pending(tools, "Merge Files"); pending(tools, "Divide File");
        // Keep the existing application tools accessible during the gradual UI migration.
        tools->addSeparator(); auto *existingTools = tools->addMenu("Workbench Tools");
        redo->setText("Redo"); replace->setText("Overwrite bytes…"); calculator->setText("Calculator…");
        interpret->setText("Encoding and interpretation"); exportAction->setText("Export 3D model…");
        for (auto *action : {redo, replace, calculator, interpret, magicAction, meshAction, vulkanAction, uvAction, scriptAction, exportAction})
            existingTools->addAction(action);
        disclaimer->setText("About"); about->addAction(disclaimer); about->addSeparator();
        docs->setText("Documentation"); about->addAction(docs);

        sessionDocks_=findChildren<QDockWidget *>(QString(),Qt::FindDirectChildrenOnly);
        for(auto *dock:sessionDocks_){dock->setProperty("initialArea",int(dockWidgetArea(dock)));dock->setProperty("sessionVisible",!dock->isHidden());}
    }
    void openFile(const QString &path){
        if(openRequested){openRequested(path);return;}
        if(view_->directEditing()){QTimer::singleShot(30,this,[this,path]{openFile(path);});return;}
        if(view_->saving())return;
        if(!resolvePending([this,path]{openFile(path);}))return;
        path_=path;view_->openFile(path);
    }
protected:
    void closeEvent(QCloseEvent *event)override{
        if(view_->directEditing()&&!closing_){QTimer::singleShot(30,this,&QWidget::close);event->ignore();return;}
        if(view_->saving()){event->ignore();return;}
        if(!closing_&&!scriptPanel_->confirmClose()){event->ignore();return;}
        if(!closing_&&!resolvePending([this]{close();})){event->ignore();return;}
        if(!closing_&&!magicPanel_->confirmClose()){event->ignore();return;}
        if(view_->busy()){
            closing_=true;menuBar()->setEnabled(false);view_->setEnabled(false);
            for(auto *toolbar:findChildren<QToolBar *>())toolbar->setEnabled(false);
            statusBar()->showMessage("Waiting for active reads before closing…");view_->shutdown();event->ignore();return;
        }
        if(aiPanel_)aiPanel_->stop();if(uiLayout_)uiLayout_->saveAndStop();
        view_->shutdown();event->accept();
    }
private:
    void createNewFile() {
        if(newRequested){newRequested();return;}
        if(view_->saving()||!resolvePending([this]{createNewFile();}))return;
        view_->newMemoryDocument();
    }
    bool saveCurrent(bool saveAs){
        if(view_->directEditing()){QTimer::singleShot(30,this,[this,saveAs]{saveCurrent(saveAs);});return true;}
        QString destination=path_;
        if(saveAs||destination.isEmpty())destination=QFileDialog::getSaveFileName(this,"Save File As",path_.isEmpty()?untitledName_+".bin":path_);
        if(!destination.isEmpty()&&saveDestinationAllowed&&!saveDestinationAllowed(destination))return false;
        return !destination.isEmpty()&&view_->saveFile(destination);
    }
    bool resolvePending(std::function<void()> next){
        if(auto *setup=meshSetupSession(sessionDock("meshControlsDock"));setup&&!setup->confirmClose())return false;
        if(!view_->dirty())return formatPanel_->confirmLeave();
        const auto answer=QMessageBox::warning(this,"Unsaved Changes",
            "You have unsaved changes. Save them before continuing?",
            QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
        if(answer==QMessageBox::Cancel)return false;
        if(answer==QMessageBox::Discard){if(!formatPanel_->confirmLeave())return false;view_->discardEdits();return true;}
        afterSave_=std::move(next);
        if(!saveCurrent(false))afterSave_={};
        return false;
    }
    void showOptions(){
        if(settingsRequested){settingsRequested();return;}
        QSettings settings("BinaryWorkbench","Workspace");SettingsDialog dialog(settings,[this]{view_->applyDisplayPreferences();},this);dialog.exec();
    }
    void showCalculator(){
        auto *dialog=[this]()->ProgrammerCalculator *{for(auto *child:children())if(auto *calculator=dynamic_cast<ProgrammerCalculator *>(child))return calculator;return nullptr;}();
        if(!dialog){dialog=new ProgrammerCalculator(this);dialog->setAttribute(Qt::WA_DeleteOnClose);}
        dialog->show();dialog->raise();dialog->activateWindow();
    }
    void showDocs(){
        auto *dialog=new QDialog(this);dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle("Documentation · 9XD");
        auto *layout=new QVBoxLayout(dialog);auto *text=new QTextBrowser;layout->addWidget(text);
        const auto bundledGuide=QDir(QCoreApplication::applicationDirPath()).filePath("docs/user-guide.md");
        QFile file(QFileInfo::exists(bundledGuide)?bundledGuide:QString(WORKBENCH_DOCUMENTATION));
        if(file.open(QIODevice::ReadOnly))text->setMarkdown(QString::fromUtf8(file.readAll()));else text->setPlainText("The English user guide was not found in the project folder.");
        dialog->resize(780,680);dialog->show();
    }
    QString recentPath_;QString untitledName_="Untitled";QList<QDockWidget *> sessionDocks_;
    std::unique_ptr<RecentFiles> recent_;
    UiLayout *uiLayout_=nullptr;
    AiChatPanel *aiPanel_=nullptr;ScriptPanel *scriptPanel_;HexView *view_;FormatPanel *formatPanel_;MagicPanel *magicPanel_;QString path_;bool closing_=false;std::function<void()> afterSave_;
};
#include "workspace_window.h"
#ifndef WORKBENCH_NO_MAIN
int main(int argc,char **argv){
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc,argv);QCoreApplication::setOrganizationName("BinaryWorkbench");QCoreApplication::setApplicationName("BinaryWorkbench");QGuiApplication::setApplicationDisplayName("9XD");QCoreApplication::setApplicationVersion("0.3.0");app.setWindowIcon(QIcon(":/branding/logo-sticker.png"));
    if(app.arguments().contains("--deployment-check")){
        const auto formats=QImageReader::supportedImageFormats();
        for(const auto &format:{"png","jpeg","bmp","tiff","webp"})if(!formats.contains(format))return 3;
        const QDir folder(QCoreApplication::applicationDirPath());
        return folder.exists("script_runner.exe")&&folder.exists("docs/user-guide.md")&&folder.exists("python/python.exe")?0:4;
    }
    WorkspaceWindow window(UiLayout::defaultPath());window.show();const auto args=app.arguments();
    if(args.size()>1&&!args[1].startsWith("--"))window.openFile(args[1]);
    if(args.contains("--smoke-test"))QTimer::singleShot(1000,&window,&QWidget::close);
    return app.exec();
}

#endif
