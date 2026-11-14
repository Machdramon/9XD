#pragma once
#include "workbench_icons.h"
#include "empty_state.h"
#include "youtube_video_source.h"
#include "reconstruction_job.h"
#include <QThread>
#include <QtConcurrent>
#include <QCheckBox>
#include <QTemporaryDir>
#include <QSettings>
#include <memory>
#include <QWidget>
#include <QFrame>
#include <QFormLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QListWidget>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSaveFile>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QTime>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QTimer>
#include <QStyle>
#include <QImageReader>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QPointer>
#include <QSet>
#include <QHash>
#include <algorithm>
#include <cmath>
#include <functional>

// Video frame extraction and image/spline point editing inspectors for the 3D module.
// Object names, job.json and the spline project schema are contracts used by tests and docs.
namespace Reconstruction {
inline QString toolPath(const QString &name){
    QDir root(QCoreApplication::applicationDirPath()+"/../reconstruction-tools");
    if(name=="yt-dlp.exe"||name=="deno.exe"){
        QFile receipt(root.filePath("youtube/receipt.json"));if(receipt.open(QIODevice::ReadOnly))for(auto value:QJsonDocument::fromJson(receipt.readAll()).object()["tools"].toArray()){
            auto path=value.toObject()["executable"].toString();if(QFileInfo(path).fileName()==name&&QFileInfo(path).isExecutable())return path;
        }
    }
    QDirIterator it(root.absolutePath(),{name},QDir::Files,QDirIterator::Subdirectories);
    return it.hasNext()?it.next():QString();
}
inline bool writeJson(const QString &path,const QJsonObject &value){QSaveFile file(path);if(!file.open(QIODevice::WriteOnly))return false;auto bytes=QJsonDocument(value).toJson();return file.write(bytes)==bytes.size()&&file.commit();}

// ---- Shared presentation helpers -------------------------------------------------------------
inline void repolish(QWidget *widget){widget->style()->unpolish(widget);widget->style()->polish(widget);widget->update();}
// A titled section card; styled by QFrame#inspectorCard in theme.h.
inline QVBoxLayout *card(QVBoxLayout *parent,const QString &title,QFrame **frameOut=nullptr){
    auto *frame=new QFrame;frame->setObjectName("inspectorCard");frame->setAttribute(Qt::WA_StyledBackground);
    auto *layout=new QVBoxLayout(frame);layout->setContentsMargins(12,10,12,12);layout->setSpacing(8);
    auto *heading=new QLabel(title);heading->setObjectName("inspectorCardTitle");layout->addWidget(heading);
    parent->addWidget(frame);if(frameOut)*frameOut=frame;return layout;
}
inline QFormLayout *formIn(QVBoxLayout *layout){auto *form=new QFormLayout;form->setContentsMargins(0,0,0,0);form->setHorizontalSpacing(10);form->setVerticalSpacing(6);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);layout->addLayout(form);return form;}
inline QLabel *hint(const QString &text,const QString &name="inspectorHint"){auto *label=new QLabel(text);label->setObjectName(name);label->setWordWrap(true);return label;}
inline QToolButton *iconButton(const QString &icon,const QString &tip,const QString &name={}){
    auto *button=new QToolButton;button->setIcon(workbenchIcon(icon));button->setIconSize({16,16});button->setToolTip(tip);button->setAccessibleName(tip);if(!name.isEmpty())button->setObjectName(name);return button;
}
// Path field with a folder button. The QLineEdit keeps the contract object name.
inline QLineEdit *pathField(QFormLayout *form,const QString &label,const QString &id,bool directory=false,const QString &filter={}){
    auto *row=new QWidget;auto *layout=new QHBoxLayout(row);layout->setContentsMargins(0,0,0,0);layout->setSpacing(4);
    auto *field=new QLineEdit;field->setObjectName(id);field->setAccessibleName(label);field->setClearButtonEnabled(true);
    auto *browse=iconButton("folder",directory?"Choose folder…":"Choose file…");layout->addWidget(field,1);layout->addWidget(browse);form->addRow(label,row);
    QObject::connect(browse,&QToolButton::clicked,row,[field,directory,label,filter]{auto path=directory?QFileDialog::getExistingDirectory(field,label,field->text()):QFileDialog::getOpenFileName(field,label,field->text(),filter);if(!path.isEmpty())field->setText(QDir::toNativeSeparators(path));});
    return field;
}
// Status line whose border and text colour follow its state (theme.h: [state="…"]).
inline void setState(QLabel *label,const QString &state,const QString &text){label->setText(text);if(label->property("state").toString()!=state){label->setProperty("state",state);repolish(label);}}
// Collapsible section: a disclosure button that shows or hides `body`.
inline QToolButton *disclosure(const QString &text,QWidget *body,bool open=false){
    auto *toggle=new QToolButton;toggle->setObjectName("inspectorDisclosure");toggle->setText(text);toggle->setCheckable(true);toggle->setChecked(open);
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);toggle->setArrowType(open?Qt::DownArrow:Qt::RightArrow);if(!open)body->hide();
    QObject::connect(toggle,&QToolButton::toggled,body,[toggle,body](bool shown){toggle->setArrowType(shown?Qt::DownArrow:Qt::RightArrow);body->setVisible(shown);});
    return toggle;
}
inline QString formatDuration(double seconds){const int s=int(std::lround(seconds));return s>=3600?QString("%1:%2:%3").arg(s/3600).arg(s/60%60,2,10,QChar('0')).arg(s%60,2,10,QChar('0')):QString("%1:%2").arg(s/60).arg(s%60,2,10,QChar('0'));}
// Scrollable inspector body, so narrow or short docks never clip controls.
inline QVBoxLayout *scrollingBody(QWidget *owner){
    auto *outer=new QVBoxLayout(owner);outer->setContentsMargins(0,0,0,0);
    auto *scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);outer->addWidget(scroll);
    auto *content=new QWidget;scroll->setWidget(content);auto *layout=new QVBoxLayout(content);layout->setContentsMargins(10,10,10,10);layout->setSpacing(10);return layout;
}

