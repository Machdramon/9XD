#pragma once
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QRegularExpression>
#include <QDirIterator>
#include <QTemporaryDir>
inline QString scriptLibraryRoot(){auto custom=qApp->property("scriptLibraryRoot").toString();return custom.isEmpty()?QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath("BinaryWorkbench/libraries"):custom;}
inline QStringList scriptLibraryPaths(const QString &language){return {QDir(scriptLibraryRoot()).filePath(language),QDir(QCoreApplication::applicationDirPath()).filePath("libraries/"+language)};}
inline bool scriptLibraryName(const QString &name){return QRegularExpression("^[A-Za-z_][A-Za-z0-9_]{0,63}$").match(name).hasMatch()&&name!="workbench"&&!QRegularExpression("^(con|prn|aux|nul|com[1-9]|lpt[1-9])$",QRegularExpression::CaseInsensitiveOption).match(name).hasMatch();}
inline bool scriptLibraryRelative(const QString &relative){if(QDir::isAbsolutePath(relative)||relative.contains('\\'))return false;const auto parts=relative.split('/');if(parts.size()>16)return false;for(const auto &part:parts)if(part.isEmpty()||part=="."||part==".."||part.contains(':'))return false;return true;}
inline QString scriptLibraryFile(const QString &language,const QString &relative){
    if((language!="python"&&language!="cpp")||!scriptLibraryRelative(relative))return {};
    auto root=QDir(scriptLibraryRoot()).filePath(language);QString path=root;for(const auto &part:relative.split('/')){if(QFileInfo(path).isSymLink())return {};path=QDir(path).filePath(part);}if(QFileInfo(path).isSymLink()||QFileInfo(root).isSymLink())return {};return path;
}
inline bool saveScriptLibrary(const QString &language,const QString &relative,const QByteArray &code,QString &error,bool overwrite=false){
    auto path=scriptLibraryFile(language,relative);if(path.isEmpty()||code.size()>1048576||code.contains('\0')){error="Invalid library path or source (maximum 1 MiB).";return false;}
    if(!overwrite&&(QFileInfo::exists(path)||QFileInfo::exists(path+".disabled"))){error="A library with that name already exists.";return false;}if(!QDir().mkpath(QFileInfo(path).absolutePath())){error="Cannot create library folder.";return false;}
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(code)!=code.size()||!file.commit()){error="Could not save library.";return false;}return true;
}
inline bool importScriptLibraryFolder(const QString &language,const QString &source,QString &error){
    QFileInfo origin(source);auto name=origin.fileName();if(!origin.isDir()||origin.isSymLink()||!scriptLibraryName(name)){error="Choose a source folder with a valid module name.";return false;}
    auto target=scriptLibraryFile(language,name);if(target.isEmpty()||QFileInfo::exists(target)||QFileInfo::exists(target+".disabled")){error="The destination already exists or is invalid.";return false;}
    if(language=="python"&&!QFileInfo::exists(QDir(source).filePath("__init__.py"))){error="A Python package folder must contain __init__.py.";return false;}
    QList<QPair<QString,QByteArray>> files;qint64 total=0;QDirIterator it(source,QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
    while(it.hasNext()){auto path=it.next();QFileInfo info(path);if(info.isSymLink()){error="Package symlinks are not supported.";return false;}if(info.isDir())continue;auto relative=QDir(source).relativeFilePath(path);if(relative.contains("__pycache__/")||info.suffix()=="pyc")continue;
        if(!QStringList{"py","pyi","h","hpp","hh","inl","cpp","cc","json","txt","csv"}.contains(info.suffix().toLower()))continue;
        if(!scriptLibraryRelative(relative)||info.size()>1048576||(total+=info.size())>32*1024*1024||files.size()>=1024){error="Package exceeds path, file (1 MiB), count (1024), or total size (32 MiB) limits.";return false;}QFile file(path);if(!file.open(QIODevice::ReadOnly)){error="Cannot read package file.";return false;}auto bytes=file.read(1048577);total+=bytes.size()-info.size();if(bytes.size()>1048576||total>32*1024*1024){error="Package source grew beyond the import limits.";return false;}files.append({relative,bytes});
    }
    if(files.isEmpty()){error="No supported library files found.";return false;}
    // Stage files separately; rename only after every write succeeds.
    QDir().mkpath(scriptLibraryRoot());QTemporaryDir stage(QDir(scriptLibraryRoot()).filePath("import-XXXXXX"));if(!stage.isValid()){QDir().mkpath(scriptLibraryRoot());stage.setAutoRemove(true);error="Cannot stage import; refresh and retry.";return false;}
    for(auto entry:files){auto path=QDir(stage.path()).filePath(entry.first);QDir().mkpath(QFileInfo(path).absolutePath());QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(entry.second)!=entry.second.size()||!file.commit()){error="Could not copy package.";return false;}}
    if(!QDir().mkpath(QFileInfo(target).absolutePath())||!QDir().rename(stage.path(),target)){error="Could not install package.";return false;}stage.setAutoRemove(false);return true;
}
