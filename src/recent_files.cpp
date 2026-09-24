#include "recent_files.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
RecentFiles::RecentFiles(QString path):file_(std::move(path)){
    if(file_.isEmpty())return;QFile f(file_);if(!f.open(QIODevice::ReadOnly)||f.size()>65536)return;auto json=QJsonDocument::fromJson(f.readAll()).object();if(json["version"].toInt()!=1)return;
    for(auto value:json["files"].toArray()){auto p=value.toString();if(!p.isEmpty()&&QDir::isAbsolutePath(p)&&!paths_.contains(p)&&paths_.size()<12)paths_.append(p);}
}
bool RecentFiles::add(const QString &path){QFileInfo info(path);if(!info.isFile())return true;QString p=info.canonicalFilePath();if(p.isEmpty())p=info.absoluteFilePath();if(!paths_.isEmpty()&&paths_.first()==p)return true;paths_.removeAll(p);paths_.prepend(p);while(paths_.size()>12)paths_.removeLast();return save();}
bool RecentFiles::clear(){paths_.clear();return save();}
bool RecentFiles::save(){if(file_.isEmpty())return true;if(!QDir().mkpath(QFileInfo(file_).absolutePath()))return false;QJsonObject json{{"version",1},{"files",QJsonArray::fromStringList(paths_)}};auto bytes=QJsonDocument(json).toJson();QSaveFile f(file_);return f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size()&&f.commit();}
