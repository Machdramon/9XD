#pragma once
#include <QAbstractButton>
#include <QApplication>
#include <QEvent>
#include <QHash>
#include <QIcon>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QToolButton>
#include <QtMath>

// Line icons drawn in code on a 24×24 grid. They take their color from the current
// palette when painted, so they follow theme changes and need no image plugins.
class WorkbenchIconEngine final : public QIconEngine {
public:
    explicit WorkbenchIconEngine(QString name,QColor fixed={}):name_(std::move(name)),fixed_(std::move(fixed)){}
    void paint(QPainter *painter,const QRect &rect,QIcon::Mode mode,QIcon::State)override{
        const auto palette=qApp->palette();
        QColor color=mode==QIcon::Disabled?palette.color(QPalette::Disabled,QPalette::ButtonText):palette.color(QPalette::ButtonText);
        if(mode==QIcon::Selected)color=palette.color(QPalette::HighlightedText);
        // Primary buttons ask for white; follow the theme's readable colour on the accent instead (dark on olive).
        if(fixed_.isValid()&&mode!=QIcon::Disabled){color=fixed_;if(fixed_==QColor(Qt::white)){QColor on(qApp->property("workbenchOnPrimary").toString());if(on.isValid())color=on;}}
        painter->save();painter->setRenderHint(QPainter::Antialiasing);
        const qreal scale=qMin(rect.width(),rect.height())/24.0;
        painter->translate(rect.x()+(rect.width()-24*scale)/2,rect.y()+(rect.height()-24*scale)/2);painter->scale(scale,scale);
        QPen pen(color,1.8);pen.setCapStyle(Qt::RoundCap);pen.setJoinStyle(Qt::RoundJoin);painter->setPen(pen);painter->setBrush(Qt::NoBrush);
        draw(painter,color);
        painter->restore();
    }
    QPixmap pixmap(const QSize &size,QIcon::Mode mode,QIcon::State state)override{return scaledPixmap(size,mode,state,1.0);}
    QPixmap scaledPixmap(const QSize &size,QIcon::Mode mode,QIcon::State state,qreal scale)override{
        QPixmap pixmap(size*scale);pixmap.setDevicePixelRatio(scale);pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);paint(&painter,QRect(QPoint(),size),mode,state);return pixmap;
    }
    QIconEngine *clone()const override{return new WorkbenchIconEngine(name_,fixed_);}
    QString key()const override{return "workbench";}
