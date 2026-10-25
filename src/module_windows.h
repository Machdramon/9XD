#pragma once
#include <QMainWindow>
#include <QTabBar>
#include <QMouseEvent>
#include <QApplication>
#include <QCloseEvent>
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

class ModuleTabBar final : public QTabBar {
public:
    using QTabBar::QTabBar;
    std::function<void(int,QPoint)> detachRequested;
protected:
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
        QTabBar::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event)override{pressed_=-1;QTabBar::mouseReleaseEvent(event);}
private:
    int pressed_=-1;QPoint origin_;
};