// ---- Video reconstruction ---------------------------------------------------------------------
class VideoInspector final:public QWidget{
public:
    explicit VideoInspector(QWidget *parent=nullptr):QWidget(parent){
        setObjectName("videoReconstructionInspector");auto *layout=scrollingBody(this);
        // Pipeline: only frame extraction exists today. Later stages are shown, never enabled.
        auto *pipeline=new QWidget;pipeline->setObjectName("reconstructionPipeline");pipeline->setAttribute(Qt::WA_StyledBackground);
        auto *steps=new QHBoxLayout(pipeline);steps->setContentsMargins(6,5,6,5);steps->setSpacing(2);
        const QStringList names{"1  Frames","2  Sparse points","3  3D surface"};
        for(int i=0;i<3;++i){if(i){auto *arrow=new QLabel(QString::fromUtf8("›"));arrow->setObjectName("pipelineArrow");steps->addWidget(arrow);}
            auto *step=new QLabel(names[i]);step->setObjectName("pipelineStep");step->setProperty("state",i==0?"current":"pending");
            step->setToolTip(i==0?"Extract still frames from the video.":i==1?"Recover cameras and sparse points with COLMAP.":"Reconstruct a coarse or dense surface and import it into 3D.");steps_<<step;steps->addWidget(step);}
        steps->addStretch();layout->addWidget(pipeline);
        layout->addWidget(hint("Extract frames, solve cameras and points, then reconstruct a surface. Static scenes, overlapping frames and camera translation work best; moving objects, cuts and pure rotation can prevent reconstruction."));

        QFrame *source,*sampling,*output;
        auto *sourceBody=card(layout,"Source",&source);auto *sourceForm=formIn(sourceBody);
        sourceKind_=new QComboBox;sourceKind_->setObjectName("reconstructionSourceKind");sourceKind_->addItems({"Local file","YouTube URL"});sourceForm->addRow("Source",sourceKind_);
        video_=pathField(sourceForm,"Video","reconstructionVideo",false,"Video files (*.mp4 *.mov *.mkv *.avi *.webm *.m4v *.mts *.wmv);;All files (*)");
        online_=new QWidget;auto *onlineLayout=new QVBoxLayout(online_);onlineLayout->setContentsMargins(0,0,0,0);auto *onlineForm=formIn(onlineLayout);
        auto *urlRow=new QWidget;auto *urlLayout=new QHBoxLayout(urlRow);urlLayout->setContentsMargins(0,0,0,0);
        url_=new QLineEdit;url_->setObjectName("reconstructionYoutubeUrl");url_->setPlaceholderText("https://www.youtube.com/watch?v=…");url_->setAccessibleName("YouTube video link");
        inspect_=new QPushButton("Inspect Link");inspect_->setObjectName("reconstructionInspectLink");urlLayout->addWidget(url_,1);urlLayout->addWidget(inspect_);onlineForm->addRow("Link",urlRow);
        resolution_=new QComboBox;resolution_->setObjectName("reconstructionDownloadResolution");for(int h:{480,720,1080,1440,2160})resolution_->addItem(QString("Up to %1p").arg(h),h);resolution_->setCurrentIndex(2);onlineForm->addRow("Resolution",resolution_);
        budget_=new QSpinBox;budget_->setObjectName("reconstructionDownloadMiB");budget_->setRange(16,16384);budget_->setValue(1024);budget_->setSuffix(" MiB");onlineForm->addRow("Download limit",budget_);
        keep_=new QCheckBox("Keep the downloaded video with this job");keep_->setObjectName("reconstructionKeepVideo");onlineLayout->addWidget(keep_);
        onlineLayout->addWidget(hint("Temporary video data is removed after extraction or cancellation. Downloads a single video stream before trimming; audio is not needed. The limit is checked while downloading, so a small overshoot is possible."));
        sourceBody->addWidget(online_);online_->hide();
        connect(sourceKind_,&QComboBox::currentIndexChanged,this,[this,sourceForm](int index){sourceForm->setRowVisible(video_->parentWidget(),index==0);online_->setVisible(index==1);duration_=-1;metadata_={};info_->setText(index?"Inspect the link to read its title and duration.":"Choose a video to see its length and resolution.");updateEstimate();probeDelay_->start();start_->setText(index?"Import and Extract Frames":"Extract Frames");});
        info_=hint("Choose a video to see its length and resolution.","reconstructionVideoInfo");sourceBody->addWidget(info_);

        auto *samplingBody=card(layout,"Sampling",&sampling);auto *grid=new QGridLayout;grid->setHorizontalSpacing(10);grid->setVerticalSpacing(6);samplingBody->addLayout(grid);
        fps_=new QDoubleSpinBox;fps_->setObjectName("reconstructionFps");fps_->setRange(.05,10);fps_->setDecimals(2);fps_->setSingleStep(.25);fps_->setValue(1);fps_->setSuffix(" fps");fps_->setAccessibleName("Frames per second");
        count_=new QSpinBox;count_->setObjectName("reconstructionFrameLimit");count_->setRange(2,10000);count_->setValue(300);count_->setAccessibleName("Maximum frames");
        size_=new QSpinBox;size_->setObjectName("reconstructionMaxSize");size_->setRange(256,4096);size_->setSingleStep(64);size_->setValue(1600);size_->setSuffix(" px");size_->setAccessibleName("Longest side");
        grid->addWidget(new QLabel("Rate"),0,0);grid->addWidget(fps_,0,1);grid->addWidget(new QLabel("Frame limit"),0,2);grid->addWidget(count_,0,3);
        grid->addWidget(new QLabel("Longest side"),1,0);grid->addWidget(size_,1,1);grid->setColumnStretch(1,1);grid->setColumnStretch(3,1);
        auto *range=formIn(samplingBody);clipStart_=new QDoubleSpinBox;clipStart_->setObjectName("reconstructionClipStart");clipStart_->setRange(0,604800);clipStart_->setDecimals(2);clipStart_->setSuffix(" s");
        clipEnd_=new QDoubleSpinBox;clipEnd_->setObjectName("reconstructionClipEnd");clipEnd_->setRange(0,604800);clipEnd_->setDecimals(2);clipEnd_->setSpecialValueText("End of video");clipEnd_->setSuffix(" s");range->addRow("Start",clipStart_);range->addRow("End",clipEnd_);
        connect(clipStart_,&QDoubleSpinBox::valueChanged,this,[this]{updateEstimate();});connect(clipEnd_,&QDoubleSpinBox::valueChanged,this,[this]{updateEstimate();});
        estimate_=hint("","reconstructionEstimate");samplingBody->addWidget(estimate_);

        auto *outputBody=card(layout,"Output",&output);auto *outputForm=formIn(outputBody);
        output_=pathField(outputForm,"Folder","reconstructionOutput",true);
        outputBody->addWidget(hint("Each run creates a new 9xd-video-… folder inside this one. Existing output is never overwritten."));

        // Tools: FFmpeg location until shared Settings paths exist.
        auto *toolsBody=new QWidget;auto *toolsLayout=new QVBoxLayout(toolsBody);toolsLayout->setContentsMargins(0,0,0,0);auto *toolsForm=formIn(toolsLayout);
        ffmpeg_=pathField(toolsForm,"FFmpeg","reconstructionFfmpeg",false,"FFmpeg (ffmpeg ffmpeg.exe);;All files (*)");
        downloader_=pathField(toolsForm,"yt-dlp","reconstructionYtdlp",false,"yt-dlp (yt-dlp yt-dlp.exe);;All files (*)");
        deno_=pathField(toolsForm,"Deno (YouTube)","reconstructionDeno",false,"Deno (deno deno.exe);;All files (*)");
        toolsLayout->addWidget(hint("YouTube uses an external yt-dlp executable and Deno. Update these tools separately when YouTube changes. No browser cookies or account credentials are imported."));
        colmap_=pathField(toolsForm,"COLMAP 4.2+","reconstructionColmap",false,"COLMAP (colmap colmap.exe);;All files (*)");
        auto *toolsRow=new QHBoxLayout;toolsToggle_=disclosure("Tools",toolsBody);toolState_=hint("","reconstructionToolState");toolState_->setWordWrap(false);
        toolsRow->addWidget(toolsToggle_);toolsRow->addWidget(toolState_,1);layout->addLayout(toolsRow);layout->addWidget(toolsBody);
        QFrame *reconstruction;auto *reconBody=card(layout,"Reconstruction",&reconstruction);auto *reconForm=formIn(reconBody);
        matchMode_=new QComboBox;matchMode_->setObjectName("reconstructionMatching");matchMode_->addItems({"Sequential (video)","Exhaustive (short orbit)"});reconForm->addRow("Matching",matchMode_);
        sharedCamera_=new QCheckBox("Same lens / shared intrinsics");sharedCamera_->setObjectName("reconstructionSharedCamera");sharedCamera_->setChecked(true);reconForm->addRow(sharedCamera_);
        threads_=new QSpinBox;threads_->setObjectName("reconstructionThreads");threads_->setRange(1,64);threads_->setValue(qBound(1,QThread::idealThreadCount()/2,8));reconForm->addRow("CPU threads",threads_);
        surfaceMode_=new QComboBox;surfaceMode_->setObjectName("reconstructionSurfaceMode");surfaceMode_->addItems({"Dense surface (CUDA)","Coarse surface (sparse / CPU)"});reconForm->addRow("Surface",surfaceMode_);
        denseSize_=new QSpinBox;denseSize_->setObjectName("reconstructionDenseSize");denseSize_->setRange(256,2048);denseSize_->setSingleStep(128);denseSize_->setValue(1024);denseSize_->setSuffix(" px");reconForm->addRow("Dense image size",denseSize_);
        cache_=new QDoubleSpinBox;cache_->setObjectName("reconstructionCacheGiB");cache_->setRange(.25,16);cache_->setSingleStep(.25);cache_->setValue(2);cache_->setSuffix(" GiB");reconForm->addRow("Stereo cache",cache_);
        depth_=new QSpinBox;depth_->setObjectName("reconstructionMeshingDepth");depth_->setRange(6,11);depth_->setValue(8);reconForm->addRow("Meshing depth",depth_);
        trim_=new QDoubleSpinBox;trim_->setObjectName("reconstructionSurfaceTrim");trim_->setRange(0,10);trim_->setValue(4);trim_->setSingleStep(.5);reconForm->addRow("Surface trim",trim_);
        importLimit_=new QSpinBox;importLimit_->setObjectName("reconstructionImportLimit");importLimit_->setRange(1000,4000000);importLimit_->setSingleStep(100000);importLimit_->setValue(2000000);reconForm->addRow("Max imported vertices",importLimit_);
        reconBody->addWidget(hint("Sparse solving uses CPU. Dense depth needs CUDA-enabled COLMAP and an NVIDIA driver; coarse meshing uses sparse points and may have holes. Cache is a tool setting, not a hard process-memory limit. Scale is arbitrary until calibrated. Surface trim removes weakly-supported faces; too high can leave an empty mesh. Surface preview has no texture atlas."));
        auto *reconActions=new QHBoxLayout;sparseButton_=new QPushButton("Build Sparse Points");sparseButton_->setObjectName("reconstructionBuildSparse");surfaceButton_=new QPushButton("Build 3D Surface");surfaceButton_->setObjectName("reconstructionBuildSurface");loadJob_=new QPushButton("Load Job…");loadJob_->setObjectName("reconstructionLoadJob");reconActions->addWidget(loadJob_);reconActions->addWidget(sparseButton_);reconActions->addWidget(surfaceButton_);reconBody->addLayout(reconActions);
        auto *importActions=new QHBoxLayout;importPoints_=new QPushButton("Show Points in 3D");importPoints_->setObjectName("reconstructionImportPoints");importCameras_=new QPushButton("Show Cameras in 3D");importCameras_->setObjectName("reconstructionImportCameras");importSurface_=new QPushButton("Show Surface in 3D");importSurface_->setObjectName("reconstructionImportSurface");for(auto *b:{importPoints_,importCameras_,importSurface_})importActions->addWidget(b);reconBody->addLayout(importActions);
        reconstructionInfo_=hint("Extract at least 3 frames, or load an existing job.","reconstructionSummary");reconBody->addWidget(reconstructionInfo_);
        inputs_={source,sampling,output,toolsBody,reconstruction};

        auto *actions=new QHBoxLayout;actions->setSpacing(6);
        start_=new QPushButton("Extract Frames");start_->setObjectName("reconstructionExtract");start_->setProperty("primary",true);start_->setIcon(workbenchIcon("play",Qt::white));
        cancel_=new QPushButton("Cancel");cancel_->setObjectName("reconstructionCancel");cancel_->setIcon(workbenchIcon("stop"));cancel_->setEnabled(false);
        actions->addWidget(start_,1);actions->addWidget(cancel_);layout->addLayout(actions);
        progress_=new QProgressBar;progress_->setObjectName("reconstructionProgress");progress_->setTextVisible(true);progress_->setFormat("%v / %m frames");progress_->hide();layout->addWidget(progress_);
        status_=new QLabel;status_->setWordWrap(true);status_->setObjectName("reconstructionStatus");status_->setAccessibleName("Extraction status");layout->addWidget(status_);

        QFrame *results;auto *resultsBody=card(layout,"Extracted frames",&results);results_=results;results_->hide();
        auto *resultRow=new QHBoxLayout;resultText_=hint("","reconstructionResult");resultRow->addWidget(resultText_,1);
        openFolder_=new QPushButton("Open Folder");openFolder_->setObjectName("reconstructionOpenFolder");openFolder_->setIcon(workbenchIcon("folder"));openFolder_->setProperty("compact",true);resultRow->addWidget(openFolder_,0,Qt::AlignTop);resultsBody->addLayout(resultRow);
        thumbs_=new QListWidget;thumbs_->setObjectName("reconstructionThumbnails");thumbs_->setViewMode(QListView::IconMode);thumbs_->setFlow(QListView::LeftToRight);thumbs_->setWrapping(false);
        thumbs_->setIconSize({96,64});thumbs_->setFixedHeight(92);thumbs_->setMovement(QListView::Static);thumbs_->setSelectionMode(QAbstractItemView::NoSelection);thumbs_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);resultsBody->addWidget(thumbs_);

