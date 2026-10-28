#pragma once
#include <QApplication>
#include <QDockWidget>
#include <QAbstractButton>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QCursor>
#include <QEvent>
// Panel (dock) title buttons — float/maximize and close — stay invisible until the pointer is over that panel's
// title bar. They keep their place and remain clickable, so the title layout never jumps; only the paint is hidden.
class DockTitleHover final : public QObject {
public:
    using QObject::QObject;
    static void install(){static bool installed=false;if(installed||!qApp)return;installed=true;auto *hover=new DockTitleHover(qApp);qApp->installEventFilter(hover);for(auto *widget:QApplication::allWidgets())if(auto *dock=qobject_cast<QDockWidget *>(widget))attach(dock);}
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        const auto type=event->type();
        if(type==QEvent::Polish||type==QEvent::Show){if(auto *dock=qobject_cast<QDockWidget *>(object))attach(dock);}
        else if(type==QEvent::MouseMove||type==QEvent::Enter){if(auto *dock=qobject_cast<QDockWidget *>(object);dock&&dock->property("titleButtonsShown").isValid())refresh(dock);}
        return QObject::eventFilter(object,event);
    }
private:
    static QList<QAbstractButton *> buttons(QDockWidget *dock){QList<QAbstractButton *> list;for(auto *name:{"qt_dockwidget_closebutton","qt_dockwidget_floatbutton"})if(auto *button=dock->findChild<QAbstractButton *>(name,Qt::FindDirectChildrenOnly))list<<button;return list;}
    static int titleHeight(QDockWidget *dock){int height=dock->fontMetrics().height()+10;for(auto *button:buttons(dock))if(button->isVisible())height=qMax(height,button->geometry().bottom()+4);return height;}
    static bool overTitle(QDockWidget *dock){if(dock->titleBarWidget()||!dock->isVisible())return false;const auto point=dock->mapFromGlobal(QCursor::pos());return point.x()>=0&&point.x()<dock->width()&&point.y()>=0&&point.y()<titleHeight(dock);}
    static void setShown(QDockWidget *dock,bool shown){for(auto *button:buttons(dock)){auto *effect=qobject_cast<QGraphicsOpacityEffect *>(button->graphicsEffect());if(!effect){effect=new QGraphicsOpacityEffect(button);button->setGraphicsEffect(effect);}effect->setOpacity(shown?1:0);}dock->setProperty("titleButtonsShown",shown);}
    static void attach(QDockWidget *dock){if(dock->property("titleButtonsShown").isValid())return;dock->setMouseTracking(true);setShown(dock,false);}
    static void refresh(QDockWidget *dock){
        const bool over=overTitle(dock);if(over==dock->property("titleButtonsShown").toBool())return;setShown(dock,over);if(!over)return;
        // The dock gets no event when the pointer moves into its content or leaves the window, so poll briefly while shown.
        auto *timer=new QTimer(dock);timer->setInterval(120);QObject::connect(timer,&QTimer::timeout,dock,[dock,timer]{if(overTitle(dock))return;setShown(dock,false);timer->deleteLater();});timer->start();
    }
};
