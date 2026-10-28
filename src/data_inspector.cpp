#include "data_inspector.h"
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QApplication>
#include <QClipboard>
#include <QShortcut>
#include <QPushButton>
#include <QSignalBlocker>
#include <QDateTime>
#include <QTimeZone>

DataInspector::DataInspector(QWidget *parent):QWidget(parent){
    setObjectName("inspector");
    auto *layout=new QVBoxLayout(this);
    position_=new QLabel("Select a byte to interpret it.");position_->setWordWrap(true);position_->setMinimumHeight(position_->fontMetrics().lineSpacing()*3);layout->addWidget(position_);
    values_=new QTableWidget(0,2);values_->setObjectName("inspectorValues");values_->setHorizontalHeaderLabels({"Type","Value"});
    values_->verticalHeader()->hide();values_->setAlternatingRowColors(true);values_->setShowGrid(false);
    values_->setEditTriggers(QAbstractItemView::NoEditTriggers);values_->setSelectionBehavior(QAbstractItemView::SelectRows);
    values_->setSelectionMode(QAbstractItemView::SingleSelection);
    values_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    values_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    // Options sit between the anchor summary and the values so they stay visible when the table is long.
    auto *options=new QHBoxLayout;options->setContentsMargins(0,0,0,0);
    endian_=new QComboBox;endian_->setObjectName("inspectorEndian");endian_->addItems({"Little Endian","Big Endian"});endian_->setToolTip("Byte order used for the values below (synchronized with the Hex Viewer)");options->addWidget(endian_);
    hex_=new QCheckBox("Hex integers");hex_->setObjectName("inspectorHex");hex_->setToolTip("Show integers in hexadecimal (bit patterns)");options->addWidget(hex_);options->addStretch();
    auto *copy=new QPushButton("Copy Value");copy->setObjectName("inspectorCopy");copy->setProperty("compact",true);copy->setToolTip("Copy the selected value (Ctrl+C)");copy->setEnabled(false);options->addWidget(copy);
    layout->addLayout(options);layout->addWidget(values_,1);
    connect(values_,&QTableWidget::itemSelectionChanged,copy,[this,copy]{copy->setEnabled(values_->currentRow()>=0);});
    auto copyValue=[this]{int row=values_->currentRow();if(row>=0)QApplication::clipboard()->setText(values_->item(row,1)->text());};
    connect(copy,&QPushButton::clicked,this,copyValue);
    auto *shortcut=new QShortcut(QKeySequence::Copy,values_);shortcut->setContext(Qt::WidgetWithChildrenShortcut);connect(shortcut,&QShortcut::activated,this,copyValue);
    connect(hex_,&QCheckBox::toggled,this,[this]{refresh();});
    connect(endian_,&QComboBox::currentIndexChanged,this,[this](int index){refresh();emit endianChanged(index==1);});
    values_->setToolTip("Values start at the selection anchor, including subsequent bytes outside the selection. Ctrl+C: copy value. Hexadecimal integers show their bit patterns.");
}
void DataInspector::setPlainText(const QString &message){position_->setText(message);bytes_.clear();refresh();}
QString DataInspector::toPlainText()const{QString result=position_->text();for(int row=0;row<values_->rowCount();++row)result+='\n'+values_->item(row,0)->text()+": "+values_->item(row,1)->text();return result;}
void DataInspector::setData(const QByteArray &bytes,const ByteSelection &selection,qint64 fileSize,const QString &encoding,bool bigEndian){
    bytes_=bytes;encoding_=encoding;{QSignalBlocker block(endian_);endian_->setCurrentIndex(bigEndian?1:0);}
    position_->setText(QString("Anchor: 0x%1 · End: 0x%2\n%3 bytes · %4 · Stride: %5")
        .arg(selection.anchor,0,16).arg(selection.cursor,0,16).arg(selection.count(fileSize)).arg(selection.rectangular?"Rectangular":"Sequential").arg(selection.stride));refresh();
}
void DataInspector::refresh(){
    int row=0;bool big=endian_->currentIndex()==1;
    const QString missing="N/A · insufficient bytes";
    auto add=[&](const QString &type,const QString &value){if(row>=values_->rowCount())values_->insertRow(row);for(int column=0;column<2;++column){auto *item=values_->item(row,column);if(!item){item=new QTableWidgetItem;values_->setItem(row,column,item);}item->setText(column?value:type);item->setToolTip(column?value:type);item->setForeground(column&&value==missing?palette().brush(QPalette::Disabled,QPalette::Text):QBrush());}++row;};
    auto raw=[&](int width){quint64 n=0;for(int i=0;i<width;++i)n|=quint64(quint8(bytes_[i]))<<(8*(big?width-1-i:i));return n;};
    add("Binary (8 bits)",bytes_.isEmpty()?missing:QString::number(quint8(bytes_[0]),2).rightJustified(8,'0'));
    for(const auto &type:{"UInt8","Int8","UInt16","Int16","UInt32","Int32","UInt64","Int64","Float32","Float64"}){
        int width=numberWidth(type);QString value=missing;
        if(bytes_.size()>=width)value=hex_->isChecked()&&!QString(type).startsWith("Float")?"0x"+QString::number(raw(width),16).rightJustified(width*2,'0').toUpper():decodeNumber(bytes_,type,big);
        add(type,value);
    }
    add("ASCII",bytes_.isEmpty()?missing:decodeText(bytes_.first(1),"ASCII"));
    add("UTF-16",bytes_.size()<2?missing:decodeText(bytes_.first(2),big?"UTF-16BE":"UTF-16LE"));
    add("Text · "+encoding_,bytes_.isEmpty()?missing:decodeText(bytes_.first(qMin<qsizetype>(64,bytes_.size())),encoding_));
    auto utc=[](qint64 seconds){if(seconds < -62135596800LL || seconds > 253402300799LL)return QString("Out of range");return QDateTime::fromSecsSinceEpoch(seconds,QTimeZone::UTC).toString("yyyy-MM-dd HH:mm:ss 'UTC'");};
    add("Unix Int32",bytes_.size()<4?missing:utc(qint32(raw(4))));
    add("Unix UInt32",bytes_.size()<4?missing:utc(raw(4)));
    add("Unix Int64",bytes_.size()<8?missing:utc(qint64(raw(8))));
    add("FILETIME",bytes_.size()<8?missing:utc(qint64(raw(8)/10000000ULL)-11644473600LL));
    if(bytes_.size()<2){add("DOS date",missing);add("DOS time",missing);}else{
        quint16 n=raw(2);QDate date(1980+(n>>9),(n>>5)&15,n&31);QTime time(n>>11,(n>>5)&63,(n&31)*2);
        add("DOS date",date.isValid()?date.toString(Qt::ISODate):"Invalid");add("DOS time",time.isValid()?time.toString(Qt::ISODate):"Invalid");
    }
    add("RGB",bytes_.size()<3?missing:QString("%1, %2, %3 · #%4").arg(quint8(bytes_[0])).arg(quint8(bytes_[1])).arg(quint8(bytes_[2])).arg(QString::fromLatin1(bytes_.first(3).toHex().toUpper())));
}
void DataInspector::changeEvent(QEvent *event){QWidget::changeEvent(event);if(event->type()==QEvent::PaletteChange&&values_)refresh();}
