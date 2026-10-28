#pragma once
#include "adaptive_layout.h"
#include <QToolButton>
#include "file_dialogs.h"
#include "hex_view.h"
#include "compare_data.h"
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QFileDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QTimer>
class ComparePanel final:public QWidget {
public:
    explicit ComparePanel(HexView *view,QWidget *parent=nullptr):QWidget(parent),view_(view){
        // Reference file and Compare on top, differences below (beside when the panel is wide), compact navigation.
        setObjectName("comparePanel");auto *outer=new QHBoxLayout(this);outer->setContentsMargins(8,8,8,8);outer->setSpacing(10);
        auto *setup=new QWidget;auto *layout=new QVBoxLayout(setup);layout->setContentsMargins(0,0,0,0);layout->setSpacing(6);
        auto *row=new QHBoxLayout;row->setSpacing(4);path_=new QLineEdit;path_->setObjectName("comparePath");path_->setPlaceholderText("Reference file");
        path_->setToolTip("Compared byte by byte at the same offsets with the current document (including unsaved changes). Insertions and deletions are not realigned.");row->addWidget(path_,1);
        auto *browse=new QToolButton;browse->setText(QString(QChar(0x2026)));browse->setToolTip("Choose reference file");browse->setAccessibleName("Choose reference file");browse->setProperty("keepTooltip",true);browse->setProperty("compactTool",true);row->addWidget(browse);layout->addLayout(row);
        connect(browse,&QToolButton::clicked,this,[this]{auto path=WorkbenchFiles::getOpenFileName(this,"Choose reference file");if(!path.isEmpty())path_->setText(path);});
        auto *buttons=new QHBoxLayout;buttons->setSpacing(4);start_=new QPushButton("Compare");start_->setObjectName("compareStart");start_->setProperty("primary",true);cancel_=new QPushButton("Cancel");cancel_->setEnabled(false);cancel_->setProperty("compact",true);buttons->addWidget(start_,1);buttons->addWidget(cancel_);layout->addLayout(buttons);
        progress_=new QProgressBar;progress_->setRange(0,1000);progress_->setTextVisible(false);progress_->setMaximumHeight(8);layout->addWidget(progress_);
        status_=new QLabel("Choose a reference file.");status_->setObjectName("compareStatus");status_->setWordWrap(true);layout->addWidget(status_);layout->addStretch();
        auto *found=new QWidget;auto *foundLayout=new QVBoxLayout(found);foundLayout->setContentsMargins(0,0,0,0);foundLayout->setSpacing(4);
        results_=new QTreeWidget;results_->setObjectName("compareResults");results_->setHeaderLabels({"Offset","Bytes","Current / Reference"});results_->setRootIsDecorated(false);foundLayout->addWidget(results_,1);
        auto *navigation=new QHBoxLayout;navigation->setSpacing(2);
        auto tool=[navigation](QString text,QString tip){auto *b=new QToolButton;b->setText(text);b->setToolTip(tip);b->setAccessibleName(tip);b->setProperty("keepTooltip",true);b->setProperty("compactTool",true);navigation->addWidget(b);return b;};
        auto *previous=tool("↑","Previous difference"),*next=tool("↓","Next difference");navigation->addStretch();auto *copy=tool("Copy","Copy results");foundLayout->addLayout(navigation);
        outer->addWidget(setup,2);outer->addWidget(found,3);AdaptiveLayout::flipColumns(this,outer);
        connect(start_,&QPushButton::clicked,this,[this]{startComparison();});connect(cancel_,&QPushButton::clicked,this,[this]{if(state_)state_->cancel.store(true);});
        connect(previous,&QToolButton::clicked,this,[this]{step(-1);});connect(next,&QToolButton::clicked,this,[this]{step(1);});
        connect(results_,&QTreeWidget::currentItemChanged,this,[this](QTreeWidgetItem *item){if(!item||stale_)return;int i=results_->indexOfTopLevelItem(item);auto range=result_.ranges[i];if(range.offset<view_->fileSize())view_->selectMatch(range.offset,qMin(range.length,view_->fileSize()-range.offset));else status_->setText("These bytes exist only in the reference file; no corresponding byte in the current document.");});
        connect(copy,&QToolButton::clicked,this,[this]{if(stale_)return;QString text="Offset\tBytes\n";for(auto range:result_.ranges)text+=QString("0x%1\t%2\n").arg(range.offset,0,16).arg(range.length);QApplication::clipboard()->setText(text);});
        auto invalidate=[this]{stale_=true;if(state_)state_->cancel.store(true);results_->clear();status_->setText("Source changed; compare again.");};
        connect(path_,&QLineEdit::textChanged,this,invalidate);connect(view_,&HexView::documentChanged,this,invalidate);
        connect(&watcher_,&QFutureWatcher<CompareResult>::finished,this,[this]{
            start_->setEnabled(true);cancel_->setEnabled(false);path_->setEnabled(true);if(stale_)return;result_=watcher_.result();
            if(!result_.error.isEmpty()){status_->setText(result_.error);return;}if(result_.cancelled){status_->setText("Comparison canceled.");return;}
            progress_->setValue(1000);results_->clear();for(auto range:result_.ranges)new QTreeWidgetItem(results_,{QString("0x%1").arg(range.offset,0,16),QString::number(range.length),range.offset>=leftSize_?"Missing / Present":range.offset>=rightSize_?"Present / Missing":"Different"});
            status_->setText(QString("%1 differing bytes in %2 ranges. Sizes: %3 / %4 bytes.%5").arg(result_.differentBytes).arg(result_.ranges.size()).arg(leftSize_).arg(rightSize_).arg(result_.truncated?" Result limit reached; comparison incomplete.":result_.ranges.isEmpty()?" Identical.":""));
        });
        auto *timer=new QTimer(this);timer->setInterval(100);connect(timer,&QTimer::timeout,this,[this]{if(watcher_.isRunning()&&state_)progress_->setValue(int(double(state_->progress.load())/qMax(qint64(1),qMax(leftSize_,rightSize_))*1000));});timer->start();
    }
    ~ComparePanel()override{if(state_)state_->cancel.store(true);}
    void step(int direction){if(stale_||results_->topLevelItemCount()==0)return;int row=results_->indexOfTopLevelItem(results_->currentItem());row=qBound(0,row<0?0:row+direction,results_->topLevelItemCount()-1);results_->setCurrentItem(results_->topLevelItem(row));}
    void startComparison(){
        if(watcher_.isRunning())return;if(!view_->hasFile()||view_->saving()||view_->directEditing()){status_->setText("Wait for editing or saving to finish.");return;}
        auto right=readPage({path_->text(),0,-1,{},0});if(!right.error.isEmpty()){status_->setText(right.error);return;}
        auto left=view_->sourceRequest(0,0);ReadRequest reference{path_->text(),0,right.size,right.modified,0};leftSize_=left.expectedSize;rightSize_=right.size;
        auto patches=view_->pendingPatches();state_=std::make_shared<CompareState>();stale_=false;result_={};results_->clear();start_->setEnabled(false);cancel_->setEnabled(true);path_->setEnabled(false);progress_->setValue(0);status_->setText("Comparing...");
        watcher_.setFuture(QtConcurrent::run([left,reference,patches,state=state_]{return compareDocuments(left,reference,patches,*state);}));
    }
private:
    HexView *view_;QLineEdit *path_;QPushButton *start_,*cancel_;QProgressBar *progress_;QTreeWidget *results_;QLabel *status_;
    QFutureWatcher<CompareResult> watcher_;std::shared_ptr<CompareState> state_;CompareResult result_;bool stale_=true;qint64 leftSize_=0,rightSize_=0;
};
