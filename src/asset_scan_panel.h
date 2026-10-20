#pragma once
#include "asset_discovery.h"
#include "hex_view.h"
#include <QTreeWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <functional>
// Native read-only discovery UI. All entry points and the assistant share discoverAssets.
class AssetScanPanel final:public QWidget {
public:
    std::function<QJsonObject(const QString &,const QJsonObject &,bool)> useCandidate;
    std::function<QJsonObject(const QJsonArray &)> annotate;
    std::function<void(QJsonObject)> published;
    std::function<void()> showHex;
    AssetScanPanel(HexView *view,QString mode,QWidget *parent=nullptr):QWidget(parent),view_(view),mode_(mode){
        setObjectName("assetScanPanel_"+mode);auto *layout=new QVBoxLayout(this);
        auto *bar=new QHBoxLayout;auto *scan=new QPushButton(mode=="mesh"?"Scan for 3D Data":mode=="image"?"Scan for Textures / Images":"Scan File");scan->setObjectName("scanStart");stop_=new QPushButton("Cancel Scan");stop_->setEnabled(false);bar->addWidget(scan);bar->addWidget(stop_);bar->addStretch();layout->addLayout(bar);
        status_=new QLabel("Scan the active document for recognizable headers and supported asset layouts. Results are evidence, not a guarantee of complete format support.");status_->setObjectName("scanStatus");status_->setWordWrap(true);layout->addWidget(status_);
        progress_=new QProgressBar;progress_->setRange(0,0);progress_->hide();layout->addWidget(progress_);
        tree_=new QTreeWidget;tree_->setObjectName("scanResults");tree_->setHeaderLabels({"Type","Offset","Details","Support"});tree_->setRootIsDecorated(false);tree_->header()->setSectionResizeMode(2,QHeaderView::Stretch);layout->addWidget(tree_,1);
        details_=new QPlainTextEdit;details_->setObjectName("scanDetails");details_->setReadOnly(true);details_->setMaximumHeight(120);layout->addWidget(details_);
        auto *actions=new QHBoxLayout;go_=new QPushButton("Go to Bytes");go_->setObjectName("scanGoToBytes");load_=new QPushButton("Load Settings");load_->setObjectName("scanLoadSettings");preview_=new QPushButton("Preview Selected");preview_->setObjectName("scanPreview");mark_=new QPushButton("Mark Recognized Structures");mark_->setObjectName("scanMarkStructures");for(auto *b:{go_,load_,preview_,mark_})actions->addWidget(b);layout->addLayout(actions);
        automatic_=new QCheckBox("Mark recognized structures after scanning");automatic_->setChecked(mode=="all");automatic_->setVisible(mode=="all");layout->addWidget(automatic_);autoPreview_=new QCheckBox("Preview first supported result after scanning");autoPreview_->setObjectName("scanAutoPreview");autoPreview_->setChecked(mode!="all");autoPreview_->setVisible(mode!="all");layout->addWidget(autoPreview_);
        connect(scan,&QPushButton::clicked,this,[this]{startScan();});connect(stop_,&QPushButton::clicked,this,[this]{if(cancel_)cancel_->store(true);});
        connect(tree_,&QTreeWidget::currentItemChanged,this,[this]{selectionChanged();});connect(go_,&QPushButton::clicked,this,[this]{if(!valid_)return;const auto row=selected();auto offset=row["offset"].toString().toLongLong();if(offset<view_->fileSize())view_->selectMatch(offset,1);if(showHex)showHex();});
        auto use=[this](bool run){if(!valid_||waiting_||!useCandidate)return;const auto row=selected();auto result=useCandidate(row["kind"].toString(),row["data"].toObject(),run);waiting_=run&&result["started"].toBool();waitingKind_=row["kind"].toString()=="image"?"image":"mesh";selectionChanged();status_->setText(result.contains("error")?result["error"].toString():result["status"].toString(run?"Decoding selected result; completion will be reported here.":"Settings loaded. Review them in the decoder panel."));};
        connect(load_,&QPushButton::clicked,this,[use]{use(false);});connect(preview_,&QPushButton::clicked,this,[use]{use(true);});connect(mark_,&QPushButton::clicked,this,[this]{markStructures();if(showHex)showHex();});
        connect(view_,&HexView::documentChanged,this,[this]{++generation_;valid_=false;waiting_=false;if(cancel_)cancel_->store(true);tree_->clear();result_={};status_->setText("Document changed. Scan again for current results.");selectionChanged();});
        connect(&watcher_,&QFutureWatcher<QJsonObject>::finished,this,[this]{progress_->hide();stop_->setEnabled(false);if(started_!=generation_){status_->setText("Scan canceled: document changed.");return;}auto result=watcher_.result();if(result.contains("error")){status_->setText(result["error"].toString());return;}showResults(result);if(published)published(result);if(automatic_->isChecked())markStructures();if(autoPreview_->isChecked()){for(int i=0;i<tree_->topLevelItemCount();++i){tree_->setCurrentItem(tree_->topLevelItem(i));if(preview_->isEnabled()){preview_->click();break;}}}});selectionChanged();
    }
    ~AssetScanPanel()override{if(cancel_)cancel_->store(true);watcher_.waitForFinished();}
    void startScan(){if(watcher_.isRunning()){status_->setText("A scan is already running.");return;}if(!view_->hasFile()||view_->fileSize()==0||view_->saving()||view_->directEditing()){status_->setText("Open a nonempty document and wait for editing/saving to finish.");return;}valid_=false;waiting_=false;tree_->clear();selectionChanged();cancel_=std::make_shared<std::atomic_bool>(false);auto cancel=cancel_;auto source=view_->meshSource();auto patches=view_->pendingPatches();started_=generation_;progress_->show();stop_->setEnabled(true);status_->setText("Scanning document bytes locally, including unsaved edits...");watcher_.setFuture(QtConcurrent::run([source,patches,cancel]{return discoverAssets(source,patches,*cancel);}));}
    void showResults(QJsonObject result){result_=result;valid_=true;tree_->clear();
        auto row=[&](QString type,QString offset,QString summary,QString support,QString kind,QJsonObject data){auto *item=new QTreeWidgetItem(tree_,{type,"0x"+QString::number(offset.toLongLong(),16),summary,support});item->setData(0,Qt::UserRole,QJsonObject{{"offset",offset},{"kind",kind},{"data",data}});};
        for(auto value:result["meshCandidates"].toArray())if(mode_!="image"){const auto data=value.toObject();auto settings=data["settings"].toObject();row("3D mesh",settings["vertexOffset"].toString(),QString("%1 vertices · %2 indices").arg(settings["vertexCount"].toInteger()).arg(settings["indexCount"].toInteger()),"Decoder available","mesh",data);}
        for(auto value:result["imageCandidates"].toArray())if(mode_!="mesh"){const auto data=value.toObject();row("Image",data["settings"].toObject()["offset"].toString(),QString("%1 x %2 · %3").arg(data["width"].toInteger()).arg(data["height"].toInteger()).arg(data["basis"].toString()),"Decoder available","image",data);}
        for(auto value:result["meshEvidence"].toArray())if(mode_!="image"){const auto data=value.toObject();auto sample=data["samples"].toArray().isEmpty()?QJsonObject():data["samples"].toArray().first().toObject();auto text=sample["positionsOffset"].toString();bool ok=false;auto offset=text.toLongLong(&ok,text.startsWith("0x")?16:10);row(data["family"].toString(),QString::number(ok?offset:0),data["diagnostic"].toString(data["requiredDecoder"].toString()),data["renderSupported"].toBool()?"Recovered triangle strips":data["previewSupported"].toBool()?"Point preview only":"Importer required","evidence",data);}
        if(mode_!="mesh")for(auto value:result["meshEvidence"].toArray())for(auto reference:value.toObject()["textureReferences"].toArray()){const auto data=reference.toObject();row("Texture reference",data["offset"].toString(),data["name"].toString(),"External image reference","reference",data);}
        if(mode_=="all")for(auto value:result["structureRegions"].toArray()){const auto data=value.toObject();row("Structure",data["offset"].toString(),data["name"].toString(),"Recognized range","region",data);}
        status_->setText(QString("Scanned %1 bytes%2 · %3 result(s). Empty results do not prove that assets are absent.").arg(result["scannedBytes"].toString()).arg(result["scanTruncated"].toBool()?" (32 MiB inspection limit reached)":"").arg(tree_->topLevelItemCount()));
        if(tree_->topLevelItemCount())tree_->setCurrentItem(tree_->topLevelItem(0));selectionChanged();
    }
    void decodingFinished(QString kind,bool success,const QString &message){if(!waiting_||kind!=waitingKind_)return;waiting_=false;status_->setText((success?"Preview loaded: ":"Decoding failed: ")+message);selectionChanged();}
    QJsonObject result()const{return valid_?result_:QJsonObject();}
private:
    QJsonObject selected()const{return tree_->currentItem()?tree_->currentItem()->data(0,Qt::UserRole).toJsonObject():QJsonObject();}
    void selectionChanged(){const auto row=selected();auto kind=row["kind"].toString();bool recipe=kind=="mesh"||kind=="image",points=kind=="evidence"&&row["data"].toObject()["previewSupported"].toBool();go_->setEnabled(valid_&&!row.isEmpty());load_->setEnabled(valid_&&!waiting_&&recipe);preview_->setEnabled(valid_&&!waiting_&&(recipe||points));preview_->setText(points?(row["data"].toObject()["renderSupported"].toBool()?"Preview Recovered Mesh":"Preview Points (No Faces)"):"Preview Selected");mark_->setEnabled(valid_&&!result_["structureRegions"].toArray().isEmpty());const auto data=row["data"].toObject();details_->setPlainText(data["diagnostic"].toString(data["basis"].toString(data["evidence"].toString(data["requiredDecoder"].toString()))));}
    void markStructures(){if(!valid_||!annotate)return;auto regions=result_["structureRegions"].toArray();if(regions.isEmpty())return;auto result=annotate(regions);status_->setText(status_->text()+"\n"+(result.contains("error")?result["error"].toString():QString("%1 formatting rules added; %2 existing rules skipped. Binary bytes unchanged.").arg(result["added"].toInt()).arg(result["duplicatesSkipped"].toInt())));}
    HexView *view_;QString mode_,waitingKind_;QTreeWidget *tree_;QPlainTextEdit *details_;QLabel *status_;QPushButton *stop_,*go_,*load_,*preview_,*mark_;QCheckBox *automatic_,*autoPreview_;QProgressBar *progress_;QFutureWatcher<QJsonObject> watcher_;std::shared_ptr<std::atomic_bool> cancel_;QJsonObject result_;quint64 generation_=0,started_=0;bool valid_=false,waiting_=false;
};