        log_=new QPlainTextEdit;log_->setObjectName("reconstructionLog");log_->setReadOnly(true);log_->setMaximumBlockCount(300);log_->setMinimumHeight(110);log_->setAccessibleName("FFmpeg log");
        addEmptyState(log_,"code","No output yet","FFmpeg messages appear here during extraction.");
        logToggle_=disclosure("Log",log_);layout->addWidget(logToggle_,0,Qt::AlignLeft);layout->addWidget(log_);layout->addStretch();

        process_=new QProcess(this);process_->setProcessChannelMode(QProcess::MergedChannels);probe_=new QProcess(this);youtube_=new YoutubeSource(this);
        youtube_->log=[this](const QString &s){log_->appendPlainText(s);};
        youtube_->progress=[this](qint64 done,qint64 total){if(total>0){progress_->setRange(0,100);progress_->setValue(int(qBound(0.,100.*double(done)/total,100.)));progress_->setFormat("Downloading %p%");}else{progress_->setRange(0,0);progress_->setFormat("Downloading…");}};
        youtube_->metadataReady=[this](const QJsonObject &m){metadata_=m;duration_=m["duration"].toDouble();info_->setText(QString("%1 · %2 · %3 × %4").arg(m["title"].toString()).arg(formatDuration(duration_)).arg(m["width"].toInt()).arg(m["height"].toInt()));updateEstimate();
            if(pendingImport_){if(!validClip()){endSourceJob("Invalid clip range. Start must be before the video end; End must be after Start and within the video.");return;}beginDownload();}
            else{setBusy(false);setState(status_,"ready","Link ready. Choose the fragment and import when ready.");progress_->hide();}};
        youtube_->inspectionFailed=[this](const QString &error){if(pendingImport_)endSourceJob(error);else{setBusy(false);progress_->hide();setState(status_,canceled_?"canceled":"failed",error);}};
        youtube_->downloaded=[this](const QString &file,const QString &error){if(file.isEmpty()){endSourceJob(error);return;}activeVideo_=file;manifest_["downloadBytes"]=QString::number(QFileInfo(file).size());manifest_["downloadedVideo"]=keep_->isChecked()?file:QString();manifest_["state"]="extracting";extract();};
        recon_=new ReconstructionJob(this);importWatcher_=new QFutureWatcher<PlyResult>(this);
        recon_->log=[this](const QString &line){log_->appendPlainText(line);};recon_->progress=[this](const QString &stage,int done,int total){progress_->setRange(0,total);progress_->setValue(done);progress_->setFormat("%v / %m stages");setState(status_,"running",stage+"…");};
        recon_->finished=[this](const ReconstructionOutput &result){
            if(result.error.isEmpty()&&!result.canceled){if(result.surface.isEmpty()){sparseResult_=result;surfaceResult_={};sparseMesh_=result.geometry;surfaceMesh_.reset();setStep(1,"done");setStep(2,"pending");}else{surfaceResult_=result;surfaceMesh_=result.geometry;setStep(2,"done");}
                manifest_["reconstruction"]=resultJson();if(!writeJson(job_+"/job.json",manifest_))setState(status_,"failed","Reconstruction ready, but could not update job.json; results remain in the run folder.");else setState(status_,"done",result.surface.isEmpty()?"Sparse points and camera poses ready. Review them or build the surface.":"Surface ready. Use Show Surface in 3D to inspect and export it.");
            }else{setState(status_,result.canceled?"canceled":"failed",result.error);logToggle_->setChecked(true);}
            setBusy(false);progress_->hide();refreshReconstruction();};
        connect(sparseButton_,&QPushButton::clicked,this,[this]{buildSparse();});connect(surfaceButton_,&QPushButton::clicked,this,[this]{buildSurface();});
        connect(loadJob_,&QPushButton::clicked,this,[this]{auto path=QFileDialog::getOpenFileName(this,"Load video reconstruction job",output_->text(),"Reconstruction job (job.json)");if(!path.isEmpty())loadJob(path);});
        connect(importPoints_,&QPushButton::clicked,this,[this]{importGeometry(false);});connect(importSurface_,&QPushButton::clicked,this,[this]{importGeometry(true);});
        connect(importCameras_,&QPushButton::clicked,this,[this]{if(!importToScene)return;auto mesh=std::make_shared<MeshData>();mesh->hasUvs=false;for(auto v:sparseResult_.summary.cameras){auto c=v.toObject()["center"].toArray();if(c.size()!=3)continue;MeshVertex p{};for(int i=0;i<3;++i)p.position[i]=float(c[i].toDouble());mesh->vertices.append(p);}mesh->vertexOffsets.fill(-1,mesh->vertices.size());QString error;if(!importToScene(mesh,"SfM camera centers",sparseResult_.model+"/images.bin",error))attention(error);});
        connect(importWatcher_,&QFutureWatcher<PlyResult>::finished,this,[this]{auto r=importWatcher_->result();importing_=false;setBusy(false);if(!r.error.isEmpty()){setState(status_,"failed",r.error);return;}if(importingSurface_)surfaceMesh_=r.mesh;else sparseMesh_=r.mesh;deliver(r.mesh,importingSurface_);});
        connect(inspect_,&QPushButton::clicked,this,[this]{inspectLink(false);});
        connect(url_,&QLineEdit::textChanged,this,[this]{metadata_={};duration_=-1;info_->setText("Inspect the link to read its title and duration.");updateEstimate();});
        poll_=new QTimer(this);poll_->setInterval(400);probeDelay_=new QTimer(this);probeDelay_->setSingleShot(true);probeDelay_->setInterval(300);
        connect(start_,&QPushButton::clicked,this,[this]{start();});connect(cancel_,&QPushButton::clicked,this,[this]{cancel();});
        connect(openFolder_,&QPushButton::clicked,this,[this]{if(!job_.isEmpty())QDesktopServices::openUrl(QUrl::fromLocalFile(job_));});
        connect(process_,&QProcess::readyReadStandardOutput,this,[this]{log_->appendPlainText(QString::fromLocal8Bit(process_->readAllStandardOutput()).left(8192));});
        connect(process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart)finish(false,process_->errorString());});
        connect(process_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus exit){finish(!canceled_&&code==0&&exit==QProcess::NormalExit,process_->errorString());});
        connect(poll_,&QTimer::timeout,this,[this]{progress_->setValue(qMin(progress_->maximum(),frameCount()));});
        connect(video_,&QLineEdit::textChanged,this,[this]{duration_=-1;updateEstimate();probeDelay_->start();});connect(probeDelay_,&QTimer::timeout,this,[this]{probe();});
        connect(probe_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus exit){probed(code==0&&exit==QProcess::NormalExit);});
        connect(ffmpeg_,&QLineEdit::textChanged,this,[this]{updateTools();probeDelay_->start();});
        for(auto *spin:{static_cast<QAbstractSpinBox *>(fps_),static_cast<QAbstractSpinBox *>(count_),static_cast<QAbstractSpinBox *>(size_)})connect(spin,&QAbstractSpinBox::editingFinished,this,[this]{updateEstimate();});
        connect(fps_,&QDoubleSpinBox::valueChanged,this,[this]{updateEstimate();});connect(count_,&QSpinBox::valueChanged,this,[this]{updateEstimate();});connect(size_,&QSpinBox::valueChanged,this,[this]{updateEstimate();});
        QSettings settings;ffmpeg_->setText(settings.value("reconstruction/ffmpeg",QDir::toNativeSeparators(toolPath("ffmpeg.exe"))).toString());
        downloader_->setText(settings.value("reconstruction/ytdlp",QDir::toNativeSeparators(toolPath("yt-dlp.exe"))).toString());deno_->setText(settings.value("reconstruction/deno",QDir::toNativeSeparators(toolPath("deno.exe"))).toString());colmap_->setText(settings.value("reconstruction/colmap",QDir::toNativeSeparators(preferredColmapPath())).toString());
        for(auto pair:{qMakePair(ffmpeg_,QString("ffmpeg")),qMakePair(downloader_,QString("ytdlp")),qMakePair(deno_,QString("deno")),qMakePair(colmap_,QString("colmap"))})connect(pair.first,&QLineEdit::textChanged,this,[this,pair]{if(isVisible())QSettings().setValue("reconstruction/"+pair.second,pair.first->text());});
        updateTools();updateEstimate();refreshReconstruction();
        setState(status_,"ready","Ready to extract frames.");
    }
    ~VideoInspector()override{importCanceled_=true;delete recon_;recon_=nullptr;importWatcher_->waitForFinished();for(auto *p:{process_,probe_})if(p->state()!=QProcess::NotRunning){p->kill();p->waitForFinished(1000);}}
    bool busy()const{return process_->state()!=QProcess::NotRunning||youtube_->busy()||pendingImport_||(recon_&&recon_->busy())||importing_;}
    QString jobDirectory()const{return job_;}
    std::function<bool(std::shared_ptr<MeshData>,const QString &,const QString &,QString &)> importToScene;
    bool loadJob(const QString &path){
        if(busy())return false;QFile f(path);if(!f.open(QIODevice::ReadOnly)||f.size()>16*1024*1024){attention("Not a readable reconstruction job.");return false;}auto m=QJsonDocument::fromJson(f.readAll()).object();auto root=QFileInfo(path).absolutePath();if(m["schema"]!=1||QDir(root+"/images").entryList({"frame-*.png"},QDir::Files).size()<3){attention("Job needs at least 3 extracted frames.");return false;}
        job_=root;manifest_=m;sparseResult_={};surfaceResult_={};sparseMesh_.reset();surfaceMesh_.reset();auto saved=m["reconstruction"].toObject();auto resolve=[&](const QString &s){auto p=QFileInfo(QDir(root).filePath(s)).canonicalFilePath();auto prefix=QFileInfo(root).canonicalFilePath()+"/";return !s.isEmpty()&&p.startsWith(prefix,Qt::CaseInsensitive)?p:QString();};
        sparseResult_.run=resolve(saved["sparseRun"].toString());sparseResult_.model=resolve(saved["model"].toString());sparseResult_.cloud=resolve(saved["cloud"].toString());auto summary=saved["summary"].toObject();sparseResult_.summary.images=summary["registeredImages"].toInt();sparseResult_.summary.points=summary["points"].toInt();sparseResult_.summary.components=summary["components"].toInt();sparseResult_.summary.meanError=summary["meanReprojectionError"].toDouble();sparseResult_.summary.cameras=summary["cameras"].toArray();if(sparseResult_.summary.cameras.size()>10000)sparseResult_.summary.cameras={};
        surfaceResult_=sparseResult_;surfaceResult_.run=resolve(saved["surfaceRun"].toString());surfaceResult_.surface=resolve(saved["surface"].toString());surfaceResult_.mode=saved["surfaceMode"].toString();if(sparseResult_.run.isEmpty()||sparseResult_.cloud.isEmpty()||sparseResult_.model.isEmpty())sparseResult_={};if(surfaceResult_.surface.isEmpty())surfaceResult_={};
        results_->show();resultText_->setText(QString("%1 saved frames in %2").arg(frameCount()).arg(QDir::toNativeSeparators(job_)));++thumbRun_;thumbs_->clear();loadThumbnails();setStep(0,"done");setStep(1,sparseResult_.cloud.isEmpty()?"pending":"done");setStep(2,surfaceResult_.surface.isEmpty()?"pending":"done");refreshReconstruction();setState(status_,"ready","Saved job loaded. Continue reconstruction without downloading the source again.");return true;
    }
    bool confirmClose(){if(!busy())return true;QMessageBox::information(this,"Video reconstruction","Cancel the active video job before closing this document.");return false;}
    void cancel(){if(!busy())return;canceled_=true;setState(status_,"running","Canceling…");if(youtube_->busy())youtube_->cancel();else if(recon_&&recon_->busy())recon_->cancel();else if(importing_)importCanceled_=true;else process_->kill();}
