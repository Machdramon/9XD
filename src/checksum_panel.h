#pragma once
#include "adaptive_layout.h"
#include "hex_view.h"
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <QFontDatabase>

class ChecksumPanel final : public QWidget {
public:
    explicit ChecksumPanel(HexView *view,QWidget *parent=nullptr):QWidget(parent),view_(view){
        setObjectName("checksumPanel");auto *layout=new QVBoxLayout(this);layout->setContentsMargins(8,8,8,8);layout->setSpacing(6);
        auto *splitter=new QSplitter(Qt::Horizontal);splitter->setObjectName("checksumSplitter");layout->addWidget(splitter);auto *results=new QWidget;auto *resultsLayout=new QVBoxLayout(results);resultsLayout->setContentsMargins(0,0,0,0);auto *controls=new QWidget;auto *controlsLayout=new QVBoxLayout(controls);controlsLayout->setContentsMargins(0,0,0,0);// Settings first (left when wide, on top when narrow), results after.
        splitter->addWidget(controls);splitter->addWidget(results);splitter->setChildrenCollapsible(false);splitter->setStretchFactor(0,0);splitter->setStretchFactor(1,1);controls->setMinimumWidth(240);splitter->setSizes({320,650});AdaptiveLayout::flip(this,splitter);auto *box=new QWidget;auto *form=new QFormLayout(box);controlsLayout->addWidget(box);
        algorithm_=new QComboBox;algorithm_->setObjectName("checksumAlgorithm");for(const auto &name:checksumAlgorithms())algorithm_->addItem(name,name);
        algorithm_->addItem("All algorithms","all");algorithm_->setCurrentIndex(algorithm_->findData("SHA-256"));form->addRow("Algorithm",algorithm_);
        scope_=new QComboBox;scope_->setObjectName("checksumScope");scope_->addItems({"Entire document","Contiguous selection","Specified range"});form->addRow("Scope",scope_);
        start_=new QLineEdit("0");start_->setObjectName("checksumStart");start_->setMaxLength(20);
        length_=new QLineEdit("4096");length_->setObjectName("checksumLength");length_->setMaxLength(20);
        start_->setToolTip("Decimal or hexadecimal (0x...) offset.");length_->setToolTip("Number of bytes, in decimal or hexadecimal (0x...).");
        form->addRow("Start",start_);form->addRow("Bytes",length_);
        expected_=new QLineEdit;expected_->setObjectName("checksumExpected");expected_->setMaxLength(512);expected_->setPlaceholderText("Optional hex to verify");form->addRow("Expected",expected_);
        expectedHelp_=new QLabel;expectedHelp_->setObjectName("inspectorHint");expectedHelp_->setWordWrap(true);form->addRow(expectedHelp_);
        auto *buttons=new QHBoxLayout;calculate_=new QPushButton("Calculate");calculate_->setObjectName("checksumCalculate");cancel_=new QPushButton("Cancel");cancel_->setObjectName("checksumCancel");calculate_->setProperty("primary",true);cancel_->setProperty("compact",true);calculate_->setToolTip("Uses the current bytes, including unsaved changes. Verification does not modify the document.");buttons->addWidget(calculate_,1);buttons->addWidget(cancel_);controlsLayout->addLayout(buttons);
        progress_=new QProgressBar;progress_->setObjectName("checksumProgress");progress_->setRange(0,1000);progress_->setValue(0);progress_->setTextVisible(false);progress_->setMaximumHeight(8);controlsLayout->addWidget(progress_);controlsLayout->addStretch();
        table_=new QTableWidget(0,3);table_->setObjectName("checksumResults");table_->setHorizontalHeaderLabels({"Algorithm","Checksum / Hash","Verification"});table_->setEditTriggers(QAbstractItemView::NoEditTriggers);table_->setSelectionBehavior(QAbstractItemView::SelectRows);table_->setSelectionMode(QAbstractItemView::SingleSelection);table_->setAlternatingRowColors(true);table_->verticalHeader()->hide();
        table_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);table_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);table_->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);table_->setWordWrap(false);resultsLayout->addWidget(table_,1);
        auto *copyButtons=new QHBoxLayout;copy_=new QPushButton("Copy");copy_->setObjectName("checksumCopy");copy_->setToolTip("Copy the selected result");copyAll_=new QPushButton("Copy all");copyAll_->setObjectName("checksumCopyAll");for(auto *b:{copy_,copyAll_})b->setProperty("compact",true);copyButtons->addStretch();copyButtons->addWidget(copy_);copyButtons->addWidget(copyAll_);resultsLayout->addLayout(copyButtons);
        status_=new QLabel("Choose an algorithm and click Calculate.");status_->setObjectName("checksumStatus");status_->setWordWrap(true);resultsLayout->addWidget(status_);
        connect(calculate_,&QPushButton::clicked,this,[this]{startCalculation();});connect(cancel_,&QPushButton::clicked,view_,&HexView::cancelChecksum);
        connect(copy_,&QPushButton::clicked,this,[this]{int row=table_->currentRow();if(valid_&&row>=0)QApplication::clipboard()->setText(table_->item(row,1)->text());});
        connect(copyAll_,&QPushButton::clicked,this,[this]{if(!valid_)return;QString text="Algorithm\tChecksum / Hash\tVerification\n";for(int row=0;row<table_->rowCount();++row)text+=table_->item(row,0)->text()+"\t"+table_->item(row,1)->text()+"\t"+table_->item(row,2)->text()+"\n";QApplication::clipboard()->setText(text);});
        auto changed=[this]{if(!view_->checksumBusy()){clearResults();status_->setText("Options changed. Click Calculate to refresh.");}updateControls();};
        connect(algorithm_,&QComboBox::currentIndexChanged,this,changed);connect(scope_,&QComboBox::currentIndexChanged,this,changed);
        connect(start_,&QLineEdit::textChanged,this,changed);connect(length_,&QLineEdit::textChanged,this,changed);
        connect(expected_,&QLineEdit::textChanged,this,[this]{updateVerification();});
        connect(view_,&HexView::documentChanged,this,[this]{stale_=true;view_->cancelChecksum();clearResults();status_->setText("The content changed. Click Calculate to refresh.");});
        connect(view_,&HexView::checksumFinished,this,[this](const ReadResult &result){
            updateControls();if(stale_)return;
            if(!result.error.isEmpty()){status_->setText(result.error);return;}
            if(result.cancelled){status_->setText("Calculation canceled.");return;}
            table_->setRowCount(algorithms_.size());
            for(int row=0;row<algorithms_.size();++row){
                table_->setItem(row,0,new QTableWidgetItem(algorithms_[row]));auto *value=new QTableWidgetItem(result.checksums.value(algorithms_[row]));value->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));value->setToolTip(value->text());table_->setItem(row,1,value);table_->setItem(row,2,new QTableWidgetItem);
            }
            valid_=true;progress_->setValue(1000);table_->setCurrentCell(0,1);updateVerification();updateControls();
            status_->setText(QString("%1 algorithm(s) · %2 bytes from 0x%3 · current document content").arg(algorithms_.size()).arg(total_).arg(begin_,0,16));
        });
        auto *timer=new QTimer(this);timer->setInterval(150);connect(timer,&QTimer::timeout,this,[this]{
            updateControls();if(view_->checksumBusy()&&!stale_){const auto done=view_->checksumProgress();progress_->setValue(total_?int(1000.0L*done/total_):0);status_->setText(QString("Calculating: %1 / %2 bytes...").arg(done).arg(total_));}
        });timer->start();updateControls();updateVerification();
    }
