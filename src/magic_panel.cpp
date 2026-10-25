#include "empty_state.h"
#include "file_dialogs.h"
#include "magic_panel.h"
#include "hex_view.h"
#include "format_panel.h"
#include <QTreeWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QSaveFile>
#include <QTimer>
#include <QSignalBlocker>
#include <algorithm>

MagicPanel::MagicPanel(HexView *view,FormatPanel *format,QWidget *parent):QWidget(parent),view_(view),format_(format){
    setObjectName("magicPanel");auto *layout=new QVBoxLayout(this);
    auto *intro=new QLabel("Supported signatures: a match is a clue; it does not validate the format or determine where it ends. Choose colors in Formatting Rules.");intro->setWordWrap(true);layout->addWidget(intro);
    auto *body=new QHBoxLayout;layout->addLayout(body);
    auto *left=new QVBoxLayout;body->addLayout(left,1);
    libraryList_=new QTreeWidget;libraryList_->setObjectName("magicLibrary");libraryList_->setHeaderLabels({"Active signature","Bytes","Offset"});libraryList_->setRootIsDecorated(false);left->addWidget(libraryList_);
    auto *buttons=new QHBoxLayout;left->addLayout(buttons);
    auto button=[&](QString label,auto callback){auto *b=new QPushButton(label);buttons->addWidget(b);connect(b,&QPushButton::clicked,this,callback);return b;};
    button("+",[this]{edit(true);});button("−",[this]{int row=libraryList_->indexOfTopLevelItem(libraryList_->currentItem());if(row<0)return;invalidate();library_.removeAt(row);dirty_=true;rebuild();});button("Edit",[this]{edit(false);});
    button("Import",[this]{
        if(!confirmClose())return;auto path=WorkbenchFiles::getOpenFileName(this,"Import Signature Library",{},"JSON library (*.json)");if(path.isEmpty())return;
        QFile file(path);QString error;QList<MagicSignature> loaded;
        if(!file.open(QIODevice::ReadOnly)||!decodeMagicLibrary(file.read(1024*1024+1),loaded,error)){QMessageBox::warning(this,"Import",error.isEmpty()?file.errorString():error);return;}
        invalidate();library_=std::move(loaded);dirty_=false;rebuild();
    });button("Export",[this]{exportLibrary();});
    connect(libraryList_,&QTreeWidget::itemDoubleClicked,this,[this](auto *,int){edit(false);});
    connect(libraryList_,&QTreeWidget::itemChanged,this,[this](auto *item,int column){if(column!=0)return;int row=libraryList_->indexOfTopLevelItem(item);if(row<0)return;invalidate();library_[row].enabled=item->checkState(0)==Qt::Checked;dirty_=true;});
    auto *right=new QVBoxLayout;body->addLayout(right,2);
    auto *controls=new QHBoxLayout;right->addLayout(controls);scope_=new QComboBox;scope_->addItems({"Entire file","Start of file","Start of selection","Starts of marked regions"});controls->addWidget(scope_);
    start_=new QPushButton("Analyze");start_->setObjectName("magicScan");controls->addWidget(start_);connect(start_,&QPushButton::clicked,this,[this]{scan();});
    auto *cancel=new QPushButton("Cancel");controls->addWidget(cancel);connect(cancel,&QPushButton::clicked,view_,&HexView::cancelMagicScan);
    results_=new QTableWidget(0,4);results_->setObjectName("magicResults");addEmptyState(results_,"search","No matches to show","Analyze searches the scope for the active signatures.");results_->setHorizontalHeaderLabels({"Matching signature","Offset ↗","Candidate start","Bytes"});results_->setEditTriggers(QAbstractItemView::NoEditTriggers);results_->setSelectionBehavior(QAbstractItemView::SelectRows);results_->setSelectionMode(QAbstractItemView::ExtendedSelection);results_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);results_->horizontalHeader()->setStretchLastSection(true);right->addWidget(results_);
    auto navigate=[this](QTableWidgetItem *item){if(!item||item->row()>=matches_.size())return;auto r=matches_[item->row()];view_->selectMatch(r.offset,r.length);};
    connect(results_,&QTableWidget::itemClicked,this,navigate);connect(results_,&QTableWidget::itemActivated,this,navigate);
    auto *markButton=new QPushButton("Outline selected rows with the current style");right->addWidget(markButton);connect(markButton,&QPushButton::clicked,this,[this]{mark();});
    status_=new QLabel("Editable library · PNG, DDS and gzip are included as a starting point.");status_->setWordWrap(true);layout->addWidget(status_);
    connect(view_,&HexView::documentChanged,this,[this]{invalidate();});
    connect(view_,&HexView::formatRulesChanged,this,[this]{if(view_->magicBusy()&&anchored_)invalidate();});
    connect(view_,&HexView::magicScanFinished,this,[this](const ReadResult &r){
        if(stale_)return;if(!r.error.isEmpty()){status_->setText(r.error);return;}if(r.cancelled){status_->setText("Analysis canceled.");return;}
        matches_.clear();for(auto m:r.matches)matches_.append({m.first,m.second});types_=r.matchTypes;results_->setUpdatesEnabled(false);results_->setRowCount(matches_.size());
        for(int i=0;i<matches_.size();++i){const auto &s=scanned_[types_[i]];auto range=matches_[i];
            QStringList cells{s.name,"0x"+QString::number(range.offset,16).toUpper(),"0x"+QString::number(range.offset-s.relativeOffset,16).toUpper(),QString::number(range.length)};
            for(int c=0;c<4;++c){auto *item=new QTableWidgetItem(cells[c]);item->setToolTip(s.notes);if(c==1){auto font=item->font();font.setUnderline(true);item->setFont(font);item->setForeground(QColor(45,125,220));}results_->setItem(i,c,item);}
        }
        results_->setUpdatesEnabled(true);status_->setText(QString("%1 matches%2. Click a row to go to its bytes; select multiple rows to outline them.").arg(matches_.size()).arg(r.truncated?" (10,000 limit reached; partial analysis)":""));
    });
    auto *timer=new QTimer(this);timer->setInterval(200);connect(timer,&QTimer::timeout,this,[this]{start_->setEnabled(view_->hasFile()&&!view_->saving()&&!view_->magicBusy());if(view_->magicBusy()&&!stale_)status_->setText(anchored_?QString("Analyzing %1 / %2 regions…").arg(view_->magicProgress()).arg(total_):QString("Analyzing %1 / %2 MiB…").arg(double(view_->magicProgress())/1048576,0,'f',1).arg(double(total_)/1048576,0,'f',1));});timer->start();rebuild();
}
void MagicPanel::rebuild(){QSignalBlocker block(libraryList_);libraryList_->clear();for(const auto &s:library_){auto *item=new QTreeWidgetItem(libraryList_,{s.name,s.pattern,QString::number(s.relativeOffset)});item->setCheckState(0,s.enabled?Qt::Checked:Qt::Unchecked);item->setToolTip(0,s.notes);}libraryList_->resizeColumnToContents(0);}
void MagicPanel::invalidate(){view_->cancelMagicScan();stale_=true;matches_.clear();types_.clear();results_->setRowCount(0);status_->setText("Results pending analysis.");}
void MagicPanel::edit(bool create){
    int row=libraryList_->indexOfTopLevelItem(libraryList_->currentItem());if(!create&&row<0)return;if(create&&library_.size()>=64){QMessageBox::information(this,"Library","Maximum 64 signatures per library.");return;}
    MagicSignature s=create?MagicSignature{}:library_[row];QDialog dialog(this);dialog.setWindowTitle(create?"Add Signature":"Edit Signature");auto *form=new QFormLayout(&dialog);
    auto *name=new QLineEdit(s.name);name->setMaxLength(200);auto *pattern=new QLineEdit(s.pattern);pattern->setPlaceholderText("89 50 4E 47 · wildcards: ?? A? ?F");auto *offset=new QLineEdit(QString::number(s.relativeOffset));auto *notes=new QPlainTextEdit(s.notes);notes->setMaximumHeight(90);
    form->addRow("Name",name);form->addRow("Hexadecimal bytes",pattern);form->addRow("Offset from start",offset);form->addRow("Notes",notes);auto *hint=new QLabel("Decimal or 0x hexadecimal offset. For full-file searches, the start is inferred by subtracting it from the match offset.");hint->setWordWrap(true);form->addRow(hint);
    auto *actions=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(actions);connect(actions,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(actions,&QDialogButtonBox::accepted,&dialog,[&]{bool ok=false;QString text=offset->text().trimmed();s.relativeOffset=text.toLongLong(&ok,text.startsWith("0x",Qt::CaseInsensitive)?16:10);s.name=name->text().trimmed();s.pattern=pattern->text();s.notes=notes->toPlainText();QString error;if(!ok||!validateMagic(s,error)){QMessageBox::warning(&dialog,"Invalid Signature",ok?error:"Invalid offset.");return;}dialog.accept();});
    if(dialog.exec()!=QDialog::Accepted)return;invalidate();if(create)library_.append(s);else library_[row]=s;dirty_=true;rebuild();
}
bool MagicPanel::exportLibrary(){auto path=QFileDialog::getSaveFileName(this,"Export Signature Library","firmas.magic.json","JSON library (*.json)");if(path.isEmpty())return false;QSaveFile file(path);auto bytes=encodeMagicLibrary(library_).toJson();if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){QMessageBox::warning(this,"Export",file.errorString());return false;}dirty_=false;return true;}
bool MagicPanel::confirmClose(){if(!dirty_)return true;auto answer=QMessageBox::question(this,"Modified Library","Export changes to the signature library?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);if(answer==QMessageBox::Save)return exportLibrary();if(answer==QMessageBox::Discard){dirty_=false;return true;}return false;}
void MagicPanel::scan(){
    MagicSpec spec;spec.signatures=library_;spec.anchored=scope_->currentIndex()!=0;QString error;
    if(scope_->currentIndex()==1)spec.regions={{0,view_->fileSize()}};
    else if(scope_->currentIndex()==2){auto s=view_->selection();if(s.anchor<0||s.rectangular)error="Select a contiguous region to check its start.";else spec.regions={{qMin(s.anchor,s.cursor),qAbs(s.cursor-s.anchor)+1}};}
    else if(scope_->currentIndex()==3){spec.regions=view_->markedRegions(error);if(error.isEmpty()&&spec.regions.isEmpty())error="No active marked regions.";}
    if(!error.isEmpty()){status_->setText(error);return;}
    if(view_->magicBusy())return;invalidate();scanned_=library_;anchored_=spec.anchored;total_=anchored_?spec.regions.size():view_->fileSize();stale_=!view_->startMagicScan(std::move(spec));if(stale_)status_->setText("Open a file and wait for saving to finish.");
}
void MagicPanel::mark(){
    QVector<ByteRange> selected;for(const auto &index:results_->selectionModel()->selectedRows())if(index.row()<matches_.size())selected.append(matches_[index.row()]);
    if(selected.isEmpty()){status_->setText("Select one or more result rows.");return;}
    std::sort(selected.begin(),selected.end(),[](auto a,auto b){return a.offset<b.offset;});QVector<ByteRange> merged;
    for(auto r:selected){if(!merged.isEmpty()&&r.offset<=merged.last().offset+merged.last().length)merged.last().length=qMax(merged.last().offset+merged.last().length,r.offset+r.length)-merged.last().offset;else merged.append(r);}
    QString error;if(!format_->markExternalMatches("Magic bytes",merged,error))status_->setText(error);else status_->setText("Signatures outlined in hexadecimal and interpreted views. Save the formatting rules to preserve the marks.");
}