private:
    void draw(QPainter *p,const QColor &color)const{
        auto lines=[&](std::initializer_list<QLineF> list){for(const auto &line:list)p->drawLine(line);};
        auto poly=[&](std::initializer_list<QPointF> points){p->drawPolyline(QList<QPointF>(points));};
        auto filled=[&](const QPainterPath &path){p->save();p->setBrush(color);p->drawPath(path);p->restore();};
        const auto &n=name_;
        if(n=="hex"){p->drawRoundedRect(QRectF(3,4,18,16),2.5,2.5);lines({{7,9,9,9},{11,9,13,9},{15,9,17,9},{7,13,9,13},{11,13,13,13},{15,13,17,13},{7,16.5,13,16.5}});}
        else if(n=="cube"){p->drawPolygon(QPolygonF({{12,3},{20,7.5},{20,16.5},{12,21},{4,16.5},{4,7.5}}));lines({{12,12,20,7.5},{12,12,12,21},{12,12,4,7.5}});}
        else if(n=="image"){p->drawRoundedRect(QRectF(3,4,18,16),2.5,2.5);p->drawEllipse(QPointF(9,10),2,2);poly({{21,16},{16,11},{7,20}});}
        else if(n=="code"){poly({{8,7},{3,12},{8,17}});poly({{16,7},{21,12},{16,17}});lines({{14,4,10,20}});}
        else if(n=="tools"){p->drawRoundedRect(QRectF(5,3,14,18),2.5,2.5);lines({{8.5,7.5,15.5,7.5},{8.5,12,10,12},{11.25,12,12.75,12},{14,12,15.5,12},{8.5,16,10,16},{11.25,16,12.75,16},{14,16,15.5,16}});}
        else if(n=="scan"){poly({{4,8},{4,4},{8,4}});poly({{16,4},{20,4},{20,8}});poly({{20,16},{20,20},{16,20}});poly({{8,20},{4,20},{4,16}});lines({{4,12,20,12}});}
        else if(n=="play"){QPainterPath path;path.moveTo(8,5.5);path.lineTo(19,12);path.lineTo(8,18.5);path.closeSubpath();filled(path);}
        else if(n=="stop"){QPainterPath path;path.addRoundedRect(QRectF(7,7,10,10),2,2);filled(path);}
        else if(n=="chart"){poly({{3,17},{8,11},{12,14},{16,7},{21,12}});lines({{3,21,21,21}});}
        else if(n=="search"){p->drawEllipse(QPointF(11,11),7,7);lines({{20,20,16,16}});}
        else if(n=="send"){lines({{21,3,10,14}});p->drawPolygon(QPolygonF({{21,3},{14,21},{10,14},{3,10}}));}
        else if(n=="check"){poly({{5,12.5},{10,17},{19,7}});}
        else if(n=="folder"){QPainterPath path;path.moveTo(3,7);path.quadTo(3,5,5,5);path.lineTo(9,5);path.lineTo(11,7);path.lineTo(19,7);path.quadTo(21,7,21,9);path.lineTo(21,17);path.quadTo(21,19,19,19);path.lineTo(5,19);path.quadTo(3,19,3,17);path.closeSubpath();p->drawPath(path);}
        else if(n=="save"){p->drawRoundedRect(QRectF(4,4,16,16),2,2);p->drawRect(QRectF(8,4,8,5));p->drawRect(QRectF(7,13,10,7));}
        else if(n=="trash"){lines({{4,7,20,7}});poly({{9,7},{9,4.5},{15,4.5},{15,7}});poly({{6,7},{7,20},{17,20},{18,7}});lines({{10,11,10,16},{14,11,14,16}});}
        else if(n=="cursor"){p->drawPolygon(QPolygonF({{5,3},{19,11},{13,13},{11,19}}));}
        else if(n=="move"){lines({{12,3,12,21},{3,12,21,12}});poly({{9,6},{12,3},{15,6}});poly({{9,18},{12,21},{15,18}});poly({{6,9},{3,12},{6,15}});poly({{18,9},{21,12},{18,15}});}
        else if(n=="rotate"){p->drawArc(QRectF(4,4,16,16),60*16,300*16);poly({{19,3.5},{19,8.5},{14,8.5}});}
        else if(n=="scale"){p->drawRect(QRectF(4,13,7,7));poly({{14,4},{20,4},{20,10}});lines({{20,4,12,12}});}
        else if(n=="lens"){p->drawPolygon(QPolygonF({{3,12},{20,5},{20,19}}));p->drawArc(QRectF(-4,4,16,16),-35*16,70*16);}
        else if(n=="eye"){QPainterPath path;path.moveTo(2,12);path.quadTo(12,2,22,12);path.quadTo(12,22,2,12);p->drawPath(path);p->drawEllipse(QPointF(12,12),3,3);}
        else if(n=="layers"){p->drawPolygon(QPolygonF({{12,4},{21,9},{12,14},{3,9}}));poly({{3,13},{12,18},{21,13}});}
        else if(n=="gear"){p->drawEllipse(QPointF(12,12),3,3);for(int k=0;k<8;++k){const qreal a=qDegreesToRadians(45.0*k);p->drawLine(QPointF(12+5.5*qCos(a),12+5.5*qSin(a)),QPointF(12+8.5*qCos(a),12+8.5*qSin(a)));}p->drawEllipse(QPointF(12,12),5.5,5.5);}
        else if(n=="sun"){p->drawEllipse(QPointF(12,12),4,4);for(int i=0;i<8;++i){const qreal a=qDegreesToRadians(45.0*i);p->drawLine(QPointF(12+7*qCos(a),12+7*qSin(a)),QPointF(12+9.5*qCos(a),12+9.5*qSin(a)));}}
        else if(n=="frame"){poly({{4,9},{4,4},{9,4}});poly({{15,4},{20,4},{20,9}});poly({{20,15},{20,20},{15,20}});poly({{9,20},{4,20},{4,15}});p->drawEllipse(QPointF(12,12),3,3);}
        else if(n=="select"){QPen pen=p->pen();pen.setDashPattern({2,2});p->setPen(pen);p->drawRoundedRect(QRectF(4,4,16,16),2,2);}
        else if(n=="plus"){lines({{12,5,12,19},{5,12,19,12}});}
        else if(n=="minus"){lines({{5,12,19,12}});}
        else if(n=="up"){poly({{6,14.5},{12,8.5},{18,14.5}});}
        else if(n=="down"){poly({{6,9.5},{12,15.5},{18,9.5}});}
        else if(n=="film"){p->drawRoundedRect(QRectF(3,5,18,14),2,2);lines({{7.5,5,7.5,19},{16.5,5,16.5,19},{3,9.5,7.5,9.5},{3,14.5,7.5,14.5},{16.5,9.5,21,9.5},{16.5,14.5,21,14.5}});}
        else if(n=="curve"){QPainterPath path;path.moveTo(4,18);path.cubicTo(8,3,16,21,20,6);p->drawPath(path);QPainterPath dots;dots.addEllipse(QPointF(4,18),2,2);dots.addEllipse(QPointF(20,6),2,2);filled(dots);}
        else if(n=="chat"){QPainterPath path;path.moveTo(4,6);path.quadTo(4,4,6,4);path.lineTo(18,4);path.quadTo(20,4,20,6);path.lineTo(20,15);path.quadTo(20,17,18,17);path.lineTo(10,17);path.lineTo(5,21);path.lineTo(6,17);path.quadTo(4,17,4,15);path.closeSubpath();p->drawPath(path);}
    }
    QString name_;QColor fixed_;
};

