#pragma once
#include "rw_txd.h"
#include "memory_budget.h"
#include <QSet>
namespace SeleneMix {
struct Resource {QString name; qint64 at=0,size=0;int type=0; qint64 expanded=0; qint64 end()const{return at+size;}};
inline QString normalize(QString name){return name.replace('\\','/').toUpper();}
inline QString base(QString name){name=name.mid(name.lastIndexOf('/')+1);return name.left(name.lastIndexOf('.'));}
inline QList<Resource> directory(const QByteArray &b){
    QList<Resource> out;if(b.size()<32||qFromLittleEndian<quint32>(b.constData())!=0)return out;
    auto word=[&](qint64 p){return qFromLittleEndian<quint32>(b.constData()+p);};auto count=word(4);qint64 table=word(8),p=table;
    if(!count||count>65535||table<32||table>=b.size())return {};
    for(quint32 i=0;i<count;++i){if(p>b.size()-4)return {};qint64 n=word(p);p+=4;if(n<2||n>512||p>b.size()-n-21)return {};QByteArray raw=b.mid(p,n);for(int j=0;j<raw.size();++j)raw[j]=char(quint8(raw[j])^((j*58)&255));p+=n;
        if(raw.back()!=0||raw.first(raw.size()-1).contains('\0'))return {};raw.chop(1);QString name=normalize(QString::fromLatin1(raw));for(auto ch:name)if(ch.unicode()<32||ch.unicode()>126)return {};
        if(name.startsWith('/')||name.contains(':')||name.split('/').contains("..")||!name.contains('/'))return {};
        Resource r{name,qint64(word(p)),qint64(word(p+4)),int(quint8(b[p+12]))};auto expanded=word(p+8);p+=21;
        if(r.at<32||r.at>table||r.size>table-r.at||r.type>1||(r.type==0&&expanded!=r.size)||!expanded)return {};r.expanded=expanded;out.append(r);
    }
    if(p!=b.size())return {};return out;
}
// LSB-first LZW: clear=256, end=257, 9->12 bits, late width change.
// This is an internal asset wrapper, independent of the MIX compression flag.
inline QByteArray lzw(const QByteArray &packed,const std::atomic_bool &cancel,QString &error,qint64 limit){
    error.clear();if(limit<1||packed.size()<3){error="Incomplete Selene LZW input.";return {};}
    QList<QByteArray> table;auto reset=[&]{table.clear();table.reserve(4096);for(int i=0;i<256;++i)table.append(QByteArray(1,char(i)));table.append(QByteArray());table.append(QByteArray());};reset();
    qint64 bit=0;int width=9;QByteArray previous,out;bool first=true;
    while(bit+width<=qint64(packed.size())*8){if(cancel.load()){error="Selene LZW canceled.";return {};}
        quint32 value=0;for(int k=0;k<width;++k)if(quint8(packed[(bit+k)/8])&(1<<((bit+k)%8)))value|=1u<<k;bit+=width;
        if(first&&value!=256){error="Selene LZW clear code missing.";return {};}first=false;
        if(value==256){reset();width=9;previous.clear();continue;}
        if(value==257){if(out.isEmpty()){error="Empty Selene LZW output.";return {};}for(qint64 p=bit;p<qint64(packed.size())*8;++p)if(quint8(packed[p/8])&(1<<(p%8))){error="Unexpected trailing Selene LZW data.";return {};}return out;}
        QByteArray current;if(value<quint32(table.size()))current=table[int(value)];else if(value==quint32(table.size())&&!previous.isEmpty())current=previous+previous.first(1);else{error="Invalid Selene LZW dictionary reference.";return {};}
        if(current.isEmpty()||out.size()>limit-current.size()){error="Selene LZW output exceeds the configured budget.";return {};}out+=current;
        if(!previous.isEmpty()&&table.size()<4096){table.append(previous+current.first(1));if(width<12&&table.size()==(1<<width))++width;}previous=current;
    }
    error="Truncated Selene LZW stream.";return {};
}
struct Assets {QByteArray bytes;QList<Resource> resources;QList<RwTxd::Entry> textures;QMap<qint64,QString> texturePaths;QMap<QString,QPair<qint64,qint64>> decodedBanks;QStringList warnings;qint64 originalSize=0;};
inline Assets prepare(const QByteArray &original,const std::atomic_bool &cancel){
    Assets a;a.bytes=original;a.originalSize=original.size();a.resources=directory(original);a.textures=RwTxd::entries(original,0,original.size(),cancel);
    if(a.resources.isEmpty())return a;
    for(const auto &r:a.resources){if(cancel.load())return {};if(!r.name.endsWith(".TXD"))continue;
        bool raw=r.size>=12&&qFromLittleEndian<quint32>(original.constData()+r.at)==RwCore::TexDictionary;
        if(raw){for(auto &e:a.textures)if(e.dictionary>=r.at&&e.dictionaryEnd<=r.end())a.texturePaths[e.dictionary]=r.name;a.decodedBanks[r.name]={r.at,r.size};continue;}
        QString error;auto decoded=lzw(original.mid(r.at,r.size),cancel,error,qMin<qint64>(64*1024*1024,AssetMemory::bytes()/16));
        if((r.expanded&&decoded.size()!=r.expanded)||decoded.size()<12||qFromLittleEndian<quint32>(decoded.constData())!=RwCore::TexDictionary||12+qint64(qFromLittleEndian<quint32>(decoded.constData()+4))!=decoded.size()){a.warnings.append(r.name+": "+(error.isEmpty()?"Decoded asset is not a bounded TXD dictionary.":error));continue;}
        if(decoded.size()>AssetMemory::bytes()/4-a.bytes.size()){a.warnings.append(r.name+": decoded bank exceeds the read budget.");continue;}
        while(a.bytes.size()%16)a.bytes.append(char(0));qint64 at=a.bytes.size();a.bytes+=decoded;auto list=RwTxd::entries(a.bytes,at,a.bytes.size(),cancel);if(list.isEmpty()){a.warnings.append(r.name+": no supported PS2 raster metadata.");continue;}for(auto &e:list)a.texturePaths[e.dictionary]=r.name;a.textures+=list;a.decodedBanks[r.name]={at,decoded.size()};
    }
    return a;
}
inline const Resource *resourceAt(const Assets &a,qint64 start,qint64 end){for(const auto &r:a.resources)if(r.at==start&&r.end()==end)return &r;return nullptr;}
inline QSet<QString> banksFor(const Assets &a,const Resource &geometry){
    QSet<QString> out;auto parts=geometry.name.split('/');if(parts.size()<3)return out;auto owner=parts[0]+"/"+parts[1]+"/";auto stem=base(geometry.name);bool car=parts[0]=="CARS"&&stem==parts[1];
    for(const auto &r:a.resources)if(r.name.startsWith(owner)&&r.name.endsWith(".TXD")){
        auto bank=base(r.name);auto suffix=bank.mid(stem.size());if(bank==stem||(car&&bank.startsWith(stem)&&suffix.size()==1&&suffix[0]>='A'&&suffix[0]<='Z'))out.insert(r.name);
    }
    return out;
}
inline QList<RwTxd::Entry> linked(const Assets &a,const QString &name,qint64 start,qint64 end){
    QList<RwTxd::Entry> out;const auto *r=resourceAt(a,start,end);if(!r)return out;auto banks=banksFor(a,*r);QSet<QByteArray> seen;
    for(const auto &e:a.textures)if(e.name.compare(name,Qt::CaseInsensitive)==0&&banks.contains(a.texturePaths.value(e.dictionary))&&!seen.contains(e.identity)){seen.insert(e.identity);out.append(e);}return out;
}
}
