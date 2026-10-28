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
#include <QFileIconProvider>
#include <QStyle>
#include <QTreeView>
#include <QListView>
#include <QHeaderView>
#include <QTimer>
#include <QProgressBar>
#include <QPointer>
namespace WorkbenchFiles {
// File pickers need names/metadata, not per-file shell handlers or thumbnail extraction.
// Shared for the application lifetime: QFileSystemModel does not own its provider.
class LightweightIcons final:public QObject,public QFileIconProvider {
public:
    explicit LightweightIcons(QObject *parent):QObject(parent),file_(qApp->style()->standardIcon(QStyle::SP_FileIcon)),folder_(qApp->style()->standardIcon(QStyle::SP_DirIcon)),drive_(qApp->style()->standardIcon(QStyle::SP_DriveHDIcon)){}
    QIcon icon(IconType type)const override{return type==Folder?folder_:type==File?file_:drive_;}
    QIcon icon(const QFileInfo &info)const override{return info.isDir()?folder_:file_;}
    QString type(const QFileInfo &info)const override{return info.isDir()?QString("Folder"):info.suffix().isEmpty()?QString("File"):info.suffix().toUpper()+" file";}
private:QIcon file_,folder_,drive_;
};
inline LightweightIcons *lightweightIcons(){static QPointer<LightweightIcons> provider;if(!provider)provider=new LightweightIcons(qApp);return provider;}
inline void fastListing(QFileDialog &dialog){
    if(dialog.property("fastListingInstalled").toBool())return;dialog.setProperty("fastListingInstalled",true);
    dialog.setOption(QFileDialog::DontUseCustomDirectoryIcons);dialog.setLabelText(QFileDialog::LookIn,"Location");
    for(auto *model:dialog.findChildren<QFileSystemModel *>()){model->setOption(QFileSystemModel::DontUseCustomDirectoryIcons);model->setIconProvider(lightweightIcons());model->setReadOnly(true);}
    for(auto *tree:dialog.findChildren<QTreeView *>()){tree->setUniformRowHeights(true);tree->setAnimated(false);auto *header=tree->header();header->setSectionResizeMode(QHeaderView::Interactive);header->setStretchLastSection(true);if(header->count()>0)header->resizeSection(0,420);if(header->count()>1)header->resizeSection(1,110);if(header->count()>2)header->resizeSection(2,120);}
    for(auto *list:dialog.findChildren<QListView *>()){list->setUniformItemSizes(true);list->setLayoutMode(QListView::Batched);list->setBatchSize(256);}
    auto *statusRow=new QWidget(&dialog);auto *layout=new QHBoxLayout(statusRow);layout->setContentsMargins(0,0,0,0);auto *status=new QLabel("Loading folder…",statusRow);status->setObjectName("openFileLoadStatus");auto *progress=new QProgressBar(statusRow);progress->setObjectName("openFileLoadProgress");progress->setRange(0,0);progress->setMaximumWidth(100);progress->setMaximumHeight(12);layout->addWidget(status,1);layout->addWidget(progress);
    if(auto *grid=qobject_cast<QGridLayout *>(dialog.layout()))grid->addWidget(statusRow,grid->rowCount(),0,1,grid->columnCount());else dialog.layout()->addWidget(statusRow);
    QObject::connect(&dialog,&QFileDialog::directoryEntered,status,[status,progress](const QString &){status->setText("Loading folder… You can keep navigating or cancel.");progress->show();});
    for(auto *model:dialog.findChildren<QFileSystemModel *>())QObject::connect(model,&QFileSystemModel::directoryLoaded,status,[&dialog,model,status,progress](const QString &path){if(QDir::cleanPath(path)!=QDir::cleanPath(dialog.directory().absolutePath()))return;status->setText(QString("%1 entries · names only, no thumbnails").arg(model->rowCount(model->index(path))));progress->hide();});
}

class NameSearchProxy final:public QSortFilterProxyModel {
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
    void search(const QString &text){patterns_.clear();for(const auto &token:text.simplified().split(' ',Qt::SkipEmptyParts)){QRegularExpression pattern(token.contains('*')||token.contains('?')?QRegularExpression::wildcardToRegularExpression(token):QRegularExpression::escape(token),QRegularExpression::CaseInsensitiveOption);pattern.optimize();patterns_.append(pattern);}invalidateFilter();}
protected:
    bool filterAcceptsRow(int row,const QModelIndex &parent)const override{
        if(patterns_.isEmpty())return true;
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
    fastListing(dialog);
    auto *proxy=new NameSearchProxy(&dialog);proxy->setObjectName("openFileSearchProxy");dialog.setProxyModel(proxy);
    auto *row=new QWidget(&dialog);auto *layout=new QHBoxLayout(row);layout->setContentsMargins(0,0,0,0);auto *search=new SearchField(row);search->setObjectName("openFileSearch");search->setMaxLength(256);search->setPlaceholderText("Name or pattern, for example: geometry or *.bin (Ctrl+F)");search->setClearButtonEnabled(true);search->setToolTip("Filters file names in the current folder. Folders remain visible for navigation. Separate terms with spaces; all terms must match. Escape clears the search.");layout->addWidget(new QLabel("Search this folder",row));layout->addWidget(search,1);
    if(auto *grid=qobject_cast<QGridLayout *>(dialog.layout()))grid->addWidget(row,grid->rowCount(),0,1,grid->columnCount());else dialog.layout()->addWidget(row);
    auto *delay=new QTimer(search);delay->setSingleShot(true);delay->setInterval(220);delay->setObjectName("openFileSearchDelay");
    QObject::connect(search,&QLineEdit::textChanged,proxy,[proxy,delay](const QString &text){if(text.isEmpty()){delay->stop();proxy->search({});}else delay->start();});QObject::connect(delay,&QTimer::timeout,proxy,[proxy,search]{proxy->search(search->text());});auto *shortcut=new QShortcut(QKeySequence::Find,&dialog);QObject::connect(shortcut,&QShortcut::activated,search,[search]{search->setFocus();search->selectAll();});
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
inline void initialLocation(QFileDialog &dialog,const QString &location){
    QFileInfo info(location);if(info.isFile()){dialog.setDirectory(info.absolutePath());dialog.selectFile(info.fileName());}else dialog.setDirectory(location);
}
inline void prepareOpen(QFileDialog &dialog,QFileDialog::FileMode mode,const QString &location,const QString &filter,QFileDialog::Options options){
    dialog.setOptions(options|QFileDialog::DontUseNativeDialog|QFileDialog::DontUseCustomDirectoryIcons);dialog.setFileMode(mode);dialog.setAcceptMode(QFileDialog::AcceptOpen);dialog.setSidebarUrls(sidebarUrls());dialog.setViewMode(QFileDialog::Detail);dialog.resize(1050,680);
    if(!filter.isEmpty())dialog.setNameFilter(filter);else if(mode!=QFileDialog::Directory)dialog.setNameFilters({"All files (*)","Binary / game resources (*.bin *.dat *.nfc *.0iv *.sdb *.pak *.arc *.farc)","Images (*.png *.jpg *.jpeg *.bmp *.tga *.dds *.tm2)","Scripts / profiles (*.py *.cpp *.h *.json)"});
    if(mode==QFileDialog::Directory)fastListing(dialog);else addSearch(dialog);
    initialLocation(dialog,location);
}
inline QString getOpenFileName(QWidget *parent=nullptr,const QString &caption={},const QString &dir={},const QString &filter={},QString *selectedFilter=nullptr,QFileDialog::Options options={}){
    QFileDialog dialog(parent,caption);dialog.setObjectName("workbenchOpenFileDialog");prepareOpen(dialog,QFileDialog::ExistingFile,dir.isEmpty()?lastDirectory():dir,filter,options);if(selectedFilter&&!selectedFilter->isEmpty())dialog.selectNameFilter(*selectedFilter);
    if(dialog.exec()!=QDialog::Accepted)return {};if(selectedFilter)*selectedFilter=dialog.selectedNameFilter();auto paths=dialog.selectedFiles();auto path=paths.isEmpty()?QString():paths.first();remember(path);return path;
}
inline QStringList getOpenFileNames(QWidget *parent=nullptr,const QString &caption={},const QString &dir={},const QString &filter={},QString *selectedFilter=nullptr,QFileDialog::Options options={}){
    QFileDialog dialog(parent,caption);dialog.setObjectName("workbenchOpenFilesDialog");prepareOpen(dialog,QFileDialog::ExistingFiles,dir.isEmpty()?lastDirectory():dir,filter,options);if(selectedFilter&&!selectedFilter->isEmpty())dialog.selectNameFilter(*selectedFilter);
    if(dialog.exec()!=QDialog::Accepted)return {};if(selectedFilter)*selectedFilter=dialog.selectedNameFilter();auto paths=dialog.selectedFiles();if(!paths.isEmpty())remember(paths.first());return paths;
}
inline QString getExistingDirectory(QWidget *parent=nullptr,const QString &caption={},const QString &dir={},QFileDialog::Options options=QFileDialog::ShowDirsOnly){
    QFileDialog dialog(parent,caption);dialog.setObjectName("workbenchFolderDialog");prepareOpen(dialog,QFileDialog::Directory,dir.isEmpty()?lastDirectory():dir,{},options|QFileDialog::ShowDirsOnly);
    if(dialog.exec()!=QDialog::Accepted)return {};auto paths=dialog.selectedFiles();auto path=paths.isEmpty()?QString():paths.first();remember(path,true);return path;
}
}
