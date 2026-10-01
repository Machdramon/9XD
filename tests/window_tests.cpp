// Exercise the actual window wiring, including unsaved-document dialogs.
#define WORKBENCH_NO_MAIN
#include "../src/main.cpp"
#include <QtTest>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QTableWidget>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QtMath>
#include <cmath>

class WindowTests : public QObject {
    Q_OBJECT
    static void answer(QMessageBox::StandardButton choice) {
        QTimer::singleShot(0, qApp, [choice] {
            auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if(box) box->button(choice)->click();
        });
    }
private slots:
    void nativeVulkanSmoke() {
        if(!qEnvironmentVariableIsSet("WORKBENCH_GPU_TEST"))
            QSKIP("Opt-in test: requires a native desktop and Vulkan GPU.");
        QVERIFY(QGuiApplication::platformName()=="windows" || QGuiApplication::platformName()=="cocoa");
        QTemporaryDir temporary;
        QString output=qEnvironmentVariable("WORKBENCH_GPU_OUTPUT",temporary.path());
        QVERIFY(QDir().mkpath(output));
        QMainWindow host;auto *dock=new QDockWidget("GPU test",&host);auto *renderer=new VulkanPanel;dock->setWidget(renderer);host.addDockWidget(Qt::RightDockWidgetArea,dock);host.resize(920,740);host.show();auto &panel=*renderer;
        auto source=readPage({WORKBENCH_SPHERE,0,-1,{},0});
        auto result=readMesh({WORKBENCH_SPHERE,0,source.size,source.modified,0},{},MeshSpec{});
        QVERIFY2(result.error.isEmpty(),qPrintable(result.error));
        QCOMPARE(result.mesh->vertices.size(),128);QCOMPARE(result.mesh->triangles.size()/3,252);
        panel.setMesh(result.mesh);
        auto *surface=panel.findChild<QWidget *>("vulkanSurface");
        auto *status=panel.findChild<QLabel *>("vulkanStatus");
        auto *mode=panel.findChild<QComboBox *>("meshSelectionMode");
        auto *labels=panel.findChild<QCheckBox *>("meshLabels");
        QTRY_VERIFY_WITH_TIMEOUT(status->text().startsWith("Vulkan ·"),15000);
        QTRY_VERIFY(status->text().contains("252 triángulos"));
        qInfo().noquote()<<status->text();
        auto capture=[&](QString name){qInfo().noquote()<<name;QTest::qWait(400);return status->text().startsWith("Vulkan ·");}; // Capture GPU frames with VK_LAYER_LUNARG_screenshot; QWidget grabs omit native Vulkan.
        QVERIFY(capture("sphere-initial"));
        mode->setCurrentIndex(1);labels->setChecked(true);
        QTest::mouseClick(surface,Qt::LeftButton,Qt::NoModifier,surface->rect().center());
        QVERIFY(panel.scene()->selection().index>=0);
        QVERIFY(capture("sphere-selected"));
        QPoint center=surface->rect().center();
        QTest::mousePress(surface,Qt::LeftButton,Qt::NoModifier,center);
        QMouseEvent move(QEvent::MouseMove,QPointF(center+QPoint(85,35)),QPointF(surface->mapToGlobal(center+QPoint(85,35))),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(surface,&move);
        QTest::mouseRelease(surface,Qt::LeftButton,Qt::NoModifier,center+QPoint(85,35));
        QWheelEvent wheel(QPointF(center),QPointF(surface->mapToGlobal(center)),QPoint(),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(surface,&wheel);
        QVERIFY(capture("sphere-camera"));
        auto before=status->text();host.resize(1050,800);
        QTRY_VERIFY(status->text()!=before);QVERIFY(status->text().startsWith("Vulkan ·"));
        QVERIFY(capture("sphere-resized"));
        panel.hide();QTest::qWait(100);panel.show();QTest::qWait(200);
        QVERIFY(status->text().startsWith("Vulkan ·"));
        dock->setFloating(true);dock->resize(900,680);QTest::qWait(400);
        QVERIFY(status->text().startsWith("Vulkan ·"));
        dock->setFloating(false);QTest::qWait(400);
        QVERIFY(status->text().startsWith("Vulkan ·"));
        auto second=panel.scene()->add(result.mesh,"Second");
        QTRY_VERIFY(status->text().contains("504 triángulos"));
        panel.scene()->setVisible(second,false);
        QTRY_VERIFY(status->text().contains("252 triángulos"));
        panel.scene()->remove(second);
        panel.scene()->select(panel.scene()->active(),MeshElement::Face,94);
        UvPanel uv(&panel);uv.resize(700,600);uv.show();QTest::qWait(200);
        QVERIFY(uv.findChild<QLabel *>("uvSelectionInfo")->text().contains("Cara 94"));
        QVERIFY(uv.grab().save(output+"/sphere-uv.png"));
        for(auto format:{ModelFormat::Glb,ModelFormat::Obj,ModelFormat::Stl}) {
            ModelExportOptions options;options.format=format;options.protectedPaths={WORKBENCH_SPHERE};
            QString suffix=format==ModelFormat::Glb?"glb":format==ModelFormat::Obj?"obj":"stl";
            auto exported=exportModel(output+"/sphere."+suffix,{{"Sphere",WORKBENCH_SPHERE,result.mesh}},options);
            QVERIFY2(exported.ok,qPrintable(exported.error));QCOMPARE(exported.vertices,quint64(128));QCOMPARE(exported.faces,quint64(252));
        }
    }


    void recentFilesMenuAndSuccessfulOpens() {
        QTemporaryDir dir;QString a=dir.filePath("a.bin"),b=dir.filePath("b.bin");for(auto path:{a,b}){QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));f.write("1234");}
        a=QFileInfo(a).canonicalFilePath();b=QFileInfo(b).canonicalFilePath();
        MainWindow window(dir.filePath("ui-layout.json"));window.show();auto *view=window.findChild<HexView *>();auto *menu=window.findChild<QMenu *>("recentFilesMenu");QVERIFY(menu);QSignalSpy opened(view,&HexView::fileOpened);
        window.openFile(a);QTRY_VERIFY(!view->busy());window.openFile(b);QTRY_VERIFY(!view->busy());QCOMPARE(opened.count(),2);RecentFiles stored(dir.filePath("recent-files.json"));QCOMPARE(stored.paths(),QStringList({b,a}));
        QMetaObject::invokeMethod(menu,"aboutToShow");QCOMPARE(menu->actions().first()->data().toString(),b);menu->actions()[1]->trigger();QTRY_VERIFY(!view->busy());QCOMPARE(RecentFiles(dir.filePath("recent-files.json")).paths().first(),a);
        QMetaObject::invokeMethod(menu,"aboutToShow");auto *clear=menu->findChild<QAction *>("clearRecentFiles");QVERIFY(clear);clear->trigger();view->selectMatch(1,1);QTRY_VERIFY(!view->busy());QVERIFY(RecentFiles(dir.filePath("recent-files.json")).paths().isEmpty());
        window.openFile(dir.filePath("missing.bin"));QTRY_VERIFY(!view->busy());QVERIFY(RecentFiles(dir.filePath("recent-files.json")).paths().isEmpty());window.close();
    }
    void exportDialogAndMenu() {
        QTemporaryDir dir;MainWindow window;window.show();auto *renderer=window.findChild<VulkanPanel *>("vulkanPanel");QVERIFY(renderer);auto *action=window.findChild<QAction *>("exportModelAction");QVERIFY(action);QVERIFY(!action->isEnabled());auto source=readPage({WORKBENCH_SPHERE,0,-1,{},0});auto result=readMesh({WORKBENCH_SPHERE,0,source.size,source.modified,0},{},MeshSpec{});QVERIFY(result.error.isEmpty());renderer->scene()->add(result.mesh,"Esfera",WORKBENCH_SPHERE);QVERIFY(action->isEnabled());
        ExportDialog dialog(renderer->scene(),WORKBENCH_SPHERE,Qt::blue,&window);dialog.show();auto *format=dialog.findChild<QComboBox *>("exportFormat");auto *scale=dialog.findChild<QDoubleSpinBox *>("exportScale");auto *axes=dialog.findChild<QComboBox *>("exportAxes");auto *save=dialog.findChild<QPushButton *>("exportSave");auto *uv=dialog.findChild<QCheckBox *>("exportUvs");QVERIFY(format&&scale&&axes&&save&&uv);QCOMPARE(dialog.selectedMeshes().size(),1);
        format->setCurrentIndex(2);QVERIFY(!uv->isEnabled());QVERIFY(dialog.findChild<QLabel *>("exportSummary")->text().contains("pierde"));format->setCurrentIndex(0);QVERIFY(uv->isEnabled());scale->setValue(2);axes->setCurrentText("YZX");QCOMPARE(dialog.options().axes,(std::array<int,3>{1,2,0}));QCOMPARE(dialog.options().scale,2.0);
        QString path=dir.filePath("sphere.glb");dialog.startExport(path);QTRY_VERIFY_WITH_TIMEOUT(save->isEnabled(),15000);QVERIFY2(dialog.findChild<QLabel *>("exportStatus")->text().contains("completada"),qPrintable(dialog.findChild<QLabel *>("exportStatus")->text()));QVERIFY(QFileInfo(path).size()>1000);
        if(qEnvironmentVariableIsSet("WORKBENCH_EXPORT_SCREENSHOT"))dialog.grab().save(qEnvironmentVariable("WORKBENCH_EXPORT_SCREENSHOT"));
        dialog.startExport(WORKBENCH_SPHERE);QTRY_VERIFY_WITH_TIMEOUT(save->isEnabled(),15000);QVERIFY(dialog.findChild<QLabel *>("exportStatus")->text().contains("binario"));dialog.reject();renderer->scene()->clear();QVERIFY(!action->isEnabled());window.close();
    }
    void layoutAutosaveDuringContinuousEvents() {
        QTemporaryDir dir;QString path=dir.filePath("layout.json");MainWindow window(path);window.show();QTimer events;events.setInterval(30);connect(&events,&QTimer::timeout,&window,[&window]{QEvent event(QEvent::LayoutRequest);QApplication::sendEvent(&window,&event);});events.start();
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(path),1500);QVERIFY(events.isActive());events.stop();window.close();
    }
    void layoutPersistsAcrossWindows() {
        QTemporaryDir dir;auto path=dir.filePath("settings/ui-layout.json");QSize floatingSize;
        {
            MainWindow window(path);window.show();auto *uv=window.findChild<QDockWidget *>("uvDock");auto *magic=window.findChild<QDockWidget *>("magicDock");auto *inspector=window.findChild<QDockWidget *>("inspectorDock");
            window.addDockWidget(Qt::LeftDockWidgetArea,magic);magic->show();uv->setFloating(true);uv->show();uv->resize(620,510);uv->move(80,90);inspector->hide();
            QTRY_VERIFY(QFile::exists(path));floatingSize=uv->size();
            // Reopening binary data must not reset the current layout.
            window.openFile(WORKBENCH_SPHERE);auto *view=window.findChild<HexView *>();QTRY_VERIFY(!view->busy());QVERIFY(uv->isFloating());QVERIFY(inspector->isHidden());
            window.close();QTRY_VERIFY(!window.isVisible());
        }
        {
            MainWindow window(path);window.show();auto *uv=window.findChild<QDockWidget *>("uvDock");auto *magic=window.findChild<QDockWidget *>("magicDock");
            QVERIFY(uv->isFloating());QVERIFY(uv->isVisible());QCOMPARE(uv->size(),floatingSize);QCOMPARE(window.dockWidgetArea(magic),Qt::LeftDockWidgetArea);QVERIFY(magic->isVisible());QVERIFY(window.findChild<QDockWidget *>("inspectorDock")->isHidden());
            auto *reset=window.findChild<QAction *>("resetUiLayout");QVERIFY(reset);reset->trigger();QVERIFY(uv->isHidden());QVERIFY(!uv->isFloating());window.close();
        }
        {
            QFile broken(path);QVERIFY(broken.open(QIODevice::WriteOnly));broken.write("not valid json");broken.close();MainWindow window(path);window.show();QVERIFY(window.findChild<QDockWidget *>("uvDock")->isHidden());QVERIFY(window.findChild<QDockWidget *>("inspectorDock")->isVisible());window.close();QVERIFY(broken.open(QIODevice::ReadOnly));QVERIFY(QJsonDocument::fromJson(broken.readAll()).isObject());
        }
    }
    void uvOverlapsAndMissingCoordinates() {
        VulkanPanel renderer;UvPanel panel(&renderer);panel.resize(600,600);panel.show();auto *canvas=panel.findChild<UvCanvas *>("uvCanvas");auto mesh=std::make_shared<MeshData>();for(auto p:{QPointF(.1,.1),QPointF(.8,.1),QPointF(.2,.8),QPointF(.1,.1)}){MeshVertex v{};v.position[0]=v.uv[0]=p.x();v.position[1]=v.uv[1]=p.y();mesh->vertices.append(v);}mesh->triangles={0,1,2};int id=renderer.scene()->add(mesh);canvas->fit();auto pixel=canvas->uvToScreen({.1,.1}).toPoint();QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,pixel);QCOMPARE(renderer.scene()->selection().index,0);QTest::mouseClick(canvas,Qt::LeftButton,Qt::AltModifier,pixel);QCOMPARE(renderer.scene()->selection().index,3);
        int duplicate=renderer.scene()->add(std::make_shared<MeshData>(*mesh));panel.findChild<QCheckBox *>("uvActiveOnly")->setChecked(false);QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,pixel);QCOMPARE(renderer.scene()->selection().mesh,duplicate);QTest::mouseClick(canvas,Qt::LeftButton,Qt::AltModifier,pixel);QCOMPARE(renderer.scene()->selection().mesh,duplicate);QCOMPARE(renderer.scene()->selection().index,3);QTest::mouseClick(canvas,Qt::LeftButton,Qt::AltModifier,pixel);QCOMPARE(renderer.scene()->selection().mesh,id);QCOMPARE(renderer.scene()->selection().index,0);renderer.scene()->remove(duplicate);panel.findChild<QCheckBox *>("uvActiveOnly")->setChecked(true);
        auto missing=std::make_shared<MeshData>(*mesh);missing->hasUvs=false;renderer.scene()->replace(id,missing);QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,pixel);QCOMPARE(renderer.scene()->selection().index,-1);QVERIFY(panel.findChild<QLabel *>("uvSelectionInfo")->text().contains("Sin coordenadas UV"));
        auto source=readPage({WORKBENCH_SPHERE,0,-1,{},0});auto sphere=readMesh({WORKBENCH_SPHERE,0,source.size,source.modified,0},{},MeshSpec{});QVERIFY(sphere.error.isEmpty());renderer.scene()->replace(id,sphere.mesh);renderer.scene()->select(id,MeshElement::Face,42);canvas->fit();if(qEnvironmentVariableIsSet("WORKBENCH_UV_SPHERE_SCREENSHOT"))panel.grab().save(qEnvironmentVariable("WORKBENCH_UV_SPHERE_SCREENSHOT"));
    }
    void meshCollectionAndUvSynchronization() {
        VulkanPanel renderer;renderer.resize(1000,650);renderer.show();UvPanel uvPanel(&renderer);uvPanel.resize(600,600);uvPanel.show();auto *canvas=uvPanel.findChild<UvCanvas *>("uvCanvas");QVERIFY(canvas);
        auto mesh=std::make_shared<MeshData>();for(auto xy:{QPointF(.1,.1),QPointF(.8,.1),QPointF(.2,.8)}){MeshVertex v{};v.position[0]=xy.x();v.position[1]=xy.y();v.uv[0]=xy.x();v.uv[1]=xy.y();mesh->vertices.append(v);}mesh->triangles={0,1,2};auto other=std::make_shared<MeshData>(*mesh);for(auto &v:other->vertices){v.position[0]+=2;v.uv[0]+=2;}
        auto *scene=renderer.scene();int first=scene->add(mesh,"Primera"),second=scene->add(other,"Segunda");QCOMPARE(scene->entries().size(),2);
        std::unique_ptr<QWidget> collection(renderer.createMeshCollection());collection->show();auto *tree=collection->findChild<QTreeWidget *>("meshCollection");QCOMPARE(tree->topLevelItemCount(),2);
        scene->select(first,MeshElement::Vertex,1);QCOMPARE(uvPanel.findChild<QLabel *>("uvSelectionInfo")->text().contains("Vértice 1"),true);canvas->fit();
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,canvas->uvToScreen({.2,.8}).toPoint());QCOMPARE(scene->selection().mesh,first);QCOMPARE(scene->selection().index,2);QVERIFY(renderer.findChild<QLabel *>("meshSelectionInfo")->text().contains("Vértice 2"));
        uvPanel.findChild<QComboBox *>("uvSelectionMode")->setCurrentIndex(1);QCOMPARE(renderer.findChild<QComboBox *>("meshSelectionMode")->currentIndex(),1);QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,canvas->uvToScreen({.35,.3}).toPoint());QCOMPARE(scene->selection().kind,MeshElement::Face);QCOMPARE(scene->selection().index,0);
        scene->select(second,MeshElement::Vertex,0);canvas->fit();QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,canvas->uvToScreen({2.8,.1}).toPoint());QCOMPARE(scene->selection().mesh,second);QCOMPARE(scene->selection().index,1);
        tree->topLevelItem(1)->setCheckState(0,Qt::Unchecked);QVERIFY(!scene->entry(second)->visible);QCOMPARE(scene->selection().index,-1);QCOMPARE(scene->assemble().spans.size(),1);
        scene->setVisible(second,true);scene->setActive(first);tree->topLevelItem(0)->setText(0,"Renombrada");QCOMPARE(scene->entry(first)->name,QString("Renombrada"));
        // A 3D hit chooses the mesh owning that triangle, then updates UV selection.
        auto *surface=renderer.findChild<QWidget *>("vulkanSurface");renderer.findChild<QComboBox *>("meshSelectionMode")->setCurrentIndex(1);auto assembly=scene->assemble();MeshDebug debug(*assembly.data);float yaw=qDegreesToRadians(35.f),pitch=qDegreesToRadians(20.f);QVector3D eye(3.5f*std::cos(pitch)*std::sin(yaw),3.5f*std::sin(pitch),3.5f*std::cos(pitch)*std::cos(yaw));QMatrix4x4 projection;projection.perspective(45,float(surface->width())/surface->height(),.01f,100.f);QMatrix4x4 correction;correction(1,1)=-1;correction(2,2)=.5f;correction(2,3)=.5f;QMatrix4x4 camera;camera.lookAt(eye,{0,0,0},{0,1,0});auto labels=debug.labels(correction*projection*camera,eye,surface->size(),MeshElement::Face,1,1);QVERIFY(!labels.isEmpty());QTest::mouseClick(surface,Qt::LeftButton,Qt::NoModifier,labels.first().pixel.toPoint());QCOMPARE(scene->selection().mesh,second);QCOMPARE(scene->selection().index,0);QVERIFY(uvPanel.findChild<QLabel *>("uvSelectionInfo")->text().contains("Segunda"));
        if(qEnvironmentVariableIsSet("WORKBENCH_UV_SCREENSHOT")){canvas->fit();uvPanel.grab().save(qEnvironmentVariable("WORKBENCH_UV_SCREENSHOT"));}
        collection->findChild<QPushButton *>("removeMesh")->click();QVERIFY(!scene->entry(second));QCOMPARE(scene->active(),first);QCOMPARE(scene->entries().size(),1);
        scene->clear();QVERIFY(uvPanel.findChild<QLabel *>("uvSelectionInfo")->text().contains("vacía"));
    }
    void meshDebugControlsAndSelection() {
        VulkanPanel panel;panel.resize(1000,650);panel.show();auto source=readPage({WORKBENCH_SPHERE,0,-1,{},0});auto result=readMesh({WORKBENCH_SPHERE,0,source.size,source.modified,0},{},MeshSpec{});QVERIFY(result.error.isEmpty());panel.setMesh(result.mesh);
        auto *mode=panel.findChild<QComboBox *>("meshSelectionMode");auto *labels=panel.findChild<QCheckBox *>("meshLabels");auto *surface=panel.findChild<QWidget *>("vulkanSurface");auto *info=panel.findChild<QLabel *>("meshSelectionInfo");QVERIFY(mode&&labels&&surface&&info);
        mode->setCurrentIndex(1);labels->setChecked(true);QTest::mouseClick(surface,Qt::LeftButton,Qt::NoModifier,surface->rect().center());QVERIFY(info->text().contains("Cara"));QVERIFY(info->text().contains("Offsets:"));
        mode->setCurrentIndex(2);QTest::mouseClick(surface,Qt::LeftButton,Qt::NoModifier,surface->rect().center());QVERIFY(info->text().contains("128 vértices"));
        panel.setMesh({});QVERIFY(info->text().contains("Colección vacía"));
    }
    void embeddedScriptingPythonAndCpp() {
        MainWindow window;window.show();window.openFile(WORKBENCH_SPHERE);auto *view=window.findChild<HexView *>();QTRY_VERIFY(!view->busy());QString error;QVERIFY(view->replaceSelection("C",error));
        auto *dock=window.findChild<QDockWidget *>("scriptDock");QVERIFY(dock);dock->show();auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");auto *console=window.findChild<QPlainTextEdit *>("scriptConsole");auto *run=window.findChild<QPushButton *>("scriptRun");auto *stop=window.findChild<QPushButton *>("scriptStop");auto *language=window.findChild<QComboBox *>("scriptLanguage");auto *renderer=static_cast<VulkanPanel *>(window.findChild<QDockWidget *>("vulkanDock")->widget());
        QString python="import workbench as bw\nassert bw.read(0, 4) == b'CWS1'\nprint('PYTHON_OK', bw.size)\nbw.set_mesh([[-1,0,0],[1,0,0],[0,1,0]],[0,1,2])\n";
        editor->setPlainText(python);run->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),15000);QVERIFY2(console->toPlainText().contains("PYTHON_OK"),qPrintable(console->toPlainText()));QVERIFY(renderer->meshSnapshot());QCOMPARE(renderer->meshSnapshot()->vertices.size(),3);
        language->setCurrentIndex(1);editor->setPlainText("#include \"workbench.hpp\"\nvoid run(){auto bytes=bw::read(0,1); if(bytes[0]!='C') throw std::runtime_error(\"read mismatch\"); auto mesh=bw::mesh(); mesh.vertices[0].position[0]=2; bw::set_mesh(mesh); std::cout << \"CPP_OK\\n\";}\n");run->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),30000);QVERIFY2(console->toPlainText().contains("CPP_OK"),qPrintable(console->toPlainText()));QCOMPARE(renderer->meshSnapshot()->vertices[0].position[0],2.f);
        editor->setPlainText("this is not C++");run->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),30000);QVERIFY(console->toPlainText().contains("Error de compilación"));
        language->setCurrentIndex(0);QCOMPARE(editor->toPlainText(),python);editor->setPlainText("raise RuntimeError('ERROR_PRUEBA')");run->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),15000);QVERIFY(console->toPlainText().contains("ERROR_PRUEBA"));
        editor->setPlainText("print('LOOP_STARTED', flush=True)\nwhile True: pass");run->click();QTRY_VERIFY_WITH_TIMEOUT(console->toPlainText().contains("LOOP_STARTED"),15000);stop->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),5000);QVERIFY(console->toPlainText().contains("Ejecución detenida"));
        editor->setPlainText("import time, workbench as bw\nprint('STALE_STARTED', flush=True)\ntime.sleep(.4)\nbw.set_mesh([[0,0,0],[1,0,0],[0,1,0]],[0,1,2])");run->click();QTRY_VERIFY_WITH_TIMEOUT(console->toPlainText().contains("STALE_STARTED"),15000);QVERIFY(view->replaceSelection("D",error));QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),15000);QVERIFY(console->toPlainText().contains("Malla descartada"));
        if(qEnvironmentVariableIsSet("WORKBENCH_SCRIPT_SCREENSHOT"))window.grab().save(qEnvironmentVariable("WORKBENCH_SCRIPT_SCREENSHOT"));
        answer(QMessageBox::Cancel);window.close();QVERIFY(window.isVisible());
    }
    void meshControlsDecodeFixture() {
        MainWindow window;window.show();window.openFile(WORKBENCH_SPHERE);auto *view=window.findChild<HexView *>();QTRY_VERIFY(!view->busy());
        auto *dock=window.findChild<QDockWidget *>("meshControlsDock");QVERIFY(dock);dock->show();auto *button=window.findChild<QPushButton *>("renderMesh");QVERIFY(button);button->click();auto *status=window.findChild<QLabel *>("meshStatus");QTRY_VERIFY(status->text().contains("128 vértices"));QVERIFY(status->text().contains("252 triángulos"));
        auto *renderer=static_cast<VulkanPanel *>(window.findChild<QDockWidget *>("vulkanDock")->widget());QCOMPARE(renderer->scene()->entries().size(),1);auto firstData=renderer->meshSnapshot();button->click();QTRY_COMPARE(renderer->scene()->entries().size(),2);QCOMPARE(renderer->scene()->entries().first().data.get(),firstData.get());auto secondData=renderer->meshSnapshot();window.findChild<QComboBox *>("meshDestination")->setCurrentIndex(1);button->click();QTRY_VERIFY(button->isEnabled());QCOMPARE(renderer->scene()->entries().size(),2);QVERIFY(renderer->meshSnapshot()!=secondData);
        if(qEnvironmentVariableIsSet("WORKBENCH_MESH_SCREENSHOT"))window.grab().save(qEnvironmentVariable("WORKBENCH_MESH_SCREENSHOT"));
        auto *offset=window.findChild<QLineEdit *>("meshVertexOffset");offset->setText("0xFFFF");button->click();QTRY_VERIFY(button->isEnabled());QVERIFY(status->text().contains("fuera del archivo"));window.close();QTRY_VERIFY(!window.isVisible());
    }
    void vulkanPanelFailsGracefullyWithoutDesktop() {
        MainWindow window;window.show();auto *dock=window.findChild<QDockWidget *>("vulkanDock");QVERIFY(dock);dock->show();
        auto *status=window.findChild<QLabel *>("vulkanStatus");QVERIFY(status);QTRY_VERIFY(status->text().contains("No se pudo iniciar"));dock->hide();dock->show();QVERIFY(window.isVisible());window.close();QTRY_VERIFY(!window.isVisible());
    }
    void entropyNavigation() {
        QTemporaryDir dir;auto path=dir.filePath("entropy.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));QByteArray data(4096,0);for(int i=0;i<4096;++i)data.append(char(i%256));file.write(data);file.close();
        MainWindow window;window.show();window.openFile(path);auto *view=window.findChild<HexView *>();QTRY_VERIFY(!view->busy());
        auto *button=window.findChild<QPushButton *>("entropyAnalyze");QTRY_VERIFY(button->isEnabled());QSignalSpy spy(view,&HexView::entropyFinished);button->click();QTRY_COMPARE(spy.size(),1);
        auto *status=window.findChild<QLabel *>("entropyStatus");QVERIFY(status->text().contains("2 regiones"));auto *graph=window.findChild<QWidget *>("entropyGraph");QVERIFY(graph);
        QTest::mouseClick(graph,Qt::LeftButton,Qt::NoModifier,QPoint(38+(graph->width()-52)*3/4,graph->height()/2));QCOMPARE(view->selection().anchor,qint64(4096));QCOMPARE(view->selection().cursor,qint64(8191));QTRY_VERIFY(!view->busy());
        if(qEnvironmentVariableIsSet("WORKBENCH_ENTROPY_SCREENSHOT"))window.grab().save(qEnvironmentVariable("WORKBENCH_ENTROPY_SCREENSHOT"));
        QString error;QVERIFY(view->replaceSelection(QByteArray(4096,'A'),error));QVERIFY(status->text().contains("cambió"));view->discardEdits();window.close();QTRY_VERIFY(!window.isVisible());
    }
    void magicNavigationAndMarking() {
        QTemporaryDir dir;auto path=dir.filePath("magic.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(QByteArray(32,'X')+QByteArray::fromHex("89504e470d0a1a0a")+QByteArray(32,'X'));file.close();
        MainWindow window;window.show();window.openFile(path);auto *view=window.findChild<HexView *>();QTRY_VERIFY(!view->busy());
        auto *dock=window.findChild<QDockWidget *>("magicDock");dock->show();auto *scan=window.findChild<QPushButton *>("magicScan");QTRY_VERIFY(scan->isEnabled());scan->click();
        auto *results=window.findChild<QTableWidget *>("magicResults");QTRY_COMPARE(results->rowCount(),1);QCOMPARE(results->item(0,0)->text(),QString("PNG"));
        results->selectRow(0);results->itemClicked(results->item(0,1));QCOMPARE(view->selection().anchor,qint64(32));QCOMPARE(view->selection().cursor,qint64(39));
        for(auto *b:window.findChildren<QPushButton *>())if(b->text().startsWith("Enmarcar filas"))b->click();QCOMPARE(view->formatRules().size(),1);QVERIFY(view->formatAt(32).style.background.isValid());QVERIFY(!view->formatAt(40).style.background.isValid());
        QTRY_VERIFY(!view->busy());
        if(qEnvironmentVariableIsSet("WORKBENCH_TEST_SCREENSHOT"))window.grab().save(qEnvironmentVariable("WORKBENCH_TEST_SCREENSHOT"));
        QString error;QVERIFY(view->replaceSelection(QByteArray(8,'Z'),error));QCOMPARE(results->rowCount(),0);
        view->discardEdits();answer(QMessageBox::Discard);window.close();QTRY_VERIFY(!window.isVisible());
    }
    void formattingControlsAndUnsavedRules() {
        QTemporaryDir dir;const auto path=dir.filePath("format.bin");QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));file.write(QByteArray(128,'A'));file.close();
        MainWindow window;window.show();window.openFile(path);
        auto *view=window.findChild<HexView *>();QTRY_VERIFY(!view->busy());
        QVERIFY(!window.findChild<QAction *>("applyByteFormat"));QCOMPARE(view->formatRules().size(),0);
        for(auto *b:window.findChildren<QToolButton *>())if(b->toolTip()=="Añadir regla")b->click();
        for(auto *b:window.findChildren<QToolButton *>())if(b->toolTip()=="Guardar regla")b->click();
        QCOMPARE(view->formatRules().size(),1);QVERIFY(view->formatAt(0).style.background.isValid());QVERIFY(!view->dirty());
        auto *tree=window.findChild<QTreeWidget *>("formatRules");QVERIFY(tree);
        tree->setCurrentItem(tree->topLevelItem(0));
        auto before=view->formatRules()[0];window.findChild<QCheckBox *>("formatBold")->setChecked(true);QCOMPARE(view->formatRules().size(),1);QVERIFY(view->formatRules()[0].style.bold);QCOMPARE(view->formatRules()[0].start,before.start);QCOMPARE(view->formatRules()[0].length,before.length);
        auto *color=window.findChild<QLineEdit *>("formatColor0");color->setText("#123456");color->textEdited("#123456");QCOMPARE(view->formatRules()[0].style.foreground,QColor("#123456"));
        window.findChild<QCheckBox *>("formatUnderline")->setChecked(true);auto *underline=window.findChild<QLineEdit *>("formatColor3");underline->setText("#ABCDEF");underline->textEdited("#ABCDEF");QCOMPARE(view->formatRules()[0].style.underlineColor,QColor("#ABCDEF"));
        auto *jump=window.findChild<QLineEdit *>("formatJumpTo");jump->setText("0x20");jump->editingFinished();QCOMPARE(view->formatRules()[0].jumpTo,qint64(32));tree->itemClicked(tree->topLevelItem(0),0);QCOMPARE(view->selection().anchor,qint64(32));QCOMPARE(view->formatRules()[0].start,before.start);
        FormatRule restored;QVERIFY(decodeRule(encodeRule(view->formatRules()[0]),restored));QCOMPARE(restored.jumpTo,qint64(32));QCOMPARE(restored.style.underlineColor,QColor("#ABCDEF"));
        jump->setText("0xFFFF");jump->editingFinished();QCOMPARE(view->formatRules()[0].jumpTo,qint64(32));
        tree->clearSelection();window.findChild<QCheckBox *>("formatBold")->setChecked(false);QVERIFY(view->formatRules()[0].style.bold);QCOMPARE(view->formatRules().size(),1);
        tree->topLevelItem(0)->setCheckState(0,Qt::Unchecked);
        QVERIFY(!view->formatAt(0).style.background.isValid());
        tree->topLevelItem(0)->setCheckState(0,Qt::Checked);
        QVERIFY(view->formatAt(0).style.background.isValid());
        for(auto *button:window.findChildren<QToolButton *>())if(button->toolTip()=="Añadir regla")button->click();
        auto *repeat=window.findChild<QLineEdit *>("ruleRepetitions");repeat->setText("4");
        for(auto *button:window.findChildren<QToolButton *>())if(button->toolTip()=="Guardar regla")button->click();
        QCOMPARE(view->formatRules().size(),2);QVERIFY(view->formatAt(16).style.background.isValid());
        tree->setCurrentItem(tree->topLevelItem(0));QVERIFY(view->formatRules()[0].style.bold);
        auto *formatDock=window.findChild<QDockWidget *>("formatRulesDock");formatDock->raise();QCoreApplication::processEvents();
        if(qEnvironmentVariableIsSet("WORKBENCH_FORMAT_SCREENSHOT"))window.grab().save(qEnvironmentVariable("WORKBENCH_FORMAT_SCREENSHOT"));
        answer(QMessageBox::Cancel);window.close();QVERIFY(window.isVisible());QCOMPARE(view->formatRules().size(),2);
        answer(QMessageBox::Discard);window.close();QTRY_VERIFY(!window.isVisible());
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray(128,'A'));
    }
    void integratedMonitorAndPendingChanges() {
        QTemporaryDir dir;const auto path=dir.filePath("edit.bin");
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write("abcd");file.close();
        MainWindow window;window.show();window.openFile(path);
        auto *view=window.findChild<HexView *>();QVERIFY(view);QTRY_VERIFY(!view->busy());
        auto *toggle=window.findChild<QToolButton *>("threadActivityToggle");
        auto *label=window.findChild<QLabel *>("fileStatus");
        auto *dock=window.findChild<QDockWidget *>("threadActivityDock");
        QVERIFY(toggle);QVERIFY(label);QVERIFY(dock);
        QCOMPARE(toggle->parentWidget(),label->parentWidget());
        toggle->setChecked(true);QVERIFY(dock->isVisible());
        dock->hide();QVERIFY(!toggle->isChecked());
        QTRY_VERIFY(!view->busy());
        QString error;QVERIFY(view->replaceSelection("Z",error));
        answer(QMessageBox::Cancel);window.close();QVERIFY(window.isVisible());QVERIFY(view->dirty());
        answer(QMessageBox::Cancel);window.openFile(path);QVERIFY(view->dirty());
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("abcd"));file.close();
        answer(QMessageBox::Save);window.close();
        QTRY_VERIFY(!window.isVisible());
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("Zbcd"));
    }
    void discardBeforeReload() {
        QTemporaryDir dir;const auto path=dir.filePath("discard.bin");
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write("original");file.close();
        MainWindow window;window.show();window.openFile(path);
        auto *view=window.findChild<HexView *>();QTRY_VERIFY(!view->busy());
        QString error;QVERIFY(view->replaceSelection("X",error));
        answer(QMessageBox::Discard);window.openFile(path);
        QTRY_VERIFY(!view->busy());QVERIFY(!view->dirty());
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("original"));
        window.close();QTRY_VERIFY(!window.isVisible());
    }
};
QTEST_MAIN(WindowTests)
#include "window_tests.moc"
