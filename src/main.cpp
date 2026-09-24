#include "script_panel.h"
#include "recent_files.h"
#include "export_dialog.h"
#include <QDir>
#include <memory>
#include "layout_persistence.h"
#include "vulkan_panel.h"
#include "uv_panel.h"
#include "entropy_panel.h"
#include "magic_panel.h"
#include "hex_view.h"
#include "format_panel.h"
#include "local_config.h"
#include <QApplication>
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
    explicit MainWindow(QString uiStatePath={}) {
        recent_=std::make_unique<RecentFiles>(uiStatePath.isEmpty()?QString():QFileInfo(uiStatePath).dir().filePath("recent-files.json"));
        setWindowTitle("Binary Workbench");
        view_=new HexView(this);setCentralWidget(view_);
        menuBar()->setNativeMenuBar(false);
        auto *file=menuBar()->addMenu("Archivo");
        auto *open=file->addAction("Abrir archivo…");open->setShortcut(QKeySequence::Open);
        connect(open,&QAction::triggered,this,[this]{const auto path=QFileDialog::getOpenFileName(this,"Abrir archivo binario local");if(!path.isEmpty())openFile(path);});
        auto *recentMenu=file->addMenu("Archivos recientes");recentMenu->setObjectName("recentFilesMenu");
        connect(recentMenu,&QMenu::aboutToShow,this,[this,recentMenu]{
            recentMenu->clear();int index=0;for(const auto &path:recent_->paths()){QFileInfo info(path);QString label=QString("%1 · %2 — %3%4").arg(++index).arg(info.fileName(),info.absolutePath(),info.isFile()?QString():QString(" · no disponible"));label.replace("&","&&");auto *action=recentMenu->addAction(label);action->setData(path);action->setToolTip(path);action->setEnabled(info.isFile());connect(action,&QAction::triggered,this,[this,path]{openFile(path);});}
            if(recent_->paths().isEmpty()){auto *empty=recentMenu->addAction("Sin archivos recientes");empty->setEnabled(false);}else{recentMenu->addSeparator();auto *clear=recentMenu->addAction("Vaciar lista");clear->setObjectName("clearRecentFiles");connect(clear,&QAction::triggered,this,[this]{if(!recent_->clear())statusBar()->showMessage("No se pudo guardar la lista de recientes.",6000);});}
        });
        auto *reload=file->addAction("Volver a abrir");
        connect(reload,&QAction::triggered,this,[this]{if(!path_.isEmpty())openFile(path_);});
        auto *save=file->addAction("Guardar");save->setShortcut(QKeySequence::Save);save->setEnabled(false);
        connect(save,&QAction::triggered,this,[this]{saveCurrent(false);});
        auto *saveAs=file->addAction("Guardar como…");saveAs->setShortcut(QKeySequence::SaveAs);
        connect(saveAs,&QAction::triggered,this,[this]{saveCurrent(true);});
        file->addSeparator();auto *options=file->addAction("Opciones…");
        connect(options,&QAction::triggered,this,[this]{showOptions();});
        file->addSeparator();auto *exit=file->addAction("Salir");exit->setShortcut(QKeySequence::Quit);connect(exit,&QAction::triggered,this,&QWidget::close);
        auto *edit=menuBar()->addMenu("Editar");
        auto *undo=edit->addAction("Deshacer");undo->setShortcut(QKeySequence::Undo);
        connect(undo,&QAction::triggered,view_,&HexView::undo);
        auto *redo=edit->addAction("Rehacer");redo->setShortcut(QKeySequence::Redo);
        connect(redo,&QAction::triggered,view_,&HexView::redo);
        auto *replace=edit->addAction("Sobrescribir bytes…");replace->setShortcut(Qt::Key_F2);
        connect(replace,&QAction::triggered,view_,&HexView::editSelection);
        auto *cut=edit->addAction("Cortar");cut->setEnabled(false);
        cut->setToolTip("La edición actual sobrescribe bytes; no cambia el tamaño ni los offsets del archivo.");
        auto *copy=edit->addAction("Copiar selección hexadecimal");copy->setShortcut(QKeySequence::Copy);copy->setEnabled(false);
        connect(copy,&QAction::triggered,view_,&HexView::copySelection);connect(view_,&HexView::selectionAvailable,copy,&QAction::setEnabled);
        auto *paste=edit->addAction("Pegar");paste->setShortcut(QKeySequence::Paste);paste->setEnabled(false);paste->setToolTip("Sobrescribir con bytes del portapapeles o texto hexadecimal.");
        connect(paste,&QAction::triggered,view_,&HexView::pasteSelection);
        auto updateActions=[this,save,saveAs,undo,redo,paste,replace,open,reload]{
            const bool writable=view_->hasFile()&&!view_->saving();
            save->setEnabled(writable&&view_->dirty());saveAs->setEnabled(writable);
            undo->setEnabled(writable&&view_->dirty());redo->setEnabled(writable&&view_->canRedo());
            paste->setEnabled(writable&&view_->selection().anchor>=0);
            replace->setEnabled(writable&&view_->selection().anchor>=0);
            open->setEnabled(!view_->saving());reload->setEnabled(writable);
            setWindowModified(view_->dirty());
        };
        connect(view_,&HexView::editStateChanged,this,updateActions);
        connect(view_,&HexView::selectionAvailable,this,[updateActions](bool){updateActions();});
        updateActions();
        connect(view_,&HexView::saveFinished,this,[this](bool success){
            auto next=std::move(afterSave_);afterSave_={};
            if(success){if(next)QTimer::singleShot(0,this,std::move(next));}
            else QMessageBox::warning(this,"Guardado incompleto","No se ha podido guardar. Consulta el mensaje inferior. Los cambios siguen pendientes.");
        });
        edit->addSeparator();auto *select=edit->addAction("Seleccionar vista");select->setShortcut(QKeySequence::SelectAll);connect(select,&QAction::triggered,view_,&HexView::selectPage);
        auto *tools=menuBar()->addMenu("Herramientas");
        auto *calculator=tools->addAction("Calculadora…");connect(calculator,&QAction::triggered,this,[this]{showCalculator();});
        auto *interpret=tools->addAction("Codificación e interpretación");connect(interpret,&QAction::triggered,view_,&HexView::focusInterpretation);
        auto *dock=new QDockWidget("Inspector",this);dock->setObjectName("inspectorDock");dock->setWidget(view_->inspector());view_->inspector()->show();addDockWidget(Qt::RightDockWidgetArea,dock);
        dock->setMinimumWidth(290);
        auto *viewMenu=menuBar()->addMenu("Ver");viewMenu->addAction(dock->toggleViewAction());
        auto *formatDock=new QDockWidget("Reglas de formato",this);formatDock->setObjectName("formatRulesDock");
        formatPanel_=new FormatPanel(view_,this);auto *formatScroll=new QScrollArea;formatScroll->setWidgetResizable(true);formatScroll->setFrameShape(QFrame::NoFrame);formatScroll->setWidget(formatPanel_);formatDock->setWidget(formatScroll);addDockWidget(Qt::RightDockWidgetArea,formatDock);
        viewMenu->addAction(formatDock->toggleViewAction());
        auto *magicDock=new QDockWidget("Biblioteca de magic bytes",this);magicDock->setObjectName("magicDock");
        magicPanel_=new MagicPanel(view_,formatPanel_,magicDock);magicDock->setWidget(magicPanel_);addDockWidget(Qt::BottomDockWidgetArea,magicDock);magicDock->hide();
        viewMenu->addAction(magicDock->toggleViewAction());auto *magicAction=tools->addAction("Biblioteca de magic bytes");connect(magicAction,&QAction::triggered,this,[magicDock]{magicDock->show();magicDock->raise();});
        auto *entropyDock=new QDockWidget("Entropía",this);entropyDock->setObjectName("entropyDock");entropyDock->setWidget(new EntropyPanel(view_,entropyDock));addDockWidget(Qt::RightDockWidgetArea,entropyDock);splitDockWidget(dock,entropyDock,Qt::Vertical);tabifyDockWidget(dock,formatDock);dock->raise();
        viewMenu->addAction(entropyDock->toggleViewAction());auto *entropyAction=tools->addAction("Analizador de entropía");connect(entropyAction,&QAction::triggered,this,[entropyDock]{entropyDock->show();entropyDock->raise();});
        auto *vulkanDock=new QDockWidget("Vista 3D · Vulkan",this);vulkanDock->setObjectName("vulkanDock");auto *vulkanPanel=new VulkanPanel(vulkanDock);vulkanDock->setWidget(vulkanPanel);addDockWidget(Qt::BottomDockWidgetArea,vulkanDock);vulkanDock->hide();viewMenu->addAction(vulkanDock->toggleViewAction());
        auto *exportAction=new QAction("Exportar modelo 3D…",this);exportAction->setObjectName("exportModelAction");file->insertAction(options,exportAction);file->insertSeparator(options);exportAction->setEnabled(false);
        connect(vulkanPanel->scene(),&MeshScene::collectionChanged,this,[vulkanPanel,exportAction]{exportAction->setEnabled(!vulkanPanel->scene()->entries().isEmpty());});
        connect(exportAction,&QAction::triggered,this,[this,vulkanPanel]{ExportDialog dialog(vulkanPanel->scene(),path_,vulkanPanel->visualStyle().face,this);dialog.exec();});
        auto *meshDock=new QDockWidget("Configuración 3D",this);meshDock->setObjectName("meshControlsDock");meshDock->setWidget(vulkanPanel->createMeshControls(view_));addDockWidget(Qt::LeftDockWidgetArea,meshDock);meshDock->hide();viewMenu->addAction(meshDock->toggleViewAction());auto *meshAction=tools->addAction("Configuración de malla 3D");connect(meshAction,&QAction::triggered,this,[meshDock]{meshDock->show();meshDock->raise();});
        auto *vulkanAction=tools->addAction("Vista 3D · Vulkan");connect(vulkanAction,&QAction::triggered,this,[vulkanDock]{vulkanDock->show();vulkanDock->raise();});
        auto *collectionDock=new QDockWidget("Colección de mallas",this);collectionDock->setObjectName("meshCollectionDock");collectionDock->setWidget(vulkanPanel->createMeshCollection());addDockWidget(Qt::LeftDockWidgetArea,collectionDock);collectionDock->hide();viewMenu->addAction(collectionDock->toggleViewAction());
        auto *uvDock=new QDockWidget("Vista UV",this);uvDock->setObjectName("uvDock");uvDock->setWidget(new UvPanel(vulkanPanel,uvDock));addDockWidget(Qt::BottomDockWidgetArea,uvDock);uvDock->hide();viewMenu->addAction(uvDock->toggleViewAction());auto *uvAction=tools->addAction("Vista UV");connect(uvAction,&QAction::triggered,this,[uvDock]{uvDock->show();uvDock->raise();});
        auto *scriptDock=new QDockWidget("Scripting · Python / C++",this);scriptDock->setObjectName("scriptDock");scriptPanel_=new ScriptPanel(view_,vulkanPanel,scriptDock);scriptDock->setWidget(scriptPanel_);addDockWidget(Qt::BottomDockWidgetArea,scriptDock);scriptDock->hide();viewMenu->addAction(scriptDock->toggleViewAction());auto *scriptAction=tools->addAction("Scripting · Python / C++");connect(scriptAction,&QAction::triggered,this,[scriptDock]{scriptDock->show();scriptDock->raise();});
        auto *about=menuBar()->addMenu("Acerca de");
        auto *docs=about->addAction("Documentación");connect(docs,&QAction::triggered,this,[this]{showDocs();});
        auto *disclaimer=about->addAction("Acerca de Binary Workbench…");disclaimer->setMenuRole(QAction::NoRole);
        connect(disclaimer,&QAction::triggered,this,[this]{QMessageBox::about(this,"Binary Workbench","Prototipo de análisis binario en C++ y Qt.\n\nLos valores son interpretaciones de bytes, no una identificación garantizada del formato. Edición por sobrescritura con cambios pendientes hasta Guardar.\n\nVista 3D Vulkan experimental. Scripting Python/C++ disponible. Inserción/borrado de bytes aún no implementados.");});
        auto *toolbar=addToolBar("Acciones comunes");toolbar->setObjectName("mainToolbar");toolbar->addAction(open);toolbar->addAction(reload);toolbar->addAction(save);toolbar->addAction(exportAction);toolbar->addSeparator();toolbar->addAction(copy);toolbar->addSeparator();toolbar->addAction(calculator);toolbar->addAction(options);
        connect(view_,&HexView::fileOpened,this,[this](const QString &path){path_=path;setWindowTitle(QFileInfo(path).fileName()+"[*] — Binary Workbench");if(!recent_->add(path))statusBar()->showMessage("No se pudo guardar la lista de recientes.",6000);});
        connect(view_,&HexView::idle,this,[this]{if(closing_){closing_=false;QTimer::singleShot(0,this,&QWidget::close);}});
        auto *activityDock = new QDockWidget("Actividad de los hilos", this);
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
        activityButton->setToolTip("Mostrar u ocultar el detalle de tareas en segundo plano");
        view_->addStatusWidget(activityButton);
        connect(activityButton, &QToolButton::toggled, activityDock, &QWidget::setVisible);
        connect(activityDock, &QDockWidget::visibilityChanged, activityButton, &QToolButton::setChecked);
        auto refreshActivity = [this, activityButton, activityText, activityDock] {
            const auto channels = view_->workerActivity();
            int active = 0, pending = 0;
            for (const auto &channel : channels) {
                active += channel.running; pending += channel.pending;
            }
            activityButton->setText(QString("Hilos · %1/8 ocupados · %2 pendientes · %3")
                .arg(active).arg(pending).arg(activityDock->isVisible() ? "Ocultar detalles" : "Ver detalles"));
            if (!activityDock->isVisible()) return;
            QString details = QString("Ocupación de tareas: %1 de 8 canales (%2 %). Pendientes: %3 de 3.\n")
                .arg(active).arg(active * 100 / 8).arg(pending);
            const int processors = QThread::idealThreadCount();
            details += QString("Procesadores lógicos detectados: %1. Interfaz: hilo principal separado.\n\n")
                .arg(processors > 0 ? QString::number(processors) : QString("no disponible"));
            const QStringList names{"Visor", "Inspector", "Copia", "Guardado", "Búsqueda", "Firmas", "Entropía", "Edición directa"};
            for (int i = 0; i < channels.size(); ++i) {
                const auto &c = channels[i];
                details += QString("%1 — %2%3 · pendientes: %4\n")
                    .arg(names[i], c.running ? "Ocupado" : "Libre", c.stopping ? " (cerrando)" : "")
                    .arg(c.pending ? 1 : 0);
                if (c.running) details += QString("  Solicitud: offset 0x%1 · %2 bytes · transcurrido: %3 ms\n")
                    .arg(QString::number(c.offset, 16).toUpper()).arg(c.length).arg(c.elapsedMs);
                details += QString("  Iniciadas: %1 · terminadas: %2 · errores: %3 · última duración: %4\n")
                    .arg(c.started).arg(c.completed).arg(c.errors)
                    .arg(c.completed ? QString::number(c.lastMs) + " ms" : "—");
                details += QString("  Resultados descartados: %1 · pendientes sustituidas: %2\n\n")
                    .arg(c.discarded).arg(c.replaced);
            }
            if(view_->saving())details+=QString("Guardado: %1 / %2 MiB procesados (pendiente de confirmar en disco).\n\n")
                .arg(double(view_->saveProgress())/(1024*1024),0,'f',2).arg(double(view_->fileSize())/(1024*1024),0,'f',2);
            details += "Cada canal tiene un pool de un hilo. Las lecturas conservan solo la última solicitud pendiente.\n"
                "Guardado, búsqueda, firmas y entropía admiten una tarea por canal, sin cola. El guardado pausa la edición.\n"
                "Los contadores abarcan esta sesión. Descartado: resultado ya obsoleto o recibido al cerrar.\n"
                "Sustituida: solicitud pendiente reemplazada por otra más reciente.\n"
                "La ocupación indica tareas en curso, no consumo de CPU ni número total de hilos del proceso.\n"
                "Incluye esperas de disco/caché y entrega del resultado a la interfaz; los tiempos son de extremo a extremo.\n"
                "Actualización cada 200 ms: las tareas muy breves pueden no verse activas, pero sí se contabilizan.";
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
            viewMenu->addSeparator();auto *reset=viewMenu->addAction("Restablecer distribución de paneles");reset->setObjectName("resetUiLayout");connect(reset,&QAction::triggered,this,[this]{uiLayout_->reset();});
        }
    }
    void openFile(const QString &path){
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
            statusBar()->showMessage("Esperando a las lecturas activas para cerrar…");view_->shutdown();event->ignore();return;
        }
        if(uiLayout_)uiLayout_->saveAndStop();
        view_->shutdown();event->accept();
    }
