#pragma once
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <functional>
#include <memory>
#include <QTimer>

inline bool localOllamaUrl(const QUrl &url){return url.scheme()=="http"&&QStringList{"localhost","127.0.0.1","::1"}.contains(url.host().toLower())&&url.userInfo().isEmpty()&&url.query().isEmpty()&&url.fragment().isEmpty()&&(url.path().isEmpty()||url.path()=="/");}
class OllamaClient final:public QObject {
public:
    using Chunk=std::function<void(QJsonObject)>;using Finished=std::function<void(QString)>;
    explicit OllamaClient(QObject *parent=nullptr):QObject(parent){network_.setProxy(QNetworkProxy::NoProxy);}
    ~OllamaClient()override{if(reply_){disconnect(reply_,nullptr,this,nullptr);reply_->abort();}}
    bool busy()const{return !reply_.isNull();}
    void cancel(){if(reply_)reply_->abort();}
    void request(const QString &endpoint,const QString &path,const QJsonObject &body,bool stream,Chunk chunk,Finished finished){
        if(busy()){finished("An Ollama request is already running.");return;}QUrl url(endpoint);if(!localOllamaUrl(url)){finished("Use a local HTTP Ollama address: localhost, 127.0.0.1 or ::1.");return;}url.setPath(path);QNetworkRequest request(url);request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);request.setTransferTimeout(120000);
        auto *reply=body.isEmpty()?network_.get(request):network_.post(request,QJsonDocument(body).toJson(QJsonDocument::Compact));reply_=reply;QTimer::singleShot(180000,reply,[reply]{if(!reply->isFinished())reply->abort();});auto buffer=std::make_shared<QByteArray>();auto total=std::make_shared<qint64>(0);auto error=std::make_shared<QString>();
        auto consume=[=]{auto bytes=reply->isOpen()?reply->readAll():QByteArray();*total+=bytes.size();if(*total>1048576){*error="Ollama response exceeds 1 MiB.";reply->abort();return;}*buffer+=bytes;if(!stream)return;while(buffer->contains('\n')){int end=buffer->indexOf('\n');auto line=buffer->left(end);buffer->remove(0,end+1);if(line.trimmed().isEmpty())continue;auto doc=QJsonDocument::fromJson(line);if(!doc.isObject()){*error="Invalid Ollama response.";reply->abort();return;}if(doc.object().contains("error"))*error=doc.object()["error"].toString();else chunk(doc.object());}};
        connect(reply,&QNetworkReply::readyRead,this,consume);
        connect(reply,&QNetworkReply::finished,this,[=,this]{consume();int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();if(error->isEmpty()&&reply->error()!=QNetworkReply::NoError)*error=reply->error()==QNetworkReply::OperationCanceledError?"Request stopped.":"Cannot contact Ollama: "+reply->errorString();if(error->isEmpty()&&(status<200||status>=300))*error="Ollama HTTP error "+QString::number(status);if(error->isEmpty()&&!buffer->trimmed().isEmpty()){auto doc=QJsonDocument::fromJson(*buffer);if(!doc.isObject())*error="Invalid Ollama JSON.";else if(doc.object().contains("error"))*error=doc.object()["error"].toString();else chunk(doc.object());}reply_=nullptr;reply->deleteLater();finished(*error);});
    }
private:QNetworkAccessManager network_;QPointer<QNetworkReply> reply_;
};
