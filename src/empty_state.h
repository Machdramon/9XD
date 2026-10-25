#pragma once
#include "workbench_icons.h"
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QPlainTextEdit>
#include <functional>

// A centered icon, title and hint shown over a list or view while it has no content.
// It ignores the mouse, so the view underneath keeps working.
class EmptyStateOverlay final : public QWidget {
public:
    EmptyStateOverlay(QAbstractScrollArea *area,QString icon,QString title,QString hint,std::function<bool()> empty)
        :QWidget(area->viewport()),icon_(std::move(icon)),title_(std::move(title)),hint_(std::move(hint)),empty_(std::move(empty)){
        setObjectName("emptyState");setAttribute(Qt::WA_TransparentForMouseEvents);
        area->viewport()->installEventFilter(this);setGeometry(area->viewport()->rect());refresh();
    }
    void refresh(){setVisible(empty_());if(isVisible())raise();}
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        if(object==parent()&&event->type()==QEvent::Resize)setGeometry(static_cast<QWidget *>(parent())->rect());
        return QWidget::eventFilter(object,event);
    }
    void paintEvent(QPaintEvent *)override{
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
        const auto palette=qApp->palette();const int width=qMin(360,this->width()-32);if(width<80)return;
        QFont titleFont=font();titleFont.setWeight(QFont::DemiBold);const QFontMetrics titleMetrics(titleFont);
        const QRect hintBox=fontMetrics().boundingRect(QRect(0,0,width,1000),Qt::AlignHCenter|Qt::TextWordWrap,hint_);
        const int iconSize=32,gap=10,text=titleMetrics.height()+6+hintBox.height();
        // Short views drop the icon so the text still fits.
        const bool showIcon=height()>=text+iconSize+gap+24;const int total=text+(showIcon?iconSize+gap:0);
        int y=qMax(6,(height()-total)/2);const int x=(this->width()-width)/2;
        if(showIcon){QPixmap icon=workbenchIcon(icon_).pixmap(QSize(iconSize,iconSize),devicePixelRatioF(),QIcon::Disabled);
            painter.drawPixmap((this->width()-iconSize)/2,y,icon);y+=iconSize+gap;}
        painter.setFont(titleFont);painter.setPen(palette.color(QPalette::Text));
        painter.drawText(QRect(x,y,width,titleMetrics.height()),Qt::AlignHCenter,title_);y+=titleMetrics.height()+6;
        painter.setFont(font());painter.setPen(palette.color(QPalette::Disabled,QPalette::Text));
        painter.drawText(QRect(x,y,width,hintBox.height()),Qt::AlignHCenter|Qt::TextWordWrap,hint_);
    }
private:
    QString icon_,title_,hint_;std::function<bool()> empty_;
};

// Shown while the view's model has no rows.
inline EmptyStateOverlay *addEmptyState(QAbstractItemView *view,const QString &icon,const QString &title,const QString &hint){
    auto *overlay=new EmptyStateOverlay(view,icon,title,hint,[view]{return !view->model()||view->model()->rowCount()==0;});
    if(auto *model=view->model()){
        auto refresh=[overlay]{overlay->refresh();};
        QObject::connect(model,&QAbstractItemModel::rowsInserted,overlay,refresh);QObject::connect(model,&QAbstractItemModel::rowsRemoved,overlay,refresh);
        QObject::connect(model,&QAbstractItemModel::modelReset,overlay,refresh);QObject::connect(model,&QAbstractItemModel::layoutChanged,overlay,refresh);
    }
    return overlay;
}
// Shown while the text is empty.
inline EmptyStateOverlay *addEmptyState(QPlainTextEdit *edit,const QString &icon,const QString &title,const QString &hint){
    auto *overlay=new EmptyStateOverlay(edit,icon,title,hint,[edit]{return edit->document()->isEmpty();});
    QObject::connect(edit,&QPlainTextEdit::textChanged,overlay,[overlay]{overlay->refresh();});return overlay;
}
// Shown while the scene has no items.
inline EmptyStateOverlay *addEmptyState(QGraphicsView *view,const QString &icon,const QString &title,const QString &hint){
    auto *overlay=new EmptyStateOverlay(view,icon,title,hint,[view]{return !view->scene()||view->scene()->items().isEmpty();});
    if(view->scene())QObject::connect(view->scene(),&QGraphicsScene::changed,overlay,[overlay]{overlay->refresh();});
    return overlay;
}
