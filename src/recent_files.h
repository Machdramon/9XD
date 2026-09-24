#pragma once
#include <QStringList>
class RecentFiles {
public:
    explicit RecentFiles(QString path={});
    const QStringList &paths()const{return paths_;}
    bool add(const QString &path);
    bool clear();
private:
    QString file_;QStringList paths_;
    bool save();
};