private:
    int frameCount()const{return job_.isEmpty()?0:int(QDir(job_+"/images").entryList({"frame-*.png"},QDir::Files).size());}
    QString ffprobePath()const{auto dir=QFileInfo(ffmpeg_->text()).dir();for(auto name:{"ffprobe.exe","ffprobe"})if(QFileInfo(dir.filePath(name)).isExecutable())return dir.filePath(name);return {};}
    void updateTools(){
        const bool found=QFileInfo(ffmpeg_->text()).isExecutable();toolState_->setText(found?"FFmpeg ready":"FFmpeg not found. Set its path under Tools.");
        toolState_->setProperty("state",found?"done":"attention");repolish(toolState_);if(!found&&!toolsToggle_->isChecked())toolsToggle_->setChecked(true);
    }
    double clipDuration()const{auto end=clipEnd_->value()>0?clipEnd_->value():duration_;return end>0?qMax(0.,end-clipStart_->value()):-1;}
    bool validClip()const{return (clipEnd_->value()==0||clipEnd_->value()>clipStart_->value())&&(duration_<=0||(clipStart_->value()<duration_&&(clipEnd_->value()==0||clipEnd_->value()<=duration_+.05)));}
    int expectedFrames()const{auto d=clipDuration();if(d<0)return count_->value();return int(qBound(0.,std::floor(d*fps_->value())+1,double(count_->value())));}
    void updateEstimate(){
        const auto side=QString("longest side ≤ %1 px").arg(size_->value());
        if(clipDuration()<0){estimate_->setText(QString("Up to %1 frames · %2.").arg(count_->value()).arg(side));return;}
        const int frames=expectedFrames();
        estimate_->setText(frames<2?QString("Only %1 frame at %2 fps from %3. Raise the rate: extraction needs at least 2 frames.").arg(frames).arg(fps_->value()).arg(formatDuration(qMax(0.,clipDuration())))
            :QString("About %1 frames from %2 at %3 fps%4 · %5.").arg(frames).arg(formatDuration(qMax(0.,clipDuration()))).arg(fps_->value()).arg(frames>=count_->value()?QString(", capped by the frame limit"):QString()).arg(side));
    }
    void probe(){
        if(sourceKind_->currentIndex()!=0||busy())return;
        if(probe_->state()!=QProcess::NotRunning){probe_->kill();probe_->waitForFinished(500);}
        duration_=-1;updateEstimate();const QFileInfo video(video_->text());
        if(video_->text().trimmed().isEmpty()){info_->setText("Choose a video to see its length and resolution.");return;}
        if(!video.isFile()){info_->setText("File not found.");return;}
        const auto ffprobe=ffprobePath();if(ffprobe.isEmpty()){info_->setText("Video details unavailable: ffprobe was not found next to FFmpeg.");return;}
        info_->setText("Reading video details…");
        probedVideo_=video.absoluteFilePath();probe_->start(ffprobe,{"-v","error","-select_streams","v:0","-show_entries","stream=width,height,avg_frame_rate:format=duration","-of","json",video.absoluteFilePath()});
    }
    void probed(bool ok){
        if(sourceKind_->currentIndex()!=0||busy()||QFileInfo(video_->text()).absoluteFilePath()!=probedVideo_)return;
        const auto json=QJsonDocument::fromJson(probe_->readAllStandardOutput().left(65536)).object();const auto streams=json["streams"].toArray();const auto stream=streams.isEmpty()?QJsonObject():streams.at(0).toObject();
        if(!ok||stream.isEmpty()){info_->setText("No readable video stream in this file.");return;}
        duration_=json["format"].toObject()["duration"].toString().toDouble();const auto rate=stream["avg_frame_rate"].toString().split('/');
        const double fps=rate.size()==2&&rate[1].toDouble()>0?rate[0].toDouble()/rate[1].toDouble():0;
        QStringList parts{QString("%1 × %2").arg(stream["width"].toInt()).arg(stream["height"].toInt())};if(duration_>0)parts<<formatDuration(duration_);if(fps>0)parts<<QString("%1 fps").arg(fps,0,'g',4);
        info_->setText(parts.join(" · "));updateEstimate();
    }
    void attention(const QString &text){setState(status_,"attention",text);}
    void setBusy(bool value){for(auto *w:inputs_)w->setEnabled(!value);start_->setEnabled(!value);cancel_->setEnabled(value);if(!value)refreshReconstruction();}
    bool sourceTools(){if(!QFileInfo(downloader_->text()).isExecutable()||!QFileInfo(deno_->text()).isExecutable()){toolsToggle_->setChecked(true);attention("Set the yt-dlp and Deno executables under Tools.");return false;}return true;}
    void inspectLink(bool importing){
        if(youtubeUrl(url_->text()).isEmpty()){attention("Enter a valid HTTPS YouTube video link.");return;}
        if(!sourceTools())return;
        pendingImport_=importing;canceled_=false;setBusy(true);progress_->setRange(0,0);progress_->show();log_->clear();setState(status_,"running","Inspecting YouTube link…");
        youtube_->inspect(downloader_->text(),deno_->text(),url_->text());
    }
    bool newJob(){QDir parent(output_->text());job_=parent.filePath("9xd-video-"+QUuid::createUuid().toString(QUuid::WithoutBraces));
        if(!QDir().mkpath(job_+"/images")){setState(status_,"failed","Cannot create the output folder.");return false;}
        sparseResult_={};surfaceResult_={};sparseMesh_.reset();surfaceMesh_.reset();setStep(1,"pending");setStep(2,"pending");activeJob_=true;manifest_={{"schema",1},{"state","preparing"},{"sourceType",sourceKind_->currentIndex()?"youtube":"local"},{"source",sourceKind_->currentIndex()?youtubeUrl(url_->text()):QFileInfo(video_->text()).absoluteFilePath()},
          {"fps",fps_->value()},{"maxFrames",count_->value()},{"maxSize",size_->value()},{"clipStart",clipStart_->value()},{"clipEnd",clipEnd_->value()}};
        results_->hide();thumbs_->clear();++thumbRun_;return true;
    }
    void beginDownload(){
        if(!newJob()){pendingImport_=false;setBusy(false);progress_->hide();return;}
        manifest_["sourceMetadata"]=metadata_;manifest_["keepVideo"]=keep_->isChecked();manifest_["downloadLimitMiB"]=budget_->value();manifest_["downloadHeight"]=resolution_->currentData().toInt();manifest_["downloader"]=downloader_->text();manifest_["javascriptRuntime"]=deno_->text();
        if(keep_->isChecked()){downloadDirectory_=job_+"/source";if(!QDir().mkpath(downloadDirectory_)){endSourceJob("Cannot create the video folder.");return;}}
        else{temporary_=std::make_unique<QTemporaryDir>(job_+"/.video-XXXXXX");if(!temporary_->isValid()){endSourceJob("Cannot create the temporary video cache.");return;}downloadDirectory_=temporary_->path();}
        manifest_["state"]="downloading";if(!writeJson(job_+"/job.json",manifest_)){endSourceJob("Cannot write the download job.");return;}
        setState(status_,"running","Downloading video…");progress_->setRange(0,0);progress_->setFormat("Downloading…");
        youtube_->download(downloader_->text(),deno_->text(),url_->text(),downloadDirectory_,resolution_->currentData().toInt(),qint64(budget_->value())*1024*1024);
    }
    void cleanupDownload(){
        if(temporary_){auto path=temporary_->path();temporary_.reset();manifest_["temporaryVideoRemoved"]=!QDir(path).exists();}
        else if(!downloadDirectory_.isEmpty()){QDir dir(downloadDirectory_);for(auto file:dir.entryInfoList(QDir::Files))if(file.fileName().contains(".part")||file.fileName().endsWith(".ytdl")||file.size()>qint64(budget_->value())*1024*1024)QFile::remove(file.absoluteFilePath());}
        downloadDirectory_.clear();activeVideo_.clear();
    }
    void endSourceJob(const QString &error){
        pendingImport_=false;cleanupDownload();if(activeJob_&&!job_.isEmpty()){manifest_["state"]=canceled_?"canceled":"failed";manifest_["error"]=error;writeJson(job_+"/job.json",manifest_);}
        activeJob_=false;setBusy(false);progress_->hide();setState(status_,canceled_?"canceled":"failed",error);logToggle_->setChecked(true);
    }
    void start(){
        if(busy())return;
        if(output_->text().trimmed().isEmpty()||!QDir(output_->text()).exists()){attention("Choose an existing output folder.");return;}
        if(!QFileInfo(ffmpeg_->text()).isExecutable()){toolsToggle_->setChecked(true);attention("Set the FFmpeg executable under Tools.");return;}
        if(!validClip()){attention("End must be after Start, and the fragment must be within the video.");return;}
        if(sourceKind_->currentIndex()==1){inspectLink(true);return;}
        if(!QFileInfo(video_->text()).isFile()){attention("Choose a source video.");return;}
        if(!newJob())return;activeVideo_=QFileInfo(video_->text()).absoluteFilePath();canceled_=false;log_->clear();extract();
    }
    void extract(){
        manifest_["state"]="extracting";
        QString filter=QString("fps=%1,scale=%2:%2:force_original_aspect_ratio=decrease:force_divisible_by=2").arg(fps_->value(),0,'f',4).arg(size_->value());
        QStringList args{"-nostdin","-v","error","-n","-ss",QString::number(clipStart_->value(),'f',3),"-i",activeVideo_};
        if(clipEnd_->value()>0)args<<"-t"<<QString::number(clipEnd_->value()-clipStart_->value(),'f',3);
        args<<"-map"<<"0:v:0"<<"-vf"<<filter<<"-frames:v"<<QString::number(count_->value())<<job_+"/images/frame-%06d.png";
        manifest_["arguments"]=QJsonArray::fromStringList(args);manifest_["executable"]=ffmpeg_->text();
        if(!writeJson(job_+"/job.json",manifest_)){endSourceJob("Cannot write the job manifest.");return;}
        setBusy(true);progress_->setFormat("%v / %m frames");progress_->setRange(0,qMax(1,expectedFrames()));progress_->setValue(0);progress_->show();poll_->start();
        setStep(0,"current");setState(status_,"running","Extracting frames…");process_->start(ffmpeg_->text(),args);
    }
    void finish(bool ok,const QString &error){
        poll_->stop();pendingImport_=false;cleanupDownload();const auto frames=frameCount();ok=ok&&frames>=2;
        manifest_["state"]=canceled_?"canceled":ok?"frames_ready":"failed";manifest_["frameCount"]=frames;if(!ok)manifest_["error"]=canceled_?"Canceled by user":error;
        bool saved=writeJson(job_+"/job.json",manifest_);activeJob_=false;for(auto *w:inputs_)w->setEnabled(true);start_->setEnabled(true);cancel_->setEnabled(false);
        progress_->setMaximum(qMax(progress_->maximum(),frames));progress_->setValue(frames);progress_->setVisible(!ok);
        if(!saved)setState(status_,"failed","Could not save the job result.");
        else if(canceled_)setState(status_,"canceled",QString("Canceled. %1 partial frames kept.").arg(frames));
        else if(ok)setState(status_,"done",QString("%1 frames ready. Build Sparse Points to recover camera poses and geometry.").arg(frames));
        else{setState(status_,"failed",frames<2&&error.isEmpty()?"Extraction produced fewer than 2 frames. Check the rate and the log.":"Extraction failed. Check the log and job.json.");logToggle_->setChecked(true);}
        setStep(0,ok?"done":"current");refreshReconstruction();
        if(frames>0){results_->show();resultText_->setText(QString("%1 frames in %2").arg(frames).arg(QDir::toNativeSeparators(job_)));loadThumbnails();}
    }
    ReconstructionOptions reconstructionOptions()const{ReconstructionOptions o;o.threads=threads_->value();o.imageSize=size_->value();o.denseSize=denseSize_->value();o.cacheGiB=cache_->value();o.poissonDepth=depth_->value();o.poissonTrim=trim_->value();o.maxVertices=importLimit_->value();o.maxFaces=importLimit_->value()*2;o.exhaustive=matchMode_->currentIndex()==1;o.singleCamera=sharedCamera_->isChecked();return o;}
    QJsonObject resultJson()const{auto relative=[this](const QString &p){return p.isEmpty()?QString():QDir(job_).relativeFilePath(p);};return {{"sparseRun",relative(sparseResult_.run)},{"model",relative(sparseResult_.model)},{"cloud",relative(sparseResult_.cloud)},{"summary",sparseResult_.summary.json()},{"surfaceRun",relative(surfaceResult_.run)},{"surface",relative(surfaceResult_.surface)},{"surfaceMode",surfaceResult_.mode}};}
    void refreshReconstruction(){if(!sparseButton_)return;bool idle=!busy();sparseButton_->setEnabled(idle&&frameCount()>=3);surfaceButton_->setEnabled(idle&&!sparseResult_.model.isEmpty());loadJob_->setEnabled(idle);importPoints_->setEnabled(idle&&!sparseResult_.cloud.isEmpty());importCameras_->setEnabled(idle&&!sparseResult_.summary.cameras.isEmpty());importSurface_->setEnabled(idle&&!surfaceResult_.surface.isEmpty());
        if(!sparseResult_.cloud.isEmpty())reconstructionInfo_->setText(QString("%1 / %2 frames registered · %3 points · mean reprojection error %4 px · %5 components (largest used).%6").arg(sparseResult_.summary.images).arg(frameCount()).arg(sparseResult_.summary.points).arg(sparseResult_.summary.meanError,0,'f',2).arg(sparseResult_.summary.components).arg(surfaceResult_.surface.isEmpty()?QString():" Surface ready: "+surfaceResult_.mode)+(sparseResult_.summary.images<frameCount()/2?" Low frame coverage; try a shorter continuous shot.":QString())+(sparseResult_.summary.meanError>2?" High reprojection error; inspect alignment carefully.":QString()));else reconstructionInfo_->setText(frameCount()>=3?QString("%1 frames ready for sparse reconstruction.").arg(frameCount()):"Extract at least 3 frames, or load an existing job.");}
    bool colmapReady(){if(!QFileInfo(colmap_->text()).isExecutable()){toolsToggle_->setChecked(true);attention("Select the COLMAP 4.2+ executable under Tools.");return false;}return true;}
    void buildSparse(){if(busy()||!colmapReady())return;if(frameCount()<3){attention("Extract at least 3 overlapping frames first.");return;}log_->clear();if(!recon_->sparse(colmap_->text(),job_,reconstructionOptions())){attention("Could not start sparse reconstruction.");return;}setBusy(true);progress_->show();setStep(1,"current");}
    void buildSurface(){if(busy()||!colmapReady()||sparseResult_.model.isEmpty())return;log_->clear();if(!recon_->surface(colmap_->text(),sparseResult_,reconstructionOptions(),surfaceMode_->currentIndex()==0)){attention("Could not start surface reconstruction.");return;}setBusy(true);progress_->show();setStep(2,"current");}
    void deliver(const std::shared_ptr<MeshData> &mesh,bool surface){if(!importToScene){attention("3D scene is unavailable in this standalone inspector.");return;}QString error;auto path=surface?surfaceResult_.surface:sparseResult_.cloud;if(importToScene(mesh,surface?"Reconstructed surface / "+surfaceResult_.mode:"Sparse reconstruction points",path,error))setState(status_,"done","Result opened in the 3D workspace. Geometry scale is uncalibrated.");else attention(error);}
    void importGeometry(bool surface){if(busy())return;auto cached=surface?surfaceMesh_:sparseMesh_;if(cached){deliver(cached,surface);return;}auto path=surface?surfaceResult_.surface:sparseResult_.cloud;if(path.isEmpty())return;auto o=reconstructionOptions();importCanceled_=false;importing_=true;importingSurface_=surface;setBusy(true);setState(status_,"running","Reading saved reconstruction…");importWatcher_->setFuture(QtConcurrent::run([this,path,o]{return readReconstructionPly(path,o.maxVertices,o.maxFaces,importCanceled_);}));}
    void setStep(int index,const QString &state){steps_[index]->setProperty("state",state);repolish(steps_[index]);}
    // First frames as thumbnails, one per event-loop turn so large frames don't stall the UI.
    void loadThumbnails(){
        const auto files=QDir(job_+"/images").entryList({"frame-*.png"},QDir::Files,QDir::Name).mid(0,8);const int run=thumbRun_;
        for(int i=0;i<files.size();++i)QTimer::singleShot(i*15,this,[this,run,path=job_+"/images/"+files[i],name=files[i]]{
            if(run!=thumbRun_)return;QImageReader reader(path);auto size=reader.size();if(size.isValid())reader.setScaledSize(size.scaled(thumbs_->iconSize(),Qt::KeepAspectRatio));
            auto image=reader.read();if(image.isNull())return;auto *item=new QListWidgetItem(QIcon(QPixmap::fromImage(image)),QString(),thumbs_);item->setToolTip(name);item->setSizeHint(thumbs_->iconSize()+QSize(8,8));
        });
    }
    QList<QWidget *> inputs_;QList<QLabel *> steps_;QLineEdit *video_,*output_,*ffmpeg_;QDoubleSpinBox *fps_;QSpinBox *count_,*size_;QPushButton *start_,*cancel_,*openFolder_;
    QLabel *status_,*info_,*estimate_,*toolState_,*resultText_;QToolButton *toolsToggle_,*logToggle_;QPlainTextEdit *log_;QProgressBar *progress_;QFrame *results_;QListWidget *thumbs_;
    QLineEdit *colmap_;QComboBox *matchMode_,*surfaceMode_;QCheckBox *sharedCamera_;QSpinBox *threads_,*denseSize_,*depth_,*importLimit_;QDoubleSpinBox *cache_,*trim_;QPushButton *sparseButton_=nullptr,*surfaceButton_,*loadJob_,*importPoints_,*importCameras_,*importSurface_;QLabel *reconstructionInfo_;
    ReconstructionJob *recon_=nullptr;ReconstructionOutput sparseResult_,surfaceResult_;std::shared_ptr<MeshData> sparseMesh_,surfaceMesh_;QFutureWatcher<PlyResult> *importWatcher_;std::atomic_bool importCanceled_=false;bool importing_=false,importingSurface_=false;
    QComboBox *sourceKind_,*resolution_;QWidget *online_;QLineEdit *url_,*downloader_,*deno_;QPushButton *inspect_;QSpinBox *budget_;QCheckBox *keep_;QDoubleSpinBox *clipStart_,*clipEnd_;
    YoutubeSource *youtube_;QJsonObject metadata_;bool pendingImport_=false,activeJob_=false;QString activeVideo_,downloadDirectory_,probedVideo_;std::unique_ptr<QTemporaryDir> temporary_;
    QProcess *process_,*probe_;QTimer *poll_,*probeDelay_;bool canceled_=false;double duration_=-1;int thumbRun_=0;QString job_;QJsonObject manifest_;
};

