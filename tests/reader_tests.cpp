#include "data_inspector.h"
#include <QCheckBox>
#include "binary_reader.h"
#include "mesh_data.h"
#include "mesh_debug.h"
#include "mesh_scene.h"
#include "model_export.h"
#include "recent_files.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QtEndian>
#include <bit>
#include <cmath>
#include "hex_view.h"
#include <QLineEdit>
#include <QTableWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QClipboard>
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QSignalSpy>
#include <QTimer>
#include <atomic>
#include <QScrollBar>
#include <QSemaphore>
#include <QtConcurrentRun>
#include <climits>
#include <QWheelEvent>

class ReaderTests : public QObject {
    Q_OBJECT
private slots:
    void recentFilesPersistence() {
        QTemporaryDir dir;QString settings=dir.filePath("prefs/recent.json");RecentFiles recent(settings);QStringList files;
        for(int i=0;i<14;++i){QString path=dir.filePath(QString("file%1.bin").arg(i));QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));f.write("test");f.close();files.append(QFileInfo(path).canonicalFilePath());QVERIFY(recent.add(path));}
        QCOMPARE(recent.paths().size(),12);QCOMPARE(recent.paths().first(),files.last());QVERIFY(recent.add(files[4]));QCOMPARE(recent.paths().first(),files[4]);QCOMPARE(recent.paths().count(files[4]),1);RecentFiles reloaded(settings);QCOMPARE(reloaded.paths(),recent.paths());QVERIFY(reloaded.add(dir.filePath("does-not-exist.bin")));QCOMPARE(reloaded.paths(),recent.paths());QVERIFY(reloaded.clear());QVERIFY(RecentFiles(settings).paths().isEmpty());
    }
    void modelExportFormatsAndTransforms() {
        QTemporaryDir dir;auto mesh=std::make_shared<MeshData>();mesh->vertices={{{0,0,0},{0,0,1},{0,0}},{{1,0,0},{0,0,1},{1,0}},{{0,1,0},{0,0,1},{0,1}}};mesh->triangles={0,1,2};auto plain=std::make_shared<MeshData>(*mesh);plain->hasUvs=false;
        auto read=[](const QString &path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return QByteArray();return f.readAll();};auto u32=[](const QByteArray &b,int at){return qFromLittleEndian<quint32>(b.constData()+at);};
        ModelExportOptions o;o.format=ModelFormat::Obj;QString obj=dir.filePath("test.obj");auto result=exportModel(obj,{{"one","",mesh},{"two","",plain}},o);QVERIFY2(result.ok,qPrintable(result.error));auto text=read(obj);QVERIFY(text.contains("f 1/1/1 2/2/2 3/3/3"));QVERIFY(text.contains("f 4//4 5//5 6//6"));QCOMPARE(result.faces,quint64(2));
        o.axes={1,2,0};o.scale=2;QVERIFY(exportModel(obj,{{"one","",mesh}},o).ok);text=read(obj);QVERIFY(text.contains("v 0 0 2"));QVERIFY(text.contains("v 2 0 0"));QVERIFY(text.contains("vn 0 1 0"));
        o={};o.format=ModelFormat::Obj;o.negate[0]=true;QVERIFY(exportModel(obj,{{"one","",mesh}},o).ok);text=read(obj);QVERIFY(text.contains("v -1 0 0"));QVERIFY(text.contains("f 1/1/1 3/3/3 2/2/2"));o.reverseFaces=true;QVERIFY(exportModel(obj,{{"one","",mesh}},o).ok);QVERIFY(read(obj).contains("f 1/1/1 2/2/2 3/3/3"));QCOMPARE(mesh->vertices[1].position[0],1.f);QCOMPARE(mesh->triangles,QVector<quint32>({0,1,2}));
        o={};o.format=ModelFormat::Glb;o.material=true;o.flipV=true;QString glb=dir.filePath("test.glb");QVERIFY(exportModel(glb,{{"one","",mesh},{"two","",plain}},o).ok);auto bytes=read(glb);QCOMPARE(u32(bytes,0),quint32(0x46546c67));QCOMPARE(u32(bytes,4),quint32(2));QCOMPARE(u32(bytes,8),quint32(bytes.size()));quint32 jsonSize=u32(bytes,12);QCOMPARE(jsonSize%4,quint32(0));QCOMPARE(u32(bytes,16),quint32(0x4e4f534a));auto json=QJsonDocument::fromJson(bytes.mid(20,jsonSize)).object();QCOMPARE(json["meshes"].toArray().size(),2);QCOMPARE(json["nodes"].toArray().size(),2);QCOMPARE(json["materials"].toArray().size(),1);auto first=json["meshes"].toArray()[0].toObject()["primitives"].toArray()[0].toObject();auto second=json["meshes"].toArray()[1].toObject()["primitives"].toArray()[0].toObject();QVERIFY(!second["attributes"].toObject().contains("TEXCOORD_0"));auto accessors=json["accessors"].toArray(),views=json["bufferViews"].toArray();int binStart=28+jsonSize;QCOMPARE(u32(bytes,24+jsonSize),quint32(0x004e4942));
        for(auto value:views){auto v=value.toObject();QCOMPARE(v["byteOffset"].toInt()%4,0);QVERIFY(binStart+v["byteOffset"].toInt()+v["byteLength"].toInt()<=bytes.size());}
        int uvId=first["attributes"].toObject()["TEXCOORD_0"].toInt();int uvOffset=views[accessors[uvId].toObject()["bufferView"].toInt()].toObject()["byteOffset"].toInt();QCOMPARE(std::bit_cast<float>(u32(bytes,binStart+uvOffset+4)),1.f);
        int indexId=first["indices"].toInt();auto indexAccessor=accessors[indexId].toObject();QCOMPARE(indexAccessor["count"].toInt(),3);int indexOffset=views[indexAccessor["bufferView"].toInt()].toObject()["byteOffset"].toInt();QCOMPARE(u32(bytes,binStart+indexOffset+8),quint32(2));
        o.format=ModelFormat::Stl;QString stl=dir.filePath("test.stl");QVERIFY(exportModel(stl,{{"one","",mesh},{"two","",plain}},o).ok);bytes=read(stl);QCOMPARE(bytes.size(),qsizetype(184));QCOMPARE(u32(bytes,80),quint32(2));QCOMPARE(std::bit_cast<float>(u32(bytes,92)),1.f);
        QFile existing(obj);QVERIFY(existing.open(QIODevice::WriteOnly));existing.write("KEEP");existing.close();o.format=ModelFormat::Obj;o.axes={0,0,2};QVERIFY(!exportModel(obj,{{"one","",mesh}},o).ok);QCOMPARE(read(obj),QByteArray("KEEP"));o={};o.format=ModelFormat::Obj;auto cancel=std::make_shared<ExportProgress>();cancel->cancel=true;QVERIFY(exportModel(obj,{{"one","",mesh}},o,cancel).cancelled);QCOMPARE(read(obj),QByteArray("KEEP"));o.protectedPaths={obj};QVERIFY(!exportModel(obj,{{"one","",mesh}},o).ok);QCOMPARE(read(obj),QByteArray("KEEP"));
        o={};o.format=ModelFormat::Obj;auto invalid=std::make_shared<MeshData>(*mesh);invalid->triangles[1]=9;QVERIFY(!exportModel(obj,{{"invalid","",invalid}},o).ok);QCOMPARE(read(obj),QByteArray("KEEP"));invalid=std::make_shared<MeshData>(*mesh);for(auto &v:invalid->vertices)for(auto &n:v.normal)n=0;QVERIFY(!exportModel(obj,{{"invalid","",invalid}},o).ok);o.normals=ExportNormals::Recalculate;QVERIFY(exportModel(obj,{{"fixed","",invalid}},o).ok);QVERIFY(read(obj).contains("vn 0 0 1"));
    }
    void exportSphereFixture() {
        QString source=QFINDTESTDATA("../examples/sphere128.bin");auto meta=readPage({source,0,-1,{},0});auto mesh=readMesh({source,0,meta.size,meta.modified,0},{},MeshSpec{});QVERIFY(mesh.error.isEmpty());QTemporaryDir temporary;QString directory=qEnvironmentVariable("WORKBENCH_EXPORT_DIR",temporary.path());QVERIFY(QDir().mkpath(directory));
        for(auto format:{ModelFormat::Obj,ModelFormat::Glb,ModelFormat::Stl}){ModelExportOptions o;o.format=format;QString extension=format==ModelFormat::Obj?"obj":format==ModelFormat::Glb?"glb":"stl";auto result=exportModel(directory+"/sphere128."+extension,{{"Sphere128",source,mesh.mesh}},o);QVERIFY2(result.ok,qPrintable(result.error));QCOMPARE(result.vertices,quint64(128));QCOMPARE(result.faces,quint64(252));}
    }
    void meshCollectionIdentityAndOcclusion() {
        auto front=std::make_shared<MeshData>();for(auto point:{QVector3D(-1,-1,1),QVector3D(1,-1,1),QVector3D(0,1,1)}){MeshVertex v{};for(int j=0;j<3;++j)v.position[j]=point[j];front->vertices.append(v);}front->triangles={0,1,2};auto back=std::make_shared<MeshData>(*front);for(auto &v:back->vertices)v.position[2]=-1;
        MeshScene scene;int a=scene.add(front,"Frente","example.bin"),b=scene.add(back,"Fondo","example.bin");auto all=scene.assemble();QCOMPARE(all.data->triangles,QVector<quint32>({0,1,2,3,4,5}));QCOMPARE(all.spans[1].id,b);MeshDebug debug(*all.data);QVERIFY(!debug.visible({0,0,3},debug.positions[3]));
        scene.setVisible(a,false);auto visible=scene.assemble();MeshDebug exposed(*visible.data,all.data.get());QCOMPARE(exposed.positions[0],debug.positions[3]);QVERIFY(exposed.visible({0,0,3},exposed.positions[0]));scene.select(b,MeshElement::Vertex,2);scene.remove(a);QCOMPARE(scene.selection().mesh,b);QCOMPARE(scene.selection().index,2);
        int c=scene.add(front);QVERIFY(c>b);scene.markStale();QVERIFY(scene.entry(b)->stale);QVERIFY(!scene.entry(c)->stale);scene.replace(b,front,"new.bin");QVERIFY(!scene.entry(b)->stale);QCOMPARE(scene.entry(b)->id,b);scene.setVisible(b,false);scene.select(b,MeshElement::Vertex,0);QCOMPARE(scene.selection().index,-1);scene.clear();QVERIFY(scene.entries().isEmpty());QCOMPARE(scene.active(),-1);
    }
    void meshLabelOcclusionAndPicking() {
        MeshData mesh;for(float z:{.5f,-.5f})for(auto p:{QVector3D(-1,-1,z),QVector3D(1,-1,z),QVector3D(0,1,z)}){MeshVertex v{};for(int j=0;j<3;++j)v.position[j]=p[j];mesh.vertices.append(v);}mesh.triangles={0,1,2,3,4,5};MeshDebug debug(mesh);
        QVector3D eye(0,0,3);QMatrix4x4 projection;projection.perspective(45,1,.01f,100);QMatrix4x4 correction;correction(1,1)=-1;correction(2,2)=.5f;correction(2,3)=.5f;QMatrix4x4 camera;camera.lookAt(eye,{0,0,0},{0,1,0});auto mvp=correction*projection*camera;
        QVERIFY(debug.visible(eye,debug.positions[0]));QVERIFY(!debug.visible(eye,debug.positions[3]));
        QCOMPARE(debug.hit(eye,{0,0,-1}),0);QCOMPARE(debug.hit({0,0,-3},{0,0,1}),1);
        auto labels=debug.labels(mvp,eye,{500,500},MeshElement::Vertex,0,6);QCOMPARE(labels.size(),3);for(auto l:labels)QVERIFY(l.id<3);
        QCOMPARE(debug.labels(mvp,eye,{500,500},MeshElement::Face,0,2).size(),1);
        QCOMPARE(debug.labels(mvp,eye,{500,500},MeshElement::Vertex,3,3).size(),0);
        QCOMPARE(debug.pick(mvp,eye,{500,500},{250,250},MeshElement::Face),0);QCOMPARE(debug.pick(mvp,eye,{500,500},{250,250},MeshElement::Mesh),0);
        QCOMPARE(debug.pick(mvp,eye,{500,500},labels.first().pixel,MeshElement::Vertex),0);
        QCOMPARE(debug.pick(mvp,eye,{500,500},{1,1},MeshElement::Face),-1);
        auto geometry=labelGeometry(labels,{500,500},MeshElement::Vertex,1.5f,0);QVERIFY(!geometry.isEmpty());QCOMPARE(geometry.size()%6,0);for(auto v:geometry){QVERIFY(std::abs(v.position[0])<=1);QVERIFY(std::abs(v.position[1])<=1);}
        QCOMPARE(indexColor(0,3,{Qt::red,Qt::green,Qt::blue}),QColor(Qt::red));QCOMPARE(indexColor(2,3,{Qt::red,Qt::green,Qt::blue}),QColor(Qt::blue));QCOMPARE(indexColor(1,3,{Qt::red,Qt::green,Qt::blue}),QColor(Qt::green));
        QMatrix4x4 behind;behind.lookAt({0,0,-3},{0,0,-4},{0,1,0});QVERIFY(debug.labels(correction*projection*behind,{0,0,-3},{500,500},MeshElement::Vertex,0,6).isEmpty());
    }
    void tabularInspector() {
        DataInspector inspector;inspector.resize(440,850);inspector.show();
        auto value=[&](QString type){auto *table=inspector.findChild<QTableWidget *>("inspectorValues");for(int i=0;i<table->rowCount();++i)if(table->item(i,0)->text()==type)return table->item(i,1)->text();return QString();};
        inspector.setData(QByteArray::fromHex("0102030405060708"),{0,7,16,false},8,"ASCII",false);
        QCOMPARE(value("UInt16"),QString("513"));QCOMPARE(value("Binario (8 bits)"),QString("00000001"));
        inspector.findChild<QComboBox *>("inspectorEndian")->setCurrentIndex(1);QCOMPARE(value("UInt16"),QString("258"));
        inspector.findChild<QCheckBox *>("inspectorHex")->setChecked(true);QCOMPARE(value("UInt64"),QString("0x0102030405060708"));
        inspector.findChild<QCheckBox *>("inspectorHex")->setChecked(false);
        inspector.setData(QByteArray(8,char(0xff)),{0,0,16,false},8,"ASCII",false);QCOMPARE(value("UInt64"),QString("18446744073709551615"));QCOMPARE(value("Int64"),QString("-1"));
        inspector.setData(QByteArray(8,0),{0,0,16,false},8,"ASCII",false);QCOMPARE(value("Unix Int64"),QString("1970-01-01 00:00:00 UTC"));QCOMPARE(value("FILETIME"),QString("1601-01-01 00:00:00 UTC"));
        inspector.setData(QByteArray(1,'A'),{0,0,16,false},1,"ASCII",false);QVERIFY(value("UInt16").contains("insuficientes"));QCOMPARE(value("ASCII"),QString("A"));
        inspector.setData(QByteArray::fromHex("73cc2a0000000000"),{0,7,16,false},8,"ASCII",false);
        if(qEnvironmentVariableIsSet("WORKBENCH_INSPECTOR_SCREENSHOT"))inspector.grab().save(qEnvironmentVariable("WORKBENCH_INSPECTOR_SCREENSHOT"));
    }
    void sphereMeshFixture() {
        QString path=QFINDTESTDATA("../examples/sphere128.bin");QVERIFY(!path.isEmpty());auto meta=readPage({path,0,-1,{},128});QCOMPARE(meta.bytes.left(4),QByteArray("BWS1"));QCOMPARE(meta.size,qint64(5296));ReadRequest source{path,0,meta.size,meta.modified,0};MeshSpec spec;
        auto result=readMesh(source,{},spec);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.mesh->vertices.size(),128);QCOMPARE(result.mesh->triangles.size(),756);QCOMPARE(result.mesh->vertexOffsets.size(),128);QCOMPARE(result.mesh->vertexOffsets[127],qint64(128+127*12));QCOMPARE(result.mesh->faceSources.size(),252);
        QFile original(path);QVERIFY(original.open(QIODevice::ReadOnly));auto raw=original.readAll();for(int face=0;face<252;++face)for(int j=0;j<3;++j){auto source=result.mesh->faceSources[face];QCOMPARE(source.offsets[j],qint64(1664+source.indexSlots[j]*2));quint16 value=quint8(raw[source.offsets[j]])|(quint16(quint8(raw[source.offsets[j]+1]))<<8);QCOMPARE(quint32(value),result.mesh->triangles[face*3+j]);}

        for(auto v:result.mesh->vertices){double length=std::hypot(v.position[0],v.position[1],v.position[2]);QVERIFY(std::abs(length-1)<.0001);QVERIFY(v.uv[0]>=0&&v.uv[0]<=1);QVERIFY(v.uv[1]>=0&&v.uv[1]<=1);QVERIFY(std::abs(std::hypot(v.normal[0],v.normal[1],v.normal[2])-1)<.00001);}
        QMap<QPair<quint32,quint32>,int> edges;for(int i=0;i<756;i+=3){auto &t=result.mesh->triangles;for(int j=0;j<3;++j){auto a=t[i+j],b=t[i+(j+1)%3];edges[{qMin(a,b),qMax(a,b)}]++;}}for(auto count:edges)QCOMPARE(count,2);
        spec.normalOffset=4528;spec.normalStride=6;spec.normalType="SNorm16";auto ints=readMesh(source,{},spec);QVERIFY(ints.error.isEmpty());for(int i=0;i<128;++i)for(int j=0;j<3;++j)QVERIFY(std::abs(result.mesh->vertices[i].normal[j]-ints.mesh->vertices[i].normal[j])<.0001);
        spec.normals=false;QVERIFY(readMesh(source,{},spec).error.isEmpty());spec.vertexCount=100001;QVERIFY(!readMesh(source,{},spec).error.isEmpty());spec=MeshSpec{};spec.vertexOffset=meta.size-1;QVERIFY(!readMesh(source,{},spec).error.isEmpty());spec=MeshSpec{};
        QVERIFY(!readMesh(source,{{1664,QByteArray::fromHex("feff")}},spec).error.isEmpty());auto edit=readMesh(source,{{128,QByteArray::fromHex("10270000")}},spec);QVERIFY(edit.error.isEmpty());QCOMPARE(edit.mesh->vertices[0].position[0],1.f);
        spec.bigEndian=true;QVERIFY(!readMesh(source,{},spec).error.isEmpty());
    }
    void nativeShiftDigitBroadcast() {
#ifdef Q_OS_MACOS
        QTemporaryDir dir;auto path=dir.filePath("native-keys.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(QByteArray::fromHex("abcd"));file.close();
        HexView view;view.resize(1200,500);view.show();view.openFile(path);QTRY_VERIFY(!view.busy());view.selectMatch(0,2);auto *table=view.findChild<QTableWidget *>("bytes");
        QKeyEvent first(QEvent::KeyPress,Qt::Key_ParenRight,Qt::ShiftModifier,0,29,0,")");QApplication::sendEvent(table,&first);QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("0B"));QCOMPARE(table->item(0,2)->text(),QString("0D"));
        QKeyEvent second(QEvent::KeyPress,Qt::Key_Equal,Qt::ShiftModifier,0,29,0,"=");QApplication::sendEvent(table,&second);QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("00"));QCOMPARE(table->item(0,2)->text(),QString("00"));QCOMPARE(view.selection().count(2),qint64(2));view.shutdown();
