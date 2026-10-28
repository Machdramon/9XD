#pragma once
#include <QWidget>
#include <QPainter>
#include <QLinearGradient>
#include <QElapsedTimer>
#include <QTimer>
#include <QEvent>
#include <functional>
#include <cmath>
#include "theme.h"
// Retro "flatbed scanner" sweep drawn over a data view while a scan runs: a bright beam with a soft glow and a fading
// trail moves down the area that actually holds data (callers supply that rectangle), over faint CRT scanlines.
// It is purely decorative: transparent for the mouse and never repaints the view underneath.
class ScanSweepOverlay final : public QWidget {
public:
    // area returns the rectangle (in host coordinates) that contains data; an empty rect hides the beam.
    explicit ScanSweepOverlay(QWidget *host,std::function<QRect()> area={}):QWidget(host),area_(std::move(area)){
        setAttribute(Qt::WA_TransparentForMouseEvents);setAttribute(Qt::WA_NoSystemBackground);setAttribute(Qt::WA_TranslucentBackground);setFocusPolicy(Qt::NoFocus);hide();
        host->installEventFilter(this);timer_.setInterval(16);QObject::connect(&timer_,&QTimer::timeout,this,[this]{if(fading_&&fade_.elapsed()>320){timer_.stop();fading_=false;hide();return;}update();});
    }
    // Calls nest: the sweep runs until every start() has a matching stop().
    void start(){if(active_++>0)return;fading_=false;clock_.start();setGeometry(parentWidget()->rect());raise();show();timer_.start();}
    void stop(){if(active_==0)return;if(--active_>0)return;fading_=true;fade_.start();}
    bool running()const{return active_>0;}
protected:
    bool eventFilter(QObject *object,QEvent *event)override{if(object==parentWidget()&&event->type()==QEvent::Resize)setGeometry(parentWidget()->rect());return QWidget::eventFilter(object,event);}
    void paintEvent(QPaintEvent *)override{
        QRect data=area_?area_():rect();data=data.intersected(rect());if(data.height()<4||data.width()<4)return;
        double opacity=fading_?qMax(0.,1-fade_.elapsed()/320.):1;QPainter p(this);p.setOpacity(opacity);p.setClipRect(data);
        QColor beam=themeColor("accent.focus",QColor("#5B9BFF"));if(beam.lightness()<150)beam=beam.lighter(140);
        // Faint CRT lines over the data area only.
        QColor line=beam;line.setAlpha(14);for(int y=data.top();y<=data.bottom();y+=3)p.fillRect(QRect(data.left(),y,data.width(),1),line);
        // One pass takes about 1.5 s whatever the height; smoothstep easing so the head slows near both edges.
        const double period=1500,t=std::fmod(clock_.elapsed(),period)/period,eased=t*t*(3-2*t);const int y=data.top()+int(eased*data.height());
        const int trail=qMin(120,data.height()/2);QColor head=beam;head.setAlpha(70);QColor clear=beam;clear.setAlpha(0);
        QLinearGradient tail(0,y-trail,0,y);tail.setColorAt(0,clear);tail.setColorAt(1,head);p.fillRect(QRect(data.left(),y-trail,data.width(),trail),tail);
        QColor glowColor=beam;glowColor.setAlpha(110);QLinearGradient glow(0,y-7,0,y+7);glow.setColorAt(0,clear);glow.setColorAt(.5,glowColor);glow.setColorAt(1,clear);p.fillRect(QRect(data.left(),y-7,data.width(),14),glow);
        QColor core=beam.lighter(130);core.setAlpha(235);p.fillRect(QRect(data.left(),y-1,data.width(),2),core);
    }
private:
    std::function<QRect()> area_;QTimer timer_;QElapsedTimer clock_,fade_;int active_=0;bool fading_=false;
};
