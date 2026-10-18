#include "magic_bytes.h"
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonObject>
#include <algorithm>

namespace {
struct Compiled {
    QByteArray values,masks,anchor;
    qsizetype anchorOffset=0;
    qint64 relativeOffset=0;
    int id=0;
};
bool compile(const MagicSignature &s,Compiled &c,QString &error){
    QString text=s.pattern;text.remove(QRegularExpression("\\s"));
    if(s.name.trimmed().isEmpty()||s.name.size()>200||s.notes.size()>2000||s.relativeOffset<0||text.isEmpty()||text.size()%2||text.size()>8192||text.contains(QRegularExpression("[^0-9a-fA-F?]"))){error="A name, nonnegative offset and hexadecimal pairs (up to 4096 bytes) are required. ??, A? and ?F are supported.";return false;}
    bool fixed=false;QByteArray run;qsizetype runStart=0;
    for(qsizetype i=0;i<text.size();i+=2){unsigned value=0,mask=0;
        for(int j=0;j<2;++j){value<<=4;mask<<=4;if(text[i+j]!='?'){value|=QString(text[i+j]).toUInt(nullptr,16);mask|=15;}}
        c.values.append(char(value));c.masks.append(char(mask));fixed|=mask!=0;
        if(mask==255){if(run.isEmpty())runStart=i/2;run.append(char(value));if(run.size()>c.anchor.size()){c.anchor=run;c.anchorOffset=runStart;}}
        else run.clear();
    }
    if(!fixed){error="A signature needs at least one fixed digit; it cannot contain only wildcards.";return false;}
    c.relativeOffset=s.relativeOffset;return true;
}
bool matches(const QByteArray &bytes,qsizetype at,const Compiled &c){
    if(at<0||at>bytes.size()||c.values.size()>bytes.size()-at)return false;
    for(qsizetype i=0;i<c.values.size();++i)if((quint8(bytes[at+i])&quint8(c.masks[i]))!=quint8(c.values[i]))return false;
    return true;
}
}
bool validateMagic(const MagicSignature &s,QString &error){Compiled c;return compile(s,c,error);}
QList<MagicSignature> defaultMagicLibrary(){
    return {{"PNG","89 50 4E 47 0D 0A 1A 0A","PNG signature; check IHDR, lengths and CRCs to validate the content.",0,true},
        {"DDS","44 44 53 20","DDS header; the signature does not determine the texture size.",0,true},
        {"gzip / DEFLATE","1F 8B 08","gzip header using DEFLATE. Decompression is not yet implemented.",0,true}};
}
QJsonDocument encodeMagicLibrary(const QList<MagicSignature> &signatures){
    QJsonArray entries;for(const auto &s:signatures)entries.append(QJsonObject{{"name",s.name},{"pattern",s.pattern},{"offset",QString::number(s.relativeOffset)},{"notes",s.notes},{"enabled",s.enabled}});
    return QJsonDocument(QJsonObject{{"kind","binary-workbench-magic"},{"version",1},{"signatures",entries}});
}
bool decodeMagicLibrary(const QByteArray &json,QList<MagicSignature> &signatures,QString &error){
    auto fail=[&]{error="Invalid library: maximum 64 signatures and 1 MiB. The current library was not changed.";return false;};
    if(json.size()>1024*1024)return fail();QJsonParseError parse;auto doc=QJsonDocument::fromJson(json,&parse);
    if(parse.error!=QJsonParseError::NoError||!doc.isObject()||doc["kind"].toString()!="binary-workbench-magic"||doc["version"].toInt()!=1||!doc["signatures"].isArray()||doc["signatures"].toArray().size()>64)return fail();
    QList<MagicSignature> loaded;
    for(const auto &entry:doc["signatures"].toArray()){
        if(!entry.isObject())return fail();auto o=entry.toObject();bool ok=false;
        MagicSignature s{o["name"].toString(),o["pattern"].toString(),o["notes"].toString(),o["offset"].toString().toLongLong(&ok),o["enabled"].toBool(true)};
        if(!ok||!validateMagic(s,error))return fail();loaded.append(s);
    }
    signatures=std::move(loaded);return true;
}
ReadResult scanMagic(const ReadRequest &source,const MagicSpec &spec,std::atomic<bool> &cancel,std::atomic<qint64> &progress){
    ReadResult out;out.request=source;
    if(spec.signatures.size()>64||source.expectedSize<0||spec.regions.size()>10000){out.error="The analysis exceeds the signature or region limits.";return out;}
    QList<Compiled> patterns;qsizetype maxLength=1;
    for(int i=0;i<spec.signatures.size();++i)if(spec.signatures[i].enabled){Compiled c;c.id=i;if(!compile(spec.signatures[i],c,out.error))return out;maxLength=qMax(maxLength,c.values.size());patterns.append(c);}
    if(patterns.isEmpty()){out.error="Enable at least one signature.";return out;}
    auto read=[&](qint64 offset,qint64 length){auto request=source;request.offset=offset;request.length=length;auto r=readPage(request);applyPatches(r.bytes,offset,spec.patches);return r;};
    auto add=[&](qint64 offset,const Compiled &c){if(out.matches.size()>=10000){out.truncated=true;return false;}out.matches.append({offset,c.values.size()});out.matchTypes.append(c.id);return true;};
    if(spec.anchored){
        for(const auto &region:spec.regions){
            if(region.offset<0||region.length<0||region.offset>source.expectedSize||region.length>source.expectedSize-region.offset){out.error="A region is outside the file.";out.matches.clear();out.matchTypes.clear();return out;}
            for(const auto &c:patterns){
                if(cancel.load()){out.cancelled=true;out.matches.clear();out.matchTypes.clear();return out;}
                if(c.relativeOffset>region.length||c.values.size()>region.length-c.relativeOffset)continue;
                const auto offset=region.offset+c.relativeOffset;auto data=read(offset,c.values.size());if(!data.error.isEmpty())return data;
                if(matches(data.bytes,0,c)&&!add(offset,c))break;
            }
            progress.fetch_add(1);if(out.truncated)break;
        }
    }else{
        constexpr qint64 block=1024*1024;QByteArray tail;
        for(qint64 position=0;position<source.expectedSize;){
            if(cancel.load()){out.cancelled=true;out.matches.clear();out.matchTypes.clear();return out;}
            auto data=read(position,qMin(block,source.expectedSize-position));if(!data.error.isEmpty())return data;
            const QByteArray bytes=tail+data.bytes;const auto base=position-tail.size();
            for(const auto &c:patterns){
                qsizetype from=0;
                while(from+c.values.size()<=bytes.size()){
                    if(cancel.load()){out.cancelled=true;out.matches.clear();out.matchTypes.clear();return out;}
                    qsizetype at=from;
                    if(!c.anchor.isEmpty()){auto found=bytes.indexOf(c.anchor,from+c.anchorOffset);if(found<0)break;at=found-c.anchorOffset;}
                    if(base+at>=c.relativeOffset&&base+at+c.values.size()>position&&matches(bytes,at,c))if(!add(base+at,c))break;
                    from=at+1;
                }
                if(out.truncated)break;
            }
            position+=data.bytes.size();progress.store(position);if(out.truncated)break;tail=bytes.right(maxLength-1);
        }
    }
    auto metadata=read(0,0);if(!metadata.error.isEmpty())return metadata;
    out.size=metadata.size;out.modified=metadata.modified;return out;
}
