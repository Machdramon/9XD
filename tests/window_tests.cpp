#include "../src/orthographic_grid.h"
#include <QTcpServer>
#include <QTcpSocket>
// Exercise the actual window wiring, including unsaved-document dialogs.
#define WORKBENCH_NO_MAIN
#include "../src/main.cpp"
#include <QtTest>
#include "../src/script_library_manager.h"
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QTableWidget>
#include <QInputDialog>
#include <QHeaderView>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QtMath>
#include <QScopeGuard>
#include <QSplitter>
#include <cmath>

class WindowTests : public QObject {
    Q_OBJECT
    static void answer(QMessageBox::StandardButton choice) {
        QTimer::singleShot(0, qApp, [choice] {
            auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if(box) box->button(choice)->click();
        });
    }
    static QByteArray arcpFixture(bool topology=false){
        QByteArray bytes(160,0);std::memcpy(bytes.data(),"ARCP",4);qToLittleEndian<quint32>(1,bytes.data()+4);qToLittleEndian<quint32>(160,bytes.data()+132);QByteArray payload;
        auto word=[&](quint32 n){char value[4];qToLittleEndian(n,value);payload.append(value,4);};word(0x01000101);word(0x68038003);float positions[]={0,0,0,1,0,0,0,1,0};for(float f:positions){quint32 bits;std::memcpy(&bits,&f,4);word(bits);}word(0x64038067);for(int i=0;i<6;++i)word(0);word(0x6e038099);for(int i=0;i<3;++i)word(0x007f0000);if(topology){word(0x20000000);word(0x3f3f3f3f);word(0x7208c067);word(0x00000181);word(0);}while(payload.size()%16)word(0);QByteArray tag(16,0);qToLittleEndian<quint32>(0x60000000|payload.size()/16,tag.data());bytes+=tag;bytes+=payload;return bytes;
    }
private slots:
    void embeddedBrandingAndAbout(){
        QVERIFY(!QPixmap(":/branding/logo-sticker.png").isNull());QTemporaryDir dir;MainWindow window(dir.filePath("layout.json"));auto *action=window.findChild<QAction *>("showAbout9XD");QVERIFY(action);action->trigger();auto *dialog=window.findChild<QDialog *>("about9xdDialog");QVERIFY(dialog);QVERIFY(dialog->isVisible());QVERIFY(dialog->windowFlags().testFlag(Qt::FramelessWindowHint));QVERIFY(dialog->testAttribute(Qt::WA_TranslucentBackground));QCOMPARE(dialog->findChildren<QLabel *>().size(),1);QVERIFY(dialog->findChild<QToolButton *>("aboutClose"));if(QFile::exists(":/branding/about-splash.png"))QVERIFY(QImage(":/branding/about-splash.png").hasAlphaChannel());QCOMPARE(QImage(":/branding/logo-sticker.png").pixelColor(0,0).alpha(),0);QCOMPARE(dialog->windowTitle(),QString("About 9XD"));QVERIFY(!dialog->windowIcon().isNull());QCOMPARE(dialog->findChild<QLabel *>("aboutSpecialThanks")->text(),QString("Special thanks to Zuz, Nines, Taku, GPT and more."));QVERIFY(dialog->findChild<QWidget *>("aboutSplash"));auto capture=qEnvironmentVariable("WORKBENCH_ABOUT_CAPTURE");if(!capture.isEmpty()){QTest::qWait(100);QVERIFY(dialog->grab().save(capture));}QPointer<QDialog> guard=dialog;dialog->findChild<QToolButton *>("aboutClose")->click();QTRY_VERIFY(guard.isNull());
    }