// ---- Image / spline modeling ------------------------------------------------------------------
// A control point drawn at constant screen size; position = image pixel coordinates.
class SplinePoint final:public QGraphicsEllipseItem{
public:
    SplinePoint(int row,const QColor &color,const QRectF &bounds,std::function<void(int,QPointF)> moved):QGraphicsEllipseItem(-5,-5,10,10),row_(row),bounds_(bounds),moved_(std::move(moved)){
        setFlags(ItemIsMovable|ItemIsSelectable|ItemSendsGeometryChanges|ItemIgnoresTransformations);setBrush(color);setPen(QPen(QColor(255,255,255,230),1.5));setZValue(2);setCursor(Qt::SizeAllCursor);
    }
    int row()const{return row_;}
    void place(QPointF at){placing_=true;setPos(at);placing_=false;}
protected:
    QVariant itemChange(GraphicsItemChange change,const QVariant &value)override{
        if(change==ItemPositionChange&&!placing_)return QPointF(qBound(bounds_.left(),value.toPointF().x(),bounds_.right()),qBound(bounds_.top(),value.toPointF().y(),bounds_.bottom()));
        if(change==ItemPositionHasChanged&&!placing_&&moved_)moved_(row_,value.toPointF());
        if(change==ItemSelectedHasChanged)setRect(value.toBool()?QRectF(-7,-7,14,14):QRectF(-5,-5,10,10));
        return QGraphicsEllipseItem::itemChange(change,value);
    }
private:int row_;QRectF bounds_;std::function<void(int,QPointF)> moved_;bool placing_=false;
};

