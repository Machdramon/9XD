#pragma once
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>
#include <functional>
#include <cmath>

namespace Reconstruction {
// Only canonical single-video URLs enter the external downloader. No shell is used.
inline QString youtubeUrl(const QString &input){
    QUrl u(input.trimmed());if(u.scheme()!="https"||!u.userInfo().isEmpty()||(u.port()!=-1&&u.port()!=443))return {};
    auto host=u.host().toLower();QString id;
    if(host=="youtu.be")id=u.path().mid(1);
    else if(host=="youtube.com"||host=="www.youtube.com"||host=="m.youtube.com"){
        if(u.path()=="/watch")id=QUrlQuery(u).queryItemValue("v");
        else if(u.path().startsWith("/shorts/")||u.path().startsWith("/live/"))id=u.path().section('/',2,2);
    }
    if(!QRegularExpression("^[A-Za-z0-9_-]{11}$").match(id).hasMatch())return {};
    return "https://www.youtube.com/watch?v="+id;
}
inline QStringList youtubeArguments(const QString &runtime){
    QStringList a{"--ignore-config","--no-plugin-dirs","--no-remote-components","--no-playlist","--no-mark-watched","--no-cache-dir","--no-update","--socket-timeout","20","--retries","2","--extractor-retries","1","--encoding","utf-8","--color","never"};
    if(!runtime.isEmpty())a<<"--no-js-runtimes"<<"--js-runtimes"<<"deno:"+runtime;
    return a;
}
class YoutubeSource final:public QObject {
public:
    enum class Stage{Idle,Inspect,Download};
    std::function<void(const QJsonObject &)> metadataReady;
    std::function<void(const QString &,const QString &)> downloaded; // file, error
    std::function<void(const QString &)> inspectionFailed,log;
    std::function<void(qint64,qint64)> progress;
    explicit YoutubeSource(QObject *parent=nullptr):QObject(parent){
        process_=new QProcess(this);timer_=new QTimer(this);timer_->setInterval(200);deadline_=new QTimer(this);deadline_->setSingleShot(true);
        connect(process_,&QProcess::started,this,[this]{attachJob();});
        connect(process_,&QProcess::readyReadStandardOutput,this,[this]{readOutput();});
        connect(process_,&QProcess::readyReadStandardError,this,[this]{auto s=process_->readAllStandardError();errors_=(errors_+s).right(16384);if(log)log(QString::fromUtf8(s).left(4096));});
        connect(process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError e){if(e==QProcess::FailedToStart)finish(false,process_->errorString());});
        connect(process_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus e){readOutput();finish(code==0&&e==QProcess::NormalExit,QString::fromUtf8(errors_).trimmed());});
        connect(timer_,&QTimer::timeout,this,[this]{if(stage_!=Stage::Download)return;qint64 size=0;for(const auto &f:QDir(directory_).entryInfoList(QDir::Files|QDir::NoDotAndDotDot))size+=f.size();if(size>limit_)stop("Download exceeded the storage limit.");});
        connect(deadline_,&QTimer::timeout,this,[this]{stop("The video source timed out. Retry or use a local file.");});
    }
    ~YoutubeSource()override{metadataReady={};downloaded={};inspectionFailed={};log={};progress={};if(busy()){process_->kill();process_->waitForFinished(1000);}closeJob();}
    bool busy()const{return stage_!=Stage::Idle;}
    Stage stage()const{return stage_;}
    void inspect(const QString &program,const QString &runtime,const QString &url){
        if(busy())return;url_=youtubeUrl(url);if(url_.isEmpty()){if(inspectionFailed)inspectionFailed("Enter a valid HTTPS YouTube video link.");return;}
        auto a=youtubeArguments(runtime);a<<"--skip-download"<<"--dump-single-json"<<"--no-progress"<<"--"<<url_;
        begin(Stage::Inspect,program,a,90000);
    }
    void download(const QString &program,const QString &runtime,const QString &url,const QString &directory,int height,qint64 limit){
        if(busy())return;url_=youtubeUrl(url);directory_=QDir(directory).absolutePath();limit_=limit;
        if(url_.isEmpty()||limit<=0||!QDir(directory_).exists()||!QDir(directory_).entryList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot).isEmpty()){
            if(downloaded)downloaded({},"Invalid video link, budget or nonempty download directory.");return;
        }
        auto a=youtubeArguments(runtime);a<<"--no-overwrites"<<"--no-continue"<<"--fixup"<<"never"<<"--downloader"<<"native"<<"--concurrent-fragments"<<"1"
          <<"--max-filesize"<<QString::number(limit)<<"--format"<<QString("bestvideo[height<=%1]/best[height<=%1]").arg(height)
          <<"--output"<<QDir(directory_).filePath("source.%(ext)s")<<"--print"<<"after_move:9XD_FILE:%(filepath)s"
          <<"--newline"<<"--progress"<<"--progress-delta"<<"0.25"<<"--progress-template"<<"download:9XD_PROGRESS:%(progress.downloaded_bytes)s:%(progress.total_bytes,progress.total_bytes_estimate)s"<<"--"<<url_;
        begin(Stage::Download,program,a,30*60*1000);timer_->start();
    }
    void cancel(){stop("Canceled by user.");}
