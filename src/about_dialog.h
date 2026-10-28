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
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QParallelAnimationGroup>
#include <QLinearGradient>
#include <QCursor>
class AboutSplash final:public QWidget {
    QPixmap art_{":/branding/about-splash.png"};
public:
    explicit AboutSplash(QWidget *parent=nullptr):QWidget(parent){if(art_.isNull())art_.load(":/branding/logo-sticker.png");setObjectName("aboutSplash");setAccessibleName("Nine's XD splash artwork; drag to move");setMinimumSize(200,120);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);}
    QSize sizeHint()const override{return {900,530};}
    // Entrance animation inputs: zoom scales the artwork around its centre; glint (0..1) sweeps a light band across it.
    void setZoom(double zoom){zoom_=zoom;update();}
    void setGlint(double glint){glint_=glint;update();}
protected:
    void paintEvent(QPaintEvent *)override{
        if(art_.isNull())return;QPainter painter(this);auto size=art_.size();size.scale((QSizeF(this->size())*zoom_).toSize(),Qt::KeepAspectRatio);QRect target(QPoint((width()-size.width())/2,(height()-size.height())/2),size);painter.setRenderHint(QPainter::SmoothPixmapTransform);
        if(glint_<=0||glint_>=1||size.isEmpty()){painter.drawPixmap(target,art_);return;}
        // The band is clipped to the artwork's own alpha (SourceAtop), so only the logo shines, not the transparent box.
        QImage layer(size,QImage::Format_ARGB32_Premultiplied);layer.fill(Qt::transparent);{QPainter p(&layer);p.setRenderHint(QPainter::SmoothPixmapTransform);p.drawPixmap(layer.rect(),art_);p.setCompositionMode(QPainter::CompositionMode_SourceAtop);const double span=size.width()+size.height(),x=-size.height()+glint_*span;QLinearGradient band(QPointF(x,0),QPointF(x+size.height()*.55,size.height()*.55));band.setColorAt(0,QColor(255,255,255,0));band.setColorAt(.5,QColor(255,255,255,120));band.setColorAt(1,QColor(255,255,255,0));p.fillRect(layer.rect(),band);}
        painter.drawImage(target,layer);
    }
