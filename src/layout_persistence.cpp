#include "layout_persistence.h"
#include <QMainWindow>
#include <QDockWidget>
#include <QToolBar>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QStatusBar>
#include <QEvent>
#include <QScreen>
#include <QGuiApplication>
namespace {constexpr int layoutVersion=1;
QByteArray digest(const QByteArray &geometry,const QByteArray &state){return QCryptographicHash::hash(geometry+state,QCryptographicHash::Sha256).toHex();}
}
QString UiLayout::defaultPath(){return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/BinaryWorkbench/ui-layout.json";}
UiLayout::UiLayout(QMainWindow *window,QString path):QObject(window),window_(window),path_(std::move(path)){
    defaultState_=window_->saveState(layoutVersion);defaultGeometry_=window_->saveGeometry();
    timer_.setSingleShot(true);timer_.setInterval(400);connect(&timer_,&QTimer::timeout,this,[this]{save();});
    restore();window_->installEventFilter(this);
    for(auto *dock:window_->findChildren<QDockWidget *>()){
        dock->installEventFilter(this);
        connect(dock,&QDockWidget::visibilityChanged,this,[this]{schedule();});
        connect(dock,&QDockWidget::topLevelChanged,this,[this]{schedule();});
        connect(dock,&QDockWidget::dockLocationChanged,this,[this]{schedule();});
    }
    for(auto *bar:window_->findChildren<QToolBar *>()){bar->installEventFilter(this);connect(bar,&QToolBar::visibilityChanged,this,[this]{schedule();});connect(bar,&QToolBar::topLevelChanged,this,[this]{schedule();});}
    connect(window_,&QMainWindow::tabifiedDockWidgetActivated,this,[this]{schedule();});
    enabled_=true;
}
void UiLayout::schedule(){if(enabled_&&started_&&!timer_.isActive())timer_.start();}
bool UiLayout::eventFilter(QObject *object,QEvent *event){
    if(object==window_&&event->type()==QEvent::Show&&!started_){started_=true;keepPanelsOnScreen();schedule();}
    switch(event->type()){case QEvent::Move:case QEvent::Resize:case QEvent::Show:case QEvent::Hide:case QEvent::LayoutRequest:case QEvent::WindowStateChange:schedule();break;default:break;}
    return QObject::eventFilter(object,event);
}
void UiLayout::restore(){
    QFile file(path_);if(!file.exists())return;
    if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024){window_->statusBar()->showMessage("Could not read the saved layout; the default will be used.",8000);return;}
    auto json=QJsonDocument::fromJson(file.readAll()).object();auto geometry=QByteArray::fromBase64(json["geometry"].toString().toLatin1()),state=QByteArray::fromBase64(json["panels"].toString().toLatin1());
    bool valid=json["version"].toInt()==layoutVersion&&!geometry.isEmpty()&&!state.isEmpty()&&json["sha256"].toString().toLatin1()==digest(geometry,state);
    if(valid&&window_->restoreGeometry(geometry)&&window_->restoreState(state,layoutVersion))return;
    window_->restoreGeometry(defaultGeometry_);window_->restoreState(defaultState_,layoutVersion);
    window_->statusBar()->showMessage("The saved layout is invalid; the default layout was restored.",8000);
}
void UiLayout::save(){
    if(!enabled_||!started_)return;
    auto geometry=window_->saveGeometry(),state=window_->saveState(layoutVersion);
    QJsonObject json{{"version",layoutVersion},{"geometry",QString::fromLatin1(geometry.toBase64())},{"panels",QString::fromLatin1(state.toBase64())},{"sha256",QString::fromLatin1(digest(geometry,state))}};
    auto bytes=QJsonDocument(json).toJson();
    // Avoid repeated disk writes when layout notifications do not change the saved state.
    QFile previous(path_);if(previous.open(QIODevice::ReadOnly)&&previous.size()<=1024*1024&&previous.readAll()==bytes)return;
    previous.close();QSaveFile file(path_);
    if(!QDir().mkpath(QFileInfo(path_).absolutePath())||!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit())window_->statusBar()->showMessage("Could not save the panel layout.",8000);
}
void UiLayout::saveAndStop(){timer_.stop();save();enabled_=false;}
void UiLayout::reset(){
    enabled_=false;timer_.stop();window_->restoreGeometry(defaultGeometry_);window_->restoreState(defaultState_,layoutVersion);keepPanelsOnScreen();enabled_=true;schedule();
}
void UiLayout::keepPanelsOnScreen(){
    auto screens=QGuiApplication::screens();if(screens.isEmpty())return;
    for(auto *dock:window_->findChildren<QDockWidget *>())if(dock->isFloating()){
        bool accessible=false;auto title=dock->frameGeometry();title.setHeight(30);for(auto *screen:screens)if(screen->availableGeometry().intersected(title).width()>=60){accessible=true;break;}
        if(!accessible){auto available=window_->screen()->availableGeometry();dock->resize(dock->size().boundedTo(available.size()));dock->move(available.topLeft()+QPoint(30,30));}
    }
}
