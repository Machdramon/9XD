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
#include <QCoreApplication>
#include <QDir>
namespace {
bool writeFile(QString path,const QByteArray &bytes){QSaveFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();}
const char *pythonExample=R"PY(import workbench as bw
print("File:", bw.path)
print("Size:", bw.size, "bytes")
print("Selection:", bw.selection)
if bw.size:
    print("First bytes:", bw.read(0, min(32, bw.size)).hex(" "))
mesh = bw.mesh()
print("Mesh:", len(mesh["vertices"]), "vertices")
# Publish a test triangle:
# bw.set_mesh([[-1, 0, 0], [1, 0, 0], [0, 1, 0]], [0, 1, 2])
)PY";
const char *cppExample=R"CPP(#include "workbench.hpp"
void run() {
    std::cout << "File: " << bw::path() << "\n";
    std::cout << "Size: " << bw::size() << " bytes\n";
    if (bw::size() > 0) {
        auto bytes = bw::read(0, 1);
        std::cout << "First byte: " << unsigned(bytes[0]) << "\n";
    }
    auto mesh = bw::mesh();
    std::cout << "Mesh: " << mesh.vertices.size() << " vertices\n";
    // Modify a copy of the mesh and publish it:
    // for (auto &v : mesh.vertices) v.position[1] *= 1.2f;
    // bw::set_mesh(mesh);
}
)CPP";
}
ScriptPanel::ScriptPanel(HexView *view,VulkanPanel *renderer,QWidget *parent):QWidget(parent),view_(view),renderer_(renderer){
    setObjectName("scriptPanel");docs_[0].text=pythonExample;docs_[1].text=cppExample;auto *layout=new QVBoxLayout(this);auto *bar=new QHBoxLayout;layout->addLayout(bar);language_=new QComboBox;language_->setObjectName("scriptLanguage");language_->addItems({"Python · embedded CPython","C++ · compile and run"});bar->addWidget(language_);
    auto *open=new QPushButton("Open…");auto *saveButton=new QPushButton("Save…");run_=new QPushButton("Run");run_->setObjectName("scriptRun");stop_=new QPushButton("Stop");stop_->setObjectName("scriptStop");stop_->setEnabled(false);auto *clear=new QPushButton("Clear Console");for(auto *b:{open,saveButton,run_,stop_,clear})bar->addWidget(b);
    auto *split=new QSplitter(Qt::Vertical);editor_=new QPlainTextEdit;editor_->setObjectName("scriptEditor");editor_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));editor_->setTabStopDistance(editor_->fontMetrics().horizontalAdvance(' ')*4);editor_->setPlainText(docs_[0].text);console_=new QPlainTextEdit;console_->setObjectName("scriptConsole");console_->setReadOnly(true);console_->setFont(editor_->font());console_->setMaximumBlockCount(4000);split->addWidget(editor_);split->addWidget(console_);split->setStretchFactor(0,3);split->setStretchFactor(1,1);layout->addWidget(split,1);
    status_=new QLabel("Each run starts with a new context. API: workbench.read / mesh / set_mesh. Scripts run with your local permissions.");status_->setWordWrap(true);layout->addWidget(status_);
    connect(editor_,&QPlainTextEdit::textChanged,this,[this]{if(!loading_){docs_[current_].text=editor_->toPlainText();docs_[current_].dirty=true;}});
    connect(language_,&QComboBox::currentIndexChanged,this,[this](int index){current_=index;loading_=true;editor_->setPlainText(docs_[index].text);loading_=false;});
    connect(clear,&QPushButton::clicked,console_,&QPlainTextEdit::clear);connect(run_,&QPushButton::clicked,this,[this]{execute();});connect(stop_,&QPushButton::clicked,this,[this]{stopped_=true;process_.kill();});connect(saveButton,&QPushButton::clicked,this,[this]{save(current_);});
    connect(open,&QPushButton::clicked,this,[this]{if(docs_[current_].dirty){auto choice=QMessageBox::question(this,"Modified Script","Save the script before replacing it?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);if(choice==QMessageBox::Cancel||(choice==QMessageBox::Save&&!save(current_)))return;}auto path=QFileDialog::getOpenFileName(this,"Open Script",{},current_==0?"Python (*.py)":"C++ (*.cpp *.cc)");if(path.isEmpty())return;QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>1048576){log("Could not open the script (maximum 1 MiB).");return;}docs_[current_]={QString::fromUtf8(file.readAll()),path,false};loading_=true;editor_->setPlainText(docs_[current_].text);loading_=false;});
    connect(view_,&HexView::documentChanged,this,[this]{++revision_;});
    process_.setProcessChannelMode(QProcess::MergedChannels);connect(&process_,&QProcess::readyReadStandardOutput,this,[this]{output();});connect(&process_,&QProcess::finished,this,[this](int code,QProcess::ExitStatus status){finish(code,status);});connect(&process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart){log("Could not start: "+process_.errorString());run_->setEnabled(true);stop_->setEnabled(false);language_->setEnabled(true);}});
}
ScriptPanel::~ScriptPanel(){disconnect(&process_,nullptr,this,nullptr);process_.kill();process_.waitForFinished(1000);}
void ScriptPanel::log(const QString &text){console_->appendPlainText(text);if(console_->document()->characterCount()>200000){auto cursor=console_->textCursor();cursor.movePosition(QTextCursor::Start);cursor.setPosition(console_->document()->characterCount()-150000,QTextCursor::KeepAnchor);cursor.removeSelectedText();}}
void ScriptPanel::output(){auto bytes=process_.readAllStandardOutput();outputBytes_+=bytes.size();if(outputBytes_>2*1024*1024){if(!stopped_)log("Execution stopped: output exceeds 2 MiB.");stopped_=true;process_.kill();return;}if(!bytes.isEmpty()){auto cursor=console_->textCursor();cursor.movePosition(QTextCursor::End);cursor.insertText(outputDecoder_(bytes));console_->setTextCursor(cursor);if(console_->document()->characterCount()>200000){cursor.movePosition(QTextCursor::Start);cursor.setPosition(console_->document()->characterCount()-150000,QTextCursor::KeepAnchor);cursor.removeSelectedText();}}}
bool ScriptPanel::save(int language){auto &doc=docs_[language];auto path=QFileDialog::getSaveFileName(this,"Save Script",doc.path.isEmpty()?(language==0?"script.py":"script.cpp"):doc.path,language==0?"Python (*.py)":"C++ (*.cpp)");if(path.isEmpty())return false;if(!writeFile(path,doc.text.toUtf8())){log("Could not save the script.");return false;}doc.path=path;doc.dirty=false;return true;}
bool ScriptPanel::confirmClose(){for(int i=0;i<2;++i)if(docs_[i].dirty){auto choice=QMessageBox::question(this,"Unsaved Script",QString("Save the %1 script before closing?").arg(i==0?"Python":"C++"),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);if(choice==QMessageBox::Cancel)return false;if(choice==QMessageBox::Save&&!save(i))return false;if(choice==QMessageBox::Discard)docs_[i].dirty=false;}return true;}
void ScriptPanel::execute(){
    if(process_.state()!=QProcess::NotRunning)return;if(view_->saving()||view_->directEditing()){log("Wait for editing or saving to finish.");return;}auto code=editor_->toPlainText().toUtf8();if(code.size()>1048576||code.contains('\0')){log("Invalid script or larger than 1 MiB.");return;}
    directory_=std::make_unique<QTemporaryDir>();if(!directory_->isValid()){log("Could not create the temporary workspace.");return;}auto source=view_->meshSource();auto selection=view_->selection();runMesh_=renderer_->meshSnapshot();runMeshId_=renderer_->scene()->active();QJsonArray patches;for(const auto &p:view_->pendingPatches())patches.append(QJsonObject{{"offset",QString::number(p.offset)},{"bytes",QString::fromLatin1(p.bytes.toBase64())}});
    QJsonObject context{{"path",source.path},{"size",QString::number(qMax(qint64(0),source.expectedSize))},{"modified",source.expectedModified.toString(Qt::ISODateWithMs)},{"patches",patches},{"selection",QJsonObject{{"start",QString::number(qMin(selection.anchor,selection.cursor))},{"end",QString::number(qMax(selection.anchor,selection.cursor))},{"rectangular",selection.rectangular},{"stride",selection.stride}}},{"mesh",scriptMeshJson(runMesh_)}};
    if(source.memory){QJsonArray blocks;for(const auto &block:source.memory->blocks)blocks.append(QString::fromLatin1(block.toBase64()));context["memoryBlocks"]=blocks;}
    auto json=QJsonDocument(context).toJson(QJsonDocument::Compact);if(json.size()>64*1024*1024||!writeFile(directory_->filePath("context.json"),json)||!writeFile(directory_->filePath(current_==0?"script.py":"script.cpp"),code)){log("Could not prepare the context (maximum 64 MiB).");return;}
    runRevision_=revision_;outputDecoder_.resetState();stopped_=false;outputBytes_=0;run_->setEnabled(false);stop_->setEnabled(true);language_->setEnabled(false);process_.setWorkingDirectory(directory_->path());process_.setStandardInputFile(QProcess::nullDevice());log(current_==0?"\n▶ Python":"\n▶ Compiling C++…");compiling_=current_==1;
    if(!compiling_){launchRunner();return;}
    QFile header(":/scripting/workbench_script_api.h");const bool headerOpen=header.open(QIODevice::ReadOnly);auto wrapper=QByteArray("#include \"workbench.hpp\"\nvoid run();\n#ifdef _WIN32\n#define EXPORT __declspec(dllexport)\n#else\n#define EXPORT __attribute__((visibility(\"default\")))\n#endif\nextern \"C\" EXPORT void bw_run(const BWApi *api){bw::api=api;run();}\n");
    if(!headerOpen||!writeFile(directory_->filePath("workbench.hpp"),header.readAll())||!writeFile(directory_->filePath("entry.cpp"),wrapper)){log("Could not prepare the C++ API.");run_->setEnabled(true);language_->setEnabled(true);stop_->setEnabled(false);return;}
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
    const auto bundledRunner=QDir(QCoreApplication::applicationDirPath()).filePath("script_runner"
#ifdef Q_OS_WIN
        ".exe"
#endif
    );
    process_.start(QFileInfo::exists(bundledRunner)?bundledRunner:QString(WORKBENCH_SCRIPT_RUNNER),{current_==0?"python":"cpp",directory_->filePath("context.json"),directory_->filePath(script),directory_->filePath("result.json")});}
void ScriptPanel::finish(int code,QProcess::ExitStatus exitStatus){
    output();if(compiling_&&!stopped_&&exitStatus==QProcess::NormalExit&&code==0){compiling_=false;log("▶ Running C++…");launchRunner();return;}
    run_->setEnabled(true);stop_->setEnabled(false);language_->setEnabled(true);
    if(stopped_){log("\n■ Execution stopped; results discarded.");return;}if(exitStatus!=QProcess::NormalExit||code!=0){log(QString("\n%1 error · code %2").arg(compiling_?"compilation":"execution").arg(code));return;}
    QFile outputFile(directory_->filePath("result.json"));if(!outputFile.open(QIODevice::ReadOnly)||outputFile.size()>64*1024*1024){log("No valid script result.");return;}auto result=QJsonDocument::fromJson(outputFile.readAll());if(!result.isObject()||!result["ok"].toBool()){log("Invalid script response.");return;}
    if(result.object().contains("mesh")){
        auto source=view_->meshSource();QFileInfo info(source.path);bool fileChanged=view_->hasFile()&&(info.size()!=source.expectedSize||info.lastModified()!=source.expectedModified);
        if(runRevision_!=revision_||runMeshId_!=renderer_->scene()->active()||renderer_->meshSnapshot()!=runMesh_||fileChanged){log("Mesh discarded: the file or mesh changed during execution.");return;}QString error;auto mesh=scriptMeshFromJson(result["mesh"].toObject(),error);if(!mesh){log("Mesh rejected: "+error);return;}renderer_->setMesh(mesh);log(QString("Mesh published: %1 vertices, %2 triangles.").arg(mesh->vertices.size()).arg(mesh->triangles.size()/3));
    }
    log("✓ Execution completed.");
}