private:
    double zoom_=1,glint_=0;
};
class FloatingAbout final:public QDialog {
    QPoint dragOffset_;bool dragging_=false,animated_=false,closing_=false;AboutSplash *art_=nullptr;
public:
    explicit FloatingAbout(QWidget *parent):QDialog(parent,Qt::Dialog|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint){
        setObjectName("about9xdDialog");setAttribute(Qt::WA_DeleteOnClose);setAttribute(Qt::WA_TranslucentBackground);setWindowTitle("About 9XD");auto icon=QGuiApplication::windowIcon();
        if(icon.isNull()){
#ifdef Q_OS_WIN
            icon=QIcon(":/branding/windows-runtime-cat.ico");
#else
            icon=QIcon(":/branding/logo-sticker.png");
#endif
        }
        setWindowIcon(icon);setStyleSheet("QDialog#about9xdDialog { background: transparent; }");auto *layout=new QVBoxLayout(this);layout->setContentsMargins(20,8,20,20);layout->setSpacing(2);
        auto *top=new QHBoxLayout;top->addStretch();auto *close=new QToolButton(this);close->setObjectName("aboutClose");close->setAccessibleName("Close About");close->setToolTip("Close (Esc)");close->setText(QString(QChar(0x00d7)));close->setFixedSize(28,28);close->setStyleSheet("QToolButton {color:white; background:transparent; border:0; font-size:26px; padding:0;} QToolButton:hover, QToolButton:focus {color:#c9a7ff; background:transparent;} QToolButton:pressed {color:#9a6cf0; background:transparent;}");close->setCursor(Qt::PointingHandCursor);auto *closeShadow=new QGraphicsDropShadowEffect(close);closeShadow->setBlurRadius(6);closeShadow->setOffset(0,1);closeShadow->setColor(QColor(0,0,0,200));close->setGraphicsEffect(closeShadow);top->addWidget(close);layout->addLayout(top);connect(close,&QToolButton::clicked,this,&QDialog::reject);
        auto *art=new AboutSplash(this);art_=art;art->installEventFilter(this);auto *shadow=new QGraphicsDropShadowEffect(art);shadow->setBlurRadius(16);shadow->setOffset(0,5);shadow->setColor(QColor(0,0,0,190));art->setGraphicsEffect(shadow);layout->addWidget(art,1);
        auto *thanks=new QLabel("Special thanks to Zuz, Nines, Taku, GPT and more.",this);thanks->setObjectName("aboutSpecialThanks");thanks->setWordWrap(true);thanks->setAlignment(Qt::AlignCenter);thanks->setStyleSheet("background:transparent; color:white; font-size:13px; padding:8px;");thanks->setTextInteractionFlags(Qt::TextSelectableByMouse);auto *textShadow=new QGraphicsDropShadowEffect(thanks);textShadow->setBlurRadius(8);textShadow->setOffset(1,2);textShadow->setColor(Qt::black);thanks->setGraphicsEffect(textShadow);layout->addWidget(thanks);
        // Centred on the screen that shows the main window (or the pointer), not on the window itself.
        auto *screen=parent&&parent->window()->screen()?parent->window()->screen():QGuiApplication::screenAt(QCursor::pos());if(!screen)screen=QGuiApplication::primaryScreen();auto available=screen->availableGeometry();int w=qMin(960,available.width()-40),h=qMin(650,available.height()-40);resize(w,h);move(available.center()-rect().center());
    }
    // Fade out instead of vanishing (Esc, the close button and accept/reject all end here).
    void done(int result)override{if(closing_)return;closing_=true;auto *fade=new QPropertyAnimation(this,"windowOpacity",this);fade->setDuration(170);fade->setStartValue(windowOpacity());fade->setEndValue(0.);fade->setEasingCurve(QEasingCurve::InCubic);connect(fade,&QPropertyAnimation::finished,this,[this,result]{QDialog::done(result);});fade->start(QAbstractAnimation::DeleteWhenStopped);}
protected:
    // Entrance: fade in while rising 28 px with a slight overshoot, the artwork settles from 92 % to full size,
    // then one light glint crosses the logo.
    void showEvent(QShowEvent *event)override{
        QDialog::showEvent(event);if(animated_)return;animated_=true;const QPoint end=pos(),start=end+QPoint(0,28);move(start);setWindowOpacity(0);
        auto *group=new QParallelAnimationGroup(this);
        auto *fade=new QPropertyAnimation(this,"windowOpacity");fade->setDuration(380);fade->setStartValue(0.);fade->setEndValue(1.);fade->setEasingCurve(QEasingCurve::OutCubic);group->addAnimation(fade);
        auto *rise=new QPropertyAnimation(this,"pos");rise->setDuration(560);rise->setStartValue(start);rise->setEndValue(end);rise->setEasingCurve(QEasingCurve::OutBack);group->addAnimation(rise);
        auto *zoom=new QVariantAnimation;zoom->setDuration(620);zoom->setStartValue(.92);zoom->setEndValue(1.);zoom->setEasingCurve(QEasingCurve::OutCubic);connect(zoom,&QVariantAnimation::valueChanged,this,[this](const QVariant &v){art_->setZoom(v.toDouble());});group->addAnimation(zoom);
        connect(group,&QParallelAnimationGroup::finished,this,[this]{auto *glint=new QVariantAnimation(this);glint->setDuration(900);glint->setStartValue(0.);glint->setEndValue(1.);glint->setEasingCurve(QEasingCurve::InOutSine);connect(glint,&QVariantAnimation::valueChanged,this,[this](const QVariant &v){art_->setGlint(v.toDouble());});connect(glint,&QVariantAnimation::finished,this,[this]{art_->setGlint(0);});glint->start(QAbstractAnimation::DeleteWhenStopped);});
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }
    bool eventFilter(QObject *object,QEvent *event)override{
        if(event->type()==QEvent::MouseButtonPress){auto *mouse=static_cast<QMouseEvent *>(event);if(mouse->button()==Qt::LeftButton){dragOffset_=mouse->globalPosition().toPoint()-pos();dragging_=true;return true;}}
        if(event->type()==QEvent::MouseMove&&dragging_){move(static_cast<QMouseEvent *>(event)->globalPosition().toPoint()-dragOffset_);return true;}
        if(event->type()==QEvent::MouseButtonRelease&&dragging_){dragging_=false;return true;}
        return QDialog::eventFilter(object,event);
    }
};
inline void showWorkbenchAbout(QWidget *parent){if(parent)if(auto *existing=parent->findChild<QDialog *>("about9xdDialog");existing&&existing->windowOpacity()>0){existing->show();existing->raise();existing->activateWindow();return;}auto *dialog=new FloatingAbout(parent);dialog->show();}
