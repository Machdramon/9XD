#pragma once
#include <QObject>
#include <QTabBar>
#include <QTimer>
#include <QEvent>

// QMdiArea owns its tab bar. Keep it compact without replacing its document handling.
class CompactDocumentTabs final : public QObject {
public:
    explicit CompactDocumentTabs(QTabBar *tabs):QObject(tabs),tabs_(tabs){
        tabs_->setObjectName("documentTabs");tabs_->setExpanding(false);
        tabs_->setElideMode(Qt::ElideMiddle);tabs_->setUsesScrollButtons(true);
        tabs_->installEventFilter(this);refresh();
    }
    void refresh(){
        if(pending_)return;pending_=true;
        QTimer::singleShot(0,this,[this]{
            pending_=false;
            const int width=qBound(110,(tabs_->width()-2)/qMax(1,tabs_->count()),180);
            if(width==width_)return;width_=width;
            // CSS widths exclude the horizontal padding.
            const int content=width-20;
            tabs_->setStyleSheet(QString("QTabBar::tab { width: %1px; min-width: %1px; max-width: %1px; padding: 5px 10px; }").arg(content));
        });
    }
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        if(event->type()==QEvent::Resize||event->type()==QEvent::LayoutRequest||event->type()==QEvent::Show)refresh();
        return QObject::eventFilter(object,event);
    }
private:
    QTabBar *tabs_;int width_=-1;bool pending_=false;
};
