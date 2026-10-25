#pragma once
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QShortcut>
#include <QKeyEvent>
namespace WorkbenchFiles {
class NameSearchProxy final:public QSortFilterProxyModel {
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
    void search(const QString &text){patterns_.clear();for(const auto &token:text.simplified().split(' ',Qt::SkipEmptyParts)){QRegularExpression pattern(token.contains('*')||token.contains('?')?QRegularExpression::wildcardToRegularExpression(token):QRegularExpression::escape(token),QRegularExpression::CaseInsensitiveOption);pattern.optimize();patterns_.append(pattern);}invalidateFilter();}
protected:
    bool filterAcceptsRow(int row,const QModelIndex &parent)const override{
        auto index=sourceModel()->index(row,0,parent);auto *files=qobject_cast<QFileSystemModel *>(sourceModel());if(files&&files->isDir(index))return true;
        auto name=sourceModel()->data(index).toString();for(const auto &pattern:patterns_)if(!pattern.match(name).hasMatch())return false;return true;
    }
private:QList<QRegularExpression> patterns_;
};
class SearchField final:public QLineEdit {
public:using QLineEdit::QLineEdit;
protected:void keyPressEvent(QKeyEvent *event)override{if(event->key()==Qt::Key_Return||event->key()==Qt::Key_Enter){event->accept();return;}if(event->key()==Qt::Key_Escape&&!text().isEmpty()){clear();event->accept();return;}QLineEdit::keyPressEvent(event);}
};
inline void addSearch(QFileDialog &dialog){
    auto *proxy=new NameSearchProxy(&dialog);proxy->setObjectName("openFileSearchProxy");dialog.setProxyModel(proxy);
    auto *row=new QWidget(&dialog);auto *layout=new QHBoxLayout(row);layout->setContentsMargins(0,0,0,0);auto *search=new SearchField(row);search->setObjectName("openFileSearch");search->setPlaceholderText("Name or pattern, for example: geometry or *.bin (Ctrl+F)");search->setClearButtonEnabled(true);search->setToolTip("Filters file names in the current folder. Folders remain visible for navigation. Separate terms with spaces; all terms must match. Escape clears the search.");layout->addWidget(new QLabel("Search this folder",row));layout->addWidget(search,1);
    if(auto *grid=qobject_cast<QGridLayout *>(dialog.layout()))grid->addWidget(row,grid->rowCount(),0,1,grid->columnCount());else dialog.layout()->addWidget(row);
    QObject::connect(search,&QLineEdit::textChanged,proxy,[proxy](const QString &text){proxy->search(text);});auto *shortcut=new QShortcut(QKeySequence::Find,&dialog);QObject::connect(shortcut,&QShortcut::activated,search,[search]{search->setFocus();search->selectAll();});
}
inline QSettings settings(){auto file=qApp->property("workspaceSettingsFile").toString();if(file.isEmpty())return QSettings("BinaryWorkbench","Workspace");return QSettings(file,QSettings::IniFormat);}
inline QString lastDirectory(){
    auto path=settings().value("files/lastOpenDirectory").toString();
    while(!path.isEmpty()&&!QDir(path).exists()){auto parent=QFileInfo(path).absolutePath();if(parent==path){path.clear();break;}path=parent;}
    return path.isEmpty()?QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation):path;
}
inline void remember(const QString &path,bool directory=false){
    if(path.isEmpty())return;QFileInfo info(path);if(!info.exists()||(directory?!info.isDir():!info.isFile()))return;
    auto folder=QDir::cleanPath(directory?info.absoluteFilePath():info.absolutePath());auto s=settings();s.setValue("files/lastOpenDirectory",folder);
    auto recent=s.value("files/recentOpenDirectories").toStringList();
    for(auto it=recent.begin();it!=recent.end();)if(QString::compare(QDir::cleanPath(*it),folder,
#ifdef Q_OS_WIN
        Qt::CaseInsensitive
#else
        Qt::CaseSensitive
#endif
    )==0)it=recent.erase(it);else ++it;
    recent.prepend(folder);while(recent.size()>12)recent.removeLast();s.setValue("files/recentOpenDirectories",recent);s.sync();
}
inline QList<QUrl> sidebarUrls(){
    QList<QUrl> urls;auto append=[&](QString path){if(path.isEmpty()||!QDir(path).exists())return;auto url=QUrl::fromLocalFile(path);if(!urls.contains(url))urls.append(url);};
    for(const auto &path:settings().value("files/recentOpenDirectories").toStringList())append(path);
    append(lastDirectory());append(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));append(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));urls.append(QUrl("file:"));return urls;
}
inline QString getOpenFileName(QWidget *parent=nullptr,const QString &caption={},const QString &dir={},const QString &filter={},QString *selectedFilter=nullptr,QFileDialog::Options options={}){
    auto path=QFileDialog::getOpenFileName(parent,caption,dir.isEmpty()?lastDirectory():dir,filter,selectedFilter,options);remember(path);return path;
}
inline QStringList getOpenFileNames(QWidget *parent=nullptr,const QString &caption={},const QString &dir={},const QString &filter={},QString *selectedFilter=nullptr,QFileDialog::Options options={}){
    QFileDialog dialog(parent,caption,dir.isEmpty()?lastDirectory():dir,filter);dialog.setObjectName("workbenchOpenFilesDialog");dialog.setOptions(options|QFileDialog::DontUseNativeDialog);dialog.setFileMode(QFileDialog::ExistingFiles);dialog.setAcceptMode(QFileDialog::AcceptOpen);dialog.setSidebarUrls(sidebarUrls());dialog.setViewMode(QFileDialog::Detail);dialog.resize(1000,650);if(selectedFilter&&!selectedFilter->isEmpty())dialog.selectNameFilter(*selectedFilter);
    addSearch(dialog);
    if(dialog.exec()!=QDialog::Accepted)return {};if(selectedFilter)*selectedFilter=dialog.selectedNameFilter();auto paths=dialog.selectedFiles();if(!paths.isEmpty())remember(paths.first());return paths;
}
inline QString getExistingDirectory(QWidget *parent=nullptr,const QString &caption={},const QString &dir={},QFileDialog::Options options=QFileDialog::ShowDirsOnly){
    auto path=QFileDialog::getExistingDirectory(parent,caption,dir.isEmpty()?lastDirectory():dir,options);remember(path,true);return path;
}
}
