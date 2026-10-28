#pragma once
#include "binary_document.h"
#include "image_containers.h"
#include "genki_sampling.h"
#include "memory_budget.h"
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QCache>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QSettings>
#include <QCoreApplication>
#include <QJsonDocument>

// Explicit context selection, never nearest-file/image-count guessing.
namespace GenkiTrack {
inline QString hash(const QByteArray &bytes){return QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());}
inline QString childPath(const QString &root,const QString &relative){
    if(relative.isEmpty()||QDir::isAbsolutePath(relative))return {};
    const auto base=QFileInfo(root).canonicalFilePath();const auto path=QFileInfo(QDir(base).filePath(relative)).canonicalFilePath();
    if(base.isEmpty()||path.isEmpty()||!QFileInfo(path).isFile()||!path.startsWith(base+"/",Qt::CaseInsensitive))return {};return path;
}
inline QByteArray read(const QString &path,const std::atomic_bool &cancel,QString &error){
    QFileInfo info(path);if(!info.isFile()||info.size()<16||info.size()>qMin<qint64>(64*1024*1024,AssetMemory::bytes()/16)){error="Texture-context file is missing or exceeds the read budget.";return {};}
    ReadRequest source;source.path=path;source.expectedSize=info.size();source.expectedModified=info.lastModified();QByteArray bytes;
    for(qint64 p=0;p<source.expectedSize;){if(cancel.load()){error="Track texture read canceled.";return {};}auto r=source;r.offset=p;r.length=qMin<qint64>(1048576,source.expectedSize-p);auto page=readPage(r);if(!page.error.isEmpty()||page.bytes.size()!=r.length){error=page.error.isEmpty()?"Incomplete track texture read.":page.error;return {};}bytes+=page.bytes;p+=page.bytes.size();}
    auto final=readPage(source);if(!final.error.isEmpty()){error=final.error;return {};}return bytes;
}
inline QJsonObject bank(const QString &root,const QJsonObject &ref,const std::atomic_bool &cancel,QString &error){
    auto path=childPath(root,ref["path"].toString());const auto expected=ref["sha256"].toString().toLower();
    if(path.isEmpty()||!QRegularExpression("^[0-9a-f]{64}$").match(expected).hasMatch()){error="Bank path/hash is missing or outside the selected folder.";return {};}
    auto bytes=read(path,cancel,error);if(!error.isEmpty())return {};if(hash(bytes)!=expected){error="Bank fingerprint does not match this track/condition profile: "+ref["path"].toString();return {};}
    auto u=[&](qint64 p){return qFromLittleEndian<quint32>(bytes.constData()+p);};qint64 at=0,end=bytes.size();int section=ref["section"].toInt(-1);
    if(section<0||section>255){error="Invalid bank section.";return {};}
    if(!bytes.startsWith("TIM2")){
        auto count=u(0);if(!count||count>256||4+qint64(count)*4>bytes.size()||section>=int(count)){error="Invalid bank directory.";return {};}
        qint64 previous=4+qint64(count)*4-1;for(quint32 i=0;i<count;++i){auto offset=u(4+i*4);if(offset<=previous||offset%16||offset>=bytes.size()){error="Invalid bank directory bounds.";return {};}previous=offset;}
        at=u(4+section*4);if(section+1<int(count))end=u(8+section*4);
    }else if(section!=0){error="Standalone TIM2 has only section zero.";return {};}
    if(end-at<16||bytes.mid(at,4)!="TIM2"||quint8(bytes[at+5])>1){error="Selected section is not supported TIM2.";return {};}
    int count=qFromLittleEndian<quint16>(bytes.constData()+at+6);if(count<1||count>4096){error="Invalid picture count.";return {};}
    qint64 p=at+(bytes[at+5]?128:16);QJsonArray pictures;
    for(int i=0;i<count;++i){if(cancel.load()){error="Track texture scan canceled.";return {};}if(p>end-48){error="Truncated TIM2 picture.";return {};}
        qint64 total=u(p),palette=u(p+4),pixels=u(p+8);int header=qFromLittleEndian<quint16>(bytes.constData()+p+12),w=qFromLittleEndian<quint16>(bytes.constData()+p+20),h=qFromLittleEndian<quint16>(bytes.constData()+p+22);
        if(total<48||total>end-p||header<48||qint64(header)+palette+pixels>total||w<1||h<1||qint64(w)*h>4194304){error="Invalid TIM2 picture extent.";return {};}
        const int format=quint8(bytes[p+16]),type=quint8(bytes[p+19]);bool supported=(format==0||tim2TransferBits(bytes,p))&&type>=1&&type<=5;
        pictures.append(QJsonObject{{"width",w},{"height",h},{"supported",supported}});p+=total;
    }
    return {{"path",path},{"sha256",expected},{"section",section},{"offset",QString::number(at)},{"length",QString::number(end-at)},{"size",QString::number(bytes.size())},{"modified",QFileInfo(path).lastModified().toString(Qt::ISODateWithMs)},{"pictures",pictures}};
}
inline QSettings contextSettings(){auto *app=QCoreApplication::instance();auto path=app?app->property("workspaceSettingsFile").toString():QString();if(path.isEmpty())return QSettings("BinaryWorkbench","Workspace");return QSettings(path,QSettings::IniFormat);}
inline QJsonArray knownContexts(){auto settings=contextSettings();return QJsonDocument::fromJson(settings.value("genki/trackContexts/v1").toByteArray()).array();}
inline void rememberContext(QJsonObject context){
    if(context["root"].toString().isEmpty()||context["profile"].toObject()["variant"].toString().isEmpty())return;
    auto all=knownContexts();QJsonArray next{context};for(auto v:all)if(v.toObject()!=context&&next.size()<8)next.append(v);
    auto settings=contextSettings();settings.setValue("genki/trackContexts/v1",QJsonDocument(next).toJson(QJsonDocument::Compact));settings.sync();
}
inline bool covers(const QJsonObject &profile,const QString &path){bool ok;auto id=QFileInfo(path).completeBaseName().toULongLong(&ok,16);if(!ok)return false;for(auto v:profile["segments"].toArray()){auto s=v.toObject();bool a,b;auto lo=s["startId"].toString().toULongLong(&a,0),hi=s["endId"].toString().toULongLong(&b,0);if(a&&b&&id>=lo&&id<hi)return true;}return false;}
inline QJsonObject decorate(QJsonObject scan,const QByteArray &geometry,ReadRequest source,const QJsonObject &context,const std::atomic_bool &cancel){
    if(cancel.load())return {{"error","Track context scan canceled."}};
    if(context.isEmpty()){
        bool genki=false;for(auto v:scan["meshCandidates"].toArray())genki|=v.toObject()["profileId"]=="native-genki-part";if(!genki||source.path.isEmpty())return scan;
        QJsonArray options;QFile builtins(":/mesh-profiles/genki-txr3-oval-contexts.json");if(builtins.open(QIODevice::ReadOnly)){
            const auto profiles=QJsonDocument::fromJson(builtins.readAll()).array();auto dir=QFileInfo(source.path).dir();
            for(int depth=0;depth<3;++depth){for(auto v:profiles)if(covers(v.toObject(),source.path))options.append(QJsonObject{{"profile",v},{"root",dir.absolutePath()}});if(!dir.cdUp())break;}
        }
        for(auto v:knownContexts())if(covers(v.toObject()["profile"].toObject(),source.path))options.append(v);
        for(auto option:options){if(cancel.load())return {{"error","Track context scan canceled."}};auto resolved=decorate(scan,geometry,source,option.toObject(),cancel);if(resolved.contains("error"))return resolved;if(resolved.contains("genkiTrackContext")){resolved["genkiTrackContextAutomatic"]=true;auto notes=resolved["notes"].toArray();notes.append("Track textures: automatically detected "+resolved["genkiTrackContext"].toString()+". Global/local companions verified.");resolved["notes"]=notes;return resolved;}}
        if(!options.isEmpty()){auto notes=scan["notes"].toArray();notes.append("Track textures: no verified companion set found beside this model or in remembered locations. Geometry remains available; Texture options can select another folder.");scan["notes"]=notes;}return scan;
    }
    QString error;auto profile=context["profile"].toObject();auto root=context["root"].toString();
    auto reject=[&](QString text){if(cancel.load())return QJsonObject{{"error","Track context scan canceled."}};auto notes=scan["notes"].toArray();notes.append("Track textures: "+text+" Geometry remains available.");scan["notes"]=notes;return scan;};
    if(profile["schema"].toInt()!=1||profile["game"]!="txr3"||profile["variant"].toString().isEmpty())return reject("Unsupported context profile.");
    bool ok=false;const auto id=QFileInfo(source.path).completeBaseName().toULongLong(&ok,16);if(!ok)return reject("The source needs a mapped hexadecimal resource name.");
    QJsonObject segment;for(auto value:profile["segments"].toArray()){auto s=value.toObject();bool loOk,hiOk;auto lo=s["startId"].toString().toULongLong(&loOk,0),hi=s["endId"].toString().toULongLong(&hiOk,0);if(!loOk||!hiOk||lo>=hi)return reject("Invalid segment range.");if(id>=lo&&id<hi){if(!segment.isEmpty())return reject("Ambiguous overlapping segments.");segment=s;}}
    if(segment.isEmpty())return reject("This resource is outside the selected track/condition context.");
    // Verify extraction identity, including copied study fixtures. Patches are still allowed.
    const auto counterpart=childPath(root,QFileInfo(source.path).fileName());auto original=read(counterpart,cancel,error);if(!error.isEmpty())return reject(error);
    QByteArray sourceBytes;for(qint64 p=0;p<source.expectedSize;){if(cancel.load())return reject("Canceled.");auto request=source;request.offset=p;request.length=qMin<qint64>(1048576,source.expectedSize-p);auto page=readPage(request);if(!page.error.isEmpty()||page.bytes.size()!=request.length)return reject("Cannot verify source identity.");sourceBytes+=page.bytes;p+=page.bytes.size();if(sourceBytes.size()>32*1024*1024)return reject("Source identity exceeds context budget.");}
    if(hash(original)!=hash(sourceBytes))return reject("Source differs from the selected extraction folder.");
    QJsonObject refs=segment["local"].toObject();const auto globals=profile["global"].toObject();for(auto i=globals.begin();i!=globals.end();++i){if(i.key()!="0"&&i.key()!="4")return reject("Invalid global selector.");refs[i.key()]=i.value();}
    QMap<QString,QJsonObject> banks;for(auto i=refs.begin();i!=refs.end();++i){auto value=bank(root,i.value().toObject(),cancel,error);if(!error.isEmpty())return reject(error);banks.insert(i.key(),value);}
    const auto geometryHash=hash(geometry);
    auto bind=[&](QJsonObject c){if(!c["textureExpected"].toBool()||!c["topologyValidated"].toBool()||c["uvType"]!="Float32")return c;auto selected=banks.value(QString::number(c["bank"].toInt(-1)));auto pictures=selected["pictures"].toArray();int image=c["textureIndex"].toInt(-1);if(selected.isEmpty()||image<0||image>=pictures.size()||!pictures[image].toObject()["supported"].toBool())return c;
        auto picture=pictures[image].toObject();selected.remove("pictures");selected["image"]=image;selected["geometrySha256"]=geometryHash;selected["descriptor"]=c["offset"];selected["bankId"]=c["bank"];selected["variant"]=profile["variant"];selected["condition"]=profile["condition"];selected["segmentStart"]=segment["startId"];
        c["resolution"]="external-track-context";c["genkiTrackBinding"]=selected;c["textureAssigned"]=true;c["textureReferenceResolved"]=true;c["textureDecodeSupported"]=true;c["textureWidth"]=picture["width"];c["textureHeight"]=picture["height"];c["uvScaleU"]=1.;c["uvScaleV"]=1.;
        const bool regionResolved=GenkiSampling::apply(c,picture["width"].toInt(),picture["height"].toInt());
        const bool exact=c["clampMinU"].toInt()==0&&c["clampMinV"].toInt()==0&&c["clampMaxU"].toInt()+1==picture["width"].toInt()&&c["clampMaxV"].toInt()+1==picture["height"].toInt();
        if(!regionResolved){c["textureWrapU"]=exact?"Clamp":"Repeat";c["textureWrapV"]=exact?"Clamp":"Repeat";}QString status="External primary image linked by explicit "+profile["variant"].toString()+" / segment "+segment["startId"].toString()+" context.";
        if(regionResolved)status+=" Contiguous GS repeat/clamp region mapped to the standalone image.";
        if(!exact&&!regionResolved){status+=" Sampler/atlas region not verified; Repeat preview is used.";c["textureWarnings"]=QJsonArray{"External binding verified by context/index; exact game sampler/atlas behavior pending."};}
        if(c["bank"].toInt()==4){status+=" Animated bank: static source image preview only.";}
        c["textureStatus"]=status;c["basis"]=status;return c;};
    QJsonArray candidates;for(auto v:scan["meshCandidates"].toArray()){auto c=v.toObject();if(c["profileId"]=="native-genki-part")c=bind(c);candidates.append(c);}scan["meshCandidates"]=candidates;
    QJsonArray evidence;for(auto v:scan["meshEvidence"].toArray()){auto e=v.toObject();if(e["family"]=="Genki PS2 0IVX/XMDL"){QJsonArray materials;for(auto m:e["materialReferences"].toArray())materials.append(bind(m.toObject()));e["materialReferences"]=materials;}evidence.append(e);}scan["meshEvidence"]=evidence;scan["genkiTrackContext"]=profile["variant"];scan["genkiDetectedContext"]=context;return scan;
}
inline QImage decode(const QJsonObject &binding,const QByteArray &geometry,int bankId,int image,qint64 descriptor,QString &error,const std::atomic_bool &cancel){
    error.clear();if(cancel.load()){error="Track image canceled.";return {};}
    if(binding["geometrySha256"].toString()!=hash(geometry)||binding["bankId"].toInt(-1)!=bankId||binding["image"].toInt(-1)!=image||binding["descriptor"].toString().toLongLong()!=descriptor){error="Track binding no longer matches geometry/material; scan again.";return {};}
    const auto path=binding["path"].toString();QFileInfo info(path);if(!info.isFile()||QString::number(info.size())!=binding["size"].toString()||info.lastModified().toString(Qt::ISODateWithMs)!=binding["modified"].toString()){error="External bank changed; rescan the context.";return {};}
    const auto key=path+"|"+binding["sha256"].toString()+"|"+binding["modified"].toString()+"|"+binding["offset"].toString()+"|"+QString::number(image);
    static QMutex mutex;static QCache<QString,QImage> cache;{QMutexLocker guard(&mutex);cache.setMaxCost(int(qMin<qint64>(262144,AssetMemory::bytes()/16/1024)));if(auto hit=cache.object(key))return *hit;}
    auto bytes=read(path,cancel,error);if(!error.isEmpty())return {};if(hash(bytes)!=binding["sha256"].toString()){error="External bank fingerprint changed.";return {};}
    bool a,b;auto start=binding["offset"].toString().toLongLong(&a),length=binding["length"].toString().toLongLong(&b);if(!a||!b||start<0||length<16||start>bytes.size()-length){error="Invalid external bank bounds.";return {};}
    ImageRecipe recipe;recipe.surface=image;auto result=decodeTim2(bytes.mid(start,length),recipe,error,cancel);if(cancel.load())return {};
    if(!result.isNull()){QMutexLocker guard(&mutex);cache.insert(key,new QImage(result),int(qMax<qint64>(1,result.sizeInBytes()/1024)));}return result;
}
}
