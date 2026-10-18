#include "entropy_panel.h"
#include "hex_view.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QToolTip>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <algorithm>
class EntropyGraph final:public QWidget {
public:
    QList<EntropyPoint> points;HexView *view;
    explicit EntropyGraph(HexView *v):view(v){setObjectName("entropyGraph");setMinimumSize(260,180);setMouseTracking(true);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);}
    QRectF plot()const{return QRectF(38,22,qMax(1,width()-52),qMax(1,height()-55));}
    int indexAt(double x)const{if(points.isEmpty())return -1;auto r=plot();double f=qBound(0.0,(x-r.left())/r.width(),1.0);qint64 begin=points.first().offset,end=points.last().offset+points.last().length;auto offset=begin+qint64(f*double(end-begin-1));auto it=std::upper_bound(points.begin(),points.end(),offset,[](qint64 a,const EntropyPoint &b){return a<b.offset;});return qMax(0,int(it-points.begin())-1);}
protected:
    void paintEvent(QPaintEvent *)override{
        QPainter p(this);p.fillRect(rect(),palette().base());p.setRenderHint(QPainter::Antialiasing);auto r=plot();p.setPen(palette().text().color());p.drawText(8,14,"Entropy · bits/byte (0–8)");
        for(int n=0;n<=8;n+=2){double y=r.bottom()-r.height()*n/8.;p.setPen(palette().mid().color());p.drawLine(QPointF(r.left(),y),QPointF(r.right(),y));p.setPen(palette().text().color());p.drawText(QRectF(0,y-8,30,16),Qt::AlignRight,QString::number(n));}
        if(points.isEmpty()){p.drawText(r,Qt::AlignCenter,"Click Analyze to generate the graph");return;}
        qint64 begin=points.first().offset,end=points.last().offset+points.last().length;QPainterPath path;
        // Preserve every window rather than smoothing away narrow transitions.
        for(int i=0;i<points.size();++i){auto point=points[i];double x=r.left()+r.width()*double(point.offset-begin)/double(end-begin);double right=r.left()+r.width()*double(point.offset+point.length-begin)/double(end-begin);double y=r.bottom()-r.height()*point.entropy/8.;if(i==0)path.moveTo(x,y);else path.lineTo(x,y);path.lineTo(right,y);}
        p.save();p.setClipRect(r.adjusted(-1,-1,1,1));p.setPen(QPen(QColor("#428DDB"),1.3));p.drawPath(path);p.restore();
        p.setPen(palette().text().color());p.drawText(QRectF(r.left(),r.bottom()+5,r.width()/2,20),Qt::AlignLeft,"0x"+QString::number(begin,16).toUpper());p.drawText(QRectF(r.center().x(),r.bottom()+5,r.width()/2,20),Qt::AlignRight,"0x"+QString::number(end-1,16).toUpper());
    }
    void mouseMoveEvent(QMouseEvent *event)override{int i=indexAt(event->position().x());if(i<0||!plot().contains(event->position()))return;auto v=points[i];QToolTip::showText(event->globalPosition().toPoint(),QString("0x%1 · %2 bytes\nEntropy: %3 / 8\nZeros: %4 % · Printable ASCII: %5 %\nClick to select this region").arg(v.offset,0,16).arg(v.length).arg(v.entropy,0,'f',3).arg(v.zeros,0,'f',1).arg(v.printable,0,'f',1),this);}
    void mousePressEvent(QMouseEvent *event)override{int i=indexAt(event->position().x());if(event->button()==Qt::LeftButton&&i>=0&&plot().contains(event->position()))view->selectMatch(points[i].offset,points[i].length);}
};
EntropyPanel::EntropyPanel(HexView *view,QWidget *parent):QWidget(parent){
    setObjectName("entropyPanel");auto *layout=new QVBoxLayout(this);auto *controls=new QHBoxLayout;layout->addLayout(controls);
    auto *scope=new QComboBox;scope->setObjectName("entropyScope");scope->addItems({"Entire file","Contiguous selection"});controls->addWidget(scope);
    auto *window=new QComboBox;window->setObjectName("entropyWindow");for(int n:{256,1024,4096,16384,65536,1048576})window->addItem(n<1024?QString("%1 B").arg(n):QString("%1 KiB").arg(n/1024),n);window->setCurrentIndex(2);window->setToolTip("Minimum region size. Increased for large files to limit the graph to 8192 points.");controls->addWidget(window);
    auto *start=new QPushButton("Analyze");start->setObjectName("entropyAnalyze");controls->addWidget(start);auto *cancel=new QPushButton("Cancel");controls->addWidget(cancel);connect(cancel,&QPushButton::clicked,view,&HexView::cancelEntropy);
    auto *graph=new EntropyGraph(view);layout->addWidget(graph,1);auto *status=new QLabel("Each point summarizes a region. High entropy does not prove compression or encryption.");status->setObjectName("entropyStatus");status->setWordWrap(true);layout->addWidget(status);
    auto stale=std::make_shared<bool>(true);auto total=std::make_shared<qint64>(0);
    connect(start,&QPushButton::clicked,this,[=]{
        if(view->entropyBusy())return;EntropySpec spec;spec.window=window->currentData().toLongLong();spec.length=view->fileSize();
        if(scope->currentIndex()==1){auto s=view->selection();if(s.anchor<0||s.rectangular){status->setText("Select a contiguous region.");return;}spec.begin=qMin(s.anchor,s.cursor);spec.length=qAbs(s.cursor-s.anchor)+1;}
        graph->points.clear();graph->update();*total=spec.length;*stale=!view->startEntropy(spec);status->setText(*stale?"Open a nonempty file and wait for saving to finish.":"Analyzing…");
    });
    connect(view,&HexView::documentChanged,this,[=]{*stale=true;view->cancelEntropy();graph->points.clear();graph->update();status->setText("The content changed. Click Analyze to refresh the graph.");});
    connect(view,&HexView::entropyFinished,this,[=](const ReadResult &r){if(*stale)return;if(!r.error.isEmpty()){status->setText(r.error);return;}if(r.cancelled){status->setText("Analysis canceled.");return;}graph->points=r.entropy;graph->update();status->setText(QString("%1 regions · effective window %2 bytes. Hover for values; click to navigate. High entropy does not prove compression.").arg(r.entropy.size()).arg(r.analysisWindow));});
    auto *timer=new QTimer(this);timer->setInterval(200);connect(timer,&QTimer::timeout,this,[=]{start->setEnabled(view->fileSize()>0&&!view->saving()&&!view->entropyBusy());cancel->setEnabled(view->entropyBusy());if(view->entropyBusy()&&!*stale)status->setText(QString("Analyzing %1 / %2 MiB…").arg(double(view->entropyProgress())/1048576,0,'f',1).arg(double(*total)/1048576,0,'f',1));});timer->start();
}
