#include "script_panel.h"
#include "hex_view.h"
#include "vulkan_panel.h"
#include "local_config.h"
#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QSaveFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFontDatabase>
#include <QTextCursor>
#include <QProcessEnvironment>
namespace {
bool writeFile(QString path,const QByteArray &bytes){QSaveFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();}
const char *pythonExample=R"PY(import workbench as bw
print("Archivo:", bw.path)
print("Tamaño:", bw.size, "bytes")
print("Selección:", bw.selection)
if bw.size:
    print("Primeros bytes:", bw.read(0, min(32, bw.size)).hex(" "))
mesh = bw.mesh()
print("Malla:", len(mesh["vertices"]), "vértices")
# Publicar un triángulo de prueba:
# bw.set_mesh([[-1, 0, 0], [1, 0, 0], [0, 1, 0]], [0, 1, 2])
)PY";
const char *cppExample=R"CPP(#include "workbench.hpp"
void run() {
    std::cout << "Archivo: " << bw::path() << "\n";
    std::cout << "Tamaño: " << bw::size() << " bytes\n";
    if (bw::size() > 0) {
        auto bytes = bw::read(0, 1);
        std::cout << "Primer byte: " << unsigned(bytes[0]) << "\n";
    }
    auto mesh = bw::mesh();
    std::cout << "Malla: " << mesh.vertices.size() << " vértices\n";
    // Modificar una copia de la malla y publicarla:
    // for (auto &v : mesh.vertices) v.position[1] *= 1.2f;
    // bw::set_mesh(mesh);
}
)CPP";
}
ScriptPanel::ScriptPanel(HexView *view,VulkanPanel *renderer,QWidget *parent):QWidget(parent),view_(view),renderer_(renderer){
    setObjectName("scriptPanel");docs_[0].text=pythonExample;docs_[1].text=cppExample;auto *layout=new QVBoxLayout(this);auto *bar=new QHBoxLayout;layout->addLayout(bar);language_=new QComboBox;language_->setObjectName("scriptLanguage");language_->addItems({"Python · CPython integrado","C++ · compilar y ejecutar"});bar->addWidget(language_);
    auto *open=new QPushButton("Abrir…");auto *saveButton=new QPushButton("Guardar…");run_=new QPushButton("Ejecutar");run_->setObjectName("scriptRun");stop_=new QPushButton("Detener");stop_->setObjectName("scriptStop");stop_->setEnabled(false);auto *clear=new QPushButton("Limpiar consola");for(auto *b:{open,saveButton,run_,stop_,clear})bar->addWidget(b);
    auto *split=new QSplitter(Qt::Vertical);editor_=new QPlainTextEdit;editor_->setObjectName("scriptEditor");editor_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));editor_->setTabStopDistance(editor_->fontMetrics().horizontalAdvance(' ')*4);editor_->setPlainText(docs_[0].text);console_=new QPlainTextEdit;console_->setObjectName("scriptConsole");console_->setReadOnly(true);console_->setFont(editor_->font());console_->setMaximumBlockCount(4000);split->addWidget(editor_);split->addWidget(console_);split->setStretchFactor(0,3);split->setStretchFactor(1,1);layout->addWidget(split,1);
    status_=new QLabel("Cada ejecución empieza en un contexto nuevo. API: workbench.read / mesh / set_mesh. Los scripts se ejecutan con tus permisos locales.");status_->setWordWrap(true);layout->addWidget(status_);
    connect(editor_,&QPlainTextEdit::textChanged,this,[this]{if(!loading_){docs_[current_].text=editor_->toPlainText();docs_[current_].dirty=true;}});
    connect(language_,&QComboBox::currentIndexChanged,this,[this](int index){current_=index;loading_=true;editor_->setPlainText(docs_[index].text);loading_=false;});
    connect(clear,&QPushButton::clicked,console_,&QPlainTextEdit::clear);connect(run_,&QPushButton::clicked,this,[this]{execute();});connect(stop_,&QPushButton::clicked,this,[this]{stopped_=true;process_.kill();});connect(saveButton,&QPushButton::clicked,this,[this]{save(current_);});
    connect(open,&QPushButton::clicked,this,[this]{if(docs_[current_].dirty){auto choice=QMessageBox::question(this,"Script modificado","¿Guardar el script antes de sustituirlo?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);if(choice==QMessageBox::Cancel||(choice==QMessageBox::Save&&!save(current_)))return;}auto path=QFileDialog::getOpenFileName(this,"Abrir script",{},current_==0?"Python (*.py)":"C++ (*.cpp *.cc)");if(path.isEmpty())return;QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>1048576){log("No se pudo abrir el script (máximo 1 MiB).");return;}docs_[current_]={QString::fromUtf8(file.readAll()),path,false};loading_=true;editor_->setPlainText(docs_[current_].text);loading_=false;});
    connect(view_,&HexView::documentChanged,this,[this]{++revision_;});
    process_.setProcessChannelMode(QProcess::MergedChannels);connect(&process_,&QProcess::readyReadStandardOutput,this,[this]{output();});connect(&process_,&QProcess::finished,this,[this](int code,QProcess::ExitStatus status){finish(code,status);});connect(&process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart){log("No se pudo iniciar: "+process_.errorString());run_->setEnabled(true);stop_->setEnabled(false);language_->setEnabled(true);}});
}
ScriptPanel::~ScriptPanel(){disconnect(&process_,nullptr,this,nullptr);process_.kill();process_.waitForFinished(1000);}
void ScriptPanel::log(const QString &text){console_->appendPlainText(text);if(console_->document()->characterCount()>200000){auto cursor=console_->textCursor();cursor.movePosition(QTextCursor::Start);cursor.setPosition(console_->document()->characterCount()-150000,QTextCursor::KeepAnchor);cursor.removeSelectedText();}}
void ScriptPanel::output(){auto bytes=process_.readAllStandardOutput();outputBytes_+=bytes.size();if(outputBytes_>2*1024*1024){if(!stopped_)log("Ejecución detenida: salida superior a 2 MiB.");stopped_=true;process_.kill();return;}if(!bytes.isEmpty()){auto cursor=console_->textCursor();cursor.movePosition(QTextCursor::End);cursor.insertText(outputDecoder_(bytes));console_->setTextCursor(cursor);if(console_->document()->characterCount()>200000){cursor.movePosition(QTextCursor::Start);cursor.setPosition(console_->document()->characterCount()-150000,QTextCursor::KeepAnchor);cursor.removeSelectedText();}}}
bool ScriptPanel::save(int language){auto &doc=docs_[language];auto path=QFileDialog::getSaveFileName(this,"Guardar script",doc.path.isEmpty()?(language==0?"script.py":"script.cpp"):doc.path,language==0?"Python (*.py)":"C++ (*.cpp)");if(path.isEmpty())return false;if(!writeFile(path,doc.text.toUtf8())){log("No se pudo guardar el script.");return false;}doc.path=path;doc.dirty=false;return true;}
bool ScriptPanel::confirmClose(){for(int i=0;i<2;++i)if(docs_[i].dirty){auto choice=QMessageBox::question(this,"Script sin guardar",QString("¿Guardar el script %1 antes de cerrar?").arg(i==0?"Python":"C++"),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);if(choice==QMessageBox::Cancel)return false;if(choice==QMessageBox::Save&&!save(i))return false;if(choice==QMessageBox::Discard)docs_[i].dirty=false;}return true;}
void ScriptPanel::execute(){
    if(process_.state()!=QProcess::NotRunning)return;if(view_->saving()||view_->directEditing()){log("Espera a que termine la edición o el guardado.");return;}auto code=editor_->toPlainText().toUtf8();if(code.size()>1048576||code.contains('\0')){log("Script no válido o superior a 1 MiB.");return;}
    directory_=std::make_unique<QTemporaryDir>();if(!directory_->isValid()){log("No se pudo crear el espacio temporal.");return;}auto source=view_->meshSource();auto selection=view_->selection();runMesh_=renderer_->meshSnapshot();runMeshId_=renderer_->scene()->active();QJsonArray patches;for(const auto &p:view_->pendingPatches())patches.append(QJsonObject{{"offset",QString::number(p.offset)},{"bytes",QString::fromLatin1(p.bytes.toBase64())}});
    QJsonObject context{{"path",source.path},{"size",QString::number(qMax(qint64(0),source.expectedSize))},{"modified",source.expectedModified.toString(Qt::ISODateWithMs)},{"patches",patches},{"selection",QJsonObject{{"start",QString::number(qMin(selection.anchor,selection.cursor))},{"end",QString::number(qMax(selection.anchor,selection.cursor))},{"rectangular",selection.rectangular},{"stride",selection.stride}}},{"mesh",scriptMeshJson(runMesh_)}};
    auto json=QJsonDocument(context).toJson(QJsonDocument::Compact);if(json.size()>64*1024*1024||!writeFile(directory_->filePath("context.json"),json)||!writeFile(directory_->filePath(current_==0?"script.py":"script.cpp"),code)){log("No se pudo preparar el contexto (máximo 64 MiB).");return;}
    runRevision_=revision_;outputDecoder_.resetState();stopped_=false;outputBytes_=0;run_->setEnabled(false);stop_->setEnabled(true);language_->setEnabled(false);process_.setWorkingDirectory(directory_->path());process_.setStandardInputFile(QProcess::nullDevice());log(current_==0?"\n▶ Python":"\n▶ Compilando C++…");compiling_=current_==1;
    if(!compiling_){launchRunner();return;}
    QFile header(":/scripting/workbench_script_api.h");const bool headerOpen=header.open(QIODevice::ReadOnly);auto wrapper=QByteArray("#include \"workbench.hpp\"\nvoid run();\n#ifdef _WIN32\n#define EXPORT __declspec(dllexport)\n#else\n#define EXPORT __attribute__((visibility(\"default\")))\n#endif\nextern \"C\" EXPORT void bw_run(const BWApi *api){bw::api=api;run();}\n");
    if(!headerOpen||!writeFile(directory_->filePath("workbench.hpp"),header.readAll())||!writeFile(directory_->filePath("entry.cpp"),wrapper)){log("No se pudo preparar la API C++.");run_->setEnabled(true);language_->setEnabled(true);stop_->setEnabled(false);return;}
    QStringList args;
#ifdef Q_OS_WIN
    if(QString(WORKBENCH_CXX_ID)=="MSVC")args={"/nologo","/std:c++latest","/utf-8","/EHsc","/LD","script.cpp","entry.cpp","/Fe:script.dll"};else args={"-std=c++23","-shared","script.cpp","entry.cpp","-o","script.dll"};
#elif defined(Q_OS_MACOS)
    args={"-std=c++23","-dynamiclib","-fPIC","script.cpp","entry.cpp","-o","script.dylib"};
#else
    args={"-std=c++23","-shared","-fPIC","script.cpp","entry.cpp","-o","script.so"};
#endif
    process_.start(WORKBENCH_CXX_COMPILER,args);
}
void ScriptPanel::launchRunner(){QString script=current_==0?"script.py":
#ifdef Q_OS_WIN
    "script.dll";
#elif defined(Q_OS_MACOS)
    "script.dylib";
#else
    "script.so";
#endif
    process_.start(WORKBENCH_SCRIPT_RUNNER,{current_==0?"python":"cpp",directory_->filePath("context.json"),directory_->filePath(script),directory_->filePath("result.json")});}
void ScriptPanel::finish(int code,QProcess::ExitStatus exitStatus){
    output();if(compiling_&&!stopped_&&exitStatus==QProcess::NormalExit&&code==0){compiling_=false;log("▶ Ejecutando C++…");launchRunner();return;}
    run_->setEnabled(true);stop_->setEnabled(false);language_->setEnabled(true);
    if(stopped_){log("\n■ Ejecución detenida; resultados descartados.");return;}if(exitStatus!=QProcess::NormalExit||code!=0){log(QString("\nError de %1 · código %2").arg(compiling_?"compilación":"ejecución").arg(code));return;}
    QFile outputFile(directory_->filePath("result.json"));if(!outputFile.open(QIODevice::ReadOnly)||outputFile.size()>64*1024*1024){log("No hay un resultado válido del script.");return;}auto result=QJsonDocument::fromJson(outputFile.readAll());if(!result.isObject()||!result["ok"].toBool()){log("Respuesta del script no válida.");return;}
    if(result.object().contains("mesh")){
        auto source=view_->meshSource();QFileInfo info(source.path);bool fileChanged=view_->hasFile()&&(info.size()!=source.expectedSize||info.lastModified()!=source.expectedModified);
        if(runRevision_!=revision_||runMeshId_!=renderer_->scene()->active()||renderer_->meshSnapshot()!=runMesh_||fileChanged){log("Malla descartada: cambió el archivo o la malla durante la ejecución.");return;}QString error;auto mesh=scriptMeshFromJson(result["mesh"].toObject(),error);if(!mesh){log("Malla rechazada: "+error);return;}renderer_->setMesh(mesh);log(QString("Malla publicada: %1 vértices, %2 triángulos.").arg(mesh->vertices.size()).arg(mesh->triangles.size()/3));
    }
    log("✓ Ejecución completada.");
}