private:
    bool saveCurrent(bool saveAs){
        if(view_->directEditing()){QTimer::singleShot(30,this,[this,saveAs]{saveCurrent(saveAs);});return true;}
        QString destination=path_;
        if(saveAs)destination=QFileDialog::getSaveFileName(this,"Guardar archivo como",path_);
        return !destination.isEmpty()&&view_->saveFile(destination);
    }
    bool resolvePending(std::function<void()> next){
        if(!view_->dirty())return formatPanel_->confirmLeave();
        const auto answer=QMessageBox::warning(this,"Cambios sin guardar",
            "Hay modificaciones pendientes. ¿Quieres guardarlas antes de continuar?",
            QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
        if(answer==QMessageBox::Cancel)return false;
        if(answer==QMessageBox::Discard){if(!formatPanel_->confirmLeave())return false;view_->discardEdits();return true;}
        afterSave_=std::move(next);
        if(!saveCurrent(false))afterSave_={};
        return false;
    }
    void showOptions(){
        QDialog dialog(this);dialog.setWindowTitle("Opciones de visualización");auto *layout=new QVBoxLayout(&dialog);
        auto *help=new QLabel("Desplazamiento vertical continuo por todo el archivo.\nRueda: avanzar por filas. Arrastrar la barra: saltar a otra zona.\nIr: posición exacta en decimal o hexadecimal.\n\nCaché compartida: hasta 20 bloques de 1 MiB.\nSe cargan bajo demanda y se descartan los menos recientes.\n\nStride: bytes por fila (1–256). Cambiarlo conserva el cursor inicial.\nLos controles de interpretación están en la segunda barra.");help->setWordWrap(true);layout->addWidget(help);
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);
        connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        dialog.exec();
    }
    void showCalculator(){
        auto *dialog=new QDialog(this);dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle("Calculadora de offsets · UInt64");
        auto *layout=new QVBoxLayout(dialog);auto *form=new QFormLayout;
        auto *a=new QLineEdit;auto *b=new QLineEdit;auto *op=new QComboBox;op->addItems({"+","−","×","÷","%","AND","OR","XOR"});
        a->setPlaceholderText("Decimal o 0x…");b->setPlaceholderText("Decimal o 0x…");form->addRow("Primer valor",a);form->addRow("Operación",op);form->addRow("Segundo valor",b);layout->addLayout(form);
        auto *run=new QPushButton("Calcular");auto *result=new QPlainTextEdit;result->setReadOnly(true);layout->addWidget(run);layout->addWidget(result);
        auto calculate=[a,b,op,result]{result->setPlainText(calculateInteger(a->text(),op->currentText(),b->text()));};
        connect(run,&QPushButton::clicked,dialog,calculate);connect(a,&QLineEdit::returnPressed,dialog,calculate);connect(b,&QLineEdit::returnPressed,dialog,calculate);
        dialog->resize(430,320);dialog->show();
    }
    void showDocs(){
        auto *dialog=new QDialog(this);dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle("Documentación · Binary Workbench");
        auto *layout=new QVBoxLayout(dialog);auto *text=new QTextBrowser;layout->addWidget(text);
        QFile file(WORKBENCH_DOCUMENTATION);
        if(file.open(QIODevice::ReadOnly))text->setMarkdown(QString::fromUtf8(file.readAll()));else text->setPlainText("No se encontró README.md en la carpeta del proyecto.");
        dialog->resize(780,680);dialog->show();
    }
    std::unique_ptr<RecentFiles> recent_;
    UiLayout *uiLayout_=nullptr;
    ScriptPanel *scriptPanel_;HexView *view_;FormatPanel *formatPanel_;MagicPanel *magicPanel_;QString path_;bool closing_=false;std::function<void()> afterSave_;
};
#ifndef WORKBENCH_NO_MAIN
int main(int argc,char **argv){
    QApplication app(argc,argv);MainWindow window(UiLayout::defaultPath());window.show();const auto args=app.arguments();
    if(args.size()>1&&!args[1].startsWith("--"))window.openFile(args[1]);
    if(args.contains("--smoke-test"))QTimer::singleShot(1000,&window,&QWidget::close);
    return app.exec();
}

#endif