#endif
    }
    void directEditingTransforms() {
        QTemporaryDir dir;auto path=dir.filePath("direct.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(QByteArray::fromHex("abcd12347856"));file.close();
        auto meta=readPage({path,0,-1,{},0});ReadRequest source{path,0,meta.size,meta.modified,6};DirectEditSpec spec;spec.ranges={{0,2},{4,2}};spec.character="0";spec.broadcast=true;
        auto r=directEdit(source,spec);QVERIFY(r.error.isEmpty());QCOMPARE(r.bytes,QByteArray::fromHex("0b0d0806"));QCOMPARE(r.matches.size(),2);
        spec.patches={{0,r.bytes.left(2)},{4,r.bytes.mid(2)}};spec.digit=1;r=directEdit(source,spec);QCOMPARE(r.bytes,QByteArray(4,0));spec.character="G";QVERIFY(!directEdit(source,spec).error.isEmpty());
        spec={};spec.hex=false;spec.type="UInt16";spec.ranges={{2,2}};spec.character="1";source.offset=2;source.length=2;r=directEdit(source,spec);QVERIFY(r.error.isEmpty());QCOMPARE(decodeNumber(r.bytes,"UInt16",false),QString("13330"));
        spec.type="UInt8";spec.ranges={{2,1}};spec.character="9";source.length=1;r=directEdit(source,spec);QVERIFY(r.error.isEmpty());QCOMPARE(quint8(r.bytes[0]),98);spec.digit=1;spec.drafts=r.directDrafts;spec.character="0";r=directEdit(source,spec);QCOMPARE(quint8(r.bytes[0]),90);
        spec.type="Int8";spec.character="9";spec.digit=0;spec.drafts={{2,"127"}};QVERIFY(!directEdit(source,spec).error.isEmpty());
        spec.type="Texto";spec.drafts.clear();spec.encoding="ASCII";spec.character="Q";r=directEdit(source,spec);QCOMPARE(r.bytes,QByteArray("Q"));spec.character=QString::fromUtf8("日");QVERIFY(!directEdit(source,spec).error.isEmpty());
        spec.encoding="UTF-8";QVERIFY(!directEdit(source,spec).error.isEmpty());
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write(QByteArray::fromHex("00004841"));file.close();meta=readPage({path,0,-1,{},0});source={path,0,meta.size,meta.modified,4};spec={};spec.hex=false;spec.type="Float32";spec.ranges={{0,4}};spec.character="2";r=directEdit(source,spec);QVERIFY(r.error.isEmpty());QCOMPARE(decodeNumber(r.bytes,"Float32",false),QString("22.5"));
    }
    void directKeyboardAndBroadcast() {
        QTemporaryDir dir;auto path=dir.filePath("keys.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(QByteArray::fromHex("abcd12345678"));file.close();HexView view;view.resize(1400,600);view.show();view.openFile(path);QTRY_VERIFY(!view.busy());auto *table=view.findChild<QTableWidget *>("bytes");
        view.selectMatch(0,2);QTest::keyClick(table,Qt::Key_0,Qt::ShiftModifier);QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("0B"));QCOMPARE(table->item(0,2)->text(),QString("0D"));QCOMPARE(view.selection().count(6),qint64(2));
        QTest::keyClick(table,Qt::Key_0,Qt::ShiftModifier);QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("00"));QCOMPARE(table->item(0,2)->text(),QString("00"));view.undo();QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,2)->text(),QString("0D"));view.undo();QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("AB"));
        view.selectMatch(0,1);QTest::keyClicks(table,"1234");QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("12"));QCOMPARE(table->item(0,2)->text(),QString("34"));QCOMPARE(view.selection().anchor,qint64(2));
        QTest::keyClicks(table,"g");QCOMPARE(table->item(0,3)->text(),QString("12"));
        auto *type=view.findChild<QComboBox *>("interpretation");type->setCurrentText("UInt8");QTRY_VERIFY(!view.busy());auto rect=table->visualItemRect(table->item(0,17));QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,rect.topLeft()+QPoint(8,10));QTest::keyClicks(table,"09");QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("09"));QCOMPARE(view.selection().anchor,qint64(1));
        type->setCurrentText("Texto");QTRY_VERIFY(!view.busy());rect=table->visualItemRect(table->item(0,17));QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,rect.topLeft()+QPoint(8,10));QTest::keyClicks(table,"Z");QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("5A"));
        view.shutdown();
    }
    void entropyKnownDistributions() {
        QTemporaryDir dir;auto path=dir.filePath("entropy.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));QByteArray bytes(1024,0);for(int i=0;i<1024;++i)bytes.append(char(i%256));bytes.append("tail");file.write(bytes);file.close();
        auto meta=readPage({path,0,-1,{},0});ReadRequest source{path,0,meta.size,meta.modified,0};EntropySpec spec{0,bytes.size(),1024,{}};std::atomic<bool> cancel{false};std::atomic<qint64> progress{0};
        auto r=analyzeEntropy(source,spec,cancel,progress);QVERIFY(r.error.isEmpty());QCOMPARE(r.entropy.size(),3);QCOMPARE(r.entropy[0].entropy,0.0);QCOMPARE(r.entropy[0].zeros,100.0);QCOMPARE(r.entropy[1].entropy,8.0);QCOMPARE(r.entropy[1].printable,100.0*95/256);QCOMPARE(r.entropy[2].length,qint64(4));
        spec.begin=1024;spec.length=1024;spec.patches={{1024,QByteArray(1024,0)}};r=analyzeEntropy(source,spec,cancel,progress);QCOMPARE(r.entropy[0].offset,qint64(1024));QCOMPARE(r.entropy[0].entropy,0.0);
        cancel=true;QVERIFY(analyzeEntropy(source,spec,cancel,progress).cancelled);cancel=false;source.expectedSize++;QVERIFY(!analyzeEntropy(source,spec,cancel,progress).error.isEmpty());source.expectedSize--;
        spec.length=0;QVERIFY(!analyzeEntropy(source,spec,cancel,progress).error.isEmpty());
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write(QByteArray(3*1024*1024,0));file.close();meta=readPage({path,0,-1,{},0});source={path,0,meta.size,meta.modified,0};spec={0,meta.size,256,{}};r=analyzeEntropy(source,spec,cancel,progress);QVERIFY(r.entropy.size()<=8192);QCOMPARE(r.entropy.last().offset+r.entropy.last().length,meta.size);QCOMPARE(r.entropy.last().entropy,0.0);
    }
    void magicLibraryAndChunkBoundaries() {
        QTemporaryDir dir;auto path=dir.filePath("magic.bin");QByteArray bytes(1024*1024+20,'Z');
        bytes.replace(1024*1024-2,4,QByteArray::fromHex("1234abef"));
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(bytes);file.close();
        auto meta=readPage({path,0,-1,{},0});ReadRequest source{path,0,meta.size,meta.modified,0};
        MagicSpec spec;spec.signatures={{"Test","12 34 A? ?F","",2,true}};
        std::atomic<bool> cancel{false};std::atomic<qint64> progress{0};
        auto r=scanMagic(source,spec,cancel,progress);QVERIFY2(r.error.isEmpty(),qPrintable(r.error));QCOMPARE(r.matches.size(),1);QCOMPARE(r.matches[0].first,qint64(1024*1024-2));QCOMPARE(r.matchTypes[0],0);
        spec.anchored=true;spec.regions={{1024*1024-4,6}};r=scanMagic(source,spec,cancel,progress);QCOMPARE(r.matches.size(),1);
        spec.regions[0].length=5;QVERIFY(scanMagic(source,spec,cancel,progress).matches.isEmpty());
        spec.anchored=false;spec.patches={{12,QByteArray::fromHex("1234acff")}};QCOMPARE(scanMagic(source,spec,cancel,progress).matches.size(),2);
        spec.signatures[0].enabled=false;QVERIFY(!scanMagic(source,spec,cancel,progress).error.isEmpty());spec.signatures[0].enabled=true;
        cancel=true;QVERIFY(scanMagic(source,spec,cancel,progress).cancelled);cancel=false;
        spec.signatures[0].pattern="5?";r=scanMagic(source,spec,cancel,progress);QCOMPARE(r.matches.size(),10000);QVERIFY(r.truncated);
        source.expectedSize++;QVERIFY(!scanMagic(source,spec,cancel,progress).error.isEmpty());
        QString error;QList<MagicSignature> decoded;QVERIFY(decodeMagicLibrary(encodeMagicLibrary(spec.signatures).toJson(),decoded,error));QCOMPARE(decoded[0].relativeOffset,qint64(2));
        QVERIFY(!decodeMagicLibrary("{}",decoded,error));QCOMPARE(decoded.size(),1);
        auto bad=decoded[0];bad.pattern="??";QVERIFY(!validateMagic(bad,error));bad.pattern="A";QVERIFY(!validateMagic(bad,error));
    }
    void patternSearchAcrossBlocksAndTextOffsets() {
        QTemporaryDir dir;const auto path=dir.filePath("search.bin");
        QByteArray bytes(1024*1024+30,'Z');bytes.replace(1024*1024-1,4,QByteArray::fromHex("488b01af"));
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(bytes);file.close();
        auto meta=readPage({path,0,-1,{},0});ReadRequest source{path,0,meta.size,meta.modified,0};
        std::atomic<bool> cancel{false};std::atomic<qint64> progress{0};
        PatternSpec spec;spec.expression="48 8B ?? A?";spec.length=bytes.size();
        auto result=searchPattern(source,spec,cancel,progress);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));
        QCOMPARE(result.matches.size(),1);QCOMPARE(result.matches[0].first,qint64(1024*1024-1));QCOMPARE(result.matches[0].second,qint64(4));
        spec.patches={ {10,QByteArray::fromHex("488b03a1")} };result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),2);
        cancel.store(true);QVERIFY(searchPattern(source,spec,cancel,progress).cancelled);cancel.store(false);
        spec.expression="??";result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),10000);QVERIFY(result.truncated);
        spec.regex=true;spec.expression="\\x48.\\x01[\\xA0-\\xAF]";spec.begin=1024*1024-5;spec.length=20;spec.patches.clear();
        result=searchPattern(source,spec,cancel,progress);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.matches.size(),1);QCOMPARE(result.matches[0].first,qint64(1024*1024-1));
        spec.length=1024*1024+1;spec.begin=0;QVERIFY(!searchPattern(source,spec,cancel,progress).error.isEmpty());
        spec.length=20;spec.expression="(";QVERIFY(!searchPattern(source,spec,cancel,progress).error.isEmpty());
        spec.expression="Z*";spec.length=20;result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),1);
        const auto text=QString::fromUtf8("AA 日本42 😀 BBB").toUtf8();
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write(text);file.close();
        meta=readPage({path,0,-1,{},0});source={path,0,meta.size,meta.modified,0};
        spec={};spec.text=true;spec.encoding="UTF-8";spec.expression=QString::fromUtf8("日本42");spec.length=text.size();
        result=searchPattern(source,spec,cancel,progress);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.matches.size(),1);QCOMPARE(result.matches[0].first,qint64(3));QCOMPARE(result.matches[0].second,qint64(8));
        spec.regex=true;spec.expression=QString::fromUtf8("日本[0-9]+");result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),1);QCOMPARE(result.matches[0].second,qint64(8));
        FormatRule r;r.name="Match";for(auto m:result.matches)r.matchRanges.append({m.first,m.second});
        QVERIFY(r.matches(3));QVERIFY(r.matches(10));QVERIFY(!r.matches(11));FormatRule restored;QVERIFY(decodeRule(encodeRule(r),restored));QCOMPARE(restored.matchRanges[0].length,qint64(8));
        spec.expression="not found";QVERIFY(searchPattern(source,spec,cancel,progress).matches.isEmpty());
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write(QByteArray::fromHex("410042006500"));file.close();
        meta=readPage({path,0,-1,{},0});source={path,0,meta.size,meta.modified,0};spec.length=meta.size;spec.encoding="UTF-16LE";spec.expression="B.";
        result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),1);QCOMPARE(result.matches[0].first,qint64(2));QCOMPARE(result.matches[0].second,qint64(4));
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write(QByteArray::fromHex("4193fa967b42"));file.close();
        meta=readPage({path,0,-1,{},0});source={path,0,meta.size,meta.modified,0};spec.length=meta.size;spec.encoding="CP932";spec.expression=QString::fromUtf8("日本");
        result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),1);QCOMPARE(result.matches[0].first,qint64(1));QCOMPARE(result.matches[0].second,qint64(4));
    }
    void formatRulesAndMultibyteMapping() {
        FormatRule repeated;repeated.name="Vectors";repeated.start=10LL*1024*1024*1024;
        repeated.length=12;repeated.step=16;repeated.repetitions=1000000;
        repeated.style.background=QColor("#f6d365");repeated.style.border=Qt::red;
        QVERIFY(repeated.matches(repeated.start));QVERIFY(repeated.matches(repeated.start+11));
        QVERIFY(!repeated.matches(repeated.start+12));QVERIFY(repeated.matches(repeated.start+16));
        QVERIFY(!repeated.matches(repeated.start+16*1000000));
        FormatRule restored;QVERIFY(decodeRule(encodeRule(repeated),restored));
        QCOMPARE(restored.start,repeated.start);QCOMPARE(restored.style.background,repeated.style.background);
        auto invalid=encodeRule(repeated);invalid["step"]="0";QVERIFY(!decodeRule(invalid,restored));
        invalid=encodeRule(repeated);invalid["repetitions"]="9223372036854775807";QVERIFY(!decodeRule(invalid,restored));
        FormatRule rectangle;rectangle.selectionRule=true;rectangle.selection={1,19,8,true};
        QVERIFY(rectangle.matches(9));QVERIFY(!rectangle.matches(12));
        QVERIFY(decodeRule(encodeRule(rectangle),restored));QVERIFY(restored.matches(17));
        auto utf8=decodeTokens(QString::fromUtf8("A日😀").toUtf8(),100,"UTF-8","Texto",false);
        QCOMPARE(utf8.size(),3);QCOMPARE(utf8[1].offset,qint64(101));QCOMPARE(utf8[1].length,qint64(3));QCOMPARE(utf8[2].length,qint64(4));
        auto utf16=decodeTokens(QByteArray::fromHex("41003dd800de"),0,"UTF-16LE","Texto",false);
        QCOMPARE(utf16.size(),2);QCOMPARE(utf16[1].length,qint64(4));
        auto japanese=decodeTokens(QByteArray::fromHex("93fa967b"),0,"CP932","Texto",false);
        QCOMPARE(japanese.size(),2);QCOMPARE(japanese[0].text,QString::fromUtf8("日"));QCOMPARE(japanese[1].offset,qint64(2));
        auto floats=decodeTokens(QByteArray::fromHex("0000803f00000040"),0,"ASCII","Float32",false);
        QCOMPARE(floats.size(),2);QCOMPARE(floats[0].length,qint64(4));QCOMPARE(floats[1].text,QString("2"));
    }
    void formattingSurvivesNavigationAndDoesNotEditBytes() {
        QTemporaryDir dir;const auto path=dir.filePath("format.bin");
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(QByteArray(8192,'A'));file.close();
        HexView view;view.resize(1100,700);view.show();view.openFile(path);QTRY_VERIFY(!view.busy());
        FormatRule r;r.name="Header";r.start=1;r.length=2;r.step=4;r.repetitions=2000;
        r.style.background=QColor("#123456");r.style.border=Qt::red;r.style.bold=true;
        view.setFormatRules({r});QVERIFY(!view.dirty());
        QCOMPARE(view.formatAt(1).style.background,QColor("#123456"));QVERIFY(!view.formatAt(3).style.background.isValid());
        // A decoded number/character adopts a rule touching any constituent byte.
        QCOMPARE(view.formatAt(0,4).style.background,QColor("#123456"));
        FormatRule override;override.selectionRule=true;override.selection={2,2,16,false};override.style.background=Qt::green;
        view.setFormatRules({r,override});QCOMPARE(view.formatAt(0,4).style.background,QColor(Qt::green));
        override.enabled=false;view.setFormatRules({r,override});QCOMPARE(view.formatAt(0,4).style.background,QColor("#123456"));
        view.jumpTo(4097);QTRY_VERIFY(!view.busy());QCOMPARE(view.formatAt(4097).style.background,QColor("#123456"));
        view.findChild<QSpinBox *>("stride")->setValue(8);QTRY_VERIFY(!view.busy());
        QCOMPARE(view.formatAt(4097).style.background,QColor("#123456"));
        auto *table=view.findChild<QTableWidget *>();
        const auto hex=table->item(0,2)->data(ByteFormatDelegate::TokensRole).toList().first().toMap();
        const auto decoded=table->item(0,9)->data(ByteFormatDelegate::TokensRole).toList()[1].toMap();
        QCOMPARE(hex["offset"],decoded["offset"]);
        QSignalSpy saved(&view,&HexView::saveFinished);QVERIFY(view.saveFile(path));QTRY_COMPARE(saved.count(),1);QTRY_VERIFY(!view.busy());
        QCOMPARE(view.formatRules().size(),2);QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray(8192,'A'));file.close();
        view.openFile(path);QTRY_VERIFY(!view.busy());QVERIFY(view.formatRules().isEmpty());
    }
    void editHistoryAndSafeStreamingSave() {
        QTemporaryDir dir;
        const auto path=dir.filePath("source.bin"), target=dir.filePath("copy.bin");
        QByteArray original(2*1024*1024+7,'A');
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(original),original.size());file.close();
        const auto metadata=readPage({path,0,-1,{},0});
        const ReadRequest source{path,0,metadata.size,metadata.modified,metadata.size};
        EditHistory edits;QString error;
        QVERIFY(edits.push({{1024*1024-1,QByteArray("XYZ")}},error));
        QVERIFY(edits.push({{1024*1024,QByteArray("12")}},error));
        auto buffer=original;applyPatches(buffer,0,edits.patches());
        QCOMPARE(buffer.mid(1024*1024-1,3),QByteArray("X12"));
        edits.undo();buffer=original;applyPatches(buffer,0,edits.patches());
        QCOMPARE(buffer.mid(1024*1024-1,3),QByteArray("XYZ"));
        edits.redo();
        std::atomic<qint64> progress{0};
        auto saved=savePatchedFile(source,target,edits.patches(),&progress);
        QVERIFY2(saved.error.isEmpty(),qPrintable(saved.error));
        QCOMPARE(progress.load(),qint64(original.size()));
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),original);file.close();
        QFile copy(target);QVERIFY(copy.open(QIODevice::ReadOnly));auto expected=original;
        expected.replace(1024*1024-1,3,"X12");QCOMPARE(copy.readAll(),expected);copy.close();
        saved=savePatchedFile(source,path,edits.patches());
        QVERIFY2(saved.error.isEmpty(),qPrintable(saved.error));
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),expected);file.close();
        // Failure must leave an existing destination intact.
        auto stale=source;stale.expectedSize++;
        QVERIFY(!savePatchedFile(stale,target,edits.patches()).error.isEmpty());
        QVERIFY(copy.open(QIODevice::ReadOnly));QCOMPARE(copy.readAll(),expected);copy.close();
        const auto fresh=readPage({path,0,-1,{},0});
        ReadRequest current{path,0,fresh.size,fresh.modified,fresh.size};
        QVERIFY(!savePatchedFile(current,dir.filePath("missing/copy.bin"),{}).error.isEmpty());
        QVERIFY(!savePatchedFile(current,target,{{fresh.size,QByteArray("!")}}).error.isEmpty());
        QVERIFY(copy.open(QIODevice::ReadOnly));QCOMPARE(copy.readAll(),expected);
        // Replacement addressing remains 64-bit without allocating the file in RAM.
        QByteArray high("abcd");applyPatches(high,10LL*1024*1024*1024,{{10LL*1024*1024*1024+2,"Z"}});
        QCOMPARE(high,QByteArray("abZd"));
        edits.undo();QVERIFY(edits.push({{0,"B"}},error));QVERIFY(!edits.canRedo());
        QVERIFY(!edits.push({{0,QByteArray(EditHistory::MaxBytes+1,'C')}},error));
    }
    void editingUpdatesViewInspectorCopyAndSave() {
        QTemporaryDir dir;const auto path=dir.filePath("edit.bin");
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));QCOMPARE(file.write(QByteArray(8192,'A')),8192);file.close();
        HexView view;view.resize(1100,700);view.show();view.openFile(path);
        QTRY_VERIFY(!view.busy());
        auto *table=view.findChild<QTableWidget *>();QString error;
        QVERIFY(view.replaceSelection(QByteArray::fromHex("4243"),error));
        QVERIFY(view.dirty());QCOMPARE(table->item(0,1)->text(),QString("42"));
        QTRY_VERIFY(!view.busy());
        QVERIFY(view.findChild<DataInspector *>("inspector")->toPlainText().contains("UInt8: 66"));
        view.copySelection();QTRY_VERIFY(!view.busy());
        QCOMPARE(QApplication::clipboard()->text(),QString("42"));
        view.undo();QCOMPARE(table->item(0,1)->text(),QString("41"));QVERIFY(!view.dirty());
        view.redo();QCOMPARE(table->item(0,1)->text(),QString("42"));
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.read(2),QByteArray("AA"));file.close();
        QSignalSpy saved(&view,&HexView::saveFinished);
        QVERIFY(view.saveFile(path));QVERIFY(view.saving());
        QVERIFY(!view.replaceSelection("D",error));
        QTRY_COMPARE(saved.count(),1);QVERIFY(saved.first().first().toBool());
        QTRY_VERIFY(!view.busy());QVERIFY(!view.dirty());QVERIFY(!view.canRedo());
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.read(2),QByteArray("BC"));file.close();
        QCOMPARE(table->item(0,1)->text(),QString("42"));
        // Rectangular replacement touches only selected columns, and undo restores them.
        auto center=[table](int row,int col){return table->visualItemRect(table->item(row,col)).center();};
        QTest::mousePress(table->viewport(),Qt::LeftButton,Qt::AltModifier,center(0,2));
        QTest::mouseMove(table->viewport(),center(1,3));
        QTest::mouseRelease(table->viewport(),Qt::LeftButton,Qt::AltModifier,center(1,3));
        QCOMPARE(view.selection().count(8192),qint64(4));
        QVERIFY(view.replaceSelection("WXYZ",error));
        QCOMPARE(table->item(0,2)->text(),QString("57"));
        QCOMPARE(table->item(1,2)->text(),QString("59"));
        QCOMPARE(table->item(0,4)->text(),QString("41"));
        QVERIFY(!view.replaceSelection("X",error));
        view.undo();QCOMPARE(table->item(0,2)->text(),QString("43"));
        view.redo();
        QVERIFY(view.saveFile(dir.filePath("missing/failure.bin")));
        QTRY_COMPARE(saved.count(),2);QVERIFY(!saved.last().first().toBool());
        QVERIFY(view.dirty());
        QTRY_VERIFY(!view.busy());
    }
    void activityTracksSupersededWork() {
        QSemaphore gate;
        BinaryReader reader(nullptr, [&](const ReadRequest &request) {
            gate.acquire();
            ReadResult result; result.request = request;
            if (request.offset == 3) result.error = "Test error";
            return result;
        });
        reader.request({"test", 1});
        reader.request({"test", 2});
        reader.request({"test", 3});
        // Release before assertions so a failed assertion cannot block destruction.
        gate.release(2);
        auto activity = reader.activity();
        QVERIFY(activity.running); QVERIFY(activity.pending);
        QCOMPARE(activity.started, quint64(1));
        QCOMPARE(activity.replaced, quint64(1));
        QTRY_VERIFY(!reader.busy());
        activity = reader.activity();
        QVERIFY(!activity.pending);
        QCOMPARE(activity.started, quint64(2));
        QCOMPARE(activity.completed, quint64(2));
        QCOMPARE(activity.discarded, quint64(1));
        QCOMPARE(activity.errors, quint64(1));
        QCOMPARE(activity.elapsedMs, qint64(0));
        reader.shutdown();
        QVERIFY(reader.activity().stopping);
    }
    void largeSparseFile() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const auto path = dir.filePath("large.bin");
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadWrite));
        constexpr qint64 size = 10LL * 1024 * 1024 * 1024;
        QVERIFY(file.resize(size));
        QVERIFY(file.seek(size - 3));
        QCOMPARE(file.write("XYZ", 3), 3);
        file.close();
        auto first = readPage({path});
        QVERIFY2(first.error.isEmpty(), qPrintable(first.error));
        QCOMPARE(first.size, size);
        QCOMPARE(first.bytes.size(), PageBytes);
        auto tail = readPage({path, size - 3, size, first.modified});
        QVERIFY(tail.error.isEmpty());
        QCOMPARE(tail.bytes, QByteArray("XYZ"));
        HexView view;view.resize(1100,700);view.show();
        view.openFile(path);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(), 3000);
        auto *table = view.findChild<QTableWidget *>();
        QVERIFY(table->rowCount()>0); QVERIFY(table->rowCount()<100);
        auto *address = view.findChild<QLineEdit *>("address");
        address->setText("0x" + QString::number(size - 3, 16));
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(), 3000);
        const int last=table->rowCount()-1;
        QCOMPARE(table->item(last, 14)->text(), QString("58"));
        QCOMPARE(table->item(last, 15)->text(), QString("59"));
        QCOMPARE(table->item(last, 16)->text(), QString("5A"));
        QCOMPARE(view.selection().anchor, size-3);
        QVERIFY(view.findChild<DataInspector *>("inspector")->toPlainText().contains("XYZ"));
        QVERIFY(!readPage({path, size + 1}).error.isEmpty());
        QVERIFY(!readPage({path, -1}).error.isEmpty());
        QVERIFY(readPage({path, size}).bytes.isEmpty());
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("short", 5), 5);
        file.close();
        QVERIFY(!readPage({path, 0, size, first.modified}).error.isEmpty());
    }
    void emptyAndMissing() {
        QTemporaryDir dir;
        const auto path = dir.filePath("empty.bin");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly)); file.close();
        const auto result = readPage({path});
        QVERIFY(result.error.isEmpty()); QCOMPARE(result.size, 0); QVERIFY(result.bytes.isEmpty());
        QVERIFY(!readPage({dir.filePath("missing")}).error.isEmpty());
        QVERIFY(!readPage({dir.path()}).error.isEmpty());
    }
    void latestRequestWinsWithoutBlocking() {
        std::atomic<int> jobs{0};
        BinaryReader reader(nullptr, [&](const ReadRequest &request) {
            ++jobs;
            QThread::msleep(80);
            ReadResult result; result.request = request; return result;
        });
        QStringList delivered;
        connect(&reader, &BinaryReader::ready, this, [&](const ReadResult &r) { delivered << r.request.path; });
        int ticks = 0;
        QTimer timer; timer.setInterval(5);
        connect(&timer, &QTimer::timeout, this, [&] { ++ticks; }); timer.start();
        reader.request({"old"});
        for (int i = 0; i < 1000; ++i) reader.request({QString::number(i)});
        QTRY_VERIFY_WITH_TIMEOUT(!reader.busy(), 3000);
        QCOMPARE(jobs.load(), 2);
        QCOMPARE(delivered, QStringList{"999"});
        QVERIFY(ticks > 2);
    }
    void shutdownDiscardsPendingAndResults() {
        BinaryReader reader(nullptr, [](const ReadRequest &request) {
            QThread::msleep(60);
            ReadResult r; r.request = request; return r;
        });
        int results = 0;
        connect(&reader, &BinaryReader::ready, this, [&](const ReadResult &) { ++results; });
        reader.request({"active"}); reader.request({"pending"});
        reader.shutdown();
        QTRY_VERIFY_WITH_TIMEOUT(!reader.busy(), 3000);
        QCOMPARE(results, 0);
    }
    void chunkCacheBoundAndReuse() {
        QTemporaryDir dir;const auto path=dir.filePath("cache.bin");QFile file(path);
        QVERIFY(file.open(QIODevice::ReadWrite));QVERIFY(file.resize(25*ChunkCache::ChunkBytes));
        QVERIFY(file.seek(ChunkCache::ChunkBytes-2));QCOMPARE(file.write("ABCD",4),4);file.close();
        int reads=0;
        ChunkCache cache([&](const ReadRequest &r){if(r.length>0)++reads;return readPage(r);});
        cache.reset(path);
        auto crossing=cache.read({path,ChunkCache::ChunkBytes-2,-1,{},4});
        QVERIFY(crossing.error.isEmpty());QCOMPARE(crossing.bytes,QByteArray("ABCD"));QCOMPARE(reads,2);
        QCOMPARE(cache.read({path,ChunkCache::ChunkBytes-2,-1,{},4}).bytes,QByteArray("ABCD"));QCOMPARE(reads,2);
        for(int i=2;i<20;++i){QVERIFY(cache.read({path,i*ChunkCache::ChunkBytes,-1,{},8}).error.isEmpty());}
        QCOMPARE(cache.chunkCount(),20);QCOMPARE(cache.retainedBytes(),20*ChunkCache::ChunkBytes);
        QVERIFY(cache.read({path,0,-1,{},8}).error.isEmpty());
        QVERIFY(cache.read({path,20*ChunkCache::ChunkBytes,-1,{},8}).error.isEmpty());
        QVERIFY(cache.contains(0));QVERIFY(!cache.contains(ChunkCache::ChunkBytes));
        QCOMPARE(cache.chunkCount(),20);QCOMPARE(cache.retainedBytes(),20*ChunkCache::ChunkBytes);
        QVERIFY(file.open(QIODevice::WriteOnly));QCOMPARE(file.write("NEW",3),3);file.close();
        QVERIFY(!cache.read({path,0,crossing.size,crossing.modified,3}).error.isEmpty());
        QCOMPARE(cache.chunkCount(),0);
        QCOMPARE(cache.read({path,0,-1,{},3}).bytes,QByteArray("NEW"));
    }
    void staleChunksNotRetainedAfterReset() {
        QSemaphore started,release;
        ChunkCache cache([&](const ReadRequest &request){
            ReadResult r;r.request=request;r.size=ChunkCache::ChunkBytes;
            if(request.length){started.release();release.acquire();r.bytes=QByteArray(ChunkCache::ChunkBytes,'x');}
            return r;
        });
        cache.reset("old");
        auto future=QtConcurrent::run([&]{return cache.read({"old",0,-1,{},1});});
        const bool waiting=started.tryAcquire(1,3000);
        cache.reset("new");release.release();future.waitForFinished();
        QVERIFY(waiting);QCOMPARE(cache.chunkCount(),0);QCOMPARE(future.result().bytes,QByteArray("x"));
    }
    void continuousScrollAndHugeOffsets() {
        QTemporaryDir dir;const auto path=dir.filePath("scroll.bin");QFile file(path);
        QVERIFY(file.open(QIODevice::ReadWrite));
        constexpr qint64 size=64LL*1024*1024*1024;
        QVERIFY(file.resize(size));QVERIFY(file.seek(size-1));QCOMPARE(file.write("Z",1),1);file.close();
        HexView view;view.resize(1100,700);view.show();view.openFile(path);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        auto *scroll=view.findChild<QScrollBar *>("fileScroll");auto *table=view.findChild<QTableWidget *>();
        QCOMPARE(scroll->maximum(),INT_MAX);
        scroll->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);QCOMPARE(view.firstVisibleOffset(),16);
        QWheelEvent wheel(QPointF(80,80),QPointF(80,80),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(table->viewport(),&wheel);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);QCOMPARE(view.firstVisibleOffset(),64);
        scroll->setValue(scroll->maximum());
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        QCOMPARE(view.firstVisibleOffset()+qint64(table->rowCount())*16,size);
        QCOMPARE(table->item(table->rowCount()-1,16)->text(),QString("5A"));
        const auto end=view.firstVisibleOffset();
        scroll->triggerAction(QAbstractSlider::SliderSingleStepSub);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);QCOMPARE(view.firstVisibleOffset(),end-16);
        for(int i=0;i<1000;++i)scroll->setValue(i*1000000);
        scroll->setValue(0);QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),5000);
        QCOMPARE(view.firstVisibleOffset(),0);QVERIFY(view.cachedBytes()<=20*ChunkCache::ChunkBytes);
        QVERIFY(table->rowCount()<100);
        auto *stride=view.findChild<QSpinBox *>("stride");stride->setValue(3);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        scroll->setValue(scroll->maximum());QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        const auto row=table->rowCount()-1;
        const int tail=int((size-1)%3)+1;QCOMPARE(table->item(row,tail)->text(),QString("5A"));
        view.shutdown();QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
    }
    void interpretationsAndCalculator() {
        QCOMPARE(decodeText(QByteArray::fromHex("e697a5e69cac"),"UTF-8"),QString::fromUtf8("日本"));
        QCOMPARE(decodeText(QByteArray::fromHex("e5652c67"),"UTF-16LE"),QString::fromUtf8("日本"));
        QCOMPARE(decodeText(QByteArray::fromHex("93fa967b"),"CP932"),QString::fromUtf8("日本"));
        QCOMPARE(decodeNumber(QByteArray::fromHex("0000803f"),"Float32",false),QString("1"));
        QCOMPARE(decodeNumber(QByteArray::fromHex("3ff0000000000000"),"Float64",true),QString("1"));
        QCOMPARE(decodeNumber(QByteArray::fromHex("ffff"),"Int16",false),QString("-1"));
        QCOMPARE(decodeNumber(QByteArray::fromHex("0102"),"UInt16",false),QString("513"));
        QCOMPARE(decodeNumber(QByteArray::fromHex("0102"),"UInt16",true),QString("258"));
        QVERIFY(decodeNumber(QByteArray(3,'\0'),"Float32",false).startsWith("Faltan"));
        QVERIFY(calculateInteger("0x100","+","16").contains("272"));
        QCOMPARE(calculateInteger("1","÷","0"),QString("División por cero"));
        QCOMPARE(calculateInteger("18446744073709551615","+","1"),QString("Desbordamiento"));
    }
    void selectionGeometry() {
        ByteSelection sequential{14,18,16,false};
        QCOMPARE(sequential.count(100),5);QVERIFY(sequential.contains(16));
        ByteSelection rect{14,34,16,true};
        QCOMPARE(rect.count(100),39);QVERIFY(!rect.contains(16));QVERIFY(rect.contains(18));
        auto ranges=rect.ranges(100,100);
        QCOMPARE(ranges.size(),3);QCOMPARE(ranges[0].offset,2);QCOMPARE(ranges[0].length,13);
        ByteSelection tail{14,34,16,true};QCOMPARE(tail.count(35),27);
        QVERIFY(tail.ranges(35,10).isEmpty());
        ByteSelection reversed{18,14,16,false};QCOMPARE(reversed.count(100),5);
    }
    void strideAndMouseSelection() {
        QTemporaryDir dir;const auto path=dir.filePath("selection.bin");QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));QByteArray data;for(int i=0;i<5000;++i)data.append(char(i%256));QCOMPARE(file.write(data),data.size());file.close();
        HexView view; view.resize(1100,700);view.show();view.openFile(path);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        auto *table=view.findChild<QTableWidget *>();auto *stride=view.findChild<QSpinBox *>("stride");
        stride->setValue(8);QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        QCOMPARE(table->columnCount(),10);QVERIFY(table->rowCount()>3);QVERIFY(table->rowCount()<100);
        auto center=[&](int row,int col){return table->visualItemRect(table->item(row,col)).center();};
        QTest::mousePress(table->viewport(),Qt::LeftButton,Qt::NoModifier,center(0,7));
        QTest::mouseMove(table->viewport(),center(1,3));
        QTest::mouseRelease(table->viewport(),Qt::LeftButton,Qt::NoModifier,center(1,3));
        QCOMPARE(view.selection().anchor,6);QCOMPARE(view.selection().cursor,10);QCOMPARE(view.selection().count(5000),5);
        QTest::mousePress(table->viewport(),Qt::LeftButton,Qt::AltModifier,center(0,2));
        QTest::mouseMove(table->viewport(),center(2,4));
        QTest::mouseRelease(table->viewport(),Qt::LeftButton,Qt::AltModifier,center(2,4));
        QVERIFY(view.selection().rectangular);QCOMPARE(view.selection().count(5000),9);
        view.copySelection();QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        QCOMPARE(QApplication::clipboard()->text(),QString("01 02 03\n09 0A 0B\n11 12 13"));
        auto *address=view.findChild<QLineEdit *>("address");address->setText("4094");QTest::keyClick(address,Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        QCOMPARE(view.selection().anchor,4094);
        auto *type=view.findChild<QComboBox *>("interpretation");type->setCurrentText("UInt32");
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        QVERIFY(view.findChild<DataInspector *>("inspector")->toPlainText().contains("16842750"));
        stride->setValue(3);QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        QCOMPARE(view.selection().anchor,4094);QCOMPARE(table->columnCount(),5);
        view.shutdown();QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
    }
};
QTEST_MAIN(ReaderTests)
#include "reader_tests.moc"