// Reference image with zoom/pan like the UV / 2D image preview, plus click-to-add points.
class SplineCanvas final:public QGraphicsView{
public:
    std::function<void(QPointF)> addRequested;std::function<void()> deleteRequested;
    explicit SplineCanvas(QWidget *parent=nullptr):QGraphicsView(parent){
        setObjectName("splineCanvas");setScene(new QGraphicsScene(this));setRenderHint(QPainter::Antialiasing);setRenderHint(QPainter::SmoothPixmapTransform);
        setTransformationAnchor(AnchorUnderMouse);setDragMode(NoDrag);setBackgroundBrush(palette().brush(QPalette::Dark));setMinimumHeight(240);setFocusPolicy(Qt::StrongFocus);setAccessibleName("Reference image drawing area");
        addEmptyState(this,"image","No reference image","Choose an image above, then click on it to place curve points.");
    }
    // Replaces the scene; callers must drop pointers to their own items first.
    void setImage(const QImage &image){
        scene()->clear();bounds_={};
        if(image.isNull()){scene()->setSceneRect({});return;}
        scene()->addPixmap(QPixmap::fromImage(image))->setZValue(0);bounds_=QRectF(QPointF(0,0),image.size());
        const qreal pad=qMax(bounds_.width(),bounds_.height())*.25;scene()->setSceneRect(bounds_.adjusted(-pad,-pad,pad,pad));fit();
    }
    QRectF bounds()const{return bounds_;}
    void fit(){autoFit_=true;if(bounds_.isEmpty())return;resetTransform();fitInView(bounds_.adjusted(-8,-8,8,8),Qt::KeepAspectRatio);}
protected:
    void mousePressEvent(QMouseEvent *event)override{
        const bool onPoint=dynamic_cast<SplinePoint *>(itemAt(event->position().toPoint()));
        if(event->button()==Qt::MiddleButton||(event->button()==Qt::LeftButton&&!onPoint)){pan_=event->button();press_=last_=event->position();dragged_=false;if(event->button()==Qt::LeftButton)scene()->clearSelection();event->accept();return;}
        QGraphicsView::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event)override{
        if(pan_==Qt::NoButton)return QGraphicsView::mouseMoveEvent(event);
        const auto delta=event->position()-last_;last_=event->position();
        if(!dragged_&&(event->position()-press_).manhattanLength()>4){dragged_=true;viewport()->setCursor(Qt::ClosedHandCursor);}
        if(dragged_){autoFit_=false;horizontalScrollBar()->setValue(horizontalScrollBar()->value()-int(std::lround(delta.x())));verticalScrollBar()->setValue(verticalScrollBar()->value()-int(std::lround(delta.y())));}
        event->accept();
    }
    void mouseReleaseEvent(QMouseEvent *event)override{
        if(pan_==Qt::NoButton||event->button()!=pan_)return QGraphicsView::mouseReleaseEvent(event);
        const bool click=!dragged_&&pan_==Qt::LeftButton;pan_=Qt::NoButton;viewport()->unsetCursor();event->accept();
        if(click&&addRequested){const auto at=mapToScene(event->position().toPoint());if(bounds_.contains(at))addRequested(at);}
    }
    void wheelEvent(QWheelEvent *event)override{const double factor=std::exp(event->angleDelta().y()*.001);const auto next=transform().m11()*factor;if(next>=.01&&next<=128){autoFit_=false;scale(factor,factor);}event->accept();}
    void resizeEvent(QResizeEvent *event)override{QGraphicsView::resizeEvent(event);if(autoFit_)fit();}
    void keyPressEvent(QKeyEvent *event)override{
        if((event->key()==Qt::Key_Delete||event->key()==Qt::Key_Backspace)&&deleteRequested){deleteRequested();event->accept();return;}
        if(event->key()==Qt::Key_F&&event->modifiers()==Qt::NoModifier){fit();event->accept();return;}
        QGraphicsView::keyPressEvent(event);
    }
private:QRectF bounds_;Qt::MouseButton pan_=Qt::NoButton;QPointF press_,last_;bool dragged_=false,autoFit_=true;
};

