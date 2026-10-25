#pragma once
#include <QByteArray>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtEndian>
namespace Pgr34Names {
struct Hint {QString path,label;QByteArray signature;qint64 offset=0;};
struct Table {QMap<int,Hint> values;QSet<int> ambiguous;};
// Inactive v2 records retain partial source-path bytes in endian-converted fields.
// Flags overwrite parts of the text. These are resource hints, never complete mesh names.
inline bool addRecord(const QByteArray &data,qint64 at,int resourceCount,Table &table){
    if(at<0||at>data.size()-62)return false;
    auto raw=data.mid(at,56);if(raw.left(4)!="c/tr"&&raw.left(4)!="c\\tr")return false;
    if(raw.mid(28,4)!=QByteArray(4,char(255))||raw.mid(48,4)!=QByteArray(4,char(255)))return false;
    int count=qFromBigEndian<quint32>(data.constData()+at+56);
    if(count<1||count>64||at+60+qint64(count)*2>data.size())return false;
    QVector<int> ids;for(int i=0;i<count;++i){int id=qFromBigEndian<quint16>(data.constData()+at+60+i*2);if(id>=resourceCount)return false;ids.append(id);}
    QByteArray text;int pos=0;for(int width:{4,2,2,2,2,4,4,4,4,4,4,4,4,4,4,4}){for(int j=width-1;j>=0;--j)text.append(raw[pos+j]);pos+=width;}
    text.replace(28,4,"????");text.replace(48,8,"????????");
    for(char c:text)if(quint8(c)<32||quint8(c)>126)return false;
    QString path=QString::fromLatin1(text).replace('\\','/');if(!path.startsWith("rt/circuits/"))return false;
    path=".../"+path.mid(3);QString label;auto parts=path.split('/');
    for(int i=parts.size()-1;i>=2;--i){QString fragment=parts[i].section('?',0,0);while(!fragment.isEmpty()&&(fragment.endsWith('_')||fragment.endsWith('.')||fragment.endsWith(' ')))fragment.chop(1);if(fragment.size()>=4&&fragment!="textures"&&fragment!="circuits"&&fragment!="tokyo"){label=fragment+(parts[i].contains('?')?"...":"");break;}}
    Hint hint{path,label,raw.left(28)+raw.mid(32,16)+raw.mid(52,4),at};
    for(int id:ids){if(table.ambiguous.contains(id))continue;auto existing=table.values.constFind(id);if(existing!=table.values.cend()&&existing->signature!=hint.signature){table.values.remove(id);table.ambiguous.insert(id);}else if(existing==table.values.cend())table.values.insert(id,hint);}
    return true;
}
}
