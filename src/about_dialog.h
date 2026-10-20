#pragma once
#include <QDialog>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
class AboutSplash final:public QWidget {
    QPixmap art_{":/branding/about-splash.png"};
public:
    explicit AboutSplash(QWidget *parent=nullptr):QWidget(parent){if(art_.isNull())art_.load(":/branding/logo-sticker.png");setObjectName("aboutSplash");setAccessibleName("Nine's XD splash artwork; drag to move");setMinimumSize(200,120);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);}
    QSize sizeHint()const override{return {900,530};}
protected:
    void paintEvent(QPaintEvent *)override{if(art_.isNull())return;QPainter painter(this);auto size=art_.size();size.scale(this->size(),Qt::KeepAspectRatio);QRect target(QPoint((width()-size.width())/2,(height()-size.height())/2),size);painter.setRenderHint(QPainter::SmoothPixmapTransform);painter.drawPixmap(target,art_);}
};
class FloatingAbout final:public QDialog {
    QPoint dragOffset_;bool dragging_=false;
public:
    explicit FloatingAbout(QWidget *parent):QDialog(parent,Qt::Dialog|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint){
        setObjectName("about9xdDialog");setAttribute(Qt::WA_DeleteOnClose);setAttribute(Qt::WA_TranslucentBackground);setWindowTitle("About 9XD");setWindowIcon(QIcon(":/branding/logo-sticker.png"));setStyleSheet("QDialog#about9xdDialog { background: transparent; }");auto *layout=new QVBoxLayout(this);layout->setContentsMargins(20,8,20,20);layout->setSpacing(2);
        auto *top=new QHBoxLayout;top->addStretch();auto *close=new QToolButton(this);close->setObjectName("aboutClose");close->setAccessibleName("Close About");close->setToolTip("Close (Esc)");close->setText(QString(QChar(0x00d7)));close->setFixedSize(28,28);close->setStyleSheet("QToolButton {color:white; background:rgba(0,0,0,100); border:0; border-radius:14px; font-size:22px;} QToolButton:hover {background:rgba(100,75,130,210);} QToolButton:focus {border:1px solid white;}");top->addWidget(close);layout->addLayout(top);connect(close,&QToolButton::clicked,this,&QDialog::close);
        auto *art=new AboutSplash(this);art->installEventFilter(this);auto *shadow=new QGraphicsDropShadowEffect(art);shadow->setBlurRadius(16);shadow->setOffset(0,5);shadow->setColor(QColor(0,0,0,190));art->setGraphicsEffect(shadow);layout->addWidget(art,1);
        auto *thanks=new QLabel("Special thanks to Zuz, Nines, Taku, GPT and more.",this);thanks->setObjectName("aboutSpecialThanks");thanks->setWordWrap(true);thanks->setAlignment(Qt::AlignCenter);thanks->setStyleSheet("background:transparent; color:white; font-size:13px; padding:8px;");thanks->setTextInteractionFlags(Qt::TextSelectableByMouse);auto *textShadow=new QGraphicsDropShadowEffect(thanks);textShadow->setBlurRadius(8);textShadow->setOffset(1,2);textShadow->setColor(Qt::black);thanks->setGraphicsEffect(textShadow);layout->addWidget(thanks);
        auto available=(parent&&parent->screen()?parent->screen():QGuiApplication::primaryScreen())->availableGeometry();int w=qMin(960,available.width()-40),h=qMin(650,available.height()-40);resize(w,h);if(parent)move(parent->frameGeometry().center()-rect().center());else move(available.center()-rect().center());
    }
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        if(event->type()==QEvent::MouseButtonPress){auto *mouse=static_cast<QMouseEvent *>(event);if(mouse->button()==Qt::LeftButton){dragOffset_=mouse->globalPosition().toPoint()-pos();dragging_=true;return true;}}
        if(event->type()==QEvent::MouseMove&&dragging_){move(static_cast<QMouseEvent *>(event)->globalPosition().toPoint()-dragOffset_);return true;}
        if(event->type()==QEvent::MouseButtonRelease&&dragging_){dragging_=false;return true;}
        return QDialog::eventFilter(object,event);
    }
};
inline void showWorkbenchAbout(QWidget *parent){if(parent)if(auto *existing=parent->findChild<QDialog *>("about9xdDialog")){existing->show();existing->raise();existing->activateWindow();return;}auto *dialog=new FloatingAbout(parent);dialog->show();}