class SplineInspector final:public QWidget{
public:
    explicit SplineInspector(QWidget *parent=nullptr):QWidget(parent){
        setObjectName("splineModelingInspector");auto *layout=scrollingBody(this);
        auto *imageBody=card(layout,"Reference image");auto *imageForm=formIn(imageBody);
        image_=pathField(imageForm,"Image","splineReferenceImage",false,"Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.webp *.tga);;All files (*)");
        imageInfo_=hint("Points are stored in this image's pixel coordinates.","splineImageInfo");imageBody->addWidget(imageInfo_);

        auto *drawBody=card(layout,"Drawing");auto *drawBar=new QHBoxLayout;drawBar->setSpacing(4);
        curve_=new QComboBox;curve_->setObjectName("splineActiveCurve");curve_->setToolTip("New points are added to this curve.");curve_->setAccessibleName("Active curve");curve_->setMinimumContentsLength(10);curve_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        auto *newCurve=iconButton("plus","New curve","splineNewCurve");auto *fit=iconButton("frame","Fit image to view (F)","splineFit");
        drawBar->addWidget(new QLabel("Curve"));drawBar->addWidget(curve_,1);drawBar->addWidget(newCurve);drawBar->addSpacing(8);drawBar->addWidget(fit);drawBody->addLayout(drawBar);
        canvas_=new SplineCanvas;drawBody->addWidget(canvas_,1);
        drawBody->addWidget(hint("Click to add a point to the active curve · drag a point to move it · Delete removes the selection · drag or middle-drag to pan · scroll to zoom"));

        auto *pointsBody=card(layout,"Points");auto *pointBar=new QHBoxLayout;pointBar->setSpacing(4);
        auto *add=iconButton("plus","Add point at the image centre","splineAddPoint"),*remove=iconButton("minus","Remove selected points","splineRemovePoint");
        auto *up=iconButton("up","Move point earlier in its curve","splineMoveUp"),*down=iconButton("down","Move point later in its curve","splineMoveDown");
        for(auto *b:{add,remove,up,down})pointBar->addWidget(b);pointBar->addStretch();count_=hint("","splinePointCount");count_->setWordWrap(false);pointBar->addWidget(count_);pointsBody->addLayout(pointBar);
        points_=new QTableWidget(0,3);points_->setObjectName("splineControlPoints");points_->setHorizontalHeaderLabels({"Curve ID","X (px)","Y (px)"});points_->horizontalHeader()->setStretchLastSection(true);
        points_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);points_->verticalHeader()->setDefaultSectionSize(24);points_->setSelectionBehavior(QAbstractItemView::SelectRows);points_->setMinimumHeight(150);points_->setAccessibleName("Control points");
        pointsBody->addWidget(points_,1);addEmptyState(points_,"select","No points yet","Click on the reference image, or use Add.");

        auto *projectRow=new QHBoxLayout;projectRow->setSpacing(6);project_=hint("","splineProject");project_->setWordWrap(false);projectRow->addWidget(project_,1);
        auto *load=new QPushButton("Load…");load->setObjectName("splineLoadProject");load->setIcon(workbenchIcon("folder"));save_=new QPushButton("Save…");save_->setObjectName("splineSaveProject");
        projectRow->addWidget(load);projectRow->addWidget(save_);layout->addLayout(projectRow);
        status_=new QLabel;status_->setObjectName("splineStatus");status_->setWordWrap(true);status_->setAccessibleName("Spline project status");layout->addWidget(status_);layout->addStretch();
        setState(status_,"ready","Choose a reference image to start drawing, or load a project.");

        imageDelay_=new QTimer(this);imageDelay_->setSingleShot(true);imageDelay_->setInterval(250);
        connect(image_,&QLineEdit::textChanged,this,[this]{if(!syncing_)markDirty();imageDelay_->start();});connect(imageDelay_,&QTimer::timeout,this,[this]{loadReference();});
        connect(points_,&QTableWidget::itemChanged,this,[this]{if(syncing_)return;markDirty();rebuild();});
        connect(points_->model(),&QAbstractItemModel::rowsInserted,this,[this]{if(!syncing_)rebuild();});connect(points_->model(),&QAbstractItemModel::rowsRemoved,this,[this]{if(!syncing_)rebuild();});
        connect(points_,&QTableWidget::itemSelectionChanged,this,[this]{selectFromTable();});
        connect(canvas_->scene(),&QGraphicsScene::selectionChanged,this,[this]{selectFromCanvas();});
        canvas_->addRequested=[this](QPointF at){addPoint(at);};canvas_->deleteRequested=[this]{removeSelected();};
        connect(add,&QToolButton::clicked,this,[this]{auto b=canvas_->bounds();addPoint(b.isEmpty()?QPointF():b.center());});
        connect(remove,&QToolButton::clicked,this,[this]{removeSelected();});connect(up,&QToolButton::clicked,this,[this]{moveSelected(-1);});connect(down,&QToolButton::clicked,this,[this]{moveSelected(1);});
        connect(newCurve,&QToolButton::clicked,this,[this]{addCurve();});connect(fit,&QToolButton::clicked,canvas_,[this]{canvas_->fit();});
        connect(save_,&QPushButton::clicked,this,[this]{auto p=QFileDialog::getSaveFileName(this,"Save spline project",path_.isEmpty()?QString("spline-project.json"):path_,"9XD spline project (*.json)");if(!p.isEmpty())saveProject(p);});
        connect(load,&QPushButton::clicked,this,[this]{if(!confirmClose())return;auto p=QFileDialog::getOpenFileName(this,"Load spline project",QFileInfo(path_).absolutePath(),"9XD spline project (*.json)");if(!p.isEmpty())loadProject(p);});
        rebuild();updateProject();
    }
    bool confirmClose(){return !dirty_||QMessageBox::question(this,"Unsaved spline project","Discard unsaved spline changes?",QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Cancel)==QMessageBox::Discard;}
    bool saveProject(const QString &path){
        QJsonArray points;for(int r=0;r<points_->rowCount();++r){auto text=[&](int c){auto *i=points_->item(r,c);return i?i->text():QString();};bool a,b;double x=text(1).toDouble(&a),y=text(2).toDouble(&b);if(text(0).isEmpty()||!a||!b||!std::isfinite(x)||!std::isfinite(y)){points_->selectRow(r);setState(status_,"attention",QString("Point %1 needs a curve ID and numeric X and Y before saving.").arg(r+1));return false;}points.append(QJsonObject{{"curve",text(0)},{"x",x},{"y",y}});}
        if(!writeJson(path,{{"schema",1},{"type","9xd-spline-project"},{"image",image_->text()},{"points",points}})){setState(status_,"failed","Could not save the project to "+QFileInfo(path).fileName()+".");return false;}
        dirty_=false;path_=path;updateProject();setState(status_,"done",QString("Saved %1 at %2.").arg(QFileInfo(path).fileName(),QTime::currentTime().toString("HH:mm")));return true;
    }
    bool loadProject(const QString &path){
        auto reject=[&](const QString &why){setState(status_,"failed",why+" Nothing was changed.");return false;};
        QFile file(path);if(!file.open(QIODevice::ReadOnly))return reject("Could not open "+QFileInfo(path).fileName()+".");if(file.size()>4*1024*1024)return reject("This file is larger than the 4 MB project limit.");
        auto obj=QJsonDocument::fromJson(file.readAll()).object();auto data=obj["points"].toArray();if(obj["schema"]!=1||obj["type"]!="9xd-spline-project"||!obj["points"].isArray()||data.size()>10000)return reject("Not a valid 9XD spline project.");
        for(auto value:data){auto p=value.toObject();if(!p["curve"].isString()||p["curve"].toString().isEmpty()||!p["x"].isDouble()||!p["y"].isDouble()||!std::isfinite(p["x"].toDouble())||!std::isfinite(p["y"].toDouble()))return reject("Not a valid 9XD spline project: a point has a missing curve or non-numeric coordinates.");}
        syncing_=true;image_->setText(obj["image"].toString());points_->setRowCount(0);
        for(auto value:data){auto p=value.toObject();int r=points_->rowCount();points_->insertRow(r);points_->setItem(r,0,new QTableWidgetItem(p["curve"].toString()));points_->setItem(r,1,new QTableWidgetItem(QString::number(p["x"].toDouble(),'g',17)));points_->setItem(r,2,new QTableWidgetItem(QString::number(p["y"].toDouble(),'g',17)));}
        syncing_=false;dirty_=false;path_=path;rebuild();updateProject();setState(status_,"done",QString("Loaded %1 · %2 points in %3 curves.").arg(QFileInfo(path).fileName()).arg(data.size()).arg(curves().size()));return true;
    }
