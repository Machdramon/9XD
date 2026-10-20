#pragma once
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
namespace WorkbenchFiles {
inline QSettings settings(){auto file=qApp->property("workspaceSettingsFile").toString();if(file.isEmpty())return QSettings("BinaryWorkbench","Workspace");return QSettings(file,QSettings::IniFormat);}
inline QString lastDirectory(){
    auto path=settings().value("files/lastOpenDirectory").toString();
    while(!path.isEmpty()&&!QDir(path).exists()){auto parent=QFileInfo(path).absolutePath();if(parent==path){path.clear();break;}path=parent;}
    return path.isEmpty()?QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation):path;
}
inline void remember(const QString &path,bool directory=false){
    if(path.isEmpty())return;QFileInfo info(path);if(!info.exists()||(directory?!info.isDir():!info.isFile()))return;
    auto s=settings();s.setValue("files/lastOpenDirectory",directory?info.absoluteFilePath():info.absolutePath());
}
inline QString getOpenFileName(QWidget *parent=nullptr,const QString &caption={},const QString &dir={},const QString &filter={},QString *selectedFilter=nullptr,QFileDialog::Options options={}){
    auto path=QFileDialog::getOpenFileName(parent,caption,dir.isEmpty()?lastDirectory():dir,filter,selectedFilter,options);remember(path);return path;
}
inline QStringList getOpenFileNames(QWidget *parent=nullptr,const QString &caption={},const QString &dir={},const QString &filter={},QString *selectedFilter=nullptr,QFileDialog::Options options={}){
    auto paths=QFileDialog::getOpenFileNames(parent,caption,dir.isEmpty()?lastDirectory():dir,filter,selectedFilter,options);if(!paths.isEmpty())remember(paths.first());return paths;
}
inline QString getExistingDirectory(QWidget *parent=nullptr,const QString &caption={},const QString &dir={},QFileDialog::Options options=QFileDialog::ShowDirsOnly){
    auto path=QFileDialog::getExistingDirectory(parent,caption,dir.isEmpty()?lastDirectory():dir,options);remember(path,true);return path;
}
}