inline QIcon workbenchIcon(const QString &name,const QColor &fixed={}){return QIcon(new WorkbenchIconEngine(name,fixed));}

// The main action of each panel: accent-colored in the theme, with a white icon.
inline const QStringList &workbenchPrimaryButtons(){static const QStringList names{"hexScanFile","scanStart","scanFor3dData","scanForTexturesImages","renderMesh","scriptRun","aiSend","applyMeshDelta","magicScan","entropyAnalyze"};return names;}

// Gives known buttons their icon when they are first polished, so panels need no changes.
class WorkbenchIconDecorator final : public QObject {
public:
    using QObject::QObject;
    static void install(){static bool installed=false;if(installed||!qApp)return;installed=true;qApp->installEventFilter(new WorkbenchIconDecorator(qApp));}
    static const QHash<QString,QString> &icons(){static const QHash<QString,QString> map{
        {"hexScanFile","scan"},{"scanStart","scan"},{"scanFor3dData","scan"},{"scanForTexturesImages","scan"},
        {"renderMesh","play"},{"scriptRun","play"},{"scriptStop","stop"},{"scriptOpen","folder"},{"scriptSave","save"},{"scriptClear","trash"},
        {"aiSend","send"},{"applyMeshDelta","check"},{"removeMesh","trash"},{"magicScan","search"},{"entropyAnalyze","chart"},
        {"viewportSelect","cursor"},{"viewportMove","move"},{"viewportRotate","rotate"},{"viewportScale","scale"},
        {"viewportViewMenu","eye"},{"viewportCameraButton","lens"},{"viewportDisplayMenu","layers"},{"viewportLightingMenu","sun"},{"viewportFrameMenu","frame"},{"viewportSelectMenu","select"}
    };return map;}
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        if(event->type()==QEvent::Polish&&object->isWidgetType()&&!object->objectName().isEmpty()){
            if(auto *button=qobject_cast<QAbstractButton *>(object);button&&button->icon().isNull()){
                const auto name=icons().value(button->objectName());
                if(!name.isEmpty()){
                    button->setIcon(workbenchIcon(name,workbenchPrimaryButtons().contains(button->objectName())?QColor(Qt::white):QColor()));
                    if(auto *tool=qobject_cast<QToolButton *>(button);tool&&tool->popupMode()==QToolButton::InstantPopup&&!tool->property("iconOnly").toBool())tool->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                }
            }
        }
        return QObject::eventFilter(object,event);
    }
};