private:
    struct Point{QString curve;QPointF at;bool valid=false;};
    Point point(int row)const{auto text=[&](int c){auto *i=points_->item(row,c);return i?i->text():QString();};Point p;bool a,b;p.curve=text(0);p.at={text(1).toDouble(&a),text(2).toDouble(&b)};p.valid=!p.curve.isEmpty()&&a&&b&&std::isfinite(p.at.x())&&std::isfinite(p.at.y());return p;}
    QStringList curves()const{QStringList list;for(int r=0;r<points_->rowCount();++r){auto c=point(r).curve;if(!c.isEmpty()&&!list.contains(c))list<<c;}return list;}
    QColor color(const QString &curve){
        static const QList<QColor> palette{QColor("#4F8EF7"),QColor("#F2A33A"),QColor("#3FB37F"),QColor("#E35D6A"),QColor("#A879F2"),QColor("#2FB8C4"),QColor("#D9C24A"),QColor("#F27BBF")};
        if(!colors_.contains(curve))colors_.insert(curve,palette[colors_.size()%palette.size()]);return colors_.value(curve);
    }
    QString activeCurve(){if(curve_->currentText().isEmpty())addCurve(false);return curve_->currentText();}
    void addCurve(bool announce=true){
        auto existing=curves();for(int i=0;i<curve_->count();++i)existing<<curve_->itemText(i);int n=1;while(existing.contains(QString("curve-%1").arg(n)))++n;const auto id=QString("curve-%1").arg(n);
        curve_->addItem(swatch(color(id)),id);curve_->setCurrentText(id);if(announce)setState(status_,"ready",QString("New curve %1. Click on the image to add its points.").arg(id));
    }
    static QIcon swatch(const QColor &c){QPixmap pixmap(12,12);pixmap.fill(Qt::transparent);{QPainter p(&pixmap);p.setRenderHint(QPainter::Antialiasing);p.setBrush(c);p.setPen(Qt::NoPen);p.drawEllipse(1,1,10,10);}return QIcon(pixmap);}
    void markDirty(){if(!dirty_){dirty_=true;updateProject();}}
    void updateProject(){
        project_->setText((path_.isEmpty()?QString("Untitled project"):QFileInfo(path_).fileName())+(dirty_?QString::fromUtf8(" · Unsaved changes"):QString()));
        if(save_->property("primary").toBool()!=dirty_){save_->setProperty("primary",dirty_);save_->setIcon(workbenchIcon("save",dirty_?QColor(Qt::white):QColor()));repolish(save_);}
    }
    void loadReference(){
        const auto path=image_->text().trimmed();QImage image;QString info;
        if(path.isEmpty())info="Points are stored in this image's pixel coordinates.";
        else if(!QFileInfo(path).isFile())info="Image not found. Points are kept; choose the image again to draw.";
        else{QImageReader reader(path);reader.setAutoTransform(false);const auto size=reader.size();
            if(size.isValid()&&qint64(size.width())*size.height()>100'000'000)info="This image is larger than the 100-megapixel preview limit.";
            else{image=reader.read();info=image.isNull()?"Cannot read this image: "+reader.errorString():QString("%1 × %2 px · points use these pixel coordinates.").arg(image.width()).arg(image.height());}}
        imageInfo_->setText(info);items_.clear();paths_.clear();selecting_=true;canvas_->setImage(image);selecting_=false;rebuild();
    }
    // The table is the source of truth; the canvas is redrawn from it.
    void rebuild(){
        selecting_=true;for(auto *item:items_)delete item;items_.clear();for(auto *path:paths_)delete path;paths_.clear();selecting_=false;
        const auto bounds=canvas_->bounds();int outside=0,invalid=0;QSet<QString> seen;
        syncing_=true;
        for(int r=0;r<points_->rowCount();++r){
            const auto p=point(r);if(!p.valid){++invalid;continue;}seen.insert(p.curve);const auto c=color(p.curve);
            if(auto *cell=points_->item(r,0))cell->setData(Qt::DecorationRole,c);
            const bool out=!bounds.isEmpty()&&!bounds.contains(p.at);outside+=out;
            for(int col=1;col<3;++col)if(auto *cell=points_->item(r,col)){cell->setForeground(out?QBrush(QColor(qApp->palette().color(QPalette::Window).lightness()<128?"#F5C87A":"#9B4F08")):QBrush());cell->setToolTip(out?QString("Outside the reference image (%1 × %2 px).").arg(bounds.width()).arg(bounds.height()):QString());}
            if(bounds.isEmpty())continue;
            auto *item=new SplinePoint(r,c,bounds,[this](int row,QPointF at){moved(row,at);});item->place(p.at);canvas_->scene()->addItem(item);items_<<item;
        }
        syncing_=false;
        // Curve list: curves in table order, plus empty curves created with New curve.
        const auto current=curve_->currentText();QStringList list=curves();for(int i=0;i<curve_->count();++i)if(!list.contains(curve_->itemText(i))&&curve_->itemText(i)==current)list<<current;
        curve_->blockSignals(true);curve_->clear();for(const auto &id:list)curve_->addItem(swatch(color(id)),id);curve_->setCurrentText(list.contains(current)?current:list.value(0));curve_->blockSignals(false);
        drawPaths();
        const int total=points_->rowCount();count_->setText(QString("%1 points · %2 curves").arg(total).arg(seen.size()));
        if(outside||invalid)setState(status_,"attention",QStringList{outside?QString("%1 points lie outside the reference image.").arg(outside):QString(),invalid?QString("%1 rows need a curve ID and numeric X/Y.").arg(invalid):QString()}.join(' ').trimmed());
        else if(status_->property("state").toString()=="attention")setState(status_,"ready","All points are valid.");
        selectFromTable();
    }
    void drawPaths(){
        for(auto *path:paths_)delete path;paths_.clear();if(canvas_->bounds().isEmpty())return;
        QHash<QString,QPainterPath> shapes;QStringList order;
        for(int r=0;r<points_->rowCount();++r){auto p=point(r);if(!p.valid)continue;if(!shapes.contains(p.curve)){shapes[p.curve].moveTo(p.at);order<<p.curve;}else shapes[p.curve].lineTo(p.at);}
        for(const auto &id:order){QPen pen(color(id),2);pen.setCosmetic(true);pen.setJoinStyle(Qt::RoundJoin);auto *item=canvas_->scene()->addPath(shapes[id],pen);item->setZValue(1);paths_<<item;}
    }
    void moved(int row,QPointF at){
        auto round=[](double v){return QString::number(std::round(v*100)/100,'g',12);};
        syncing_=true;if(auto *x=points_->item(row,1))x->setText(round(at.x()));if(auto *y=points_->item(row,2))y->setText(round(at.y()));syncing_=false;
        markDirty();drawPaths();
    }
    void addPoint(QPointF at){
        const auto curve=activeCurve();int row=points_->rowCount();for(int r=0;r<points_->rowCount();++r)if(point(r).curve==curve)row=r+1;
        auto round=[](double v){return QString::number(std::round(v*100)/100,'g',12);};
        syncing_=true;points_->insertRow(row);points_->setItem(row,0,new QTableWidgetItem(curve));points_->setItem(row,1,new QTableWidgetItem(round(at.x())));points_->setItem(row,2,new QTableWidgetItem(round(at.y())));syncing_=false;
        markDirty();rebuild();points_->selectRow(row);canvas_->setFocus();
    }
    QList<int> selectedRows()const{QSet<int> rows;for(auto *item:points_->selectedItems())rows.insert(item->row());auto list=rows.values();std::sort(list.begin(),list.end());return list;}
    void removeSelected(){
        auto rows=selectedRows();if(rows.isEmpty())return;syncing_=true;for(int i=rows.size()-1;i>=0;--i)points_->removeRow(rows[i]);syncing_=false;
        markDirty();rebuild();setState(status_,"ready",QString("Removed %1 point%2.").arg(rows.size()).arg(rows.size()==1?"":"s"));
    }
    void moveSelected(int delta){
        auto rows=selectedRows();if(rows.size()!=1)return;const int from=rows[0],to=from+delta;if(to<0||to>=points_->rowCount())return;
        if(point(from).curve!=point(to).curve){setState(status_,"attention","A point can only move within its own curve.");return;}
        syncing_=true;for(int c=0;c<3;++c){auto *a=points_->takeItem(from,c),*b=points_->takeItem(to,c);points_->setItem(from,c,b);points_->setItem(to,c,a);}syncing_=false;
        markDirty();rebuild();points_->selectRow(to);
    }
    void selectFromTable(){
        if(selecting_)return;selecting_=true;const auto rows=selectedRows();QSet<int> wanted(rows.begin(),rows.end());
        for(auto *item:items_)item->setSelected(wanted.contains(item->row()));
        if(rows.size()==1){const auto c=point(rows[0]).curve;if(!c.isEmpty())curve_->setCurrentText(c);}selecting_=false;
    }
    void selectFromCanvas(){
        if(selecting_)return;selecting_=true;points_->clearSelection();int last=-1;
        for(auto *item:canvas_->scene()->selectedItems())if(auto *p=dynamic_cast<SplinePoint *>(item)){for(int c=0;c<3;++c)if(auto *cell=points_->item(p->row(),c))cell->setSelected(true);last=p->row();}
        if(last>=0){points_->scrollToItem(points_->item(last,0));const auto c=point(last).curve;if(!c.isEmpty())curve_->setCurrentText(c);}selecting_=false;
    }
    QLineEdit *image_;QLabel *imageInfo_,*count_,*project_,*status_;QComboBox *curve_;SplineCanvas *canvas_;QTableWidget *points_;QPushButton *save_;QTimer *imageDelay_;
    QList<SplinePoint *> items_;QList<QGraphicsPathItem *> paths_;QHash<QString,QColor> colors_;QString path_;bool dirty_=false,syncing_=false,selecting_=false;
};
}
