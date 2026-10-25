#pragma once
#include <QHeaderView>
#include "file_dialogs.h"
#include "script_libraries.h"
#include "local_config.h"
#include <QWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QTextBrowser>
#include <QFormLayout>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QProcess>
#include <QTimer>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QCryptographicHash>
#include <QStringDecoder>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
class ScriptLibraryManager final:public QWidget {
public:
    explicit ScriptLibraryManager(QWidget *parent=nullptr):QWidget(parent){
        setObjectName("scriptLibraryManager");auto *layout=new QVBoxLayout(this);auto *hint=new QLabel("Default Modules come with the application or runtime. My Modules contains only the modules you add.");hint->setWordWrap(true);layout->addWidget(hint);
        auto *buttons=new QHBoxLayout;auto *create=new QPushButton("Add Module..."),*refresh=new QPushButton("Refresh");create->setObjectName("libraryAdd");buttons->addWidget(create);buttons->addStretch();buttons->addWidget(refresh);layout->addLayout(buttons);
        auto *split=new QSplitter;layout->addWidget(split,1);tree_=new QTreeWidget;tree_->setObjectName("scriptLibraries");tree_->setHeaderLabels({"Library","Type / Version","Status"});tree_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);split->addWidget(tree_);auto *right=new QWidget;auto *form=new QVBoxLayout(right);split->addWidget(right);split->setSizes({560,360});
        language_=new QComboBox;language_->setObjectName("libraryLanguage");language_->addItems({"Python","C++"});name_=new QLineEdit("my_library");name_->setObjectName("libraryName");form->addWidget(language_);form->addWidget(name_);editor_=new QPlainTextEdit;editor_->setObjectName("libraryCode");tabs_=new QTabWidget;documentation_=new QTextBrowser;documentation_->setObjectName("libraryDocumentation");tabs_->addTab(documentation_,"Documentation");tabs_->addTab(editor_,"Code");form->addWidget(tabs_,1);
        auto *actions=new QHBoxLayout;save_=new QPushButton("Save Changes");save_->setObjectName("librarySave");auto *copy=new QPushButton("Copy Code");toggle_=new QPushButton("Enable / Disable");toggle_->setObjectName("libraryToggle");for(auto *b:{save_,copy,toggle_})actions->addWidget(b);form->addLayout(actions);
        status_=new QLabel;status_->setObjectName("libraryStatus");status_->setWordWrap(true);layout->addWidget(status_);
        connect(editor_,&QPlainTextEdit::textChanged,this,[this]{if(!loading_)dirty_=true;});
        connect(create,&QPushButton::clicked,this,[this]{addModule();});connect(save_,&QPushButton::clicked,this,[this]{saveCurrent();});
        connect(copy,&QPushButton::clicked,this,[this]{QApplication::clipboard()->setText(editor_->toPlainText());});
        connect(refresh,&QPushButton::clicked,this,[this]{refreshLibraries();scanRuntime();});
        connect(tree_,&QTreeWidget::currentItemChanged,this,[this](QTreeWidgetItem *item,QTreeWidgetItem *previous){if(!item||loading_)return;selecting_=true;if(!confirmClose()){selecting_=false;QSignalBlocker block(tree_);tree_->setCurrentItem(previous);return;}selecting_=false;select(item);});
        connect(toggle_,&QPushButton::clicked,this,[this]{if(relative_.isEmpty()||!confirmClose())return;auto root=relative_.section('/',0,0);auto from=scriptLibraryFile(currentLanguage_,root);auto to=scriptLibraryFile(currentLanguage_,root.endsWith(".disabled")?root.chopped(9):root+".disabled");if(from.isEmpty()||to.isEmpty()||QFileInfo::exists(to)||!QDir().rename(from,to)){status_->setText("Could not change library state.");return;}relative_.clear();editor_->clear();dirty_=false;refreshLibraries();status_->setText("Library state changed. Run scripts again to use the change.");});
        connect(&inventory_,&QProcess::finished,this,[this](int exitCode,QProcess::ExitStatus exit){inventoryBytes_+=inventory_.readAllStandardOutput();auto bytes=inventoryBytes_;inventoryBytes_.clear();if(exitCode||exit!=QProcess::NormalExit||bytes.size()>2*1024*1024){status_->setText("Could not inspect the Python runtime.");return;}auto doc=QJsonDocument::fromJson(bytes);if(!doc.isObject()){status_->setText("Invalid Python inventory response.");return;}runtime_=doc.object();refreshLibraries();status_->setText(QString("Python %1 · %2 standard modules · %3 installed distributions. Custom libraries: %4").arg(runtime_["version"].toString()).arg(runtime_["stdlib"].toArray().size()).arg(runtime_["packages"].toArray().size()).arg(scriptLibraryRoot()));});
        connect(&inventory_,&QProcess::readyReadStandardOutput,this,[this]{inventoryBytes_+=inventory_.readAllStandardOutput();if(inventoryBytes_.size()>2*1024*1024)inventory_.kill();});
        connect(&inventory_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){status_->setText("Python inventory unavailable. Custom library editing remains available.");});
        newLibrary();refreshLibraries();documentation_->setPlainText("Select a module to see its origin, usage and source.\n\nTo add your own module, choose Add Module..., select Python or C++, and create, paste or import your source.");tabs_->setCurrentIndex(0);
    }
    ~ScriptLibraryManager()override{inventory_.kill();inventory_.waitForFinished(1000);}
    void refreshRuntime(){refreshLibraries();scanRuntime();}
    bool confirmClose(){if(!dirty_)return true;auto result=QMessageBox::question(this,"Unsaved Library","Save changes to this library?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);if(result==QMessageBox::Cancel)return false;if(result==QMessageBox::Save)return saveCurrent();dirty_=false;return true;}
    bool newLibrary(){if(!confirmClose())return false;{QSignalBlocker block(tree_);tree_->setCurrentItem(nullptr);}relative_.clear();openedHash_.clear();currentLanguage_=language_->currentIndex()==0?"python":"cpp";name_->setReadOnly(false);name_->setText("my_library");language_->setEnabled(true);editor_->setReadOnly(false);loading_=true;editor_->clear();loading_=false;dirty_=false;save_->setEnabled(true);toggle_->setEnabled(false);documentation_->setPlainText("New user module. Choose a name using letters, digits and underscores. Python modules use .py; C++ modules use .hpp. Save it, then use the import or include statement in your scripts.");tabs_->setCurrentIndex(1);return true;}
    bool saveCurrent(){
        QString lang=relative_.isEmpty()?(language_->currentIndex()==0?"python":"cpp"):currentLanguage_;QString relative=relative_;if(relative.isEmpty()){if(!scriptLibraryName(name_->text())){status_->setText("Use an ASCII module name: letters, digits and underscore; start with a letter or underscore. workbench is reserved.");return false;}relative=name_->text()+(lang=="python"?".py":".hpp");}
        if(!relative_.isEmpty()&&!openedHash_.isEmpty()){QFile existing(scriptLibraryFile(lang,relative_));if(!existing.open(QIODevice::ReadOnly)||existing.size()>1048576||QCryptographicHash::hash(existing.readAll(),QCryptographicHash::Sha256)!=openedHash_){status_->setText("The library changed on disk. Reopen it before saving.");return false;}}
        QString error;if(!saveScriptLibrary(lang,relative,editor_->toPlainText().toUtf8(),error,!relative_.isEmpty())){status_->setText(error);return false;}relative_=relative;currentLanguage_=lang;openedHash_=QCryptographicHash::hash(editor_->toPlainText().toUtf8(),QCryptographicHash::Sha256);dirty_=false;name_->setReadOnly(true);language_->setEnabled(false);if(!selecting_)refreshLibraries();toggle_->setEnabled(true);status_->setText(lang=="python"?"Saved. Use import "+relative.section('/',0,0).section('.',0,0)+" in Python scripts.":"Saved. Use #include \""+relative+"\" in C++ scripts.");return true;
    }
    bool importSource(const QString &path){QFileInfo info(path);if(info.isSymLink()||!QStringList{"py","h","hpp","hh","inl"}.contains(info.suffix().toLower())||!scriptLibraryName(info.completeBaseName())){status_->setText("Choose a Python module or C++ header with a valid name.");return false;}QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>1048576){status_->setText("Cannot read source (maximum 1 MiB).");return false;}QString error;bool ok=saveScriptLibrary(info.suffix().toLower()=="py"?"python":"cpp",info.fileName(),file.readAll(),error);refreshLibraries();status_->setText(ok?"Library imported.":error);return ok;}
    void refreshLibraries(){
        QSignalBlocker block(tree_);tree_->clear();auto *defaults=new QTreeWidgetItem(tree_,{"Default Modules",{},"Read only"});auto *own=new QTreeWidgetItem(tree_,{"My Modules",{},"Added by you"});defaults->setExpanded(true);own->setExpanded(true);auto *api=new QTreeWidgetItem(defaults,{"Workbench API","Python workbench / C++ workbench.hpp","Built in"});api->setData(0,Qt::UserRole+2,"Workbench scripting API: read bytes, inspect selection and get/set mesh data.");
        auto *standard=new QTreeWidgetItem(defaults,{"Python standard library",runtime_["version"].toString(),"Runtime"});for(auto v:runtime_["stdlib"].toArray())new QTreeWidgetItem(standard,{v.toString(),"Standard module","Read only"});
        auto *packages=new QTreeWidgetItem(defaults,{"Python installed distributions",{},"Runtime"});for(auto v:runtime_["packages"].toArray()){auto p=v.toObject();new QTreeWidgetItem(packages,{p["name"].toString(),p["version"].toString(),"Read only"});}
        new QTreeWidgetItem(defaults,{"C++ standard library",QFileInfo::exists(WORKBENCH_CXX_COMPILER)?"Compiler toolchain available":"Requires local compiler","Read only"});
        for(auto lang:{QString("python"),QString("cpp")})for(int bundled=0;bundled<2;++bundled){auto root=scriptLibraryPaths(lang)[bundled];auto *group=new QTreeWidgetItem(bundled?defaults:own,{lang=="python"?"Python modules":"C++ headers",bundled?"Included":"User",{}});QDirIterator it(root,QDir::Files|QDir::NoSymLinks,QDirIterator::Subdirectories);int count=0;while(it.hasNext()&&count++<4096){auto path=it.next();auto rel=QDir(root).relativeFilePath(path);if(rel.contains("__pycache__/")||rel.endsWith(".pyc"))continue;bool disabled=rel.endsWith(".disabled")||rel.contains(".disabled/");auto *item=new QTreeWidgetItem(group,{rel,lang,disabled?"Disabled":bundled?"Bundled":"Enabled"});item->setData(0,Qt::UserRole,rel);item->setData(0,Qt::UserRole+1,lang);item->setData(0,Qt::UserRole+3,bundled);item->setToolTip(0,path);}group->setExpanded(true);}
    }
protected:
    void showEvent(QShowEvent *event)override{QWidget::showEvent(event);refreshLibraries();if(runtime_.isEmpty())scanRuntime();}
private:
    void select(QTreeWidgetItem *item){auto rel=item->data(0,Qt::UserRole).toString();auto lang=item->data(0,Qt::UserRole+1).toString();bool bundled=item->data(0,Qt::UserRole+3).toBool();relative_=bundled?QString():rel;currentLanguage_=lang;loading_=true;bool editable=false;
        if(!rel.isEmpty()){auto path=bundled?QDir(scriptLibraryPaths(lang)[1]).filePath(rel):scriptLibraryFile(lang,rel);QFile file(path);bool readable=file.open(QIODevice::ReadOnly)&&file.size()<=1048576;auto bytes=readable?file.readAll():QByteArray();openedHash_=readable?QCryptographicHash::hash(bytes,QCryptographicHash::Sha256):QByteArray();QStringDecoder decoder(QStringDecoder::Utf8);auto text=decoder(bytes);editable=readable&&!decoder.hasError()&&!bundled;editor_->setPlainText(readable?text:QString("Source unavailable or exceeds 1 MiB."));name_->setText(rel);language_->setCurrentIndex(lang=="python"?0:1);}
        else{editor_->setPlainText(item->data(0,Qt::UserRole+2).toString());name_->setText(item->text(0));}
        QString usage;
        if(!rel.isEmpty())usage=lang=="python"?"import "+rel.section('/',0,0).section('.',0,0):"#include \""+rel+"\"";
        QString details=item->data(0,Qt::UserRole+2).toString();
        if(details.isEmpty()&&rel.isEmpty())details="Available through the Python runtime or C++ toolchain. Select an individual source module to inspect its code. For runtime modules, use Python help(module) in the scripting console to inspect its API.";
        documentation_->setPlainText(item->text(0)+"\n\nOrigin: "+(!rel.isEmpty()&&!bundled?"My Modules — added by you":"Default Modules — included / runtime")+"\nStatus: "+item->text(2)+"\n\n"+(usage.isEmpty()?QString():"Use in a script:\n"+usage+"\n\n")+details+(!rel.isEmpty()?"\n\nThe Code tab shows the module source and its inline documentation. "+QString(editable?"You can edit your module and save changes here.":"Included modules are read-only."):QString()));
        tabs_->setCurrentIndex(0);toggle_->setText(rel.contains(".disabled")?"Enable Module":"Disable Module");
        editor_->setReadOnly(!editable);name_->setReadOnly(true);language_->setEnabled(false);save_->setEnabled(editable);toggle_->setEnabled(!bundled&&!rel.isEmpty());loading_=false;dirty_=false;
    }
    void addModule(){
        if(!confirmClose())return;
        QDialog dialog(this);dialog.setWindowTitle("Add Module");dialog.resize(560,420);auto *layout=new QVBoxLayout(&dialog);
        auto *intro=new QLabel("Add a reusable module to My Modules. It will be available to all your scripts.");intro->setWordWrap(true);layout->addWidget(intro);
        auto *form=new QFormLayout;auto *language=new QComboBox;language->addItems({"Python","C++"});auto *method=new QComboBox;method->addItems({"Create module / paste code","Import source file","Import package / header folder"});auto *name=new QLineEdit("my_module");auto *code=new QPlainTextEdit;code->setPlaceholderText("Write or paste module code here...");form->addRow("Language",language);form->addRow("Source",method);form->addRow("Module name",name);layout->addLayout(form);layout->addWidget(code,1);
        auto *note=new QLabel("Python: name.py, used with import name. C++: name.hpp, used with #include \"name.hpp\". Packages keep their folder name.");note->setWordWrap(true);layout->addWidget(note);auto *error=new QLabel;error->setWordWrap(true);layout->addWidget(error);
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons->button(QDialogButtonBox::Ok)->setText("Add Module");layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        connect(method,&QComboBox::currentIndexChanged,&dialog,[=](int index){name->setEnabled(index==0);code->setVisible(index==0);});
        connect(buttons,&QDialogButtonBox::accepted,&dialog,[&,this]{
            QString lang=language->currentIndex()==0?"python":"cpp",message;bool ok=false;
            if(method->currentIndex()==0){if(!scriptLibraryName(name->text())){error->setText("Use a module name with letters, digits and underscores, starting with a letter or underscore.");return;}ok=saveScriptLibrary(lang,name->text()+(lang=="python"?".py":".hpp"),code->toPlainText().toUtf8(),message);}
            else if(method->currentIndex()==1){auto path=WorkbenchFiles::getOpenFileName(&dialog,"Import module",{},lang=="python"?"Python (*.py)":"C++ headers (*.h *.hpp *.hh *.inl)");if(path.isEmpty())return;ok=importSource(path);if(!ok)message=status_->text();}
            else{auto path=WorkbenchFiles::getExistingDirectory(&dialog,"Import package / header folder");if(path.isEmpty())return;QDir().mkpath(scriptLibraryRoot());ok=importScriptLibraryFolder(lang,path,message);}
            if(!ok){error->setText(message);return;}refreshLibraries();status_->setText("Module added to My Modules. Select it to view usage and code.");dialog.accept();
        });dialog.exec();
    }
    void scanRuntime(){if(inventory_.state()!=QProcess::NotRunning)return;status_->setText("Inspecting Python runtime...");auto bundled=QDir(QCoreApplication::applicationDirPath()).filePath("script_runner"
#ifdef Q_OS_WIN
    ".exe"
#endif
    );inventoryBytes_.clear();auto revision=++inventoryRevision_;inventory_.start(QFileInfo::exists(bundled)?bundled:QString(WORKBENCH_SCRIPT_RUNNER),{"--library-inventory"});QTimer::singleShot(10000,this,[this,revision]{if(inventoryRevision_==revision&&inventory_.state()!=QProcess::NotRunning)inventory_.kill();});}
    QTabWidget *tabs_;QTextBrowser *documentation_;QTreeWidget *tree_;QPlainTextEdit *editor_;QLineEdit *name_;QComboBox *language_;QPushButton *save_,*toggle_;QLabel *status_;QProcess inventory_;QByteArray openedHash_,inventoryBytes_;quint64 inventoryRevision_=0;QJsonObject runtime_;QString relative_,currentLanguage_;bool loading_=false,dirty_=false,selecting_=false;
};
