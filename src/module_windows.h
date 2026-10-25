#pragma once
#include <QMainWindow>
#include <QTabBar>
#include <QMouseEvent>
#include <QApplication>
#include <QCloseEvent>
#include <QPainter>
#include <QIcon>
#include <functional>

class ModuleWindow final : public QMainWindow {
public:
    ModuleWindow():QMainWindow(nullptr,Qt::Window){}
    std::function<void()> returnRequested;
protected:
    void closeEvent(QCloseEvent *event)override{
        // Call a copy: the callback clears returnRequested, which would destroy the running lambda.
        if(returnRequested){auto callback=returnRequested;callback();event->ignore();}
        else QMainWindow::closeEvent(event);
    }
};

// Vertical rail of icon-only module tabs. Dragging a tab out of the rail detaches the module.
class ModuleTabBar final : public QTabBar {
public:
    explicit ModuleTabBar(QWidget *parent=nullptr):QTabBar(parent){setShape(QTabBar::RoundedWest);setMouseTracking(true);setDrawBase(false);}
    std::function<void(int,QPoint)> detachRequested;
protected:
    QSize tabSizeHint(int)const override{return {60,52};}
    QSize minimumTabSizeHint(int index)const override{return tabSizeHint(index);}
    void paintEvent(QPaintEvent *)override{
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);const auto palette=this->palette();
        for(int i=0;i<count();++i){
            const QRect tab=tabRect(i);const bool selected=i==currentIndex(),hovered=i==hovered_;
            const QRect box(tab.center().x()-22,tab.center().y()-22,44,44);
            if(selected||hovered){QColor fill=palette.color(QPalette::Highlight);fill.setAlpha(selected?150:60);painter.setPen(Qt::NoPen);painter.setBrush(fill);painter.drawRoundedRect(box,10,10);}
            if(selected){painter.setPen(Qt::NoPen);painter.setBrush(palette.color(QPalette::Link));painter.drawRoundedRect(QRectF(tab.left()+1,tab.center().y()-12,3,24),1.5,1.5);}
            const auto pixmap=tabIcon(i).pixmap(QSize(22,22),devicePixelRatioF(),selected?QIcon::Selected:QIcon::Normal);
            painter.drawPixmap(QRect(tab.center().x()-11,tab.center().y()-11,22,22),pixmap);
        }
    }
    void leaveEvent(QEvent *event)override{hovered_=-1;update();QTabBar::leaveEvent(event);}

    void mousePressEvent(QMouseEvent *event)override{
        pressed_=event->button()==Qt::LeftButton?tabAt(event->position().toPoint()):-1;
        origin_=event->position().toPoint();QTabBar::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event)override{
        if(pressed_>=0&&event->buttons().testFlag(Qt::LeftButton)
            &&(event->position().toPoint()-origin_).manhattanLength()>=QApplication::startDragDistance()
            &&!rect().adjusted(-12,-12,12,12).contains(event->position().toPoint())){
            const int tab=pressed_;pressed_=-1;
            if(detachRequested)detachRequested(tab,event->globalPosition().toPoint());
            return;
        }
        if(const int hover=tabAt(event->position().toPoint());hover!=hovered_){hovered_=hover;update();}
        QTabBar::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event)override{pressed_=-1;QTabBar::mouseReleaseEvent(event);}
private:
    int pressed_=-1,hovered_=-1;QPoint origin_;
};