    void arcpTriangleControls(){
        std::atomic_bool cancel=false;auto bytes=arcpFixture(true);auto e=arcpEvidence(bytes,bytes.size(),cancel);QVERIFY(e["renderSupported"].toBool());QCOMPARE(e["validatedTriangles"].toInt(),1);
        QTemporaryDir dir;MainWindow window(dir.filePath("layout.json"));auto *view=window.documentView();view->newMemoryDocument();QString error;QVERIFY(view->replaceSelection(bytes,error));auto result=readArcpGeometry(view->meshSource(),view->pendingPatches(),cancel,true);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.mesh->triangles,QVector<quint32>({0,1,2}));
        auto *panel=dynamic_cast<AssetScanPanel *>(window.findChild<QWidget *>("assetScanPanel_mesh"));panel->startScan();auto *renderer=window.findChild<VulkanPanel *>();QTRY_COMPARE(renderer->scene()->entries().size(),1);QCOMPARE(renderer->scene()->entries().first().data->triangles.size(),3);
        auto control=e["previewChunks"].toArray().first().toObject()["topologyOffset"].toString().toLongLong();auto bad=bytes;bad[control+1]=2;QVERIFY(!arcpEvidence(bad,bad.size(),cancel)["renderSupported"].toBool());bad=bytes;bad[control-8]=0;QVERIFY(!arcpEvidence(bad,bad.size(),cancel)["renderSupported"].toBool());QVERIFY(!arcpEvidence(bytes,bytes.size()+1,cancel)["renderSupported"].toBool());view->discardEdits();
    }
    void arcpEvidenceAndNativeScanActions(){
        auto bytes=arcpFixture();std::atomic_bool cancel=false;auto evidence=arcpEvidence(bytes,bytes.size(),cancel);QCOMPARE(evidence["validatedPackets"].toInt(),1);QCOMPARE(evidence["validatedVertices"].toInt(),3);QVERIFY(evidence["previewSupported"].toBool());QVERIFY(!evidence["renderSupported"].toBool());QCOMPARE(evidence["samples"].toArray().first().toObject()["positionsOffset"].toString(),QString("0xb8"));
        auto bad=bytes;qToLittleEndian<quint32>(0x7fc00000,bad.data()+184);QVERIFY(!arcpEvidence(bad,bad.size(),cancel)["previewSupported"].toBool());QVERIFY(!arcpEvidence(bytes.chopped(1),bytes.size(),cancel)["previewSupported"].toBool());bad=bytes;qToLittleEndian<quint32>(9000,bad.data()+132);QVERIFY(arcpEvidence(bad,bad.size(),cancel)["diagnostic"].toString().contains("outside"));cancel=true;QVERIFY(!arcpEvidence(bytes,bytes.size(),cancel)["previewSupported"].toBool());cancel=false;
        QTemporaryDir dir;MainWindow window(dir.filePath("layout.json"));auto *view=window.documentView();view->newMemoryDocument();QString error;QVERIFY(view->replaceSelection(bytes,error));auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));QVERIFY(!chat->findChild<QPushButton *>("aiRender3d"));QVERIFY(!chat->findChild<QPushButton *>("aiCalculate2d"));chat->applicationContext();QVERIFY(chat->cachedDiscovery().isEmpty());
        auto *hex=dynamic_cast<AssetScanPanel *>(window.findChild<QWidget *>("assetScanPanel_all"));auto *mesh=dynamic_cast<AssetScanPanel *>(window.findChild<QWidget *>("assetScanPanel_mesh"));QVERIFY(hex&&mesh);view->findChild<QPushButton *>("hexScanFile")->click();QTRY_VERIFY(!hex->result().isEmpty());QTRY_VERIFY(view->formatRules().size()>0);QVERIFY(view->formatRules().first().name.startsWith("Scan / metadata"));QVERIFY(!chat->cachedDiscovery().isEmpty());
        auto *tree=mesh->findChild<QTreeWidget *>("scanResults");QCOMPARE(tree->topLevelItemCount(),1);tree->setCurrentItem(tree->topLevelItem(0));QVERIFY(mesh->findChild<QPlainTextEdit *>("scanDetails")->toPlainText().contains("3 validated vertices"));QVERIFY(!mesh->findChild<QPushButton *>("scanLoadSettings")->isEnabled());auto *preview=mesh->findChild<QPushButton *>("scanPreview");QVERIFY(preview->isEnabled());QVERIFY(preview->text().contains("No Faces"));auto *renderer=window.findChild<VulkanPanel *>();QSignalSpy finished(renderer,&VulkanPanel::meshCalculationFinished);preview->click();QTRY_COMPARE(finished.size(),1);QVERIFY2(finished.first()[0].toBool(),qPrintable(finished.first()[1].toString()));QCOMPARE(renderer->scene()->entries().last().data->vertices.size(),3);QVERIFY(renderer->scene()->entries().last().data->triangles.isEmpty());QCOMPARE(renderer->scene()->entries().last().data->vertexOffsets.first(),qint64(184));QVERIFY(mesh->findChild<QLabel *>("scanStatus")->text().contains("0 faces"));
        view->newMemoryDocument();QVERIFY(hex->result().isEmpty());QVERIFY(chat->cachedDiscovery().isEmpty());QVERIFY(!preview->isEnabled());view->discardEdits();
    }
    void nativeImageScanAndPreview(){
        QTemporaryDir dir;MainWindow window(dir.filePath("layout.json"));auto *view=window.documentView();view->newMemoryDocument();QImage expected(3,2,QImage::Format_RGBA8888);expected.fill(QColor("#2569ac"));QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);QVERIFY(expected.save(&buffer,"PNG"));QByteArray bytes(32,'?');bytes+=png;QString error;QVERIFY(view->replaceSelection(bytes,error));window.findChild<QPushButton *>("scanForTexturesImages")->click();auto *panel=dynamic_cast<AssetScanPanel *>(window.findChild<QWidget *>("assetScanPanel_image"));QTRY_VERIFY(!panel->result().isEmpty());QVERIFY(panel->findChild<QCheckBox *>("scanAutoPreview")->isChecked());auto *images=dynamic_cast<ImageWorkbench *>(window.findChild<QObject *>("imageWorkbench"));QTRY_COMPARE(images->imageCount(),1);auto *tree=panel->findChild<QTreeWidget *>("scanResults");QCOMPARE(tree->topLevelItemCount(),1);QCOMPARE(tree->topLevelItem(0)->text(1),QString("0x20"));panel->findChild<QPushButton *>("scanGoToBytes")->click();QCOMPARE(view->selection().cursor,qint64(32));panel->findChild<QPushButton *>("scanPreview")->click();QTRY_COMPARE(images->imageCount(),2);QCOMPARE(images->entry(0).image.convertToFormat(QImage::Format_RGBA8888),expected);QVERIFY(panel->findChild<QLabel *>("scanStatus")->text().startsWith("Preview loaded"));view->discardEdits();
    }
    void localArcpAiInspectionSmoke(){
        if(!qEnvironmentVariableIsSet("WORKBENCH_AI_ARCP_SMOKE"))QSKIP("Optional installed-model ARCP analysis.");auto source=qEnvironmentVariable("WORKBENCH_ARCP_FIXTURE");QVERIFY(!source.isEmpty());QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("arcp-ai.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});MainWindow window(dir.filePath("layout.json"));window.openInPlace(source);QTRY_COMPARE(window.documentView()->fileSize(),qint64(107154));auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));chat->refreshModels();auto *models=chat->findChild<QComboBox *>("ollamaModels");QTRY_VERIFY_WITH_TIMEOUT(models->count()>0,5000);models->setCurrentText("qwen3:14b");chat->findChild<QPlainTextEdit *>("aiQuestion")->setPlainText("Inspect this file and locate the 3D data. Report measured offsets and counts. Do not render.");chat->findChild<QPushButton *>("aiSend")->click();QTRY_VERIFY_WITH_TIMEOUT(!chat->isRunning(),180000);auto text=chat->findChild<QPlainTextEdit *>("aiTranscript")->toPlainText();qInfo().noquote()<<text;qInfo().noquote()<<chat->findChild<QLabel *>("aiStatus")->text();QVERIFY2(text.contains("ARCP"),qPrintable(text));QVERIFY2(text.contains("0x20c0",Qt::CaseInsensitive)||text.contains("3256"),qPrintable(text));QVERIFY2(!text.contains("Analysis budget reached"),qPrintable(text));QCOMPARE(window.findChild<VulkanPanel *>()->scene()->entries().size(),0);QVERIFY(!window.documentView()->dirty());chat->findChild<QPlainTextEdit *>("aiQuestion")->setPlainText("Render the recovered ARCP model in 3D now.");chat->findChild<QPushButton *>("aiSend")->click();QTRY_VERIFY_WITH_TIMEOUT(!chat->isRunning(),180000);auto *renderer=window.findChild<VulkanPanel *>();QCOMPARE(renderer->scene()->entries().size(),1);QCOMPARE(renderer->scene()->entries().first().data->triangles.size()/3,2222);QVERIFY(!window.documentView()->dirty());qInfo().noquote()<<chat->findChild<QPlainTextEdit *>("aiTranscript")->toPlainText();
    }
    void localArcpScanFixture(){
        auto path=qEnvironmentVariable("WORKBENCH_ARCP_FIXTURE");if(path.isEmpty())QSKIP("Optional user ARCP fixture.");QTemporaryDir dir;MainWindow window(dir.filePath("layout.json"));window.openInPlace(path);QTRY_COMPARE(window.documentView()->fileSize(),qint64(107154));auto *view=window.documentView();std::atomic_bool cancel=false;auto result=discoverAssets(view->meshSource(),view->pendingPatches(),cancel);auto item=result["meshEvidence"].toArray().first().toObject();QCOMPARE(item["validatedPackets"].toInt(),10);QCOMPARE(item["validatedVertices"].toInt(),3256);QCOMPARE(item["samples"].toArray().first().toObject()["positionsOffset"].toString(),QString("0x20c0"));auto points=readArcpPointPreview(view->meshSource(),view->pendingPatches(),cancel);QVERIFY2(points.error.isEmpty(),qPrintable(points.error));QCOMPARE(points.mesh->vertices.size(),3256);QVERIFY(points.mesh->triangles.isEmpty());QVERIFY(item["renderSupported"].toBool());QCOMPARE(item["validatedStrips"].toInt(),517);auto faces=readArcpGeometry(view->meshSource(),view->pendingPatches(),cancel,true);QVERIFY2(faces.error.isEmpty(),qPrintable(faces.error));QCOMPARE(faces.mesh->triangles.size()/3,2222);for(auto index:faces.mesh->triangles)QVERIFY(index<quint32(faces.mesh->vertices.size()));points=faces;qInfo().noquote()<<item["diagnostic"].toString();auto screenshot=qEnvironmentVariable("WORKBENCH_SCAN_UI");if(!screenshot.isEmpty()){auto *panel=dynamic_cast<AssetScanPanel *>(window.findChild<QWidget *>("assetScanPanel_mesh"));panel->showResults(result);panel->resize(1100,500);panel->show();QTest::qWait(50);panel->grab().save(screenshot);}
        if(qEnvironmentVariableIsSet("WORKBENCH_GPU_TEST")){window.resize(1280,900);window.show();auto *renderer=window.findChild<VulkanPanel *>();window.sessionDock("vulkanDock")->show();renderer->setMesh(points.mesh);renderer->frameView(1);auto *status=renderer->findChild<QLabel *>("vulkanStatus");QTRY_VERIFY2_WITH_TIMEOUT(status->text().startsWith("Vulkan"),qPrintable(status->text()),15000);QTest::qWait(1500);}
    }

    void viewportRenderingStyles(){
        VulkanPanel panel;panel.resize(1000,740);auto mesh=std::make_shared<MeshData>();
        for(int y=0;y<=16;++y)for(int x=0;x<=32;++x){double latitude=M_PI*y/16.,longitude=2*M_PI*x/32.;MeshVertex v{};v.position[0]=v.normal[0]=std::sin(latitude)*std::cos(longitude);v.position[1]=v.normal[1]=std::cos(latitude);v.position[2]=v.normal[2]=std::sin(latitude)*std::sin(longitude);mesh->vertices.append(v);}
        for(int y=0;y<16;++y)for(int x=0;x<32;++x){quint32 a=y*33+x,b=a+33;mesh->triangles+={a,a+1,b,a+1,b+1,b};}panel.scene()->add(mesh,"Style preview");panel.scene()->setSelectedMeshes({});
        int capture=qEnvironmentVariableIsSet("WORKBENCH_STYLE_CAPTURE")?qEnvironmentVariableIntValue("WORKBENCH_STYLE_CAPTURE"):-1;if(capture>=0)panel.setRenderStyle(capture);if(qEnvironmentVariableIsSet("WORKBENCH_FACE_NORMALS"))panel.setNormalMode(1);bool gpu=qEnvironmentVariableIsSet("WORKBENCH_GPU_TEST");if(gpu){panel.show();auto *status=panel.findChild<QLabel *>("vulkanStatus");QTRY_VERIFY2_WITH_TIMEOUT(status->text().startsWith("Vulkan"),qPrintable(status->text()),15000);}
        for(int i=0;i<4;++i){if(capture>=0&&i!=capture)continue;auto *action=panel.findChild<QAction *>(QString("viewportRenderStyle%1").arg(i));QVERIFY(action);action->trigger();QVERIFY(action->isChecked());QCOMPARE(panel.findChild<QWidget *>("vulkanSurface")->property("renderStyle").toInt(),i);if(gpu)QTest::qWait(1000);}
        auto originalNormal=mesh->vertices[100].normal[0];for(int mode=0;mode<2;++mode){panel.findChild<QAction *>(QString("viewportNormalMode%1").arg(mode))->trigger();QCOMPARE(panel.findChild<QWidget *>("vulkanSurface")->property("normalMode").toInt(),mode);QCOMPARE(mesh->vertices[100].normal[0],originalNormal);}QCOMPARE(mesh->vertices.size(),17*33);panel.setRenderStyle(3);panel.setViewportLayout(3);for(auto *view:panel.findChildren<VulkanPanel *>())QCOMPARE(view->findChild<QWidget *>("vulkanSurface")->property("renderStyle").toInt(),3);panel.setNormalMode(0);for(auto *view:panel.findChildren<VulkanPanel *>())QCOMPARE(view->findChild<QWidget *>("vulkanSurface")->property("normalMode").toInt(),0);panel.setRenderStyle(1);for(auto *view:panel.findChildren<VulkanPanel *>())QCOMPARE(view->findChild<QWidget *>("vulkanSurface")->property("renderStyle").toInt(),1);panel.setViewportLayout(0);
    }
    void orthographicViewsVisibilityAndBounds(){
        VulkanPanel panel;panel.resize(1000,720);auto *surface=panel.findChild<QWidget *>("vulkanSurface");
        for(int view=1;view<=6;++view){panel.setViewOrientation(view);QVERIFY(surface->property("orthographic").toBool());auto m=surface->property("viewMatrix").value<QMatrix4x4>();bool invertible=false;m.inverted(&invertible);QVERIFY(invertible);auto a=m*QVector4D(1,2,3,1),b=m*QVector4D(1,2,-3,1);QCOMPARE(a.w(),b.w());QVERIFY(std::isfinite(a.x()));}
        auto grid=orthographicGrid({1000,720},{0,0,0},{1,0,0},{0,1,0},10);QVERIFY(grid.lines.size()>10);QVERIFY(grid.lines.size()<100);QVERIFY(grid.spacing>0);auto shifted=orthographicGrid({1000,720},{float(grid.spacing),0,0},{1,0,0},{0,1,0},10);QCOMPARE(grid.lines,shifted.lines);auto zoom=orthographicGrid({1000,720},{0,0,0},{1,0,0},{0,1,0},100);QVERIFY(zoom.spacing>grid.spacing);QVERIFY(zoom.lines.size()<100);
        auto mesh=std::make_shared<MeshData>();mesh->vertices={{{-1,-2,-3},{0,0,1},{0,0}},{{1,2,3},{0,0,1},{1,0}},{{-1,2,-3},{0,0,1},{0,1}},{{1,-2,-3},{0,0,1},{1,1}}};mesh->triangles={0,1,2,0,3,1,0,2,3,1,3,2};auto *scene=panel.scene();int a=scene->add(mesh,"A"),b=scene->add(mesh,"B"),c=scene->add(mesh,"C");scene->setPositionDelta(b,{10,0,0});scene->setVisible(c,false);scene->setSelectedMeshes({a,b},a);QCOMPARE(scene->bounds().dimensions(),QVector3D(12,4,6));
        std::unique_ptr<QWidget> collection(panel.createMeshCollection());auto *tree=collection->findChild<QTreeWidget *>("meshCollection");QCOMPARE(tree->selectedItems().size(),2);QVERIFY(collection->findChild<QLabel *>("meshBoundsDimensions")->text().contains("X 12"));
        scene->setMeshesVisible(false,true);QVERIFY(!scene->bounds().valid);scene->setMeshesVisible(true,true);QVERIFY(!scene->entry(c)->visible);scene->setSelectedMeshes({b},b);scene->isolateSelected();QVERIFY(scene->isolated());QVERIFY(!scene->entry(a)->visible);QVERIFY(scene->entry(b)->visible);QCOMPARE(scene->bounds().dimensions(),QVector3D(2,4,6));scene->restoreVisibility();QVERIFY(scene->entry(a)->visible);QVERIFY(!scene->entry(c)->visible);QVERIFY(!scene->isolated());
        scene->setTransform(b,{10,0,0},{0,0,90},{2,1,1});auto d=scene->bounds(true).dimensions();QVERIFY(std::abs(d.x()-4)<1e-4);QVERIFY(std::abs(d.y()-4)<1e-4);QCOMPARE(d.z(),6.f);QCOMPARE(mesh->vertices[0].position[0],-1.f);
        if(qEnvironmentVariableIsSet("WORKBENCH_GPU_TEST")){panel.show();QVERIFY(panel.frameView(1));panel.findChild<QAction *>("viewportBounds")->setChecked(true);auto *status=panel.findChild<QLabel *>("vulkanStatus");QTRY_VERIFY_WITH_TIMEOUT(status->text().startsWith("Vulkan"),15000);for(int view=1;view<=6;++view){panel.setViewOrientation(view);QTest::qWait(650);QVERIFY(surface->property("gridSpacing").toDouble()>0);QVERIFY2(status->text().startsWith("Vulkan"),qPrintable(status->text()));}scene->setSelectedMeshes({a},a);panel.findChild<QAction *>("meshIsolateSelected")->trigger();QTest::qWait(200);scene->restoreVisibility();QTest::qWait(200);}
        scene->setMeshesVisible(false);QVERIFY(!scene->bounds().valid);scene->setMeshesVisible(true);QVERIFY(scene->entry(c)->visible);scene->clear();QVERIFY(scene->selectedMeshes().isEmpty());
    }

    void approvedThemeDefaultsRespectPersonalPreferences(){
        auto original=qApp->property("workbenchTheme").toString();auto library=qApp->property("workbenchThemeLibrary");auto restore=qScopeGuard([=]{qApp->setProperty("workbenchThemeLibrary",library);applyWorkbenchTheme(original);});QTemporaryDir dir;QSettings settings(dir.filePath("defaults.ini"),QSettings::IniFormat);loadWorkbenchPreferences(settings);QCOMPARE(qApp->property("workbenchTheme").toString(),QString("default"));QCOMPARE(qApp->palette().color(QPalette::Window),QColor("#0e1115"));QCOMPARE(standardAppearance()["name"].toString(),QString("My Theme"));QVERIFY(!standardWorkspaceLayout()["toolbarVisible"].toBool(true));QCOMPARE(standardWorkspaceLayout()["modules"].toObject().size(),5);QVERIFY(settings.allKeys().isEmpty());
        auto *selector=new QComboBox;ThemeEditor editor(selector,settings);QCOMPARE(selector->currentData().toString(),QString("default"));QVERIFY(!editor.setOverride("background.window",Qt::red));auto id=editor.createTheme("Copy of default");QVERIFY(id.startsWith("custom:"));QCOMPARE(editor.current()["colors"].toObject()["background.window"].toString(),QString("#0e1115"));QVERIFY(editor.setOverride("background.window",Qt::red));QCOMPARE(standardAppearance()["colors"].toObject()["background.window"].toString(),QString("#0e1115"));editor.restoreDefaults();QCOMPARE(selector->currentData().toString(),QString("default"));
        settings.setValue("theme","light");loadWorkbenchPreferences(settings);QCOMPARE(qApp->property("workbenchTheme").toString(),QString("light"));QCOMPARE(settings.value("theme").toString(),QString("light"));
    }
    void frameMenuHomesAndFitsTheSelection(){
        VulkanPanel panel;panel.resize(1000,720);panel.show();QVERIFY(!panel.frameView(2));QVERIFY(panel.frameView(0));auto *surface=panel.findChild<QWidget *>("vulkanSurface");QCOMPARE(surface->property("viewOrientation").toInt(),0);QVERIFY(panel.findChild<QAction *>("viewportHome"));QVERIFY(panel.findChild<QAction *>("viewportFrameAll"));QVERIFY(panel.findChild<QAction *>("viewportFrameSelected"));
        auto mesh=std::make_shared<MeshData>();mesh->vertices={{{-1,-1,0},{0,0,1},{0,0}},{{1,-1,0},{0,0,1},{1,0}},{{0,1,0},{0,0,1},{0,1}}};mesh->triangles={0,1,2};int first=panel.scene()->add(mesh,"Near"),second=panel.scene()->add(mesh,"Far");QVERIFY(panel.scene()->setPositionDelta(second,{10000,0,0}));panel.setViewOrientation(1);QVERIFY(panel.frameView(1));QCOMPARE(surface->property("viewOrientation").toInt(),1);QVERIFY(surface->property("framingRadius").toDouble()>4000);
        panel.scene()->setActive(first);QVERIFY(panel.frameView(2));QVERIFY(surface->property("framingRadius").toDouble()<2);QCOMPARE(surface->property("viewOrientation").toInt(),1);QCOMPARE(surface->property("cameraTarget").value<QVector3D>(),QVector3D());QCOMPARE(panel.scene()->entry(second)->positionDelta[0],10000.);
        panel.scene()->select(first,MeshElement::Vertex,1);QVERIFY(panel.frameView(2));QCOMPARE(surface->property("framingCenter").value<QVector3D>(),QVector3D(1,-1,0));QVERIFY(surface->property("cameraDistance").toDouble()>0);panel.scene()->select(first,MeshElement::Face,0);QVERIFY(panel.frameView(2));QCOMPARE(surface->property("framingCenter").value<QVector3D>(),QVector3D());QCOMPARE(mesh->vertices[0].position[0],-1.f);
        panel.scene()->setVisible(first,false);QVERIFY(!panel.frameView(2));QVERIFY(panel.frameView(1));QCOMPARE(surface->property("framingCenter").value<QVector3D>(),QVector3D(10000,0,0));panel.setViewOrientation(3);panel.findChild<QAction *>("viewportHome")->trigger();QCOMPARE(surface->property("viewOrientation").toInt(),0);QCOMPARE(surface->property("framingCenter").value<QVector3D>(),QVector3D(10000,0,0));
    }

    void viewportHandlesAndTiledLayouts(){
        VulkanPanel panel;panel.resize(1000,800);panel.show();auto mesh=std::make_shared<MeshData>();mesh->vertices={{{-1,-1,0},{0,0,1},{0,0}},{{1,-1,0},{0,0,1},{1,0}},{{0,1,0},{0,0,1},{0,1}}};mesh->triangles={0,1,2};int id=panel.scene()->add(mesh,"Triangle");panel.setViewOrientation(1);QTest::qWait(20);auto *surface=panel.findChild<QWidget *>("vulkanSurface");
        auto drag=[&](QPointF from,QPointF to,bool cancel=false){QTest::mousePress(surface,Qt::LeftButton,Qt::NoModifier,from.toPoint());QMouseEvent move(QEvent::MouseMove,to,QPointF(surface->mapToGlobal(to.toPoint())),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(surface,&move);if(cancel)QTest::keyClick(surface,Qt::Key_Escape);QTest::mouseRelease(surface,Qt::LeftButton,Qt::NoModifier,to.toPoint());};
        panel.setTransformTool(1);auto origin=panel.gizmoAxisPosition(-1),x=panel.gizmoAxisPosition(0);QVERIFY(x.x()>origin.x()+10);drag(x,x+QPointF(40,0));QVERIFY(panel.scene()->entry(id)->positionDelta[0]>.1);QCOMPARE(panel.scene()->entry(id)->positionDelta[1],0.);QCOMPARE(mesh->vertices[0].position[0],-1.f);auto movedPivot=panel.gizmoAxisPosition(-1);QVERIFY(movedPivot.x()>origin.x()+20); // Translation must not recenter the camera.
        auto saved=panel.scene()->entry(id)->positionDelta;x=panel.gizmoAxisPosition(0);drag(x,x+QPointF(30,0),true);QCOMPARE(panel.scene()->entry(id)->positionDelta,saved);QVERIFY(panel.scene()->setTransform(id,{},{},{1,1,1}));
        panel.setTransformTool(3);x=panel.gizmoAxisPosition(0);drag(x,x+QPointF(30,0));QVERIFY(panel.scene()->entry(id)->scale[0]>1.1);QCOMPARE(panel.scene()->entry(id)->scale[1],1.);QVERIFY(panel.scene()->setTransform(id,{},{},{1,1,1}));
        panel.setTransformTool(2);auto ring=panel.gizmoAxisPosition(2);origin=panel.gizmoAxisPosition(-1);auto v=ring-origin;drag(ring,origin+QPointF(-v.y(),v.x()));QVERIFY(std::abs(panel.scene()->entry(id)->rotationDegrees[2])>70);QCOMPARE(mesh->triangles,QVector<quint32>({0,1,2}));
        panel.setTransformTool(1);origin=panel.gizmoAxisPosition(-1);drag(origin,origin+QPointF(20,-20));QVERIFY(panel.scene()->entry(id)->positionDelta[0]>.05);QVERIFY(panel.scene()->entry(id)->positionDelta[1]>.05);
        panel.setViewportLayout(3);QCOMPARE(panel.findChildren<VulkanPanel *>().size(),3);int visibleBars=0;for(auto *bar:panel.findChildren<QWidget *>("viewportTransformBar"))visibleBars+=bar->isVisible();QCOMPARE(visibleBars,1);QVERIFY(panel.findChild<QToolButton *>("viewportTitle")->isHidden());QCOMPARE(surface->property("viewOrientation").toInt(),1);QSet<int> orientations{1};for(auto *view:panel.findChildren<VulkanPanel *>()){QCOMPARE(view->scene(),panel.scene());orientations.insert(view->findChild<QWidget *>("vulkanSurface")->property("viewOrientation").toInt());}QCOMPARE(orientations.size(),4);
        panel.setViewportLayout(1);QCOMPARE(panel.findChildren<VulkanPanel *>().size(),1);auto *split=panel.findChild<QSplitter *>();QVERIFY(split);QCOMPARE(split->orientation(),Qt::Horizontal);panel.setViewportLayout(2);QCOMPARE(panel.findChildren<VulkanPanel *>().size(),1);split=panel.findChild<QSplitter *>();QCOMPARE(split->orientation(),Qt::Vertical);panel.setViewportLayout(0);QCOMPARE(panel.findChildren<VulkanPanel *>().size(),0);
        panel.findChild<QAction *>("viewportLabelsAction")->trigger();auto *inspector=panel.findChild<QDialog *>("viewportLabelsInspector");QVERIFY(inspector->isVisible());for(auto name:{"meshLabelLimit","meshFaceLabelLimit","meshMeshLabelLimit"}){auto *count=panel.findChild<QLineEdit *>(name);QVERIFY(count);QVERIFY(count->text().isEmpty());QCOMPARE(count->placeholderText(),QString("All"));count->setText("12");count->clear();QVERIFY(count->text().isEmpty());}panel.findChild<QCheckBox *>("meshLabels")->setChecked(true);panel.findChild<QGroupBox *>("meshLabelFaces")->setChecked(true);panel.findChild<QGroupBox *>("meshLabelMeshes")->setChecked(true);QVERIFY(panel.findChild<QGroupBox *>("meshLabelVertices")->isChecked());QVERIFY(panel.findChild<QLabel *>("meshLabelTotals")->text().contains("3 vertices"));if(qEnvironmentVariableIsSet("WORKBENCH_LABELS_UI")){inspector->grab().save(qEnvironmentVariable("WORKBENCH_LABELS_UI"));}inspector->close();
        panel.setViewportLayout(3); // Destruction must release every shared surface safely.
    }

    void sceneTransformsMaterialsAndSharedViewports(){
        QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("scene.ini"));QSettings sceneSettings(dir.filePath("scene.ini"),QSettings::IniFormat);sceneSettings.setValue("3d/materials",QJsonDocument(QJsonArray{QJsonObject{{"name","Saved paint"},{"color","#123456"},{"unlit",true}}}).toJson());sceneSettings.sync();auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});
        VulkanPanel renderer;auto mesh=std::make_shared<MeshData>();mesh->vertices={{{1,0,0},{0,0,1},{0,0}},{{0,1,0},{0,0,1},{1,0}},{{0,0,0},{0,0,1},{0,1}}};mesh->triangles={0,1,2};int id=renderer.scene()->add(mesh,"Triangle");
        QVERIFY(renderer.scene()->setTransform(id,{10,20,30},{0,0,90},{2,3,1}));auto placed=renderer.scene()->entry(id)->placedData();QVERIFY(std::abs(placed->vertices[0].position[0]-10)<1e-5);QVERIFY(std::abs(placed->vertices[0].position[1]-22)<1e-5);QCOMPARE(mesh->vertices[0].position[0],1.f);
        QVERIFY(!renderer.scene()->setTransform(id,{},{},{0,1,1}));QCOMPARE(renderer.scene()->entry(id)->positionDelta[0],10.);QVERIFY(renderer.scene()->setTransform(id,{},{},{-1,1,1}));QCOMPARE(renderer.scene()->assemble().data->triangles,QVector<quint32>({0,2,1}));QCOMPARE(renderer.scene()->entry(id)->placedData()->vertices[0].normal[2],1.f);
        QVERIFY(renderer.scene()->setMaterial(id,{"Paint",QColor("#e02040"),true}));std::unique_ptr<QWidget> inspector(renderer.createMeshCollection());auto *shader=inspector->findChild<QComboBox *>("meshMaterialShader");QVERIFY(shader);QCOMPARE(shader->currentIndex(),1);auto *presets=inspector->findChild<QComboBox *>("materialLibrary");QCOMPARE(presets->count(),1);inspector->findChild<QPushButton *>("assignMaterialPreset")->click();QCOMPARE(renderer.scene()->entry(id)->material.baseColor,QColor("#123456"));shader->setCurrentIndex(0);QVERIFY(!renderer.scene()->entry(id)->material.unlit);QCOMPARE(inspector->findChild<QDoubleSpinBox *>("activeMeshScaleX")->value(),-1.);
        auto *action=renderer.findChild<QAction *>("new3dViewport");QVERIFY(action);action->trigger();auto views=renderer.findChildren<VulkanPanel *>();QCOMPARE(views.size(),1);QCOMPARE(views.first()->scene(),renderer.scene());QVERIFY(renderer.scene()->setPositionDelta(id,{4,0,0}));QCOMPARE(views.first()->scene()->entry(id)->positionDelta[0],4.);views.first()->close();
        ExportDialog dialog(renderer.scene(),{},Qt::white);auto exported=dialog.selectedMeshes();QCOMPARE(exported.first().surface.name,QString("Saved paint"));QCOMPARE(exported.first().data->vertices[0].position[0],3.f);
        if(qEnvironmentVariableIsSet("WORKBENCH_SCENE_UI")){inspector->resize(480,650);inspector->show();QTest::qWait(100);inspector->grab().save(qEnvironmentVariable("WORKBENCH_SCENE_UI"));}
    }


    void genkiPacketEvidenceIsBoundedAndNotARawRecipe(){
        QByteArray bytes(1024,0);auto put=[&](int at,quint32 v){qToLittleEndian(v,bytes.data()+at);};put(0,2);put(4,64);put(8,768);std::memcpy(bytes.data()+64,"0IVX00.1",8);std::memcpy(bytes.data()+80,"LDMX00.2",8);std::memcpy(bytes.data()+88,"MRON00.2",8);
        QByteArray payload;auto append=[&](quint32 v){char data[4];qToLittleEndian(v,data);payload.append(data,4);};append(0x01000202);append(0x6c028000);payload+=QByteArray(32,0);append(0x01000104);append(0x68038002);for(int i=0;i<3;++i){float x=i;quint32 bits;std::memcpy(&bits,&x,4);append(bits);append(i<2?1:0);append(0);}while(payload.size()%16)append(0);put(256,0x10000000|payload.size()/16);put(260,0);put(264,1);put(268,3);std::memcpy(bytes.data()+272,payload.constData(),payload.size());
        auto evidence=genkiMeshEvidence(bytes,bytes.size());QCOMPARE(evidence["family"].toString(),QString("Genki PS2 0IVX/XMDL"));QVERIFY(!evidence["renderSupported"].toBool(true));QVERIFY(!evidence.contains("settings"));auto sample=evidence["samples"].toArray().first().toObject();QCOMPARE(sample["positionsOffset"].toString(),QString("0x140"));QCOMPARE(sample["vertexCount"].toInt(),3);QCOMPARE(sample["yLowBitSetVertices"].toInt(),2);
        auto bad=bytes;qToLittleEndian<quint32>(0x1000ffff,bad.data()+256);QVERIFY(genkiMeshEvidence(bad,bad.size()).isEmpty());bad=bytes;bad[64]='X';QVERIFY(genkiMeshEvidence(bad,bad.size()).isEmpty());QVERIFY(genkiMeshEvidence(bytes.left(300),1024).isEmpty());bad=bytes;qToLittleEndian<quint32>(0x7fc00000,bad.data()+320);QVERIFY(genkiMeshEvidence(bad,bad.size()).isEmpty());bad=bytes;qToLittleEndian<quint32>(0x12345678,bad.data()+272);QVERIFY(genkiMeshEvidence(bad,bad.size()).isEmpty());
        auto triangle=bytes;qToLittleEndian<quint32>(0x3f800000,triangle.data()+348);auto supported=genkiMeshEvidence(triangle,triangle.size());QVERIFY(supported["renderSupported"].toBool());QCOMPARE(supported["validatedTriangles"].toInt(),1);QCOMPARE(genkiPacketFaces(triangle,320,3,1),QVector<quint32>({1,0,2}));qToLittleEndian<quint32>(0x3f800001,triangle.data()+348);QVERIFY(!genkiMeshEvidence(triangle,triangle.size())["renderSupported"].toBool());std::atomic_bool cancelled=true;QVERIFY(genkiMeshEvidence(bytes,bytes.size(),&cancelled).contains("error"));

    }
    void localGenkiNativeScanAndExport(){
        auto path=qEnvironmentVariable("WORKBENCH_GENKI_FIXTURE");if(path.isEmpty())QSKIP("Optional original Genki file.");QTemporaryDir dir;MainWindow window(dir.filePath("layout.json"));window.openInPlace(path);auto *view=window.documentView();QTRY_COMPARE(view->fileSize(),qint64(1668064));QFile original(path);QVERIFY(original.open(QIODevice::ReadOnly));auto hash=QCryptographicHash::hash(original.readAll(),QCryptographicHash::Sha256);original.close();std::atomic_bool cancel=false;auto facts=discoverAssets(view->meshSource(),{},cancel);auto e=facts["meshEvidence"].toArray().first().toObject();QVERIFY(e["renderSupported"].toBool());QCOMPARE(e["validatedPackets"].toInt(),867);QCOMPARE(e["validatedVertices"].toInt(),26532);QCOMPARE(e["validatedTriangles"].toInt(),21942);QCOMPARE(facts["imageCandidates"].toArray().size(),40);
        auto *panel=dynamic_cast<AssetScanPanel *>(window.findChild<QWidget *>("assetScanPanel_mesh"));panel->startScan();auto *renderer=window.findChild<VulkanPanel *>();QTRY_COMPARE_WITH_TIMEOUT(renderer->scene()->entries().size(),1,20000);auto mesh=renderer->scene()->entries().first().data;QCOMPARE(mesh->vertices.size(),26532);QCOMPARE(mesh->triangles.size()/3,21942);QCOMPARE(mesh->faceSources.size(),21942);for(auto index:mesh->triangles)QVERIFY(index<quint32(mesh->vertices.size()));
        auto output=qEnvironmentVariable("WORKBENCH_GENKI_EXPORT_DIR",dir.path());QVERIFY(QDir().mkpath(output));for(auto format:{ModelFormat::Glb,ModelFormat::Fbx}){ModelExportOptions options;options.format=format;auto file=QDir(output).filePath(format==ModelFormat::Glb?"000004b8-native.glb":"000004b8-native.fbx");auto result=exportModel(file,{{"Genki all local parts",path,mesh}},options);QVERIFY2(result.ok,qPrintable(result.error));QCOMPARE(result.faces,quint64(21942));QVERIFY(QFileInfo(file).size()>1000);}
        int images=0;for(auto value:facts["imageCandidates"].toArray()){auto settings=value.toObject()["settings"].toObject();ImageRecipe recipe;recipe.offset=settings["offset"].toString().toLongLong();recipe.format="Encoded image";recipe.surface=settings["surface"].toInt();auto result=decodeBinaryImage(view->meshSource(),{},recipe,cancel);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QVERIFY(!result.image.isNull());if(qEnvironmentVariableIsSet("WORKBENCH_GENKI_EXPORT_DIR"))QVERIFY(result.image.save(QDir(output).filePath(QString("texture-%1.png").arg(images,2,10,QChar('0')))));++images;}QCOMPARE(images,40);
        QVERIFY(original.open(QIODevice::ReadOnly));QCOMPARE(QCryptographicHash::hash(original.readAll(),QCryptographicHash::Sha256),hash);QVERIFY(!view->dirty());qInfo().noquote()<<e["diagnostic"].toString();
        if(qEnvironmentVariableIsSet("WORKBENCH_GPU_TEST")){window.resize(1400,900);window.show();window.sessionDock("vulkanDock")->show();renderer->frameView(1);auto *status=renderer->findChild<QLabel *>("vulkanStatus");QTRY_VERIFY2_WITH_TIMEOUT(status->text().startsWith("Vulkan"),qPrintable(status->text()),15000);QTest::qWait(1500);}
    }
    void localOllamaGenkiInspectionSmoke(){
        if(!qEnvironmentVariableIsSet("WORKBENCH_AI_GENKI_SMOKE"))QSKIP("Optional installed-model Genki investigation.");auto source=qEnvironmentVariable("WORKBENCH_GENKI_FIXTURE");QVERIFY(!source.isEmpty());QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("genki-ai.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});{auto settings=aiSettings();settings.setValue("ai/context",8192);}
        MainWindow window(dir.filePath("layout.json"));window.openInPlace(source);QTRY_COMPARE(window.documentView()->fileSize(),qint64(1668064));auto request=window.documentView()->meshSource();std::atomic_bool cancel=false;auto facts=discoverAssets(request,{},cancel);QVERIFY(facts["meshCandidates"].toArray().isEmpty());QCOMPARE(facts["meshEvidence"].toArray().size(),1);QCOMPARE(facts["meshEvidence"].toArray().first().toObject()["samples"].toArray().first().toObject()["positionsOffset"].toString(),QString("0x410"));
        auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));chat->refreshModels();auto *models=chat->findChild<QComboBox *>("ollamaModels");QTRY_VERIFY_WITH_TIMEOUT(models->count()>0,5000);models->setCurrentText(qEnvironmentVariable("WORKBENCH_GENKI_MODEL","qwen3:14b"));for(bool enabled:{false,true}){if(enabled)for(auto *b:chat->findChildren<QPushButton *>())if(b->text()=="New Chat")b->click();chat->findChild<QCheckBox *>("aiAllowRun")->setChecked(enabled);chat->findChild<QCheckBox *>("aiAllowStructure")->setChecked(enabled);chat->findChild<QPlainTextEdit *>("aiQuestion")->setPlainText("can you read the file and locate the 3d data?");chat->findChild<QPushButton *>("aiSend")->click();QTRY_VERIFY_WITH_TIMEOUT(!chat->isRunning(),180000);auto transcript=chat->findChild<QPlainTextEdit *>("aiTranscript")->toPlainText();qInfo().noquote()<<transcript;qInfo().noquote()<<chat->findChild<QLabel *>("aiStatus")->text();QVERIFY2(transcript.contains("Genki",Qt::CaseInsensitive)||transcript.contains("VIF",Qt::CaseInsensitive),qPrintable(transcript));auto reply=transcript.mid(transcript.lastIndexOf("Finding:"));QVERIFY2(reply.contains("0x410",Qt::CaseInsensitive)||reply.contains("0x3d0",Qt::CaseInsensitive),qPrintable(transcript));QVERIFY(chat->findChild<QLabel *>("aiStatus")->text().contains("native scan 1668064"));QCOMPARE(window.findChild<VulkanPanel *>()->scene()->entries().size(),0);QVERIFY(!window.documentView()->dirty());}
        chat->findChild<QPlainTextEdit *>("aiQuestion")->setPlainText("Render this Genki model in 3D now using the native importer.");chat->findChild<QPushButton *>("aiSend")->click();QTRY_VERIFY_WITH_TIMEOUT(!chat->isRunning(),180000);auto *renderer=window.findChild<VulkanPanel *>();QCOMPARE(renderer->scene()->entries().size(),1);QCOMPARE(renderer->scene()->entries().first().data->triangles.size()/3,21942);QVERIFY(!window.documentView()->dirty());qInfo().noquote()<<chat->findChild<QPlainTextEdit *>("aiTranscript")->toPlainText();

    }

    void generatedGlbChatReviewSmoke(){
        auto source=qEnvironmentVariable("WORKBENCH_REVIEW_GLB");if(source.isEmpty())QSKIP("Optional generated geometry review test.");QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("review.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});MainWindow window(dir.filePath("layout.json"));window.openInPlace(source);QTRY_COMPARE(window.documentView()->fileSize(),QFileInfo(source).size());if(qEnvironmentVariableIsSet("WORKBENCH_GPU_TEST")){window.resize(1280,900);window.show();}
        auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));auto *renderer=window.findChild<VulkanPanel *>();QSignalSpy finished(renderer,&VulkanPanel::meshCalculationFinished);chat->requestMeshDecode();QTRY_COMPARE_WITH_TIMEOUT(finished.size(),1,20000);QVERIFY2(finished.first()[0].toBool(),qPrintable(finished.first()[1].toString()));QCOMPARE(renderer->scene()->entries().size(),1);QCOMPARE(renderer->scene()->entries().first().data->vertices.size(),qEnvironmentVariableIntValue("WORKBENCH_REVIEW_VERTICES"));QCOMPARE(renderer->scene()->entries().first().data->triangles.size()/3,qEnvironmentVariableIntValue("WORKBENCH_REVIEW_TRIANGLES"));
        if(qEnvironmentVariableIsSet("WORKBENCH_GPU_TEST")){auto *status=renderer->findChild<QLabel *>("vulkanStatus");QTRY_VERIFY2_WITH_TIMEOUT(status->text().startsWith("Vulkan"),qPrintable(status->text()),15000);qInfo().noquote()<<status->text();QTest::qWait(1000);}
    }

    void chatRenderDiscoversBlankSetupAndPreservesPartialInput(){
        QTemporaryDir dir;MainWindow window(dir.filePath("layout.json"));auto *view=window.documentView();view->newMemoryDocument();
        QByteArray json=R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":42}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},{"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"}],"meshes":[{"primitives":[{"attributes":{"POSITION":0},"indices":1,"mode":4}]}]})";while(json.size()%4)json+=' ';QByteArray glb;auto append32=[&](quint32 n){char data[4];qToLittleEndian(n,data);glb.append(data,4);};glb+="glTF";append32(2);append32(12+8+json.size()+8+44);append32(json.size());glb+="JSON";glb+=json;append32(44);glb.append("BIN\0",4);QByteArray binary(44,0);float positions[9]={0,0,0,1,0,0,0,1,0};std::memcpy(binary.data(),positions,36);for(int i=0;i<3;++i)qToLittleEndian<quint16>(i,binary.data()+36+i*2);glb+=binary;
        QString error;QVERIFY(view->replaceSelection(glb,error));auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));auto *renderer=window.findChild<VulkanPanel *>();QSignalSpy finished(renderer,&VulkanPanel::meshCalculationFinished);QVERIFY(meshSetupSession(window.sessionDock("meshControlsDock"))->empty());chat->requestMeshDecode();QVERIFY(chat->decoderBusy(true));QTRY_COMPARE(finished.size(),1);QVERIFY2(finished.last()[0].toBool(),qPrintable(finished.last()[1].toString()));QCOMPARE(renderer->scene()->entries().size(),1);QCOMPARE(renderer->scene()->entries().first().data->vertices.size(),3);QCOMPARE(renderer->scene()->entries().first().data->triangles.size(),3);QVERIFY(!renderer->scene()->entries().first().data->hasUvs);QVERIFY(chat->findChild<QLabel *>("aiDecoderStatus")->text().startsWith("3D complete"));
        view->newMemoryDocument();QVERIFY(view->replaceSelection(QByteArray(32,'?'),error));chat->requestMeshDecode();QTRY_COMPARE(finished.size(),2);QVERIFY(!finished.last()[0].toBool());QVERIFY(finished.last()[1].toString().contains("No supported model recipe"));QVERIFY(meshSetupSession(window.sessionDock("meshControlsDock"))->empty());
        auto *offset=window.findChild<QLineEdit *>("meshVertexOffset");offset->setText("7");chat->requestMeshDecode();QCOMPARE(offset->text(),QString("7"));QCOMPARE(finished.size(),2);QVERIFY(chat->findChild<QLabel *>("aiDecoderStatus")->text().startsWith("3D failed"));view->discardEdits();
    }
    void naniChatRenderButtonBlankSetupSmoke(){
        auto source=qEnvironmentVariable("WORKBENCH_NANI_FIXTURE");if(source.isEmpty())QSKIP("Optional local nani Render 3D button test.");QTemporaryDir dir;MainWindow window(dir.filePath("layout.json"));window.openInPlace(source);QTRY_COMPARE(window.documentView()->fileSize(),qint64(16691362));auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));auto *renderer=window.findChild<VulkanPanel *>();QSignalSpy finished(renderer,&VulkanPanel::meshCalculationFinished);chat->requestMeshDecode();QTRY_COMPARE_WITH_TIMEOUT(finished.size(),1,20000);QVERIFY2(finished.first()[0].toBool(),qPrintable(finished.first()[1].toString()));QCOMPARE(renderer->scene()->entries().size(),1);QCOMPARE(renderer->scene()->entries().first().data->vertices.size(),87207);QCOMPARE(renderer->scene()->entries().first().data->triangles.size()/3,29069);QVERIFY(!renderer->scene()->entries().first().data->hasUvs);
    }

    void assistantStructureRulesAreAtomicPersistentAndRepeated(){
        HexView view;view.newMemoryDocument();QString error;QVERIFY(view.replaceSelection(QByteArray(1024,'x'),error));view.discardEdits();view.newMemoryDocument();QVERIFY(view.replaceSelection(QByteArray(1024,'x'),error));FormatPanel panel(&view);FormatRule user;user.name="User rule";user.length=4;view.setFormatRules({user});auto patches=view.pendingPatches();
        QJsonObject region{{"name","Position X Float32"},{"offset","0x80"},{"length","4"},{"step","16"},{"repetitions",8},{"evidence","Repeated layout inferred from inspected records"}};QJsonObject args{{"regions",QJsonArray{region}}};auto result=AiStructure::apply(&view,&panel,args);QVERIFY2(!result.contains("error"),qPrintable(result["error"].toString()));QCOMPARE(result["added"].toInt(),1);QCOMPARE(view.formatRules().size(),2);QCOMPARE(view.formatRules().first().name,QString("User rule"));auto r=view.formatRules().last();QVERIFY(r.name.contains("hypothesis"));QVERIFY(r.matches(128));QVERIFY(r.matches(128+7*16+3));QVERIFY(!r.matches(132));QCOMPARE(view.pendingPatches().size(),patches.size());
        auto repeat=AiStructure::apply(&view,&panel,args);QCOMPARE(repeat["added"].toInt(),0);QCOMPARE(repeat["duplicatesSkipped"].toInt(),1);QCOMPARE(view.formatRules().size(),2);
        auto bad=region;bad["offset"]="9223372036854775807";auto invalid=AiStructure::apply(&view,&panel,{{"regions",QJsonArray{region,bad}}});QVERIFY(invalid.contains("error"));QCOMPARE(view.formatRules().size(),2);bad=region;bad["step"]="0";QVERIFY(AiStructure::apply(&view,&panel,{{"regions",QJsonArray{bad}}}).contains("error"));bad=region;bad["repetitions"]=1.5;QVERIFY(AiStructure::apply(&view,&panel,{{"regions",QJsonArray{bad}}}).contains("error"));
        FormatRule restored;QVERIFY(decodeRule(encodeRule(r),restored));QCOMPARE(restored.start,r.start);QCOMPARE(restored.step,r.step);QCOMPARE(restored.repetitions,r.repetitions);QCOMPARE(restored.name,r.name);answer(QMessageBox::Cancel);QVERIFY(!panel.confirmLeave());answer(QMessageBox::Discard);QVERIFY(panel.confirmLeave());view.discardEdits();
    }
    void localOllamaStructureNaniSmoke(){
        if(!qEnvironmentVariableIsSet("WORKBENCH_AI_STRUCTURE_SMOKE"))QSKIP("Optional local structure annotation test.");auto source=qEnvironmentVariable("WORKBENCH_NANI_FIXTURE");QVERIFY(!source.isEmpty());QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("structure.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});{auto settings=aiSettings();settings.setValue("ai/context",8192);}
        MainWindow window(dir.filePath("layout.json"));window.openInPlace(source);QTRY_COMPARE(window.documentView()->fileSize(),qint64(16691362));auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));chat->refreshModels();auto *models=chat->findChild<QComboBox *>("ollamaModels");QTRY_VERIFY_WITH_TIMEOUT(models->count()>0,5000);models->setCurrentText("qwen3:4b");chat->findChild<QPlainTextEdit *>("aiQuestion")->setPlainText("Create Structure/Formatting annotations for the recognized headers and geometry in this binary in the Hex Viewer. Do not render a model.");chat->findChild<QPushButton *>("aiSend")->click();QTRY_VERIFY_WITH_TIMEOUT(!chat->isRunning(),180000);auto text=chat->findChild<QPlainTextEdit *>("aiTranscript")->toPlainText();QVERIFY2(!window.documentView()->formatRules().isEmpty(),qPrintable(text));bool positions=false;for(const auto &rule:window.documentView()->formatRules())if(rule.name.contains("Positions")&&rule.repetitions==87207)positions=true;QVERIFY2(positions,qPrintable(text));QVERIFY(!window.documentView()->dirty());QCOMPARE(window.findChild<VulkanPanel *>()->scene()->entries().size(),0);qInfo().noquote()<<text;
    }

    void meshSetupDraftRoundtripAndCloseProtection(){
        QTemporaryDir dir;MainWindow window(dir.filePath("layout.json"));window.show();auto *view=window.documentView();view->newMemoryDocument();auto *dock=window.sessionDock("meshControlsDock");auto *session=meshSetupSession(dock);QVERIFY(session);QVERIFY(session->empty());QVERIFY(!session->dirty());
        for(auto *button:dock->findChildren<QPushButton *>())QVERIFY(!button->text().contains("Sphere"));
        auto *offset=dock->findChild<QLineEdit *>("meshVertexOffset");QVERIFY(offset->text().isEmpty());QCOMPARE(dock->findChild<QComboBox *>("meshPositionType")->currentIndex(),-1);
        offset->setText("0x400");dock->findChild<QDoubleSpinBox *>("meshRotationX")->setValue(35);dock->findChild<QDoubleSpinBox *>("meshPositionDeltaX")->setValue(12.5);QVERIFY(session->dirty());
        answer(QMessageBox::Cancel);window.close();QVERIFY(window.isVisible());QCOMPARE(offset->text(),QString("0x400"));
        auto path=dir.filePath("draft.bwmesh");auto expected=session->state();QVERIFY(session->save(path));QVERIFY(!session->dirty());offset->setText("16");QVERIFY(session->dirty());auto result=AiDecoderSetup::loadMeshFile(dock,path,0);QVERIFY2(!result.contains("error"),qPrintable(result["error"].toString()));QCOMPARE(session->state(),expected);QVERIFY(!session->dirty());
        offset->setText("changed");answer(QMessageBox::Save);QVERIFY(session->confirmClose());QVERIFY(!session->dirty());QFile saved(path);QVERIFY(saved.open(QIODevice::ReadOnly));QCOMPARE(QJsonDocument::fromJson(saved.readAll()).object()["controls"].toObject()["meshVertexOffset"].toString(),QString("changed"));saved.close();
        QString error;QVERIFY(view->replaceSelection("DATA",error));QVERIFY(view->saveFile(dir.filePath("source.bin")));QTRY_VERIFY(!view->saving());QCOMPARE(offset->text(),QString("changed"));view->newMemoryDocument();QVERIFY(session->empty());QVERIFY(!session->dirty());QVERIFY(offset->text().isEmpty());QCOMPARE(dock->findChild<QDoubleSpinBox *>("meshRotationX")->value(),0.);
        offset->setText("128");answer(QMessageBox::Discard);window.close();QTRY_VERIFY(!window.isVisible());
    }
    void localOllamaAutonomousNaniSmoke(){
        if(!qEnvironmentVariableIsSet("WORKBENCH_AI_AUTONOMOUS_SMOKE"))QSKIP("Optional local autonomous asset test.");auto source=qEnvironmentVariable("WORKBENCH_NANI_FIXTURE");QVERIFY(!source.isEmpty());
        QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("autonomous.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});{auto settings=aiSettings();settings.setValue("ai/context",8192);}
        MainWindow window(dir.filePath("layout.json"));window.openInPlace(source);QTRY_COMPARE(window.documentView()->fileSize(),qint64(16691362));auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));chat->refreshModels();auto *models=chat->findChild<QComboBox *>("ollamaModels");QTRY_VERIFY_WITH_TIMEOUT(models->count()>0,5000);models->setCurrentText("qwen3:4b");
        chat->findChild<QPlainTextEdit *>("aiQuestion")->setPlainText("Inspect the current file and render its largest supported 3D model. Use the available tools to do it now.");chat->findChild<QPushButton *>("aiSend")->click();QTRY_VERIFY_WITH_TIMEOUT(!chat->isRunning(),180000);auto transcript=chat->findChild<QPlainTextEdit *>("aiTranscript")->toPlainText();auto *renderer=window.findChild<VulkanPanel *>();QVERIFY2(!renderer->scene()->entries().isEmpty(),qPrintable(transcript));QCOMPARE(renderer->scene()->entries().last().data->vertices.size(),87207);QVERIFY(!renderer->scene()->entries().last().data->hasUvs);QVERIFY2(transcript.contains("3D complete"),qPrintable(transcript));qInfo().noquote()<<transcript;
    }
    void lastOpenFolderPersistsAndCancelKeepsIt(){
        QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("folders.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});
        auto folder=dir.filePath("assets");QVERIFY(QDir().mkpath(folder));auto path=QDir(folder).filePath("fixture.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write("TEST");file.close();
        WorkbenchFiles::remember(path);QCOMPARE(WorkbenchFiles::lastDirectory(),folder);{auto settings=WorkbenchFiles::settings();QCOMPARE(settings.value("files/lastOpenDirectory").toString(),folder);}
        WorkbenchFiles::remember(QString());WorkbenchFiles::remember(dir.filePath("missing.bin"));QCOMPARE(WorkbenchFiles::lastDirectory(),folder);
        QString observed;QTimer::singleShot(0,qApp,[&]{auto *dialog=qobject_cast<QFileDialog *>(QApplication::activeModalWidget());if(dialog){observed=dialog->directory().absolutePath();dialog->reject();}});
        QVERIFY(WorkbenchFiles::getOpenFileName(nullptr,"Test folder",{}, {},nullptr,QFileDialog::DontUseNativeDialog).isEmpty());QCOMPARE(observed,folder);QCOMPARE(WorkbenchFiles::lastDirectory(),folder);
        auto next=dir.filePath("next");QVERIFY(QDir().mkpath(next));auto opened=QDir(next).filePath("opened.bin");QFile binary(opened);QVERIFY(binary.open(QIODevice::WriteOnly));binary.write("DATA");binary.close();
        MainWindow window(dir.filePath("layout.json"));window.openInPlace(opened);QTRY_COMPARE(window.documentView()->fileSize(),qint64(4));QTRY_COMPARE(WorkbenchFiles::lastDirectory(),next);
        {auto settings=WorkbenchFiles::settings();settings.setValue("files/lastOpenDirectory",QDir(next).filePath("deleted/subfolder"));}QCOMPARE(WorkbenchFiles::lastDirectory(),next);
    }
    void assistantFillsRealDecodersAndRejectsInvalidRanges(){
        QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("setup.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});
        MainWindow window(dir.filePath("layout.json"));auto *view=window.documentView();view->newMemoryDocument();QByteArray bytes(20000,0);for(int i=0;i<3;++i){float record[8]={float(i==1),float(i==2),0,0,0,1,float(i==1),float(i==2)};std::memcpy(bytes.data()+128+i*32,record,32);}qToLittleEndian<quint16>(0,bytes.data()+768);qToLittleEndian<quint16>(1,bytes.data()+770);qToLittleEndian<quint16>(2,bytes.data()+772);float delta[3]={10,20,30};std::memcpy(bytes.data()+1024,delta,12);for(int i=0;i<16;++i)bytes[4096+i]=char(255);
        QString error;QVERIFY(view->replaceSelection(bytes,error));auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));QVERIFY(chat);QVERIFY(chat->fillImageSettings&&chat->fillMeshSettings);
        auto *images=dynamic_cast<ImageWorkbench *>(window.findChild<QObject *>("imageWorkbench"));QVERIFY(images);
        QJsonObject image{{"offset","0x1000"},{"width",2},{"height",2},{"format","RGBA8"}};auto result=chat->fillImageSettings(image);QVERIFY2(!result.contains("error"),qPrintable(result["error"].toString()));QCOMPARE(images->imageCount(),0);QCOMPARE(window.findChild<QLineEdit *>("imageOffset")->text(),QString("4096"));QCOMPARE(window.findChild<QComboBox *>("imageSource")->currentIndex(),0);
        auto invalid=image;invalid["width"]=8192;QVERIFY(chat->fillImageSettings(invalid).contains("error"));QCOMPARE(window.findChild<QSpinBox *>("imageWidth")->value(),2);invalid=image;invalid["format"]="Indexed8";invalid["paletteOffset"]="19999";QVERIFY(chat->fillImageSettings(invalid).contains("error"));invalid=image;invalid["run_script"]="bad";QVERIFY(chat->fillImageSettings(invalid).contains("error"));
        chat->requestImageDecode();QTRY_VERIFY(!images->busy());QCOMPARE(images->imageCount(),1);QCOMPARE(images->entry(0).image.pixelColor(0,0),QColor(Qt::white));QVERIFY(!window.sessionDock("uvDock")->isHidden());QVERIFY(chat->findChild<QLabel *>("aiDecoderStatus")->text().startsWith("2D complete"));
        QJsonObject mesh{{"vertexOffset","0x80"},{"vertexCount",3},{"vertexStride",32},{"positionType","Float32"},{"positionScale",1.},{"indexOffset","0x300"},{"indexCount",3},{"topology","Triangle list"},{"uv",true},{"uvOffset","0x98"},{"uvStride",32},{"uvType","Float32"},{"normals",true},{"normalOffset","0x8c"},{"normalStride",32},{"normalType","Float32"},{"deltaFromBinary",true},{"deltaOffset","0x400"}};
        window.findChild<QDoubleSpinBox *>("meshRotationX")->setValue(90);window.findChild<QDoubleSpinBox *>("meshPositionDeltaX")->setValue(50);
        result=chat->fillMeshSettings(mesh);QVERIFY2(!result.contains("error"),qPrintable(result["error"].toString()));auto *renderer=window.findChild<VulkanPanel *>();QVERIFY(renderer);QCOMPARE(renderer->scene()->entries().size(),0);QCOMPARE(window.findChild<QDoubleSpinBox *>("meshRotationX")->value(),0.);QCOMPARE(window.findChild<QDoubleSpinBox *>("meshPositionDeltaX")->value(),0.);QCOMPARE(window.findChild<QComboBox *>("meshDeltaSource")->currentIndex(),1);QCOMPARE(window.findChild<QComboBox *>("meshTopology")->currentText(),QString("Triangle list"));
        invalid=mesh;invalid["vertexOffset"]="9223372036854775807";QVERIFY(chat->fillMeshSettings(invalid).contains("error"));QCOMPARE(window.findChild<QLineEdit *>("meshVertexOffset")->text(),QString("128"));invalid=mesh;invalid["vertexStride"]=1;QVERIFY(chat->fillMeshSettings(invalid).contains("error"));invalid=mesh;invalid["indexCount"]=3.5;QVERIFY(chat->fillMeshSettings(invalid).contains("error"));invalid=mesh;invalid["positionType"]="Unknown";QVERIFY(chat->fillMeshSettings(invalid).contains("error"));
        auto snapshot=chat->contextSnapshot();QCOMPARE(snapshot["imageSettings"].toObject()["width"].toInt(),2);QCOMPARE(snapshot["meshSettings"].toObject()["meshPositionScale"].toDouble(),1.);
        auto *render=window.findChild<QPushButton *>("renderMesh");chat->requestMeshDecode();QTRY_VERIFY(render->isEnabled());QVERIFY(!window.sessionDock("vulkanDock")->isHidden());QVERIFY(chat->findChild<QLabel *>("aiDecoderStatus")->text().startsWith("3D complete"));QCOMPARE(renderer->scene()->entries().size(),1);auto entry=renderer->scene()->entries().first();QCOMPARE(entry.data->triangles.size(),3);QCOMPARE(entry.positionDelta[0],10.);QCOMPARE(entry.positionDelta[1],20.);QCOMPARE(entry.positionDelta[2],30.);view->discardEdits();
    }
    void loadMeshSetupDisablesStaleInvalidUvReading(){
        QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("mesh.ini"));auto oldDialogs=QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs);QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs,true);auto restore=qScopeGuard([old,oldDialogs]{qApp->setProperty("workspaceSettingsFile",old);QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs,oldDialogs);});
        MainWindow window(dir.filePath("layout.json"));auto *view=window.documentView();view->newMemoryDocument();QByteArray bytes(4096,0);float positions[9]={0,0,0,1,0,0,0,1,0};std::memcpy(bytes.data()+128,positions,sizeof positions);for(int i=0;i<3;++i)qToLittleEndian<quint16>(i,bytes.data()+768+i*2);for(int i=0;i<6;++i)qToLittleEndian<quint32>(0x7fc00000,bytes.data()+1024+i*4);QString error;QVERIFY(view->replaceSelection(bytes,error));
        auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));auto *renderer=window.findChild<VulkanPanel *>();QSignalSpy finished(renderer,&VulkanPanel::meshCalculationFinished);
        QJsonObject bad{{"vertexOffset","128"},{"vertexCount",3},{"vertexStride",12},{"positionType","Float32"},{"positionScale",1.},{"indexOffset","768"},{"indexCount",3},{"uv",true},{"uvOffset","1024"},{"uvStride",8},{"uvType","Float32"}};QVERIFY(!chat->fillMeshSettings(bad).contains("error"));chat->requestMeshDecode();QTRY_COMPARE(finished.size(),1);QVERIFY(!finished.first()[0].toBool());QVERIFY(finished.first()[1].toString().contains("vertex 0 (offset 0x400)"));QCOMPARE(renderer->scene()->entries().size(),0);
        auto setup=bad;setup["uv"]=false;setup["normals"]=false;setup.remove("uvOffset");setup.remove("uvStride");setup.remove("uvType");auto path=dir.filePath("geometry-only.json");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(setup).toJson());file.close();
        QTimer::singleShot(0,qApp,[path]{auto *dialog=qobject_cast<QFileDialog *>(QApplication::activeModalWidget());if(dialog){dialog->selectFile(path);QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);}});auto *load=window.findChild<QPushButton *>("meshLoadSetup");QVERIFY(load);load->click();
        QVERIFY(!window.findChild<QCheckBox *>("meshReadUvs")->isChecked());QVERIFY(!window.findChild<QCheckBox *>("meshReadNormals")->isChecked());QVERIFY(window.findChild<QLabel *>("meshStatus")->text().contains("UVs off"));QCOMPARE(renderer->scene()->entries().size(),0);
        chat->requestMeshDecode();QTRY_COMPARE(finished.size(),2);QVERIFY(finished.last()[0].toBool());QCOMPARE(renderer->scene()->entries().size(),1);QVERIFY(!renderer->scene()->entries().first().data->hasUvs);QCOMPARE(renderer->scene()->entries().first().data->triangles.size(),3);QVERIFY(chat->findChild<QPlainTextEdit *>("aiTranscript")->toPlainText().contains("UVs off"));
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write("{invalid");file.close();auto result=AiDecoderSetup::loadMeshFile(window.sessionDock("meshControlsDock"),path,view->fileSize());QVERIFY(result.contains("error"));QVERIFY(!window.findChild<QCheckBox *>("meshReadUvs")->isChecked());view->discardEdits();
    }
    void naniGeometrySetupSmoke(){
        auto source=qEnvironmentVariable("WORKBENCH_NANI_FIXTURE"),setup=qEnvironmentVariable("WORKBENCH_NANI_SETUP");if(source.isEmpty()||setup.isEmpty())QSKIP("Optional local nani candidate test.");
        QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("nani.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});MainWindow window(dir.filePath("layout.json"));window.openInPlace(source);QTRY_COMPARE(window.documentView()->fileSize(),qint64(16691362));
        std::atomic_bool cancel=false;auto discovered=discoverAssets(window.documentView()->meshSource(),{},cancel);QVERIFY2(!discovered.contains("error"),qPrintable(discovered["error"].toString()));bool found=false;for(auto value:discovered["meshCandidates"].toArray())if(value.toObject()["settings"].toObject()["vertexCount"]==87207)found=true;QVERIFY(found);auto *dock=window.sessionDock("meshControlsDock");QVERIFY(!dock->findChild<QCheckBox *>("meshReadUvs")->isChecked());auto result=AiDecoderSetup::loadMeshFile(dock,setup,window.documentView()->fileSize());QVERIFY2(!result.contains("error"),qPrintable(result["error"].toString()));QVERIFY(!dock->findChild<QCheckBox *>("meshReadUvs")->isChecked());QVERIFY(!dock->findChild<QCheckBox *>("meshReadNormals")->isChecked());
        if(qEnvironmentVariableIsSet("WORKBENCH_GPU_TEST")){window.resize(1280,900);window.show();}
        auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));auto *renderer=window.findChild<VulkanPanel *>();QSignalSpy finished(renderer,&VulkanPanel::meshCalculationFinished);chat->requestMeshDecode();QTRY_COMPARE_WITH_TIMEOUT(finished.size(),1,20000);QVERIFY2(finished.first()[0].toBool(),qPrintable(finished.first()[1].toString()));QCOMPARE(renderer->scene()->entries().size(),1);auto mesh=renderer->scene()->entries().first().data;QCOMPARE(mesh->vertices.size(),87207);QCOMPARE(mesh->triangles.size()/3,29069);QVERIFY(!mesh->hasUvs);
        if(qEnvironmentVariableIsSet("WORKBENCH_GPU_TEST")){auto *status=renderer->findChild<QLabel *>("vulkanStatus");QTRY_VERIFY2_WITH_TIMEOUT(status->text().startsWith("Vulkan ·"),qPrintable(status->text()),15000);QTRY_VERIFY(status->text().contains("29069 triangles"));qInfo().noquote()<<status->text();}
    }
    void assistantDecodesEmbeddedEncodedImage(){
        QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("embedded.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});
        QImage expected(2,3,QImage::Format_RGBA8888);expected.fill(QColor(32,96,192));QByteArray png;QBuffer buffer(&png);QVERIFY(buffer.open(QIODevice::WriteOnly));QVERIFY(expected.save(&buffer,"PNG"));
        MainWindow window(dir.filePath("layout.json"));auto *view=window.documentView();view->newMemoryDocument();QString error;QVERIFY(view->replaceSelection(QByteArray(64,char(0xCD))+png,error));auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));
        QJsonObject args{{"offset","0x40"},{"width",1},{"height",1},{"format","Encoded image"}};auto result=chat->fillImageSettings(args);QVERIFY2(!result.contains("error"),qPrintable(result["error"].toString()));chat->requestImageDecode();auto *images=dynamic_cast<ImageWorkbench *>(window.findChild<QObject *>("imageWorkbench"));QTRY_VERIFY(!images->busy());QCOMPARE(images->imageCount(),1);QCOMPARE(images->entry(0).image.convertToFormat(QImage::Format_RGBA8888),expected);
        auto bad=args;bad["offset"]=QString::number(view->fileSize());QVERIFY(chat->fillImageSettings(bad).contains("error"));bad=args;bad["count"]=2;QVERIFY(chat->fillImageSettings(bad).contains("error"));view->discardEdits();
    }
    void chatDecoderErrorsAndModuleRouting(){
        QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("routing.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});
        WorkspaceWindow window(dir.filePath("layout.json"));window.show();auto *document=window.activeDocument();auto *view=document->documentView();QString error;QVERIFY(view->replaceSelection(QByteArray(4096,0),error));
        auto *chat=dynamic_cast<AiChatPanel *>(document->sessionDock("aiChatDock")->widget());QVERIFY(chat);auto *status=chat->findChild<QLabel *>("aiDecoderStatus");
        // Invalid current controls report the actual decoder failure, never "complete".
        chat->requestImageDecode();QVERIFY(status->text().startsWith("2D failed"));QVERIFY(!chat->decoderBusy(false));
        QJsonObject image{{"offset","0"},{"width",2},{"height",2},{"format","RGBA8"}};QVERIFY(!chat->fillImageSettings(image).contains("error"));window.selectModule("tools");chat->requestImageDecode();QTRY_VERIFY(!chat->decoderBusy(false));QVERIFY(status->text().startsWith("2D complete"));QVERIFY(window.moduleHost("uv")->isVisible());QVERIFY(document->sessionDock("uvDock")->isVisible());
        // Valid ranges with invalid index contents fail in the worker and propagate back to chat.
        view->selectMatch(768,6);QVERIFY(view->replaceSelection(QByteArray::fromHex("ffff01000200"),error));
        QJsonObject mesh{{"vertexOffset","128"},{"vertexCount",3},{"vertexStride",12},{"positionType","Float32"},{"positionScale",1.},{"indexOffset","768"},{"indexCount",3}};QVERIFY(!chat->fillMeshSettings(mesh).contains("error"));window.selectModule("tools");chat->requestMeshDecode();QTRY_VERIFY(!chat->decoderBusy(true));QVERIFY2(status->text().contains("Face index outside"),qPrintable(status->text()));QVERIFY(window.moduleHost("3d")->isVisible());QCOMPARE(document->sessionDock("vulkanDock")->findChild<VulkanPanel *>()->scene()->entries().size(),0);view->discardEdits();answer(QMessageBox::Discard);window.close();
    }
    void ps2FixtureChatDecodingSmoke(){
        auto fixture=qEnvironmentVariable("WORKBENCH_PS2_FIXTURE");if(fixture.isEmpty())QSKIP("Optional generated PS2 fixture test.");
        QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("fixture.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});MainWindow window(dir.filePath("layout.json"));window.openInPlace(fixture);QTRY_COMPARE(window.documentView()->fileSize(),qint64(217242));
        auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));auto *images=dynamic_cast<ImageWorkbench *>(window.findChild<QObject *>("imageWorkbench"));
        QJsonObject image{{"offset","4096"},{"width",128},{"height",128},{"format","PS2 RGBA8"},{"layout","PS2 GS pages"},{"ps2Alpha",true}};QVERIFY(!chat->fillImageSettings(image).contains("error"));chat->requestImageDecode();QTRY_VERIFY(!images->busy());QCOMPARE(images->imageCount(),1);QCOMPARE(images->entry(0).image.size(),QSize(128,128));QImage expected(QFileInfo(fixture).dir().filePath("answer-key/reactor_rgba.png"));QVERIFY(!expected.isNull());QCOMPARE(images->entry(0).image.convertToFormat(QImage::Format_ARGB32),expected.convertToFormat(QImage::Format_ARGB32));
        QJsonObject mesh{{"vertexOffset","133120"},{"vertexCount",24},{"vertexStride",32},{"positionType","Float32"},{"positionScale",1.},{"indexOffset","135168"},{"indexCount",36},{"uv",true},{"uvOffset","133144"},{"uvStride",32},{"uvType","Float32"},{"normals",true},{"normalOffset","133132"},{"normalStride",32},{"normalType","Float32"},{"deltaFromBinary",true},{"deltaOffset","137216"}};QVERIFY(!chat->fillMeshSettings(mesh).contains("error"));chat->requestMeshDecode();QTRY_VERIFY(!chat->decoderBusy(true));auto *renderer=window.findChild<VulkanPanel *>();QCOMPARE(renderer->scene()->entries().size(),1);auto body=renderer->scene()->entries().first();QCOMPARE(body.data->vertices.size(),24);QCOMPARE(body.data->triangles.size(),36);QCOMPARE(body.positionDelta[1],3.);QVERIFY(!window.sessionDock("uvDock")->isHidden());QVERIFY(!window.sessionDock("vulkanDock")->isHidden());
    }
    void localOllamaChatToolsAndCancellation(){
        QTemporaryDir directory;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",directory.filePath("ai.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});
        QVERIFY(localOllamaUrl(QUrl("http://127.0.0.1:11434")));QVERIFY(!localOllamaUrl(QUrl("https://example.com")));QVERIFY(!localOllamaUrl(QUrl("http://localhost@evil.test")));QVERIFY(!localOllamaUrl(QUrl("http://localhost/redirect")));
        QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost));QList<QJsonObject> requests;bool hang=false;int generation=0;QString requestedPlan="none";QString requestedTool="read_bytes";QJsonObject requestedArgs{{"offset","0"},{"count",4}};
        connect(&server,&QTcpServer::newConnection,this,[&]{auto *socket=server.nextPendingConnection();auto bytes=std::make_shared<QByteArray>();connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);connect(socket,&QTcpSocket::readyRead,socket,[&,socket,bytes]{*bytes+=socket->readAll();int header=bytes->indexOf("\r\n\r\n");if(header<0)return;auto headers=bytes->left(header);int length=0;for(auto line:headers.split('\n'))if(line.toLower().startsWith("content-length:"))length=line.mid(15).trimmed().toInt();if(bytes->size()<header+4+length)return;QByteArray body;
            if(headers.startsWith("GET"))body=R"({"models":[{"name":"qwen3:4b"},{"name":"test:cloud"}]})";
            else{auto request=QJsonDocument::fromJson(bytes->mid(header+4,length)).object();if(request.contains("format")){auto reply=QJsonDocument(QJsonObject{{"message",QJsonObject{{"role","assistant"},{"content",QString::fromUtf8(QJsonDocument(QJsonObject{{"action",requestedPlan}}).toJson(QJsonDocument::Compact))}}},{"done",true}}).toJson(QJsonDocument::Compact);socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "+QByteArray::number(reply.size())+"\r\nConnection: close\r\n\r\n"+reply);socket->disconnectFromHost();bytes->clear();return;}requests.append(request);if(hang)return;
                auto messages=request["messages"].toArray();bool toolResult=false;for(auto message:messages)if(message.toObject()["role"]=="tool")toolResult=true;
                QJsonObject message{{"role","assistant"},{"content",toolResult?"<think>hidden reasoning</think>Evidence: ABCD at 0x0.":""}};
                if(!toolResult){QJsonObject function{{"name",requestedTool},{"arguments",requestedArgs}};message["tool_calls"]=QJsonArray{QJsonObject{{"function",function}}};}
                QJsonObject response{{"message",message},{"done",true},{"prompt_eval_count",100},{"eval_count",20}};body=QJsonDocument(response).toJson(QJsonDocument::Compact)+"\n";++generation;
            }
            auto response="HTTP/1.1 200 OK\r\nContent-Type: application/x-ndjson\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body;int half=response.size()/2;socket->write(response.left(half));QTimer::singleShot(1,socket,[socket,response,half]{socket->write(response.mid(half));socket->disconnectFromHost();});bytes->clear();});});
        {auto settings=aiSettings();settings.setValue("ai/endpoint","http://127.0.0.1:"+QString::number(server.serverPort()));settings.setValue("ai/context",8192);settings.setValue("ai/instructions","Project: inspect test models.");}
        HexView view;view.newMemoryDocument();QString error;QVERIFY(view.replaceSelection("ABCD",error));MeshScene scene;AiChatPanel chat(&view,&scene);chat.show();chat.refreshModels();auto *models=chat.findChild<QComboBox *>("ollamaModels");QTRY_COMPARE(models->count(),1);auto *question=chat.findChild<QPlainTextEdit *>("aiQuestion");auto *send=chat.findChild<QPushButton *>("aiSend");auto *transcript=chat.findChild<QPlainTextEdit *>("aiTranscript");question->setPlainText("Read four bytes at zero.");send->click();QTRY_VERIFY_WITH_TIMEOUT(!chat.isRunning(),5000);QVERIFY(transcript->toPlainText().contains("Evidence: ABCD"));QCOMPARE(requests.size(),2);QCOMPARE(requests[0]["options"].toObject()["num_predict"].toInt(),1536);QVERIFY(requests[0]["messages"].toArray()[0].toObject()["content"].toString().contains("at most 80 words"));QVERIFY(!transcript->toPlainText().contains("hidden reasoning"));QVERIFY(!transcript->toPlainText().contains("<think>"));QVERIFY(requests[0]["messages"].toArray()[0].toObject()["content"].toString().contains("9XD"));QVERIFY(requests[0]["messages"].toArray()[0].toObject()["content"].toString().contains("Project: inspect test models."));auto tool=requests[1]["messages"].toArray();bool found=false;for(auto value:tool)if(value.toObject()["role"]=="tool"){auto output=QJsonDocument::fromJson(value.toObject()["content"].toString().toUtf8()).object();QCOMPARE(output["hex"].toString(),QString("41 42 43 44"));found=true;}QVERIFY(found);
        QJsonObject applied;int setups=0;chat.fillImageSettings=[&](const QJsonObject &args){applied=args;++setups;return QJsonObject{{"status","Image settings filled; nothing calculated."}};};requestedTool="set_image_settings";requestedArgs=QJsonObject{{"offset","0"},{"width",1},{"height",1},{"format","RGBA8"}};
        question->setPlainText("Fill the image settings with my supplied values.");send->click();QTRY_VERIFY_WITH_TIMEOUT(!chat.isRunning(),5000);QCOMPARE(setups,1);QCOMPARE(applied,requestedArgs);bool advertised=false;for(auto tool:requests[2]["tools"].toArray())if(tool.toObject()["function"].toObject()["name"]=="set_image_settings")advertised=true;QVERIFY(advertised);
        chat.findChild<QCheckBox *>("aiAllowSetup")->setChecked(false);question->setPlainText("Do not change settings.");send->click();QTRY_VERIFY_WITH_TIMEOUT(!chat.isRunning(),5000);QCOMPARE(setups,1);for(auto tool:requests[4]["tools"].toArray())QVERIFY(tool.toObject()["function"].toObject()["name"]!="set_image_settings");

        chat.findChild<QCheckBox *>("aiAllowSetup")->setChecked(true);requestedTool="run_decoder";requestedArgs=QJsonObject{{"kind","mesh"}};int runs=0;chat.renderMesh=[&]{++runs;return QJsonObject{{"started",true}};};
        auto beforeRun=requests.size();question->setPlainText("Render prepared mesh.");send->click();QTRY_COMPARE(runs,1);QVERIFY(chat.isRunning());QCOMPARE(requests.size(),beforeRun+1);chat.decoderFinished(true,false,"Invalid mesh contents");QTRY_VERIFY_WITH_TIMEOUT(!chat.isRunning(),5000);QCOMPARE(requests.size(),beforeRun+2);bool sawFailure=false;for(auto value:requests.last()["messages"].toArray())if(value.toObject()["role"]=="tool"){auto receipt=QJsonDocument::fromJson(value.toObject()["content"].toString().toUtf8()).object();sawFailure=receipt["error"]=="Invalid mesh contents";}QVERIFY(sawFailure);
        chat.findChild<QCheckBox *>("aiAllowRun")->setChecked(false);question->setPlainText("Calculation disabled.");send->click();QTRY_VERIFY_WITH_TIMEOUT(!chat.isRunning(),5000);QCOMPARE(runs,1);for(auto tool:requests.last()["tools"].toArray())QVERIFY(tool.toObject()["function"].toObject()["name"]!="run_decoder");

        QByteArray json=R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":42}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},{"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"}],"meshes":[{"primitives":[{"attributes":{"POSITION":0},"indices":1,"mode":4}]}]})";while(json.size()%4)json+=' ';QByteArray glb;auto append32=[&](quint32 n){char bytes[4];qToLittleEndian(n,bytes);glb.append(bytes,4);};glb+="glTF";append32(2);append32(12+8+json.size()+8+44);append32(json.size());glb+="JSON";glb+=json;append32(44);glb.append("BIN\0",4);glb+=QByteArray(44,0);view.newMemoryDocument();QVERIFY(view.replaceSelection(glb,error));
        QJsonObject autoSettings;chat.fillMeshSettings=[&](const QJsonObject &args){autoSettings=args;return QJsonObject{{"status","applied"}};};requestedPlan="mesh";chat.findChild<QCheckBox *>("aiAllowRun")->setChecked(true);question->setPlainText("Show the largest supported 3D model.");send->click();QTRY_COMPARE(runs,2);QVERIFY(chat.isRunning());QCOMPARE(autoSettings["vertexCount"].toInt(),3);QCOMPARE(autoSettings["uv"].toBool(true),false);QCOMPARE(autoSettings["normals"].toBool(true),false);chat.decoderFinished(true,true,"3 vertices loaded");QTRY_VERIFY_WITH_TIMEOUT(!chat.isRunning(),5000);requestedPlan="none";

        int annotations=0;chat.annotateStructure=[&](const QJsonObject &,bool metadata){++annotations;return QJsonObject{{"added",1},{"duplicatesSkipped",0}};};requestedTool="set_structure_rules";requestedArgs=QJsonObject{{"regions",QJsonArray{QJsonObject{{"name","Header"},{"offset","0"},{"length","4"},{"evidence","Read magic"}}}}};question->setPlainText("Mark my supplied header range.");send->click();QTRY_VERIFY_WITH_TIMEOUT(!chat.isRunning(),5000);QCOMPARE(annotations,1);chat.findChild<QCheckBox *>("aiAllowStructure")->setChecked(false);question->setPlainText("Keep annotations unchanged.");send->click();QTRY_VERIFY_WITH_TIMEOUT(!chat.isRunning(),5000);QCOMPARE(annotations,1);
        auto beforeHang=requests.size();
        chat.findChild<QCheckBox *>("aiAllowBytes")->setChecked(false);question->setPlainText("Explain the workspace.");hang=true;send->click();QTRY_COMPARE(requests.size(),beforeHang+1);for(auto tool:requests.last()["tools"].toArray())QVERIFY(tool.toObject()["function"].toObject()["name"]!="read_bytes");QVERIFY(view.replaceSelection("Z",error));QTRY_VERIFY(!chat.isRunning());QVERIFY(chat.findChild<QLabel *>("aiStatus")->text().contains("document changed"));view.discardEdits();
    }
    void localOllamaFillsDecoderSettingsSmoke(){
        if(!qEnvironmentVariableIsSet("WORKBENCH_AI_SETUP_SMOKE"))QSKIP("Optional installed Ollama decoder-setup test.");
        QTemporaryDir dir;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",dir.filePath("ai-setup.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});
        {auto settings=aiSettings();settings.setValue("ai/context",8192);}
        MainWindow window(dir.filePath("layout.json"));auto *view=window.documentView();view->newMemoryDocument();QString error;QVERIFY(view->replaceSelection(QByteArray(20000,0),error));
        auto *chat=dynamic_cast<AiChatPanel *>(window.findChild<QWidget *>("aiChatPanel"));QVERIFY(chat);chat->refreshModels();auto *models=chat->findChild<QComboBox *>("ollamaModels");QTRY_VERIFY_WITH_TIMEOUT(models->count()>0,5000);models->setCurrentText("qwen3:4b");
        auto *question=chat->findChild<QPlainTextEdit *>("aiQuestion");auto *send=chat->findChild<QPushButton *>("aiSend");auto *transcript=chat->findChild<QPlainTextEdit *>("aiTranscript");
        question->setPlainText("Please call set_image_settings now with these confirmed values: offset '4096', width 2, height 2, format 'RGBA8', layout 'Linear'. Do not calculate. Do not read bytes; I supplied all settings.");send->click();QTRY_VERIFY_WITH_TIMEOUT(!chat->isRunning(),180000);QVERIFY2(window.findChild<QSpinBox *>("imageWidth")->value()==2,qPrintable(transcript->toPlainText()+chat->findChild<QLabel *>("aiStatus")->text()));QCOMPARE(window.findChild<QLineEdit *>("imageOffset")->text(),QString("4096"));
        question->setPlainText("Please call set_mesh_settings now: vertexOffset '128', vertexCount 3, vertexStride 32, positionType 'Float32', positionScale 1, indexOffset '768', indexCount 3, topology 'Triangle list', uv false, normals false, deltaFromBinary true, deltaOffset '1024'. These are confirmed settings. Do not render or read bytes.");send->click();QTRY_VERIFY_WITH_TIMEOUT(!chat->isRunning(),180000);QVERIFY2(window.findChild<QLineEdit *>("meshVertexCount")->text()=="3",qPrintable(transcript->toPlainText()+chat->findChild<QLabel *>("aiStatus")->text()));QCOMPARE(window.findChild<QComboBox *>("meshDeltaSource")->currentIndex(),1);
        auto *images=dynamic_cast<ImageWorkbench *>(window.findChild<QObject *>("imageWorkbench"));QCOMPARE(images->imageCount(),0);QCOMPARE(window.findChild<VulkanPanel *>()->scene()->entries().size(),0);view->discardEdits();
    }
    void localOllamaInstalledSmoke(){
        if(!qEnvironmentVariableIsSet("WORKBENCH_OLLAMA_SMOKE"))QSKIP("Optional installed Ollama test.");
        QTemporaryDir directory;auto old=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",directory.filePath("ai.ini"));auto restore=qScopeGuard([old]{qApp->setProperty("workspaceSettingsFile",old);});{auto settings=aiSettings();settings.setValue("ai/context",8192);}
        HexView view;view.newMemoryDocument();QString error;QVERIFY(view.replaceSelection(QByteArray::fromHex("4142434400010203"),error));MeshScene scene;AiChatPanel chat(&view,&scene);chat.show();chat.refreshModels();auto *models=chat.findChild<QComboBox *>("ollamaModels");QTRY_VERIFY_WITH_TIMEOUT(models->count()>0,5000);int index=models->findText("qwen3:4b");QVERIFY(index>=0);models->setCurrentIndex(index);chat.findChild<QPlainTextEdit *>("aiQuestion")->setPlainText("Use read_bytes to read exactly 4 bytes at decimal offset 0. Report their hexadecimal values and ASCII text, without guessing.");chat.findChild<QPushButton *>("aiSend")->click();QTRY_VERIFY_WITH_TIMEOUT(!chat.isRunning(),180000);auto text=chat.findChild<QPlainTextEdit *>("aiTranscript")->toPlainText();QVERIFY2(text.contains("ABCD"),qPrintable(text));QVERIFY2(chat.findChild<QLabel *>("aiStatus")->text().startsWith("Complete"),qPrintable(chat.findChild<QLabel *>("aiStatus")->text()));if(qEnvironmentVariableIsSet("OLLAMA_SCREENSHOT"))chat.grab().save(qEnvironmentVariable("OLLAMA_SCREENSHOT"));view.discardEdits();
    }
    void customThemesPreviewApplyImportAndCancel(){
        auto original=qApp->property("workbenchTheme").toString();auto originalLibrary=qApp->property("workbenchThemeLibrary");auto restore=qScopeGuard([=]{qApp->setProperty("workbenchThemeLibrary",originalLibrary);applyWorkbenchTheme(original);});
        QTemporaryDir directory;QSettings settings(directory.filePath("theme.ini"),QSettings::IniFormat);settings.setValue("theme","dark");applyWorkbenchTheme("dark");auto initial=qApp->palette().color(QPalette::Window);int applied=0;SettingsDialog dialog(settings,[&]{++applied;});auto *editor=dynamic_cast<ThemeEditor *>(dialog.findChild<QWidget *>("themeEditor"));QVERIFY(editor);QVERIFY(!editor->setOverride("text.primary",Qt::red));auto custom=editor->createTheme("Midnight Teal");QVERIFY(custom.startsWith("custom:"));QVERIFY(editor->setOverride("background.window",QColor("#18282b")));QVERIFY(editor->setOverride("accent.selection",QColor("#216b73")));QVERIFY(editor->setOverride("graphics.background",QColor("#102a2c")));QVERIFY(editor->setOverride("calculator.bits",QColor("#70d8c2")));QVERIFY(editor->setOverride("hex.bytes",QColor("#b8e3db")));QVERIFY(!editor->setOverride("unknown.key",Qt::red));QCOMPARE(qApp->palette().color(QPalette::Window),initial);QVERIFY(!settings.contains("themes/v1"));
        auto *buttons=dialog.findChild<QDialogButtonBox *>("settingsButtons");buttons->button(QDialogButtonBox::Apply)->click();QCOMPARE(applied,1);QCOMPARE(qApp->property("workbenchTheme").toString(),custom);QCOMPARE(qApp->palette().color(QPalette::Window),QColor("#18282b"));QCOMPARE(graphicsBackground(),QColor("#102a2c"));VulkanPanel renderer;QCOMPARE(renderer.visualStyle().background,QColor("#102a2c"));ProgrammerCalculator calculator;QVERIFY(calculator.styleSheet().contains("#70d8c2"));
        // A graphics-only edit must refresh existing views even when palette roles do not change.
        QVERIFY(editor->setOverride("graphics.background",QColor("#123236")));buttons->button(QDialogButtonBox::Apply)->click();QCOMPARE(renderer.visualStyle().background,QColor("#123236"));
        QString error;auto path=directory.filePath("theme.bwtheme");QVERIFY(editor->exportTheme(path,error));QVERIFY(editor->importTheme(path,error));QCOMPARE(editor->current()["colors"].toObject()["hex.bytes"].toString(),QString("#b8e3db"));auto invalid=editor->current();invalid["version"]=99;QVERIFY(!validateTheme(invalid,error));invalid=editor->current();auto colors=invalid["colors"].toObject();colors["text.primary"]="red; background: url(x)";invalid["colors"]=colors;QVERIFY(!validateTheme(invalid,error));
        // Cancel keeps the last applied theme and excludes later imports/edits.
        auto saved=settings.value("themes/v1").toByteArray();QVERIFY(editor->setOverride("background.window",Qt::red));dialog.reject();QCOMPARE(settings.value("themes/v1").toByteArray(),saved);QCOMPARE(qApp->palette().color(QPalette::Window),QColor("#18282b"));
        SettingsDialog reopened(settings,[]{});auto *again=dynamic_cast<ThemeEditor *>(reopened.findChild<QWidget *>("themeEditor"));QVERIFY(again);QCOMPARE(again->current()["name"].toString(),QString("Midnight Teal"));QCOMPARE(again->current()["colors"].toObject()["graphics.background"].toString(),QString("#123236"));auto *sections=reopened.findChild<QTreeWidget *>("settingsSections");sections->setCurrentItem(sections->topLevelItem(1));reopened.show();QTest::qWait(30);if(qEnvironmentVariableIsSet("THEME_EDITOR_SCREENSHOT"))QVERIFY(reopened.grab().save(qEnvironmentVariable("THEME_EDITOR_SCREENSHOT")));
        auto *tokens=reopened.findChild<QTreeWidget *>("themeTokens");tokens->setCurrentItem(tokens->topLevelItem(0));auto *inherit=reopened.findChild<QCheckBox *>("themeUseDefault");inherit->setChecked(true);QVERIFY(!again->current()["colors"].toObject().contains("background.window"));reopened.reject();
    }
    void standardWorkspaceLayoutAndPersonalPersistence(){
        QTemporaryDir directory;auto previous=qApp->property("workspaceSettingsFile");qApp->setProperty("workspaceSettingsFile",directory.filePath("workspace.ini"));auto restore=qScopeGuard([previous]{qApp->setProperty("workspaceSettingsFile",previous);});auto statePath=directory.filePath("layout.json");Qt::DockWidgetArea standardArea,personalArea;QByteArray personalState;
        {auto settings=workspaceSettings();for(auto id:{"hex","3d","uv","scripting","tools"}){QVERIFY(!standardModuleLayout(id).isEmpty());QCOMPARE(workspaceModuleLayout(settings,id),standardModuleLayout(id));}QVERIFY(settings.allKeys().isEmpty());}
        {
            WorkspaceWindow window(statePath);window.show();window.selectModule("3d");auto *host=window.moduleHost("3d");auto *dock=window.activeDocument()->sessionDock("meshCollectionDock");standardArea=host->dockWidgetArea(dock);QVERIFY(standardArea!=Qt::NoDockWidgetArea);personalArea=standardArea==Qt::LeftDockWidgetArea?Qt::RightDockWidgetArea:Qt::LeftDockWidgetArea;host->addDockWidget(personalArea,dock);dock->show();QCoreApplication::processEvents();personalState=host->saveState();window.close();QVERIFY(!window.isVisible());
        }
        {auto settings=workspaceSettings();QCOMPARE(settings.value("moduleLayouts/v1/3d").toByteArray(),personalState);QCOMPARE(workspaceModuleLayout(settings,"3d"),personalState);}
        {
            WorkspaceWindow window(statePath);window.show();window.selectModule("3d");auto *host=window.moduleHost("3d");auto *dock=window.activeDocument()->sessionDock("meshCollectionDock");QCOMPARE(host->dockWidgetArea(dock),personalArea);window.resetPanelLayout();QCOMPARE(host->dockWidgetArea(dock),standardArea);window.close();
        }
        // Corrupt personal state falls back to the shipped layout without deleting other settings.
        {auto settings=workspaceSettings();settings.setValue("moduleLayouts/v1/3d",QByteArray("invalid"));settings.setValue("testPreference",42);}
        {WorkspaceWindow window(statePath);window.show();QCOMPARE(window.moduleHost("3d")->dockWidgetArea(window.activeDocument()->sessionDock("meshCollectionDock")),standardArea);window.close();}
        {auto settings=workspaceSettings();QCOMPARE(settings.value("testPreference").toInt(),42);}
    }
    void programmerCalculatorKeyboardAndWords(){
        ProgrammerCalculator calculator;calculator.show();auto *display=calculator.findChild<QLineEdit *>("calculatorDisplay");auto *base=calculator.findChild<QComboBox *>("calculatorBase");auto *word=calculator.findChild<QComboBox *>("calculatorWord");auto *sign=calculator.findChild<QCheckBox *>("calculatorSigned");
        QTest::keyClicks(display,"12+30");QTest::keyClick(display,Qt::Key_Return);QCOMPARE(display->text(),QString("42"));
        QTest::keyClick(display,Qt::Key_Escape);base->setCurrentIndex(0);QTest::keyClicks(display,"FF&0F=");QCOMPARE(display->text(),QString("F"));
        QTest::keyClick(display,Qt::Key_Escape);word->setCurrentIndex(3);QTest::keyClicks(display,"FF+1=");QCOMPARE(display->text(),QString("0"));
        QTest::keyClick(display,Qt::Key_Escape);QTest::keyClicks(display,"FF");sign->setChecked(true);base->setCurrentIndex(1);QCOMPARE(display->text(),QString("-1"));
        QTest::keyClicks(display,">1=");QCOMPARE(display->text(),QString("-1"));
        QTest::keyClick(display,Qt::Key_Escape);QTest::keyClicks(display,"7/0=");QVERIFY(calculator.findChild<QLabel *>("calculatorExpression")->text().contains("zero"));
        QTest::keyClick(display,Qt::Key_Escape);QApplication::clipboard()->setText("0x80");QTest::keyClick(display,Qt::Key_V,Qt::ControlModifier);QCOMPARE(display->text(),QString("-128"));QTest::keyClicks(display,"/1=");QCOMPARE(display->text(),QString("-128"));
        QTest::keyClick(display,Qt::Key_Escape);base->setCurrentIndex(0);QTest::keyClicks(display,"81");calculator.findChild<QPushButton *>("calculatorKey_ROL")->click();QCOMPARE(display->text(),QString("3"));calculator.findChild<QPushButton *>("calculatorKey_ROR")->click();QCOMPARE(display->text(),QString("81"));
        sign->setChecked(false);word->setCurrentIndex(0);QTest::keyClick(display,Qt::Key_Escape);QTest::keyClicks(display,"FFFFFFFFFFFFFFFF/1=");QCOMPARE(display->text(),QString("FFFFFFFFFFFFFFFF"));
        base->setCurrentIndex(3);QVERIFY(!calculator.findChild<QPushButton *>("calculatorKey_A")->isEnabled());QTest::keyClick(display,Qt::Key_Escape);QTest::keyClicks(display,"101");QTest::keyClick(display,Qt::Key_Backspace);QCOMPARE(display->text(),QString("10"));QTest::keyClick(display,Qt::Key_C,Qt::ControlModifier);QCOMPARE(QApplication::clipboard()->text(),QString("10"));
    }
    void scriptingLibraryManagerAndImports(){
        QTemporaryDir dir;auto old=qApp->property("scriptLibraryRoot");qApp->setProperty("scriptLibraryRoot",dir.filePath("own libraries"));auto restore=qScopeGuard([old]{qApp->setProperty("scriptLibraryRoot",old);});
        WorkspaceWindow workspace;workspace.show();auto *document=workspace.activeDocument();auto *dock=document->sessionDock("scriptLibraryDock");QVERIFY(dock);QCOMPARE(dock->parentWidget(),workspace.moduleHost("scripting"));auto *manager=dynamic_cast<ScriptLibraryManager *>(dock->widget());QVERIFY(manager);
        manager->findChild<QLineEdit *>("libraryName")->setText("custom_tools");manager->findChild<QPlainTextEdit *>("libraryCode")->setPlainText("def answer():\n    return 42\n");QVERIFY(manager->saveCurrent());QVERIFY(QFileInfo::exists(scriptLibraryFile("python","custom_tools.py")));
        auto *moduleTree=manager->findChild<QTreeWidget *>("scriptLibraries");QCOMPARE(moduleTree->topLevelItem(0)->text(0),QString("Default Modules"));QCOMPARE(moduleTree->topLevelItem(1)->text(0),QString("My Modules"));
        moduleTree->setCurrentItem(moduleTree->topLevelItem(0)->child(0));QVERIFY(manager->findChild<QPlainTextEdit *>("libraryCode")->isReadOnly());QVERIFY(manager->findChild<QTextBrowser *>("libraryDocumentation")->toPlainText().contains("Default Modules"));
        QTimer::singleShot(0,qApp,[]{auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());if(dialog){dialog->findChild<QLineEdit *>()->setText("dialog_module");dialog->findChild<QPlainTextEdit *>()->setPlainText("value = 9\n");dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();}});manager->findChild<QPushButton *>("libraryAdd")->click();QVERIFY(QFileInfo::exists(scriptLibraryFile("python","dialog_module.py")));
        QString error;QVERIFY(!saveScriptLibrary("python","../escape.py","bad",error));QVERIFY(!saveScriptLibrary("python","custom_tools.py","overwrite",error));QVERIFY(!scriptLibraryName("workbench"));QVERIFY(!scriptLibraryName("CON"));
        auto package=dir.filePath("demo_pkg");QVERIFY(QDir().mkpath(package));QFile init(QDir(package).filePath("__init__.py"));QVERIFY(init.open(QIODevice::WriteOnly));init.write("from .helper import value\n");init.close();QFile helper(QDir(package).filePath("helper.py"));QVERIFY(helper.open(QIODevice::WriteOnly));helper.write("value = 7\n");helper.close();QVERIFY2(importScriptLibraryFolder("python",package,error),qPrintable(error));
        QVERIFY(saveScriptLibrary("cpp","custom_tools.hpp","#pragma once\ninline int own_value(){return 42;}\n",error));
        auto *editor=document->sessionDock("scriptDock")->findChild<QPlainTextEdit *>("scriptEditor");auto *console=document->sessionDock("scriptConsoleDock")->findChild<QPlainTextEdit *>("scriptConsole");auto *run=document->sessionDock("scriptDock")->findChild<QPushButton *>("scriptRun");auto *language=document->sessionDock("scriptDock")->findChild<QComboBox *>("scriptLanguage");
        QVERIFY(editor&&console&&run&&language);editor->setPlainText("import custom_tools, demo_pkg\nassert custom_tools.answer()==42 and demo_pkg.value==7\nprint('OWN_PY_OK')");run->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),15000);QVERIFY2(console->toPlainText().contains("OWN_PY_OK"),qPrintable(console->toPlainText()));
        language->setCurrentIndex(1);editor->setPlainText("#include <iostream>\n#include \"custom_tools.hpp\"\nvoid run(){if(own_value()!=42)throw 1;std::cout<<\"OWN_CPP_OK\\n\";}");run->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),30000);QVERIFY2(console->toPlainText().contains("OWN_CPP_OK"),qPrintable(console->toPlainText()));
        workspace.selectModule("scripting");dock->show();QTRY_VERIFY_WITH_TIMEOUT(manager->isVisible(),1000);QTRY_VERIFY2_WITH_TIMEOUT(manager->findChild<QTreeWidget *>("scriptLibraries")->topLevelItem(0)->child(1)->childCount()>0,qPrintable(manager->findChild<QLabel *>("libraryStatus")->text()),10000);QVERIFY(manager->findChild<QTreeWidget *>("scriptLibraries")->topLevelItemCount()==2);
        auto *tree=manager->findChild<QTreeWidget *>("scriptLibraries");auto items=tree->findItems("custom_tools.py",Qt::MatchExactly|Qt::MatchRecursive,0);QCOMPARE(items.size(),1);tree->setCurrentItem(items.first());manager->findChild<QPushButton *>("libraryToggle")->click();QVERIFY(QFileInfo::exists(scriptLibraryFile("python","custom_tools.py.disabled")));QVERIFY(!QFileInfo::exists(scriptLibraryFile("python","custom_tools.py")));
        items=tree->findItems("custom_tools.py.disabled",Qt::MatchExactly|Qt::MatchRecursive,0);QCOMPARE(items.size(),1);tree->setCurrentItem(items.first());manager->findChild<QPushButton *>("libraryToggle")->click();QVERIFY(QFileInfo::exists(scriptLibraryFile("python","custom_tools.py")));
        QVERIFY(manager->newLibrary());manager->findChild<QPlainTextEdit *>("libraryCode")->setPlainText("# unsaved draft");answer(QMessageBox::Cancel);QVERIFY(!manager->confirmClose());answer(QMessageBox::Discard);QVERIFY(manager->newLibrary());

    }
    void imageRecipeFilesAndMeshTextures(){
        QTemporaryDir dir;WorkspaceWindow workspace;workspace.show();auto *document=workspace.activeDocument();auto *tools=dynamic_cast<ImageWorkbench *>(document->sessionDock("uvDock")->findChild<QObject *>("imageWorkbench"));QVERIFY(tools);
        ImageRecipe recipe;recipe.format="BC1";recipe.width=8;recipe.height=4;recipe.offset=16;recipe.flipX=true;tools->applyRecipe(recipe);auto path=dir.filePath("recipe.bwir");QVERIFY(tools->saveRecipe(path));tools->applyRecipe(ImageRecipe{});QVERIFY(tools->loadRecipe(path));auto *settings=document->sessionDock("imageCalculatorDock");QCOMPARE(settings->findChild<QComboBox *>("imageFormat")->currentText(),QString("BC1"));QCOMPARE(settings->findChild<QSpinBox *>("imageWidth")->value(),8);
        auto *renderer=static_cast<VulkanPanel *>(document->sessionDock("vulkanDock")->widget());auto mesh=std::make_shared<MeshData>();mesh->vertices={{{0,0,0},{0,0,1},{0,0}},{{1,0,0},{0,0,1},{1,0}},{{0,1,0},{0,0,1},{0,1}}};mesh->triangles={0,1,2};int first=renderer->scene()->add(mesh,"Textured"),second=renderer->scene()->add(mesh,"Other");renderer->scene()->setActive(first);QImage texture(2,2,QImage::Format_RGBA8888);texture.fill(Qt::red);QVERIFY(tools->assignTexture(texture,"Red"));QCOMPARE(renderer->scene()->entry(first)->texture.pixelColor(0,0),QColor(Qt::red));QVERIFY(renderer->scene()->entry(second)->texture.isNull());QCOMPARE(document->sessionDock("uvDock")->findChild<QTabWidget *>("uvImageTabs")->currentIndex(),1);
        renderer->scene()->setActive(second);QVERIFY(renderer->scene()->setTexture(second,texture,"Second"));QVERIFY(renderer->scene()->setTexture(first,{},{}));QVERIFY(renderer->scene()->entry(first)->texture.isNull());QVERIFY(!renderer->scene()->entry(second)->texture.isNull());
        auto noUvs=std::make_shared<MeshData>(*mesh);noUvs->hasUvs=false;int third=renderer->scene()->add(noUvs);QVERIFY(!renderer->scene()->setTexture(third,texture,"Invalid"));workspace.close();QTRY_VERIFY(!workspace.isVisible());
    }
    void toolbarsMenuControlsWorkspaceBars(){
        WorkspaceWindow workspace;workspace.show();auto *menu=workspace.findChild<QMenu *>("toolbarsMenu");QVERIFY(menu);QVERIFY(workspace.menuBar()->actions().contains(menu->menuAction()));
        QMetaObject::invokeMethod(menu,"aboutToShow",Qt::DirectConnection);QCOMPARE(menu->actions().size(),1);auto *bar=workspace.findChild<QToolBar *>("mainToolbar");QVERIFY(bar);auto *toggle=menu->actions().first();QVERIFY(toggle->isChecked());toggle->trigger();QVERIFY(bar->isHidden());
        workspace.newDocument();QVERIFY(bar->isHidden());QVERIFY(workspace.menuBar()->actions().contains(menu->menuAction()));toggle->trigger();QVERIFY(!bar->isHidden());
        auto *extra=workspace.addToolBar("Extra tools");extra->setObjectName("extraToolbar");QTRY_VERIFY(extra->isVisible());QMetaObject::invokeMethod(menu,"aboutToShow",Qt::DirectConnection);QCOMPARE(menu->actions().size(),2);QVERIFY(menu->actions().contains(extra->toggleViewAction()));extra->toggleViewAction()->trigger();QVERIFY(extra->isHidden());workspace.close();QTRY_VERIFY(!workspace.isVisible());
    }
    void advancedSearchAndCompareInspectors(){
        QTemporaryDir dir;auto path=dir.filePath("reference.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write("ABXDE");file.close();
        WorkspaceWindow workspace;workspace.show();auto *document=workspace.activeDocument();auto *view=document->documentView();QString error;QVERIFY(view->replaceSelection("ABCD",error));
        auto *dock=document->sessionDock("compareDock");QVERIFY(dock);QCOMPARE(dock->parentWidget(),workspace.moduleHost("hex"));dock->show();
        auto *panel=dynamic_cast<ComparePanel *>(dock->widget());QVERIFY(panel);panel->findChild<QLineEdit *>("comparePath")->setText(path);panel->findChild<QPushButton *>("compareStart")->click();auto *results=panel->findChild<QTreeWidget *>("compareResults");QTRY_COMPARE(results->topLevelItemCount(),2);results->setCurrentItem(results->topLevelItem(0));QCOMPARE(view->selection().cursor,qint64(2));
        auto *search=document->sessionDock("searchDock");search->show();auto *pattern=search->findChild<QLineEdit *>("patternExpression");pattern->setText("41");QAbstractButton *find=nullptr;for(auto *button:search->findChildren<QAbstractButton *>())if(button->text()=="Find")find=button;QVERIFY(find);find->click();auto *matches=search->findChild<QTreeWidget *>("patternResults");QTRY_COMPARE(matches->topLevelItemCount(),1);QCOMPARE(matches->topLevelItem(0)->text(0),QString("0x0"));
        QVERIFY(view->replaceSelection("Z",error));QCOMPARE(results->topLevelItemCount(),0);QCOMPARE(matches->topLevelItemCount(),0);view->discardEdits();workspace.close();QTRY_VERIFY(!workspace.isVisible());
    }
    void hexDisplayControlsAndOffsetNavigation(){
        HexView view;view.resize(1400,600);view.show();view.newMemoryDocument();QString error;
        QVERIFY(view.replaceSelection(QByteArray(512,'A'),error));view.jumpTo(0);QTRY_VERIFY(!view.busy());
        auto *table=view.findChild<QTableWidget *>("bytes");auto *address=view.findChild<QLineEdit *>("offsetHeaderEditor");auto *stride=view.findChild<QSpinBox *>("stride");
        QCOMPARE(table->item(1,0)->text(),QString("0x00000010"));
        view.setOffsetBase(10);QCOMPARE(table->item(1,0)->text(),QString("00000016"));
        address->setText("31");QTest::keyClick(address,Qt::Key_Return);QCOMPARE(view.selection().cursor,qint64(31));
        view.setOffsetBase(8);QVERIFY(table->horizontalHeaderItem(0)->text().contains(": 0c"));address->setText("0c40");QTest::keyClick(address,Qt::Key_Return);QCOMPARE(view.selection().cursor,qint64(32));
        address->setText("0x2F");QTest::keyClick(address,Qt::Key_Return);QCOMPARE(view.selection().cursor,qint64(47));
        address->setText("0d9223372036854775808");QTest::keyClick(address,Qt::Key_Return);QCOMPARE(view.selection().cursor,qint64(47));
        view.jumpTo(0);QTRY_VERIFY(!view.busy());view.setColumnMode(2);QVERIFY(table->isColumnHidden(1));QVERIFY(!table->isColumnHidden(17));
        auto textRect=table->visualItemRect(table->item(0,17));QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,QPoint(textRect.left()+8,textRect.center().y()));QCOMPARE(view.selection().cursor,qint64(0));
        view.setColumnMode(1);QVERIFY(!table->isColumnHidden(1));QVERIFY(table->isColumnHidden(17));
        QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,table->visualItemRect(table->item(0,4)).center());QCOMPARE(view.selection().cursor,qint64(3));QVERIFY(table->horizontalHeaderItem(0)->text().endsWith(": 0c3"));
        view.setByteGroup(3);QVERIFY(table->columnWidth(3)>table->columnWidth(2));QCOMPARE(table->item(0,4)->font(),table->item(0,17)->font());
        QCOMPARE(table->item(0,0)->font(),table->item(0,17)->font());QCOMPARE(table->horizontalHeaderItem(0)->font(),table->item(0,17)->font());QCOMPARE(table->horizontalHeader()->font(),table->font());
        QTest::mouseClick(table->horizontalHeader()->viewport(),Qt::LeftButton,Qt::NoModifier,QPoint(40,10));auto *editor=view.findChild<QLineEdit *>("offsetHeaderEditor");QVERIFY(editor->isVisible());editor->setText("0d48");QTest::keyClick(editor,Qt::Key_Return);QCOMPARE(view.selection().cursor,qint64(48));QVERIFY(!editor->isVisible());
        view.setColumnMode(0);stride->setValue(7);QTRY_VERIFY(!view.busy());QVERIFY(!table->isColumnHidden(8));QCOMPARE(view.byteGroup(),3);QCOMPARE(view.selection().cursor,qint64(48));
        QTimer::singleShot(0,&view,[]{auto *dialog=qobject_cast<QInputDialog *>(QApplication::activeModalWidget());if(dialog){dialog->setIntValue(24);dialog->accept();}});view.chooseBytesPerRow();QCOMPARE(stride->value(),24);
        view.discardEdits();view.close();
    }
    void imageCalculatorCompressedRamAndContainerMip(){
        WorkspaceWindow workspace;workspace.show();auto *document=workspace.activeDocument();auto *view=document->documentView();QString error;
        QVERIFY(view->replaceSelection(QByteArray::fromHex("00f8000000000000"),error));auto *dock=document->sessionDock("imageCalculatorDock");dock->toggleViewAction()->trigger();
        auto *format=dock->findChild<QComboBox *>("imageFormat");format->setCurrentText("BC1");dock->findChild<QSpinBox *>("imageWidth")->setValue(3);dock->findChild<QSpinBox *>("imageHeight")->setValue(2);auto *calculate=dock->findChild<QPushButton *>("imageCalculate");calculate->click();auto *tools=dynamic_cast<ImageWorkbench *>(document->sessionDock("uvDock")->findChild<QObject *>("imageWorkbench"));QVERIFY(tools);QTRY_COMPARE(tools->imageCount(),1);QCOMPARE(tools->entry(0).image.pixelColor(2,1),QColor(Qt::red));
        QByteArray dds(128,0);dds.replace(0,4,"DDS ");auto put=[&](int at,quint32 v){qToLittleEndian(v,reinterpret_cast<uchar *>(dds.data()+at));};put(4,124);put(12,4);put(16,4);put(28,2);put(76,32);put(80,4);dds.replace(84,4,"DXT1");dds+=QByteArray::fromHex("00f8000000000000e007000000000000");view->selectWholeDocument();QVERIFY(view->replaceSelection(dds,error));dock->findChild<QComboBox *>("imageSource")->setCurrentIndex(0);format->setCurrentText("Encoded image");auto *mip=dock->findChild<QSpinBox *>("imageMipLevel");QVERIFY(mip->isEnabled());mip->setValue(1);calculate->click();QTRY_COMPARE(tools->imageCount(),2);QCOMPARE(tools->entry(1).image.size(),QSize(2,2));QCOMPARE(tools->entry(1).image.pixelColor(0,0),QColor(Qt::green));
        QCOMPARE(dock->parentWidget(),workspace.moduleHost("uv"));QVERIFY(dock->findChild<QComboBox *>("imageMemoryLayout")->findText("Xbox 360 tiled")>=0);QVERIFY(dock->findChild<QComboBox *>("imagePaletteFormat")->findText("PS1 BGR555")>=0);
        view->discardEdits();workspace.close();QTRY_VERIFY(!workspace.isVisible());
    }
    void imageLibraryUsesDocumentSnapshotsAndOwnModule(){
        WorkspaceWindow workspace;workspace.show();auto *document=workspace.activeDocument();auto *view=document->documentView();QString error;
        QVERIFY(view->replaceSelection(QByteArray::fromHex("ff0000ff00ff00ff"),error));
        auto *library=document->sessionDock("imageLibraryDock"),*calculator=document->sessionDock("imageCalculatorDock");QVERIFY(library);QVERIFY(calculator);
        calculator->toggleViewAction()->trigger();QCOMPARE(calculator->parentWidget(),workspace.moduleHost("uv"));QCOMPARE(library->parentWidget(),workspace.moduleHost("uv"));
        auto *width=calculator->findChild<QSpinBox *>("imageWidth"),*height=calculator->findChild<QSpinBox *>("imageHeight");width->setValue(2);height->setValue(1);
        auto *button=calculator->findChild<QPushButton *>("imageCalculate");button->click();auto *list=library->findChild<QListWidget *>("imageLibrary");QTRY_COMPARE(list->count(),1);
        auto *tools=dynamic_cast<ImageWorkbench *>(document->sessionDock("uvDock")->findChild<QObject *>("imageWorkbench"));QVERIFY(tools);QCOMPARE(tools->entry(0).image.pixelColor(0,0),QColor(Qt::red));QCOMPARE(tools->entry(0).image.pixelColor(1,0),QColor(Qt::green));
        QTemporaryDir temp;const auto path=temp.filePath("library.bwil");tools->saveLibrary(path);QVERIFY(QFileInfo::exists(path));tools->loadLibrary(path);QCOMPARE(list->count(),2);QCOMPARE(tools->entry(1).image,tools->entry(0).image);
        calculator->findChild<QComboBox *>("imageSource")->setCurrentIndex(0);width->setValue(1);calculator->findChild<QSpinBox *>("imageCount")->setValue(2);button->click();QTRY_COMPARE(list->count(),4);QCOMPARE(tools->entry(3).recipe.offset,qint64(4));QCOMPARE(tools->entry(3).image.pixelColor(0,0),QColor(Qt::green));
        const auto folder=temp.filePath("images");QVERIFY(QDir().mkdir(folder));QImage blue(3,2,QImage::Format_ARGB32);blue.fill(Qt::blue);QVERIFY(blue.save(folder+"/test.png"));tools->openFolder(folder);QCOMPARE(list->count(),5);list->setCurrentRow(4);QCOMPARE(calculator->findChild<QComboBox *>("imageFormat")->currentText(),QString("Encoded image"));calculator->findChild<QSpinBox *>("imageCount")->setValue(1);button->click();QTRY_COMPARE(list->count(),6);QCOMPARE(tools->entry(5).image.pixelColor(2,1),QColor(Qt::blue));
        workspace.newDocument();QCOMPARE(workspace.activeDocument()->sessionDock("imageLibraryDock")->findChild<QListWidget *>("imageLibrary")->count(),0);
        view->discardEdits();workspace.close();QTRY_VERIFY(!workspace.isVisible());
    }
    void checksumInspectorVerificationAndDocumentIsolation(){
        WorkspaceWindow workspace;workspace.show();auto *first=workspace.activeDocument();auto *view=first->documentView();QString error;
        QVERIFY(view->replaceSelection("123456789",error));auto *dock=first->sessionDock("checksumDock");QVERIFY(dock);
        auto *action=dock->toggleViewAction();QVERIFY(first->findChild<QMenu *>("moduleHexMenu")->actions().contains(action));QVERIFY(action->isEnabled());action->trigger();
        QCOMPARE(dock->parentWidget(),workspace.moduleHost("hex"));QVERIFY(dock->isVisible());
        auto *algorithm=dock->findChild<QComboBox *>("checksumAlgorithm");algorithm->setCurrentIndex(algorithm->findData("CRC-32"));
        auto *expected=dock->findChild<QLineEdit *>("checksumExpected");expected->setText("0x cb f4 39 26");
        auto *calculate=dock->findChild<QPushButton *>("checksumCalculate");QTRY_VERIFY(calculate->isEnabled());
        QSignalSpy finished(view,&HexView::checksumFinished);calculate->click();workspace.newDocument();
        auto *second=workspace.activeDocument();QVERIFY(second!=first);QTRY_COMPARE(finished.count(),1);
        auto *results=dock->findChild<QTableWidget *>("checksumResults");QCOMPARE(results->rowCount(),1);
        QCOMPARE(results->item(0,1)->text(),QString("CBF43926"));QCOMPARE(results->item(0,2)->text(),QString("Match"));
        QCOMPARE(second->sessionDock("checksumDock")->findChild<QTableWidget *>("checksumResults")->rowCount(),0);
        expected->setText("00000000");QCOMPARE(results->item(0,2)->text(),QString("Mismatch"));expected->clear();QCOMPARE(results->item(0,2)->text(),QString("Not compared"));
        expected->setText("xyz");calculate->click();QVERIFY(!view->checksumBusy());QVERIFY(dock->findChild<QLabel *>("checksumStatus")->text().contains("expected result"));
        expected->clear();auto *scope=dock->findChild<QComboBox *>("checksumScope");scope->setCurrentIndex(2);
        dock->findChild<QLineEdit *>("checksumStart")->setText("0x1");dock->findChild<QLineEdit *>("checksumLength")->setText("3");
        calculate->click();QTRY_COMPARE(finished.count(),2);QCOMPARE(results->rowCount(),1);
        QCOMPARE(results->item(0,1)->text(),QString("0D717969"));
        scope->setCurrentIndex(0);algorithm->setCurrentIndex(algorithm->findData("all"));calculate->click();
        view->selectMatch(0,1);QVERIFY(view->replaceSelection("A",error));QTRY_VERIFY(!view->checksumBusy());QCOMPARE(results->rowCount(),0);
        QVERIFY(dock->findChild<QLabel *>("checksumStatus")->text().contains("content changed"));
        view->discardEdits();workspace.close();QTRY_VERIFY(!workspace.isVisible());
    }
    void compactTabsAndPinnedHexFooter(){
        WorkspaceWindow workspace;workspace.resize(1100,760);workspace.show();QTest::qWait(40);
        auto *host=workspace.moduleHost("hex");auto *first=workspace.activeDocument();
        auto *mdi=host->findChild<QMdiArea *>("documentWorkspace");QVERIFY(mdi);
        auto *tabs=mdi->findChild<QTabBar *>("documentTabs");QVERIFY(tabs);QVERIFY(!tabs->expanding());
        QTRY_VERIFY(tabs->tabRect(0).width()<=190);QVERIFY(tabs->tabRect(0).width()<tabs->width()/2);
        auto *firstFooter=first->documentView()->statusFooter();QCOMPARE(firstFooter->parentWidget(),host->statusBar());
        auto footerAtBottom=[host](QWidget *footer){return footer->mapTo(host,QPoint()).y()+footer->height()>=host->height()-4;};
        QTRY_VERIFY(footerAtBottom(firstFooter));QVERIFY(!first->documentView()->isAncestorOf(firstFooter));
        auto *inspector=first->sessionDock("inspectorDock");
        QVERIFY(inspector->mapTo(host,QPoint()).y()+inspector->height()<=firstFooter->mapTo(host,QPoint()).y());
        for(int i=0;i<2;++i)workspace.newDocument();QTest::qWait(40);
        QCOMPARE(tabs->count(),3);QCOMPARE(tabs->tabRect(0).width(),tabs->tabRect(1).width());
        const int regularTabWidth=tabs->tabRect(0).width();
        QVERIFY(tabs->tabRect(0).width()<=190);QCOMPARE(firstFooter->parentWidget(),first->documentView());QVERIFY(firstFooter->isHidden());
        for(int i=0;i<8;++i)workspace.newDocument();QTest::qWait(40);
        QCOMPARE(tabs->count(),11);QVERIFY(tabs->usesScrollButtons());
        QTRY_VERIFY(tabs->tabRect(0).width()<regularTabWidth);
        QVERIFY(tabs->tabRect(0).width()>=100);QVERIFY(tabs->tabRect(0).width()<=190);
        auto *footer=workspace.activeDocument()->documentView()->statusFooter();QCOMPARE(footer->parentWidget(),host->statusBar());
        workspace.detachModule("hex");workspace.moduleWindow("hex")->resize(850,600);QTest::qWait(40);
        QTRY_VERIFY(footerAtBottom(footer));QCOMPARE(footer->findChild<QToolButton *>("threadActivityToggle")->parentWidget(),footer);
        workspace.attachModule("hex");QTest::qWait(30);QTRY_VERIFY(footerAtBottom(footer));
        workspace.close();QTRY_VERIFY(!workspace.isVisible());
    }
    void moduleWindowEntryPoints(){
        WorkspaceWindow workspace;workspace.show();
        auto *tabs=workspace.findChild<QTabBar *>("moduleTabs");QVERIFY(tabs);
        const auto position=tabs->tabRect(2).center();QTest::mousePress(tabs,Qt::LeftButton,Qt::NoModifier,position);
        const QPoint outside(position.x(),tabs->height()+40);
        QMouseEvent drag(QEvent::MouseMove,QPointF(outside),QPointF(tabs->mapToGlobal(outside)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(tabs,&drag);QTest::mouseRelease(tabs,Qt::LeftButton,Qt::NoModifier,outside);
        QVERIFY(workspace.isModuleDetached("uv"));
        QTest::mouseDClick(tabs,Qt::LeftButton,Qt::NoModifier,tabs->tabRect(2).center());QVERIFY(!workspace.isModuleDetached("uv"));
        QVERIFY(workspace.moduleHost("scripting")->findChildren<QToolBar *>().isEmpty());
        auto *menu=workspace.findChild<QMenu *>("moduleWindowsMenu");QVERIFY(menu);menu->actions()[1]->trigger();
        QVERIFY(workspace.isModuleDetached("3d"));workspace.moduleWindow("3d")->close();QVERIFY(!workspace.isModuleDetached("3d"));
        workspace.close();
    }
    void detachableModulesPreserveDocuments(){
        WorkspaceWindow workspace;workspace.show();auto *first=workspace.activeDocument();QVERIFY(first);
        QString error;QVERIFY(first->documentView()->replaceSelection("AB",error));first->documentView()->selectMatch(1,1);
        workspace.detachModule("3d");QVERIFY(workspace.isModuleDetached("3d"));
        auto *three=workspace.moduleWindow("3d");QVERIFY(three);QVERIFY(three->isWindow());QVERIFY(three->isVisible());
        QCOMPARE(three->centralWidget(),workspace.moduleHost("3d"));QVERIFY(!workspace.moduleHost("3d")->isWindow());
        workspace.selectModule("hex");QVERIFY(three->isVisible());QCOMPARE(workspace.activeDocument(),first);
        auto *controls=first->sessionDock("meshControlsDock");controls->setFloating(true);controls->show();
        workspace.selectModule("hex");QVERIFY(controls->isVisible());QCOMPARE(controls->parentWidget(),workspace.moduleHost("3d"));
        workspace.detachModule("hex");QVERIFY(workspace.moduleWindow("hex")->isVisible());QVERIFY(three->isVisible());
        QCOMPARE(readPage(first->documentView()->sourceRequest(0,2)).bytes,QByteArray("AB"));
        three->activateWindow();QTest::qWait(20);QTest::keyClick(three,Qt::Key_N,Qt::ControlModifier);QCOMPARE(workspace.documentCount(),2);
        auto *second=workspace.activeDocument();QVERIFY(second!=first);QVERIFY(three->windowTitle().contains("Untitled 2"));
        QCOMPARE(second->sessionDock("meshCollectionDock")->parentWidget(),workspace.moduleHost("3d"));
        QCOMPARE(first->sessionDock("meshCollectionDock")->parentWidget(),first);
        auto *mdi=workspace.moduleHost("hex")->findChild<QMdiArea *>("documentWorkspace");QVERIFY(mdi);mdi->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(first->parentWidget()));
        QCOMPARE(workspace.activeDocument(),first);QCOMPARE(first->documentView()->selection().anchor,qint64(1));
        QVERIFY(three->windowTitle().contains("Untitled 1"));
        workspace.moduleWindow("hex")->close(); // Closing returns the module; it never closes documents.
        QVERIFY(!workspace.isModuleDetached("hex"));QCOMPARE(workspace.documentCount(),2);QVERIFY(first->documentView()->dirty());
        workspace.attachModule("3d");QVERIFY(!workspace.isModuleDetached("3d"));
        workspace.detachModule("uv",QPoint(-100000,-100000));QVERIFY(workspace.moduleWindow("uv")->isVisible());
        QVERIFY(workspace.moduleWindow("uv")->frameGeometry().intersects(workspace.screen()->availableGeometry()));
        workspace.detachModule("scripting");QVERIFY(workspace.moduleWindow("scripting")->isVisible());
        answer(QMessageBox::Cancel);QVERIFY(!workspace.close());QVERIFY(workspace.moduleWindow("uv")->isVisible());
        first->documentView()->discardEdits();workspace.close();QTRY_VERIFY(!workspace.isVisible());QVERIFY(!workspace.moduleWindow("uv")->isVisible());
    }
    void moduleMenusAndDockIsolation(){
        WorkspaceWindow workspace;workspace.show();auto *doc=workspace.activeDocument();QVERIFY(doc);
        QStringList menus;for(auto *action:workspace.menuBar()->actions())menus<<QString(action->text()).remove('&');
        QCOMPARE(menus,(QStringList{"File","Edit","Hex Viewer","3D","UV / 2D","Scripting","Other Tools","Toolbars","Windows","Help"}));
        QCOMPARE(workspace.findChildren<QDockWidget *>(QString(),Qt::FindDirectChildrenOnly).size(),0);
        for(const auto &id:{"hex","3d","uv","scripting","tools"})QVERIFY(!workspace.moduleHost(id)->isWindow());
        for(auto *dock:doc->sessionDocks())QCOMPARE(dock->parentWidget(),workspace.moduleHost(dock->property("moduleId").toString()));
        auto *search=doc->sessionDock("searchDock");QVERIFY(search);QVERIFY(search->findChild<QLineEdit *>("patternExpression"));
        QVERIFY(!doc->sessionDock("formatRulesDock")->findChild<QLineEdit *>("patternExpression"));
        auto *find=doc->findChild<QMenu *>("moduleHexMenu")->actions()[2]->menu();QVERIFY(find);find->actions().first()->trigger();
        QVERIFY(search->isVisible());QVERIFY(workspace.moduleHost("hex")->isVisible());
        QString error;QVERIFY(doc->documentView()->replaceSelection("AB",error));
        search->findChild<QLineEdit *>("patternExpression")->setText("41");
        QSignalSpy results(doc->documentView(),&HexView::patternSearchFinished);
        for(auto *button:search->findChildren<QToolButton *>())if(button->accessibleName()=="Find Pattern")button->click();
        QTRY_COMPARE(results.count(),1);QCOMPARE(qvariant_cast<ReadResult>(results.first().first()).matches.size(),1);
        doc->documentView()->discardEdits();
        auto *console=doc->sessionDock("scriptConsoleDock");QVERIFY(console->findChild<QPlainTextEdit *>("scriptConsole"));
        QVERIFY(!doc->sessionDock("scriptDock")->findChild<QPlainTextEdit *>("scriptConsole"));
        workspace.selectModule("3d");QVERIFY(workspace.moduleHost("3d")->isVisible());QVERIFY(!workspace.moduleHost("hex")->isVisible());
        auto *collection=doc->sessionDock("meshCollectionDock"),*controls=doc->sessionDock("meshControlsDock");
        controls->show();QTest::qWait(20);
        workspace.moduleHost("3d")->tabifyDockWidget(collection,controls);
        QTRY_VERIFY(workspace.moduleHost("3d")->tabifiedDockWidgets(collection).contains(controls));
        QVERIFY(!workspace.moduleHost("3d")->tabifiedDockWidgets(collection).contains(search));
        controls->setFloating(true);controls->show();workspace.selectModule("uv");QVERIFY(!controls->isVisible());
        workspace.selectModule("3d");QVERIFY(controls->isVisible());QVERIFY(controls->isFloating());
        workspace.selectModule("uv");
        workspace.newDocument();QCOMPARE(workspace.documentCount(),2);QVERIFY(workspace.activeDocument()!=doc);
        QCOMPARE(workspace.activeDocument()->sessionDock("meshCollectionDock")->parentWidget(),workspace.moduleHost("3d"));
        QCOMPARE(doc->sessionDock("meshCollectionDock")->parentWidget(),doc);
        workspace.selectModule("3d");QVERIFY(workspace.activeDocument()->sessionDock("meshControlsDock")->isVisible());
        QVERIFY(workspace.activeDocument()->sessionDock("meshControlsDock")->isFloating());
        workspace.selectModule("scripting");QVERIFY(workspace.moduleHost("scripting")->isVisible());
        QVERIFY(workspace.activeDocument()->sessionDock("scriptConsoleDock")->isVisible());
        workspace.close();QTRY_VERIFY(!workspace.isVisible());
    }
    void settingsApplyCancelAndPersistence(){
        const auto theme=qApp->property("workbenchTheme");const auto size=qApp->property("editorFontSize");
        const auto rows=qApp->property("editorAlternatingRows");const auto language=qApp->property("workbenchLanguage");
        const auto restore=qScopeGuard([=]{applyWorkbenchTheme(theme.toString());qApp->setProperty("editorFontSize",size);qApp->setProperty("editorAlternatingRows",rows);qApp->setProperty("workbenchLanguage",language);});
        QTemporaryDir directory;QVERIFY(directory.isValid());QSettings settings(directory.filePath("settings.ini"),QSettings::IniFormat);
        settings.setValue("theme","dark");settings.setValue("editor/fontSize",10);
        {SettingsDialog cancelled(settings,[]{});cancelled.findChild<QComboBox *>("settingsTheme")->setCurrentIndex(cancelled.findChild<QComboBox *>("settingsTheme")->findData("light"));cancelled.reject();}
        QCOMPARE(settings.value("theme").toString(),QString("dark"));
        HexView view;view.newMemoryDocument();QString error;QVERIFY(view.replaceSelection("ABCD",error));view.selectMatch(1,2);
        int applications=0;SettingsDialog dialog(settings,[&]{++applications;view.applyDisplayPreferences();});
        auto *sections=dialog.findChild<QTreeWidget *>("settingsSections");QVERIFY(sections);
        QCOMPARE(sections->topLevelItem(2)->child(0)->text(0),QString("Hexadecimal Viewer"));
        sections->setCurrentItem(sections->topLevelItem(2)->child(0));
        QCOMPARE(dialog.findChild<QStackedWidget *>("settingsPages")->currentIndex(),3);
        dialog.findChild<QComboBox *>("settingsTheme")->setCurrentIndex(dialog.findChild<QComboBox *>("settingsTheme")->findData("light"));
        dialog.findChild<QComboBox *>("settingsLanguage")->setCurrentIndex(1);
        dialog.findChild<QSpinBox *>("settingsEditorFontSize")->setValue(16);
        dialog.findChild<QCheckBox *>("settingsAlternatingRows")->setChecked(false);
        auto *buttons=dialog.findChild<QDialogButtonBox *>("settingsButtons");buttons->button(QDialogButtonBox::Apply)->click();
        QCOMPARE(applications,1);QCOMPARE(qApp->property("workbenchTheme").toString(),QString("light"));
        auto *table=view.findChild<QTableWidget *>("bytes");QCOMPARE(table->font().pointSize(),16);QVERIFY(!table->alternatingRowColors());
        QSignalSpy zoom(&view,&HexView::viewerFontSizeChanged);
        QWheelEvent zoomIn(QPointF(20,20),QPointF(20,20),QPoint(),QPoint(0,120),Qt::NoButton,Qt::ControlModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(table->viewport(),&zoomIn);QCOMPARE(table->font().pointSize(),17);QCOMPARE(zoom.count(),1);
        QWheelEvent zoomOut(QPointF(20,20),QPointF(20,20),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::ControlModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(table->viewport(),&zoomOut);QCOMPARE(table->font().pointSize(),16);QCOMPARE(view.firstVisibleOffset(),qint64(0));
        QCOMPARE(view.selection().anchor,qint64(1));QCOMPARE(readPage(view.sourceRequest(0,4)).bytes,QByteArray("ABCD"));QVERIFY(view.dirty());
        HexView newView;QCOMPARE(newView.findChild<QTableWidget *>("bytes")->font().pointSize(),16);
        buttons->button(QDialogButtonBox::RestoreDefaults)->click();dialog.reject();
        QSettings reopened(directory.filePath("settings.ini"),QSettings::IniFormat);
        QCOMPARE(reopened.value("editor/fontSize").toInt(),16);QCOMPARE(reopened.value("language").toString(),QString("en"));
        SettingsDialog saved(reopened,[]{});QCOMPARE(saved.findChild<QComboBox *>("settingsTheme")->currentData().toString(),QString("light"));
        view.discardEdits();view.shutdown();newView.shutdown();
    }
    void ramDocumentsGrowUndoAndSave(){
        MainWindow window;window.show();auto *view=window.documentView();view->newMemoryDocument();
        QVERIFY(view->isMemoryDocument());QVERIFY(view->filePath().isEmpty());QCOMPARE(view->fileSize(),qint64(0));
        auto *table=view->findChild<QTableWidget *>("bytes");QVERIFY(table);table->setFocus();
        QTest::keyClicks(table,"4142");QCOMPARE(view->fileSize(),qint64(2));QCOMPARE(readPage(view->sourceRequest(0,2)).bytes,QByteArray("AB"));
        view->undo();QCOMPARE(view->fileSize(),qint64(1));view->redo();QCOMPARE(view->fileSize(),qint64(2));
        view->selectMatch(0,1);view->setInsertMode(true);QString error;
        QVERIFY2(view->replaceSelection("C",error),qPrintable(error));QCOMPARE(readPage(view->sourceRequest(0,3)).bytes,QByteArray("CAB"));
        view->selectMatch(1,1);view->deleteSelection();QCOMPARE(readPage(view->sourceRequest(0,2)).bytes,QByteArray("CB"));
        view->undo();QCOMPARE(readPage(view->sourceRequest(0,3)).bytes,QByteArray("CAB"));
        auto snapshot=view->sourceRequest(0,3);view->selectMatch(0,1);view->setInsertMode(false);QVERIFY(view->replaceSelection("D",error));
        QCOMPARE(readPage(snapshot).bytes,QByteArray("CAB"));QCOMPARE(readPage(view->sourceRequest(0,3)).bytes,QByteArray("DAB"));
        QTemporaryDir temporary;QVERIFY(temporary.isValid());auto path=temporary.filePath("ram.bin");
        QVERIFY(view->saveFile(path));QTRY_VERIFY(!view->saving());QVERIFY(!view->dirty());QCOMPARE(view->filePath(),path);
        QFile saved(path);QVERIFY(saved.open(QIODevice::ReadOnly));QCOMPARE(saved.readAll(),QByteArray("DAB"));saved.close();
        view->selectMatch(0,1);QVERIFY(view->replaceSelection("E",error));QVERIFY(view->dirty());
        QTimer dismiss;connect(&dismiss,&QTimer::timeout,this,[]{if(auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))box->accept();});dismiss.start(10);
        QSignalSpy finished(view,&HexView::saveFinished);QVERIFY(view->saveFile(temporary.filePath("missing/failed.bin")));
        QTRY_COMPARE(finished.count(),1);QVERIFY(!finished.first().first().toBool());QVERIFY(view->dirty());
        dismiss.stop();
        QCOMPARE(readPage(view->sourceRequest(0,3)).bytes,QByteArray("EAB"));view->discardEdits();
        QCOMPARE(readPage(view->sourceRequest(0,3)).bytes,QByteArray("DAB"));window.close();
    }
    void memorySnapshotsSupportSearchAndEntropy(){
        HexView view;view.newMemoryDocument();QString error;QVERIFY(view.replaceSelection(QByteArray(4096,'A')+QByteArray("BC"),error));
        PatternSpec spec;spec.expression="42 43";spec.begin=0;spec.length=view.fileSize();
        QSignalSpy results(&view,&HexView::patternSearchFinished);QVERIFY(view.startPatternSearch(spec));
        QTRY_COMPARE(results.count(),1);auto found=qvariant_cast<ReadResult>(results.first().first());
        QVERIFY(found.error.isEmpty());QCOMPARE(found.matches.size(),1);QCOMPARE(found.matches.first().first,qint64(4096));
        std::atomic<bool> cancel{false};std::atomic<qint64> progress{0};EntropySpec entropy;entropy.begin=0;entropy.length=view.fileSize();entropy.window=256;
        auto result=analyzeEntropy(view.meshSource(),entropy,cancel,progress);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QVERIFY(!result.entropy.isEmpty());
        view.discardEdits();view.shutdown();
    }
    void multipleDocumentSessionsAndThemes(){
        WorkspaceWindow workspace;workspace.show();QCOMPARE(workspace.documentCount(),1);
        auto *first=workspace.activeDocument();QVERIFY(first);auto *a=first->documentView();QString error;
        QVERIFY(a->replaceSelection("AB",error));a->selectMatch(0,1);a->findChild<QComboBox *>("endian")->setCurrentIndex(1);
        workspace.newDocument();QCOMPARE(workspace.documentCount(),2);auto *second=workspace.activeDocument();QVERIFY(second!=first);
        auto *b=second->documentView();QVERIFY(b->replaceSelection("XYZ",error));
        QCOMPARE(readPage(a->sourceRequest(0,2)).bytes,QByteArray("AB"));QCOMPARE(readPage(b->sourceRequest(0,3)).bytes,QByteArray("XYZ"));
        auto *mdi=workspace.findChild<QMdiArea *>("documentWorkspace");QVERIFY(mdi);mdi->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(first->parentWidget()));
        auto *tabs=mdi->findChild<QTabBar *>();QVERIFY(tabs);QCOMPARE(tabs->count(),2);
        QVERIFY(tabs->tabText(0).contains("Untitled"));QVERIFY(tabs->tabText(1).contains("Untitled"));
        for(auto *toolbar:workspace.findChildren<QToolBar *>())QVERIFY(!toolbar->isFloating());
        QCOMPARE(workspace.activeDocument(),first);QCOMPARE(a->selection().anchor,qint64(0));QCOMPARE(a->findChild<QComboBox *>("endian")->currentIndex(),1);
        QVERIFY(first->sessionDock("inspectorDock")->parentWidget()==workspace.moduleHost("hex"));
        QVERIFY(second->sessionDock("inspectorDock")->parentWidget()==second);
        QTemporaryDir directory;QVERIFY(directory.isValid());QVERIFY(a->saveFile(directory.filePath("a.bin")));
        mdi->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(second->parentWidget()));QTRY_VERIFY(!a->saving());
        QVERIFY(b->filePath().isEmpty());QVERIFY(b->dirty());QCOMPARE(readPage(b->sourceRequest(0,3)).bytes,QByteArray("XYZ"));
        workspace.setTheme("light");QVERIFY(qApp->palette().color(QPalette::Window).lightness()>128);
        auto *renderer=second->sessionDock("vulkanDock")->findChild<VulkanPanel *>();QVERIFY(renderer);QCOMPARE(renderer->visualStyle().background,graphicsBackground());
        workspace.setTheme("dark");QVERIFY(qApp->palette().color(QPalette::Window).lightness()<128);QCOMPARE(renderer->visualStyle().background,graphicsBackground());
        auto *sub=qobject_cast<QMdiSubWindow *>(second->parentWidget());answer(QMessageBox::Cancel);QVERIFY(!sub->close());QCOMPARE(workspace.documentCount(),2);
        b->discardEdits();QVERIFY(sub->close());QTRY_COMPARE(workspace.documentCount(),1);
        workspace.openFile(directory.filePath("a.bin"));QCOMPARE(workspace.documentCount(),1);QCOMPARE(workspace.activeDocument(),first);
        workspace.close();QTRY_VERIFY(!workspace.isVisible());
    }

    void xojoMenuWiring() {
        MainWindow window; window.show(); QTest::qWait(30);
        auto top = window.menuBar()->actions();
        QStringList titles; for (auto *action : top) titles << QString(action->text()).remove('&');
        QCOMPARE(titles, (QStringList{"File", "Edit", "Search", "Hex Viewer", "Analyse", "Extras", "Windows", "Help"}));
        auto *hex = top[3]->menu(); QVERIFY(hex);
        auto *encoding = hex->actions()[3]->menu(); QVERIFY(encoding);
        auto *box = window.findChild<QComboBox *>("encoding"); QVERIFY(box);
        encoding->actions()[4]->trigger(); QCOMPARE(box->currentIndex(), 4);
        box->setCurrentIndex(1); QVERIFY(encoding->actions()[1]->isChecked());
        QVERIFY(!encoding->actions()[4]->isChecked());
        auto *endian = window.findChild<QComboBox *>("endian"); QVERIFY(endian);
        auto *little = window.findChild<QAction *>("littleEndianAction"); QVERIFY(little);
        auto *big = window.findChild<QAction *>("bigEndianAction"); QVERIFY(big);
        QCOMPARE(encoding->actions().size(), 8);
        QVERIFY(encoding->actions()[5]->isSeparator());
        QVERIFY(little->isChecked()); big->trigger();
        QCOMPARE(endian->currentIndex(), 1); QVERIFY(big->isChecked()); QVERIFY(!little->isChecked());
        QVERIFY(encoding->actions()[1]->isChecked());
        endian->setCurrentIndex(0); QVERIFY(little->isChecked()); QVERIFY(!big->isChecked());
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        QFile fixture(temporary.filePath("endian.bin")); QVERIFY(fixture.open(QIODevice::WriteOnly));
        QCOMPARE(fixture.write(QByteArray::fromHex("0102")), qint64(2)); fixture.close();
        auto *view = window.findChild<HexView *>(); QVERIFY(view);
        view->openFile(fixture.fileName()); QTRY_VERIFY(view->hasFile() && !view->busy());
        view->selectMatch(0, 2); QTRY_VERIFY(!view->busy());
        auto *values = window.findChild<QTableWidget *>("inspectorValues"); QVERIFY(values);
        auto uint16Value = [values] { for (int row=0; row<values->rowCount(); ++row)
            if (values->item(row,0)->text()=="UInt16") return values->item(row,1)->text(); return QString(); };
        QTRY_COMPARE(uint16Value(), QString("513"));
        big->trigger(); QTRY_COMPARE(uint16Value(), QString("258"));
        auto *inspectorEndian = window.findChild<QComboBox *>("inspectorEndian"); QVERIFY(inspectorEndian);
        QCOMPARE(inspectorEndian->currentIndex(), 1);
        inspectorEndian->setCurrentIndex(0); QTRY_COMPARE(uint16Value(), QString("513"));
        QVERIFY(little->isChecked()); QVERIFY(!big->isChecked());
        auto *search = top[2]->menu(); QVERIFY(search);
        search->actions().first()->trigger();
        QVERIFY(window.findChild<QDockWidget *>("formatRulesDock")->isVisible());
        QVERIFY(!search->actions()[1]->isEnabled());
        if (qEnvironmentVariableIsSet("WORKBENCH_MENU_SCREENSHOT")) window.grab().save(qEnvironmentVariable("WORKBENCH_MENU_SCREENSHOT"));
    }

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
        panel.setMesh(result.mesh);panel.setBackgroundGradient(true,QColor("#18324f"),QColor("#647890"));
        auto *surface=panel.findChild<QWidget *>("vulkanSurface");
        auto *status=panel.findChild<QLabel *>("vulkanStatus");
        auto *mode=panel.findChild<QComboBox *>("meshSelectionMode");
        auto *labels=panel.findChild<QCheckBox *>("meshLabels");
        QTRY_VERIFY_WITH_TIMEOUT(status->text().startsWith("Vulkan ·"),15000);
        QTRY_VERIFY(status->text().contains("252 triangles"));
        qInfo().noquote()<<status->text();
        auto capture=[&](QString name){qInfo().noquote()<<name;QTest::qWait(400);return status->text().startsWith("Vulkan ·");}; // Capture GPU frames with VK_LAYER_LUNARG_screenshot; QWidget grabs omit native Vulkan.
        QVERIFY(capture("sphere-initial"));
        QImage checker(8,8,QImage::Format_RGBA8888);for(int y=0;y<8;++y)for(int x=0;x<8;++x)checker.setPixelColor(x,y,(x+y)%2?Qt::red:Qt::white);QVERIFY(panel.scene()->setTexture(panel.scene()->active(),checker,"Checker"));QVERIFY(capture("sphere-textured"));
        QVERIFY(panel.scene()->setMaterial(panel.scene()->active(),{"Paint",QColor("#40c0ff"),false}));QVERIFY(panel.scene()->setTransform(panel.scene()->active(),{},{0,25,0},{1,1.2,.8}));panel.findChild<QDoubleSpinBox *>("viewportAmbient")->setValue(.4);QVERIFY(capture("sphere-material-transform"));
        QVERIFY(panel.scene()->setMaterial(panel.scene()->active(),{"Unlit",Qt::white,true}));QVERIFY(capture("sphere-unlit"));QVERIFY(panel.scene()->setTransform(panel.scene()->active(),{},{},{1,1,1}));
        panel.findChild<QAction *>("new3dViewport")->trigger();auto *extra=panel.findChild<VulkanPanel *>();QVERIFY(extra);auto *extraStatus=extra->findChild<QLabel *>("vulkanStatus");QTRY_VERIFY_WITH_TIMEOUT(extraStatus->text().startsWith("Vulkan"),10000);QVERIFY(extraStatus->text().contains("252 triangles"));extra->close();
        QVERIFY(panel.scene()->setTexture(panel.scene()->active(),{},{}));
        mode->setCurrentIndex(1);labels->setChecked(true);
        QTest::mouseClick(surface,Qt::LeftButton,Qt::NoModifier,surface->rect().center());
        QVERIFY(panel.scene()->selection().index>=0);
        QVERIFY(capture("sphere-selected"));
        panel.findChild<QGroupBox *>("meshLabelFaces")->setChecked(true);panel.findChild<QGroupBox *>("meshLabelMeshes")->setChecked(true);QTRY_VERIFY(surface->property("labelVertexCount").toInt()>1);auto *vertexLimit=panel.findChild<QLineEdit *>("meshLabelLimit");vertexLimit->setText("1");QTRY_VERIFY(surface->property("labelVertexCount").toInt()<=1);vertexLimit->clear();QTRY_VERIFY(surface->property("labelVertexCount").toInt()>1);QTRY_VERIFY(surface->property("labelFaceCount").toInt()>0);QTRY_VERIFY(surface->property("labelMeshCount").toInt()>0);panel.setTransformTool(1);QVERIFY(capture("sphere-move-handles-and-labels"));panel.setTransformTool(0);
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
        QTRY_VERIFY(status->text().contains("504 triangles"));
        panel.scene()->setVisible(second,false);
        QTRY_VERIFY(status->text().contains("252 triangles"));
        panel.scene()->remove(second);
        panel.scene()->select(panel.scene()->active(),MeshElement::Face,94);
        UvPanel uv(&panel);uv.resize(700,600);uv.show();QTest::qWait(200);
        QVERIFY(uv.findChild<QLabel *>("uvSelectionInfo")->text().contains("Face 94"));
        QVERIFY(uv.grab().save(output+"/sphere-uv.png"));
        for(auto format:{ModelFormat::Glb,ModelFormat::Obj,ModelFormat::Stl}) {
            ModelExportOptions options;options.format=format;options.protectedPaths={WORKBENCH_SPHERE};
            QString suffix=format==ModelFormat::Glb?"glb":format==ModelFormat::Obj?"obj":"stl";
            auto exported=exportModel(output+"/sphere."+suffix,{{"Sphere",WORKBENCH_SPHERE,result.mesh}},options);
            QVERIFY2(exported.ok,qPrintable(exported.error));QCOMPARE(exported.vertices,quint64(128));QCOMPARE(exported.faces,quint64(252));
        }
        panel.setViewportLayout(3);host.resize(1200,950);QTest::qWait(500);QCOMPARE(panel.findChildren<VulkanPanel *>().size(),3);for(auto *view:panel.findChildren<VulkanPanel *>()){auto *state=view->findChild<QLabel *>("vulkanStatus");QTRY_VERIFY2_WITH_TIMEOUT(state->text().startsWith("Vulkan"),qPrintable(state->text()),10000);}QVERIFY(capture("four-native-viewports"));panel.setViewportLayout(0);QVERIFY(capture("single-restored"));
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
        format->setCurrentIndex(2);QVERIFY(!uv->isEnabled());QVERIFY(dialog.findChild<QLabel *>("exportSummary")->text().contains("lost"));format->setCurrentIndex(0);QVERIFY(uv->isEnabled());scale->setValue(2);axes->setCurrentText("YZX");QCOMPARE(dialog.options().axes,(std::array<int,3>{1,2,0}));QCOMPARE(dialog.options().scale,2.0);
        QString path=dir.filePath("sphere.glb");dialog.startExport(path);QTRY_VERIFY_WITH_TIMEOUT(save->isEnabled(),15000);QVERIFY2(dialog.findChild<QLabel *>("exportStatus")->text().contains("complete"),qPrintable(dialog.findChild<QLabel *>("exportStatus")->text()));QVERIFY(QFileInfo(path).size()>1000);
        if(qEnvironmentVariableIsSet("WORKBENCH_EXPORT_SCREENSHOT"))dialog.grab().save(qEnvironmentVariable("WORKBENCH_EXPORT_SCREENSHOT"));
        dialog.startExport(WORKBENCH_SPHERE);QTRY_VERIFY_WITH_TIMEOUT(save->isEnabled(),15000);QVERIFY(dialog.findChild<QLabel *>("exportStatus")->text().contains("binary"));dialog.reject();renderer->scene()->clear();QVERIFY(!action->isEnabled());window.close();
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
        auto missing=std::make_shared<MeshData>(*mesh);missing->hasUvs=false;renderer.scene()->replace(id,missing);QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,pixel);QCOMPARE(renderer.scene()->selection().index,-1);QVERIFY(panel.findChild<QLabel *>("uvSelectionInfo")->text().contains("No UV coordinates"));
        auto source=readPage({WORKBENCH_SPHERE,0,-1,{},0});auto sphere=readMesh({WORKBENCH_SPHERE,0,source.size,source.modified,0},{},MeshSpec{});QVERIFY(sphere.error.isEmpty());renderer.scene()->replace(id,sphere.mesh);renderer.scene()->select(id,MeshElement::Face,42);canvas->fit();if(qEnvironmentVariableIsSet("WORKBENCH_UV_SPHERE_SCREENSHOT"))panel.grab().save(qEnvironmentVariable("WORKBENCH_UV_SPHERE_SCREENSHOT"));
    }
    void meshTransformAndCollapsibleControls(){
        MainWindow window;auto *panel=window.findChild<QWidget *>("meshControls");QVERIFY(panel);auto *header=panel->findChild<QToolButton *>("meshSection1");auto *body=panel->findChild<QWidget *>("meshSectionBody1");QVERIFY(header&&body);QVERIFY(body->isHidden());header->click();QVERIFY(!body->isHidden());auto *rotation=panel->findChild<QDoubleSpinBox *>("meshRotationZ");auto *scale=panel->findChild<QDoubleSpinBox *>("meshTransformScaleX");rotation->setValue(90);scale->setValue(2);header->click();header->click();QCOMPARE(rotation->value(),90.);QCOMPARE(scale->value(),2.);panel->findChild<QPushButton *>("meshResetTransform")->click();QCOMPARE(rotation->value(),0.);QCOMPARE(scale->value(),1.);auto *types=panel->findChild<QComboBox *>("meshPositionType");QVERIFY(types->findText("Fixed16.16")>=0);QVERIFY(types->findText("SNorm16")>=0);QCOMPARE(panel->findChild<QComboBox *>("meshCoordinateLayout")->count(),2);
    }
    void meshPositionDeltaPlacement(){
        VulkanPanel renderer;auto mesh=std::make_shared<MeshData>();mesh->vertices={{{1,2,3},{0,0,1},{.2f,.4f}},{{2,2,3},{0,0,1},{1,0}},{{1,3,3},{0,0,1},{0,1}}};mesh->triangles={0,1,2};mesh->vertexOffsets={16,28,40};int first=renderer.scene()->add(mesh,"Part A"),second=renderer.scene()->add(mesh,"Part B");
        std::unique_ptr<QWidget> collection(renderer.createMeshCollection());renderer.scene()->setActive(first);collection->findChild<QDoubleSpinBox *>("activeMeshDeltaX")->setValue(10);collection->findChild<QDoubleSpinBox *>("activeMeshDeltaY")->setValue(-5);collection->findChild<QDoubleSpinBox *>("activeMeshDeltaZ")->setValue(.5);collection->findChild<QPushButton *>("applyMeshDelta")->click();
        auto all=renderer.scene()->assemble();QCOMPARE(all.data->vertices[0].position[0],11.f);QCOMPARE(all.data->vertices[0].position[1],-3.f);QCOMPARE(all.data->vertices[0].position[2],3.5f);QCOMPARE(all.data->vertices[3].position[0],1.f);QCOMPARE(mesh->vertices[0].position[0],1.f);QCOMPARE(renderer.meshSnapshot().get(),mesh.get());QCOMPARE(renderer.scene()->entry(first)->placedData()->vertexOffsets,mesh->vertexOffsets);QCOMPARE(all.data->vertices[0].uv[0],.2f);QCOMPARE(all.data->vertices[0].normal[2],1.f);
        ExportDialog dialog(renderer.scene(),{},Qt::white);auto exported=dialog.selectedMeshes();QCOMPARE(exported[0].data->vertices[0].position[0],11.f);QCOMPARE(exported[1].data->vertices[0].position[0],1.f);
        QVERIFY(!renderer.scene()->setPositionDelta(first,{std::numeric_limits<double>::infinity(),0,0}));QCOMPARE(renderer.scene()->entry(first)->positionDelta[0],10.);renderer.scene()->setVisible(first,false);QCOMPARE(renderer.scene()->assemble().data->vertices.size(),3);QCOMPARE(renderer.scene()->assemble(true).data->vertices[0].position[0],11.f);
        renderer.scene()->setActive(second);QCOMPARE(collection->findChild<QDoubleSpinBox *>("activeMeshDeltaX")->value(),0.);renderer.scene()->setActive(first);QCOMPARE(collection->findChild<QDoubleSpinBox *>("activeMeshDeltaX")->value(),10.);QVERIFY(renderer.scene()->setPositionDelta(first,{}));QCOMPARE(renderer.scene()->entry(first)->placedData().get(),mesh.get());
    }
    void meshCollectionAndUvSynchronization() {
        VulkanPanel renderer;renderer.resize(1000,650);renderer.show();UvPanel uvPanel(&renderer);uvPanel.resize(600,600);uvPanel.show();auto *canvas=uvPanel.findChild<UvCanvas *>("uvCanvas");QVERIFY(canvas);
        auto mesh=std::make_shared<MeshData>();for(auto xy:{QPointF(.1,.1),QPointF(.8,.1),QPointF(.2,.8)}){MeshVertex v{};v.position[0]=xy.x();v.position[1]=xy.y();v.uv[0]=xy.x();v.uv[1]=xy.y();mesh->vertices.append(v);}mesh->triangles={0,1,2};auto other=std::make_shared<MeshData>(*mesh);for(auto &v:other->vertices){v.position[0]+=2;v.uv[0]+=2;}
        auto *scene=renderer.scene();int first=scene->add(mesh,"Primera"),second=scene->add(other,"Segunda");QCOMPARE(scene->entries().size(),2);
        std::unique_ptr<QWidget> collection(renderer.createMeshCollection());collection->show();auto *tree=collection->findChild<QTreeWidget *>("meshCollection");QCOMPARE(tree->topLevelItemCount(),2);
        scene->select(first,MeshElement::Vertex,1);QCOMPARE(uvPanel.findChild<QLabel *>("uvSelectionInfo")->text().contains("Vertex 1"),true);canvas->fit();
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,canvas->uvToScreen({.2,.8}).toPoint());QCOMPARE(scene->selection().mesh,first);QCOMPARE(scene->selection().index,2);QVERIFY(renderer.findChild<QLabel *>("meshSelectionInfo")->text().contains("Vertex 2"));
        uvPanel.findChild<QComboBox *>("uvSelectionMode")->setCurrentIndex(1);QCOMPARE(renderer.findChild<QComboBox *>("meshSelectionMode")->currentIndex(),1);QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,canvas->uvToScreen({.35,.3}).toPoint());QCOMPARE(scene->selection().kind,MeshElement::Face);QCOMPARE(scene->selection().index,0);
        scene->select(second,MeshElement::Vertex,0);canvas->fit();QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,canvas->uvToScreen({2.8,.1}).toPoint());QCOMPARE(scene->selection().mesh,second);QCOMPARE(scene->selection().index,1);
        tree->topLevelItem(1)->setCheckState(0,Qt::Unchecked);QVERIFY(!scene->entry(second)->visible);QCOMPARE(scene->selection().index,-1);QCOMPARE(scene->assemble().spans.size(),1);
        scene->setVisible(second,true);scene->setActive(first);tree->topLevelItem(0)->setText(0,"Renombrada");QCOMPARE(scene->entry(first)->name,QString("Renombrada"));
        // A 3D hit chooses the mesh owning that triangle, then updates UV selection.
        auto *surface=renderer.findChild<QWidget *>("vulkanSurface");renderer.findChild<QComboBox *>("meshSelectionMode")->setCurrentIndex(1);auto assembly=scene->assemble();MeshDebug debug(*assembly.data);float yaw=qDegreesToRadians(35.f),pitch=qDegreesToRadians(20.f);QVector3D eye(3.5f*std::cos(pitch)*std::sin(yaw),3.5f*std::sin(pitch),3.5f*std::cos(pitch)*std::cos(yaw));QMatrix4x4 projection;projection.perspective(45,float(surface->width())/surface->height(),.01f,100.f);QMatrix4x4 correction;correction(1,1)=-1;correction(2,2)=.5f;correction(2,3)=.5f;QMatrix4x4 camera;camera.lookAt(eye,{0,0,0},{0,1,0});auto labels=debug.labels(correction*projection*camera,eye,surface->size(),MeshElement::Face,1,1);QVERIFY(!labels.isEmpty());QTest::mouseClick(surface,Qt::LeftButton,Qt::NoModifier,labels.first().pixel.toPoint());QCOMPARE(scene->selection().mesh,second);QCOMPARE(scene->selection().index,0);QVERIFY(uvPanel.findChild<QLabel *>("uvSelectionInfo")->text().contains("Segunda"));
        if(qEnvironmentVariableIsSet("WORKBENCH_UV_SCREENSHOT")){canvas->fit();uvPanel.grab().save(qEnvironmentVariable("WORKBENCH_UV_SCREENSHOT"));}
        collection->findChild<QPushButton *>("removeMesh")->click();QVERIFY(!scene->entry(second));QCOMPARE(scene->active(),first);QCOMPARE(scene->entries().size(),1);
        scene->clear();QVERIFY(uvPanel.findChild<QLabel *>("uvSelectionInfo")->text().contains("Empty"));
    }
    void meshDebugControlsAndSelection() {
        VulkanPanel panel;panel.resize(1000,650);panel.show();auto source=readPage({WORKBENCH_SPHERE,0,-1,{},0});auto result=readMesh({WORKBENCH_SPHERE,0,source.size,source.modified,0},{},MeshSpec{});QVERIFY(result.error.isEmpty());panel.setMesh(result.mesh);
        auto *mode=panel.findChild<QComboBox *>("meshSelectionMode");auto *labels=panel.findChild<QCheckBox *>("meshLabels");auto *surface=panel.findChild<QWidget *>("vulkanSurface");auto *info=panel.findChild<QLabel *>("meshSelectionInfo");QVERIFY(mode&&labels&&surface&&info);
        mode->setCurrentIndex(1);labels->setChecked(true);QTest::mouseClick(surface,Qt::LeftButton,Qt::NoModifier,surface->rect().center());QVERIFY(info->text().contains("Face"));QVERIFY(info->text().contains("Offsets:"));
        mode->setCurrentIndex(2);QTest::mouseClick(surface,Qt::LeftButton,Qt::NoModifier,surface->rect().center());QVERIFY(info->text().contains("128 vertices"));
        panel.setMesh({});QVERIFY(info->text().contains("Empty collection"));
    }
    void embeddedScriptingPythonAndCpp() {
        MainWindow window;window.show();window.openFile(WORKBENCH_SPHERE);auto *view=window.findChild<HexView *>();QTRY_VERIFY(!view->busy());QString error;QVERIFY(view->replaceSelection("C",error));
        auto *dock=window.findChild<QDockWidget *>("scriptDock");QVERIFY(dock);dock->show();auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");auto *console=window.findChild<QPlainTextEdit *>("scriptConsole");auto *run=window.findChild<QPushButton *>("scriptRun");auto *stop=window.findChild<QPushButton *>("scriptStop");auto *language=window.findChild<QComboBox *>("scriptLanguage");auto *renderer=static_cast<VulkanPanel *>(window.findChild<QDockWidget *>("vulkanDock")->widget());
        QString python="import workbench as bw\nassert bw.read(0, 4) == b'CWS1'\nprint('PYTHON_OK', bw.size)\nbw.set_mesh([[-1,0,0],[1,0,0],[0,1,0]],[0,1,2])\n";
        editor->setPlainText(python);run->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),15000);QVERIFY2(console->toPlainText().contains("PYTHON_OK"),qPrintable(console->toPlainText()));QVERIFY(renderer->meshSnapshot());QCOMPARE(renderer->meshSnapshot()->vertices.size(),3);
        language->setCurrentIndex(1);editor->setPlainText("#include \"workbench.hpp\"\nvoid run(){auto bytes=bw::read(0,1); if(bytes[0]!='C') throw std::runtime_error(\"read mismatch\"); auto mesh=bw::mesh(); mesh.vertices[0].position[0]=2; bw::set_mesh(mesh); std::cout << \"CPP_OK\\n\";}\n");run->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),30000);QVERIFY2(console->toPlainText().contains("CPP_OK"),qPrintable(console->toPlainText()));QCOMPARE(renderer->meshSnapshot()->vertices[0].position[0],2.f);
        editor->setPlainText("this is not C++");run->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),30000);QVERIFY(console->toPlainText().contains("compilation error"));
        language->setCurrentIndex(0);QCOMPARE(editor->toPlainText(),python);editor->setPlainText("raise RuntimeError('ERROR_PRUEBA')");run->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),15000);QVERIFY(console->toPlainText().contains("ERROR_PRUEBA"));
        editor->setPlainText("print('LOOP_STARTED', flush=True)\nwhile True: pass");run->click();QTRY_VERIFY_WITH_TIMEOUT(console->toPlainText().contains("LOOP_STARTED"),15000);stop->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),5000);QVERIFY(console->toPlainText().contains("Execution stopped"));
        editor->setPlainText("import time, workbench as bw\nprint('STALE_STARTED', flush=True)\ntime.sleep(.4)\nbw.set_mesh([[0,0,0],[1,0,0],[0,1,0]],[0,1,2])");run->click();QTRY_VERIFY_WITH_TIMEOUT(console->toPlainText().contains("STALE_STARTED"),15000);QVERIFY(view->replaceSelection("D",error));QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),15000);QVERIFY(console->toPlainText().contains("Mesh discarded"));
        if(qEnvironmentVariableIsSet("WORKBENCH_SCRIPT_SCREENSHOT"))window.grab().save(qEnvironmentVariable("WORKBENCH_SCRIPT_SCREENSHOT"));
        answer(QMessageBox::Cancel);window.close();QVERIFY(window.isVisible());
    }
    void meshControlsDecodeFixture() {
        MainWindow window;window.show();window.openFile(WORKBENCH_SPHERE);auto *view=window.findChild<HexView *>();QTRY_VERIFY(!view->busy());
        auto *dock=window.findChild<QDockWidget *>("meshControlsDock");QVERIFY(dock);QJsonObject fixture{{"vertexOffset","128"},{"vertexCount",128},{"vertexStride",12},{"positionType","Int32"},{"positionScale",0.0001},{"indexOffset","1664"},{"indexCount",402},{"indexStride",2},{"indexType","UInt16"},{"topology","Triangle strip"},{"uv",true},{"uvOffset","2480"},{"uvStride",4},{"uvType","Float16"},{"normals",true},{"normalOffset","2992"},{"normalStride",12},{"normalType","Float32"}};QVERIFY(!AiDecoderSetup::applyMesh(dock,fixture,view->fileSize()).contains("error"));dock->show();auto *button=window.findChild<QPushButton *>("renderMesh");QVERIFY(button);window.findChild<QDoubleSpinBox *>("meshPositionDeltaX")->setValue(12.5);button->click();auto *status=window.findChild<QLabel *>("meshStatus");QTRY_VERIFY(status->text().contains("128 vertices"));QVERIFY(status->text().contains("252 triangles"));
        auto *renderer=static_cast<VulkanPanel *>(window.findChild<QDockWidget *>("vulkanDock")->widget());QCOMPARE(renderer->scene()->entries().size(),1);auto firstData=renderer->meshSnapshot();QCOMPARE(renderer->scene()->entries().first().positionDelta[0],12.5);QCOMPARE(renderer->scene()->assemble().data->vertices[0].position[0],float(firstData->vertices[0].position[0]+12.5));button->click();QTRY_COMPARE(renderer->scene()->entries().size(),2);QCOMPARE(renderer->scene()->entries().first().data.get(),firstData.get());auto secondData=renderer->meshSnapshot();window.findChild<QComboBox *>("meshDestination")->setCurrentIndex(1);button->click();QTRY_VERIFY(button->isEnabled());QCOMPARE(renderer->scene()->entries().size(),2);QVERIFY(renderer->meshSnapshot()!=secondData);
        auto *deltaSource=window.findChild<QComboBox *>("meshDeltaSource");deltaSource->setCurrentIndex(1);QVERIFY(!window.findChild<QDoubleSpinBox *>("meshPositionDeltaX")->isEnabled());auto *deltaOffset=window.findChild<QLineEdit *>("meshDeltaOffset");QVERIFY(deltaOffset->isEnabled());view->jumpTo(128);window.findChild<QPushButton *>("meshDeltaUseCursor")->click();QCOMPARE(deltaOffset->text(),QString("0x80"));deltaOffset->setText("0x10");window.findChild<QComboBox *>("meshDeltaType")->setCurrentText("Int32");QString patchError;view->selectMatch(16,12);QVERIFY(view->replaceSelection(QByteArray::fromHex("0a000000ecffffff1e000000"),patchError));button->click();QTRY_VERIFY(button->isEnabled());auto placed=renderer->scene()->entry(renderer->scene()->active());QCOMPARE(placed->positionDelta[0],10.);QCOMPARE(placed->positionDelta[1],-20.);QCOMPARE(placed->positionDelta[2],30.);view->discardEdits();
        if(qEnvironmentVariableIsSet("WORKBENCH_MESH_SCREENSHOT"))window.grab().save(qEnvironmentVariable("WORKBENCH_MESH_SCREENSHOT"));
        auto *offset=window.findChild<QLineEdit *>("meshVertexOffset");offset->setText("0xFFFF");button->click();QTRY_VERIFY(button->isEnabled());QVERIFY(status->text().contains("outside the file"));answer(QMessageBox::Discard);window.close();QTRY_VERIFY(!window.isVisible());
    }
    void vulkanPanelFailsGracefullyWithoutDesktop() {
        MainWindow window;window.show();auto *dock=window.findChild<QDockWidget *>("vulkanDock");QVERIFY(dock);dock->show();
        auto *status=window.findChild<QLabel *>("vulkanStatus");QVERIFY(status);QTRY_VERIFY(status->text().contains("Could not initialize"));dock->hide();dock->show();QVERIFY(window.isVisible());window.close();QTRY_VERIFY(!window.isVisible());
    }
    void entropyNavigation() {
        QTemporaryDir dir;auto path=dir.filePath("entropy.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));QByteArray data(4096,0);for(int i=0;i<4096;++i)data.append(char(i%256));file.write(data);file.close();
        MainWindow window;window.show();window.openFile(path);auto *view=window.findChild<HexView *>();QTRY_VERIFY(!view->busy());
        auto *button=window.findChild<QPushButton *>("entropyAnalyze");QTRY_VERIFY(button->isEnabled());QSignalSpy spy(view,&HexView::entropyFinished);button->click();QTRY_COMPARE(spy.size(),1);
        auto *status=window.findChild<QLabel *>("entropyStatus");QVERIFY(status->text().contains("2 regions"));auto *graph=window.findChild<QWidget *>("entropyGraph");QVERIFY(graph);
        QTest::mouseClick(graph,Qt::LeftButton,Qt::NoModifier,QPoint(38+(graph->width()-52)*3/4,graph->height()/2));QCOMPARE(view->selection().anchor,qint64(4096));QCOMPARE(view->selection().cursor,qint64(8191));QTRY_VERIFY(!view->busy());
        if(qEnvironmentVariableIsSet("WORKBENCH_ENTROPY_SCREENSHOT"))window.grab().save(qEnvironmentVariable("WORKBENCH_ENTROPY_SCREENSHOT"));
        QString error;QVERIFY(view->replaceSelection(QByteArray(4096,'A'),error));QVERIFY(status->text().contains("changed"));view->discardEdits();window.close();QTRY_VERIFY(!window.isVisible());
    }
    void magicNavigationAndMarking() {
        QTemporaryDir dir;auto path=dir.filePath("magic.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(QByteArray(32,'X')+QByteArray::fromHex("89504e470d0a1a0a")+QByteArray(32,'X'));file.close();
        MainWindow window;window.show();window.openFile(path);auto *view=window.findChild<HexView *>();QTRY_VERIFY(!view->busy());
        auto *dock=window.findChild<QDockWidget *>("magicDock");dock->show();auto *scan=window.findChild<QPushButton *>("magicScan");QTRY_VERIFY(scan->isEnabled());scan->click();
        auto *results=window.findChild<QTableWidget *>("magicResults");QTRY_COMPARE(results->rowCount(),1);QCOMPARE(results->item(0,0)->text(),QString("PNG"));
        results->selectRow(0);results->itemClicked(results->item(0,1));QCOMPARE(view->selection().anchor,qint64(32));QCOMPARE(view->selection().cursor,qint64(39));
        for(auto *b:window.findChildren<QPushButton *>())if(b->text().startsWith("Outline selected rows"))b->click();QCOMPARE(view->formatRules().size(),1);QVERIFY(view->formatAt(32).style.background.isValid());QVERIFY(!view->formatAt(40).style.background.isValid());
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
        for(auto *b:window.findChildren<QToolButton *>())if(b->toolTip()=="Add Rule")b->click();
        for(auto *b:window.findChildren<QToolButton *>())if(b->toolTip()=="Save Rule")b->click();
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
        for(auto *button:window.findChildren<QToolButton *>())if(button->toolTip()=="Add Rule")button->click();
        auto *repeat=window.findChild<QLineEdit *>("ruleRepetitions");repeat->setText("4");
        for(auto *button:window.findChildren<QToolButton *>())if(button->toolTip()=="Save Rule")button->click();
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