private:
    void begin(Stage stage,const QString &program,const QStringList &args,int timeout){
        stage_=stage;buffer_.clear();errors_.clear();file_.clear();forcedError_.clear();
        auto env=QProcessEnvironment::systemEnvironment();env.insert("YTDLP_NO_PLUGINS","1");process_->setProcessEnvironment(env);
        process_->start(program,args);deadline_->start(timeout);
    }
    void stop(const QString &error){if(!busy())return;forcedError_=error;
        terminateJob();
        if(process_->state()!=QProcess::NotRunning)process_->kill();else finish(false,error);
    }
    void readOutput(){
        buffer_+=process_->readAllStandardOutput();
        if(stage_==Stage::Inspect){if(buffer_.size()>2*1024*1024)stop("Video metadata exceeds the supported size.");return;}
        qsizetype at;while((at=buffer_.indexOf('\n'))>=0){auto line=QString::fromUtf8(buffer_.left(at)).trimmed();buffer_.remove(0,at+1);
            if(line.startsWith("9XD_FILE:"))file_=line.mid(9);
            else if(line.startsWith("9XD_PROGRESS:")){auto p=line.mid(13).split(':');if(p.size()==2&&progress)progress(p[0].toLongLong(),p[1].toLongLong());}
            else if(log&&!line.isEmpty())log(line.left(4096));
        }
        if(buffer_.size()>65536)stop("Unexpected downloader output.");
    }
    void finish(bool ok,QString error){
        if(!busy())return;auto old=stage_;stage_=Stage::Idle;timer_->stop();deadline_->stop();closeJob();
        if(!forcedError_.isEmpty()){ok=false;error=forcedError_;}
        if(old==Stage::Inspect){
            QJsonParseError e;auto data=QJsonDocument::fromJson(buffer_,&e).object();double seconds=data["duration"].toDouble(-1);
            if(ok&&(e.error!=QJsonParseError::NoError||data["_type"].toString()=="playlist"||data["is_live"].toBool()||data["live_status"].toString()=="is_upcoming"||!std::isfinite(seconds)||seconds<=0||seconds>604800)){
                ok=false;error="Choose a completed video with a readable duration; playlists and live streams are not supported.";
            }
            if(ok){QJsonObject clean{{"url",url_},{"id",data["id"].toString().left(64)},{"title",data["title"].toString().left(256)},{"duration",seconds},{"width",data["width"]},{"height",data["height"]},{"downloaderVersion",data["_version"].toObject()["version"]}};if(metadataReady)metadataReady(clean);}
            else if(inspectionFailed)inspectionFailed(error.isEmpty()?"Could not inspect the link. Check the downloader and Deno paths or update yt-dlp.":error.left(2000));
        }else{
            QFileInfo f(file_);auto actual=f.canonicalFilePath();auto parent=QFileInfo(directory_).canonicalFilePath();
            if(ok&&(!f.isFile()||f.isSymLink()||QFileInfo(actual).absolutePath()!=parent||f.size()>limit_||f.size()==0)){
                ok=false;error="No complete video within the storage limit was downloaded.";
            }
            if(downloaded)downloaded(ok?actual:QString(),ok?QString():error.isEmpty()?"Download failed. Check the log or update yt-dlp.":error.left(2000));
        }
    }
    void attachJob();
    void closeJob();
    void terminateJob();
    QProcess *process_;QTimer *timer_,*deadline_;Stage stage_=Stage::Idle;QByteArray buffer_,errors_;QString url_,directory_,file_,forcedError_;qint64 limit_=0;
    void *job_=nullptr;
};
}
