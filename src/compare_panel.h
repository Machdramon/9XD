#pragma once
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
        setObjectName("comparePanel");auto *layout=new QVBoxLayout(this);
        auto *hint=new QLabel("Compare the current document (including unsaved changes) with a file, byte by byte at the same offsets. Insertions and deletions are not realigned.");hint->setWordWrap(true);layout->addWidget(hint);
        auto *row=new QHBoxLayout;path_=new QLineEdit;path_->setObjectName("comparePath");path_->setPlaceholderText("Reference file");row->addWidget(path_);auto *browse=new QPushButton("Open file...");row->addWidget(browse);layout->addLayout(row);
        connect(browse,&QPushButton::clicked,this,[this]{auto path=QFileDialog::getOpenFileName(this,"Choose reference file");if(!path.isEmpty())path_->setText(path);});
        auto *buttons=new QHBoxLayout;start_=new QPushButton("Compare");start_->setObjectName("compareStart");cancel_=new QPushButton("Cancel");cancel_->setEnabled(false);buttons->addWidget(start_);buttons->addWidget(cancel_);layout->addLayout(buttons);
        progress_=new QProgressBar;progress_->setRange(0,1000);layout->addWidget(progress_);
        results_=new QTreeWidget;results_->setObjectName("compareResults");results_->setHeaderLabels({"Offset","Bytes","Current / Reference"});results_->setRootIsDecorated(false);layout->addWidget(results_,1);
        auto *navigation=new QHBoxLayout;auto *previous=new QPushButton("Previous difference"),*next=new QPushButton("Next difference"),*copy=new QPushButton("Copy results");navigation->addWidget(previous);navigation->addWidget(next);navigation->addWidget(copy);layout->addLayout(navigation);
        status_=new QLabel("Choose a reference file.");status_->setObjectName("compareStatus");status_->setWordWrap(true);layout->addWidget(status_);
        connect(start_,&QPushButton::clicked,this,[this]{startComparison();});connect(cancel_,&QPushButton::clicked,this,[this]{if(state_)state_->cancel.store(true);});
        connect(previous,&QPushButton::clicked,this,[this]{step(-1);});connect(next,&QPushButton::clicked,this,[this]{step(1);});
        connect(results_,&QTreeWidget::currentItemChanged,this,[this](QTreeWidgetItem *item){if(!item||stale_)return;int i=results_->indexOfTopLevelItem(item);auto range=result_.ranges[i];if(range.offset<view_->fileSize())view_->selectMatch(range.offset,qMin(range.length,view_->fileSize()-range.offset));else status_->setText("These bytes exist only in the reference file; no corresponding byte in the current document.");});
        connect(copy,&QPushButton::clicked,this,[this]{if(stale_)return;QString text="Offset\tBytes\n";for(auto range:result_.ranges)text+=QString("0x%1\t%2\n").arg(range.offset,0,16).arg(range.length);QApplication::clipboard()->setText(text);});
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
