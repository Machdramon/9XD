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
    std::function<bool(const QString &)> saveDestinationAllowed;
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
        setWindowTitle("Binary Workbench");
        view_=new HexView(this);setCentralWidget(view_);
        menuBar()->setNativeMenuBar(false);
        auto *file=menuBar()->addMenu("File");
        auto *open=file->addAction("Open File…");open->setShortcut(QKeySequence::Open);
        connect(open,&QAction::triggered,this,[this]{const auto path=QFileDialog::getOpenFileName(this,"Open local binary file");if(!path.isEmpty())openFile(path);});
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
        imageTools->imageSelected=[views,uvDock](const QImage &){views->setCurrentIndex(0);uvDock->show();uvDock->raise();};
        auto *scriptDock=new QDockWidget("Scripting · Python / C++",this);scriptDock->setObjectName("scriptDock");scriptPanel_=new ScriptPanel(view_,vulkanPanel,scriptDock);scriptDock->setWidget(scriptPanel_);addDockWidget(Qt::BottomDockWidgetArea,scriptDock);scriptDock->hide();viewMenu->addAction(scriptDock->toggleViewAction());auto *scriptAction=tools->addAction("Scripting · Python / C++");connect(scriptAction,&QAction::triggered,this,[scriptDock]{scriptDock->show();scriptDock->raise();});
        auto *about=menuBar()->addMenu("About");
        auto *docs=about->addAction("Documentation");connect(docs,&QAction::triggered,this,[this]{showDocs();});
        auto *disclaimer=about->addAction("About Binary Workbench…");disclaimer->setMenuRole(QAction::NoRole);
        connect(disclaimer,&QAction::triggered,this,[this]{QMessageBox::about(this,"Binary Workbench","Binary analysis prototype built with C++ and Qt.\n\nValues are interpretations of bytes, not guaranteed format identifications. Editing overwrites bytes, with changes pending until you save.\n\nExperimental Vulkan 3D view. Python/C++ scripting is available. New RAM documents support insertion, deletion and growth. Disk-backed files use overwrite editing.");});
        auto *toolbar=addToolBar("Common Actions");toolbar->setObjectName("mainToolbar");toolbar->addAction(open);toolbar->addAction(reload);toolbar->addAction(save);toolbar->addAction(exportAction);toolbar->addSeparator();toolbar->addAction(copy);toolbar->addSeparator();toolbar->addAction(calculator);toolbar->addAction(options);
        connect(view_,&HexView::fileOpened,this,[this](const QString &path){path_=path;if(path.isEmpty()){setWindowTitle(untitledName_+"[*]");return;}setWindowTitle(QFileInfo(path).fileName()+"[*] — Binary Workbench");if(!recent_->add(path))statusBar()->showMessage("Could not save the recent files list.",6000);});
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
            auto *address = view_->findChild<QLineEdit *>("address"); address->setFocus(); address->selectAll();
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
            auto *stride = view_->findChild<QSpinBox *>("stride"); stride->setFocus(); stride->selectAll();
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
        for (const auto &label : {"Hexadecimal", "Decimal", "Octal"}) pending(offsetMenu, label);
        auto *columnsMenu = hexMenu->addMenu("Columns");
        for (const auto &label : {"Hexadecimal + Text", "Hexadecimal", "Text"}) pending(columnsMenu, label);
        auto *groupMenu = hexMenu->addMenu("Byte Group");
        for (const auto &label : {"1", "2", "4", "8", "14", "32"}) pending(groupMenu, label);
        groupMenu->addSeparator(); pending(groupMenu, "...");
        hexMenu->addSeparator(); auto *inspectorAction = dock->toggleViewAction();
        inspectorAction->setText("Data Inspector"); inspectorAction->setShortcut(QKeySequence("Ctrl+Shift+D"));
        hexMenu->addAction(inspectorAction);
        auto *matchesAction = hexMenu->addAction("Found Coincidences");
        connect(matchesAction, &QAction::triggered, this, showSearch);
        hexMenu->addSeparator(); reload->setText("Update Viewer"); hexMenu->addAction(reload);
        entropyAction->setText("Statistics"); analyseMenu->addAction(entropyAction);analyseMenu->addAction(checksumDock->toggleViewAction());
        auto *compareMenu = analyseMenu->addMenu("Compare Data");
        for (const auto &label : {"Compare", "Next difference", "Previous difference"}) pending(compareMenu, label);
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
        if(uiLayout_)uiLayout_->saveAndStop();
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
        auto *dialog=new QDialog(this);dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle("Offset Calculator · UInt64");
        auto *layout=new QVBoxLayout(dialog);auto *form=new QFormLayout;
        auto *a=new QLineEdit;auto *b=new QLineEdit;auto *op=new QComboBox;op->addItems({"+","−","×","÷","%","AND","OR","XOR"});
        a->setPlaceholderText("Decimal or 0x…");b->setPlaceholderText("Decimal or 0x…");form->addRow("First value",a);form->addRow("Operation",op);form->addRow("Second value",b);layout->addLayout(form);
        auto *run=new QPushButton("Calculate");auto *result=new QPlainTextEdit;result->setReadOnly(true);layout->addWidget(run);layout->addWidget(result);
        auto calculate=[a,b,op,result]{result->setPlainText(calculateInteger(a->text(),op->currentText(),b->text()));};
        connect(run,&QPushButton::clicked,dialog,calculate);connect(a,&QLineEdit::returnPressed,dialog,calculate);connect(b,&QLineEdit::returnPressed,dialog,calculate);
        dialog->resize(430,320);dialog->show();
    }
    void showDocs(){
        auto *dialog=new QDialog(this);dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle("Documentation · Binary Workbench");
        auto *layout=new QVBoxLayout(dialog);auto *text=new QTextBrowser;layout->addWidget(text);
        QFile file(WORKBENCH_DOCUMENTATION);
        if(file.open(QIODevice::ReadOnly))text->setMarkdown(QString::fromUtf8(file.readAll()));else text->setPlainText("The English user guide was not found in the project folder.");
        dialog->resize(780,680);dialog->show();
    }
    QString recentPath_;QString untitledName_="Untitled";QList<QDockWidget *> sessionDocks_;
    std::unique_ptr<RecentFiles> recent_;
    UiLayout *uiLayout_=nullptr;
    ScriptPanel *scriptPanel_;HexView *view_;FormatPanel *formatPanel_;MagicPanel *magicPanel_;QString path_;bool closing_=false;std::function<void()> afterSave_;
};
#include "workspace_window.h"
#ifndef WORKBENCH_NO_MAIN
int main(int argc,char **argv){
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc,argv);QCoreApplication::setOrganizationName("BinaryWorkbench");QCoreApplication::setApplicationName("BinaryWorkbench");WorkspaceWindow window(UiLayout::defaultPath());window.show();const auto args=app.arguments();
    if(args.size()>1&&!args[1].startsWith("--"))window.openFile(args[1]);
    if(args.contains("--smoke-test"))QTimer::singleShot(1000,&window,&QWidget::close);
    return app.exec();
}

#endif