private:
    QString expectedValue(bool &valid)const{
        auto text=expected_->text().trimmed();if(text.isEmpty()){valid=true;return {};}
        text.remove(QRegularExpression("\\s+"));if(text.startsWith("0x",Qt::CaseInsensitive))text.remove(0,2);
        valid=QRegularExpression("^[0-9A-Fa-f]+$").match(text).hasMatch();return text.toUpper();
    }
    void clearResults(){valid_=false;table_->setRowCount(0);progress_->setValue(0);copy_->setEnabled(false);copyAll_->setEnabled(false);}
    void updateControls(){
        const bool busy=view_->checksumBusy();algorithm_->setEnabled(!busy);scope_->setEnabled(!busy);start_->setEnabled(!busy&&scope_->currentIndex()==2);length_->setEnabled(!busy&&scope_->currentIndex()==2);
        // Start and Bytes only matter for a specified range, so the rows are shown only then.
        if(auto *form=qobject_cast<QFormLayout *>(start_->parentWidget()->layout())){const bool range=scope_->currentIndex()==2;form->setRowVisible(start_,range);form->setRowVisible(length_,range);}
        calculate_->setEnabled(!busy&&view_->hasFile()&&!view_->saving()&&!view_->directEditing());cancel_->setEnabled(busy);copy_->setEnabled(valid_);copyAll_->setEnabled(valid_);
    }
    void updateVerification(){
        bool valid;const auto expected=expectedValue(valid);
        expectedHelp_->setText(!valid?"Use hexadecimal digits (spaces and 0x are allowed).":expected.isEmpty()?QString():"Compared with the selected algorithm; it does not identify the format.");expectedHelp_->setVisible(!expectedHelp_->text().isEmpty());
        for(int row=0;row<table_->rowCount();++row){const auto actual=table_->item(row,1)->text();table_->item(row,2)->setText(!valid?"Invalid expected result":expected.isEmpty()?"Not compared":expected.size()!=actual.size()?"Different length":expected==actual?"Match":"Mismatch");}
    }
    void startCalculation(){
        if(view_->checksumBusy())return;
        ChecksumSpec spec;spec.length=view_->fileSize();
        if(scope_->currentIndex()==1){const auto selection=view_->selection();if(selection.anchor<0||selection.rectangular){status_->setText("Select a contiguous region first.");return;}spec.begin=qMin(selection.anchor,selection.cursor);spec.length=qAbs(selection.cursor-selection.anchor)+1;}
        if(scope_->currentIndex()==2){auto parse=[](QString text,bool &ok){text=text.trimmed();return text.startsWith("0x",Qt::CaseInsensitive)?text.mid(2).toLongLong(&ok,16):text.toLongLong(&ok,10);};bool beginOk,lengthOk;spec.begin=parse(start_->text(),beginOk);spec.length=parse(length_->text(),lengthOk);if(!beginOk||!lengthOk){status_->setText("Start and Bytes must be valid decimal or hexadecimal numbers.");return;}}
        if(spec.begin<0||spec.length<0||spec.begin>view_->fileSize()||spec.length>view_->fileSize()-spec.begin){status_->setText("The checksum region is outside the document.");return;}
        spec.algorithms=algorithm_->currentData().toString()=="all"?checksumAlgorithms():QStringList{algorithm_->currentData().toString()};
        bool expectedOk;const auto expected=expectedValue(expectedOk);bool lengthOk=expected.isEmpty();for(const auto &name:spec.algorithms)lengthOk=lengthOk||checksumHexLength(name)==expected.size();
        if(!expectedOk||!lengthOk){status_->setText("The expected result must be hexadecimal and have the correct length for an algorithm being calculated.");return;}
        clearResults();algorithms_=spec.algorithms;begin_=spec.begin;total_=spec.length;stale_=false;
        if(!view_->startChecksum(spec)){stale_=true;status_->setText("Open a document and wait for saving or direct editing to finish.");}
        else status_->setText("Calculating...");updateControls();
    }
    HexView *view_;QComboBox *algorithm_,*scope_;QLineEdit *start_,*length_,*expected_;QLabel *expectedHelp_,*status_;
    QPushButton *calculate_,*cancel_,*copy_,*copyAll_;QProgressBar *progress_;QTableWidget *table_;
    QStringList algorithms_;qint64 begin_=0,total_=0;bool stale_=true,valid_=false;
};
