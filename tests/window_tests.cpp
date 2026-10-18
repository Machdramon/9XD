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
#include <QScopeGuard>
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
        QCOMPARE(menus,(QStringList{"File","Edit","Hex Viewer","3D","UV / 2D","Scripting","Other Tools","Windows","Help"}));
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
        {SettingsDialog cancelled(settings,[]{});cancelled.findChild<QComboBox *>("settingsTheme")->setCurrentIndex(1);cancelled.reject();}
        QCOMPARE(settings.value("theme").toString(),QString("dark"));
        HexView view;view.newMemoryDocument();QString error;QVERIFY(view.replaceSelection("ABCD",error));view.selectMatch(1,2);
        int applications=0;SettingsDialog dialog(settings,[&]{++applications;view.applyDisplayPreferences();});
        auto *sections=dialog.findChild<QTreeWidget *>("settingsSections");QVERIFY(sections);
        QCOMPARE(sections->topLevelItem(2)->child(0)->text(0),QString("Hexadecimal Viewer"));
        sections->setCurrentItem(sections->topLevelItem(2)->child(0));
        QCOMPARE(dialog.findChild<QStackedWidget *>("settingsPages")->currentIndex(),3);
        dialog.findChild<QComboBox *>("settingsTheme")->setCurrentIndex(1);
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
        panel.setMesh(result.mesh);
        auto *surface=panel.findChild<QWidget *>("vulkanSurface");
        auto *status=panel.findChild<QLabel *>("vulkanStatus");
        auto *mode=panel.findChild<QComboBox *>("meshSelectionMode");
        auto *labels=panel.findChild<QCheckBox *>("meshLabels");
        QTRY_VERIFY_WITH_TIMEOUT(status->text().startsWith("Vulkan ·"),15000);
        QTRY_VERIFY(status->text().contains("252 triangles"));
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
        auto *dock=window.findChild<QDockWidget *>("meshControlsDock");QVERIFY(dock);dock->show();auto *button=window.findChild<QPushButton *>("renderMesh");QVERIFY(button);button->click();auto *status=window.findChild<QLabel *>("meshStatus");QTRY_VERIFY(status->text().contains("128 vertices"));QVERIFY(status->text().contains("252 triangles"));
        auto *renderer=static_cast<VulkanPanel *>(window.findChild<QDockWidget *>("vulkanDock")->widget());QCOMPARE(renderer->scene()->entries().size(),1);auto firstData=renderer->meshSnapshot();button->click();QTRY_COMPARE(renderer->scene()->entries().size(),2);QCOMPARE(renderer->scene()->entries().first().data.get(),firstData.get());auto secondData=renderer->meshSnapshot();window.findChild<QComboBox *>("meshDestination")->setCurrentIndex(1);button->click();QTRY_VERIFY(button->isEnabled());QCOMPARE(renderer->scene()->entries().size(),2);QVERIFY(renderer->meshSnapshot()!=secondData);
        if(qEnvironmentVariableIsSet("WORKBENCH_MESH_SCREENSHOT"))window.grab().save(qEnvironmentVariable("WORKBENCH_MESH_SCREENSHOT"));
        auto *offset=window.findChild<QLineEdit *>("meshVertexOffset");offset->setText("0xFFFF");button->click();QTRY_VERIFY(button->isEnabled());QVERIFY(status->text().contains("outside the file"));window.close();QTRY_VERIFY(!window.isVisible());
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
