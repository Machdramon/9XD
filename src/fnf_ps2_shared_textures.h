#pragma once
#include "fnf_ps2_surfaces.h"
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QMutex>
namespace FnfPs2Shared {
struct Reference {QString file;qint64 at=0,size=0,fileSize=0;QDateTime modified;int width=0,height=0,psm=0;};
struct Resolution {QImage image;QString error,file;qint64 offset=-1;QDateTime modified;qint64 fileSize=0;};
struct State {QMutex mutex;QString root;QDateTime directoryModified;QMap<quint32,QVector<Reference>> index;QMap<QString,Resolution> images;qint64 bytes=0;};
inline State &state(){static State s;return s;}
inline bool unchanged(const Reference &r){QFileInfo info(r.file);return info.exists()&&info.size()==r.fileSize&&info.lastModified()==r.modified;}
inline Resolution resolve(const QString &source,const FnfPs2Surfaces::Texture &expected,const std::atomic_bool &cancel){
    Resolution out;auto fail=[&](QString e){out.image={};out.error=e;return out;};QFileInfo owner(source);if(source.isEmpty()||!owner.isFile()||owner.suffix().compare("arc",Qt::CaseInsensitive))return fail("Shared ARCP image needs an extracted .arc source path.");QString root=owner.absolutePath(),key=QString::number(expected.id)+":"+QString::number(expected.width)+":"+QString::number(expected.height)+":"+QString::number(expected.psm);QVector<Reference> choices;
    {
        auto &s=state();QMutexLocker lock(&s.mutex);auto stamp=QFileInfo(root).lastModified();if(s.root!=root||s.directoryModified!=stamp){s.root=root;s.directoryModified=stamp;s.index.clear();s.images.clear();s.bytes=0;
            const auto files=QDir(root).entryInfoList({"*.arc"},QDir::Files,QDir::Name);if(files.size()>30000)return fail("Shared ARCP directory exceeds the bounded index limit.");
            qint64 indexed=0;for(const auto &info:files){if(cancel.load()){s.root.clear();return fail("Shared ARCP indexing canceled.");}QFile file(info.absoluteFilePath());if(!file.open(QIODevice::ReadOnly))continue;auto head=file.read(128);if(head.size()!=128||!head.startsWith("ARCP"))continue;qint64 count=FnfPs2Surfaces::u32(head,4),base=128+count*16;if(count<1||count>100000||base>info.size())continue;auto table=file.read(count*16);if(table.size()!=count*16)continue;
                for(int i=0;i<count;++i){auto p=i*16;if(quint8(table[p+12])!=1)continue;int size=(quint32(quint8(table[p+13]))<<16)|(quint32(quint8(table[p+14]))<<8)|quint8(table[p+15]);auto at=base+FnfPs2Surfaces::u32(table,p+4);if(size<=14||!FnfPs2Surfaces::range(at,size,info.size())||!file.seek(at))continue;auto h=file.read(14);if(h.size()!=14)continue;int psm=quint8(h[0]),tw=quint8(h[1])&15,th=quint8(h[2])&15;if((psm!=0&&psm!=19&&psm!=20)||tw>12||th>12)continue;if(++indexed*256>qMin<qint64>(64*1024*1024,AssetMemory::bytes()/64)){s.root.clear();s.index.clear();return fail("Shared ARCP index exceeds the memory budget.");}s.index[FnfPs2Surfaces::u32(table,p)].append({info.absoluteFilePath(),at,size,info.size(),info.lastModified(),1<<tw,1<<th,psm});}
            }
        }
        if(cancel.load())return fail("Shared ARCP lookup canceled.");auto hit=s.images.constFind(key);if(hit!=s.images.cend()){QFileInfo file(hit->file);if(file.size()==hit->fileSize&&file.lastModified()==hit->modified)return *hit;s.bytes-=hit->image.sizeInBytes();s.images.erase(hit);s.root.clear();}
        choices=s.index.value(expected.id);
    }
    qint64 best=0;for(const auto &r:choices)if((!expected.width||(r.width==expected.width&&r.height==expected.height&&r.psm==expected.psm)))best=qMax(best,qint64(r.width)*r.height);
    if(!best)return fail("No shared ARCP pixels with matching source image ID/dimensions were found.");QSet<QByteArray> payloads;QStringList errors;
    for(const auto &r:choices){if(cancel.load())return fail("Shared ARCP lookup canceled.");if(qint64(r.width)*r.height!=best||(expected.width&&(r.width!=expected.width||r.height!=expected.height||r.psm!=expected.psm)))continue;if(!unchanged(r)){errors.append("Shared image source changed; reopen/rescan the folder.");continue;}QFile file(r.file);if(!file.open(QIODevice::ReadOnly)||!file.seek(r.at)){errors.append("Shared image source could not be read.");continue;}auto b=file.read(r.size);if(b.size()!=r.size||!unchanged(r)){errors.append("Shared image source changed while reading.");continue;}
        auto digest=QCryptographicHash::hash(b.mid(30),QCryptographicHash::Sha256);if(payloads.contains(digest))continue;payloads.insert(digest);auto t=FnfPs2Surfaces::texture(b,0,b.size(),expected.name,expected.id);QString error;auto decoded=FnfPs2Surfaces::decode(b,t,error,cancel);if(decoded.isNull()){errors.append(error);continue;}if(!out.image.isNull()&&out.image!=decoded)return fail("Shared ARCP image ID is ambiguous: matching records contain different pixels.");if(out.image.isNull()){out.image=decoded;out.file=r.file;out.offset=r.at;out.modified=r.modified;out.fileSize=r.fileSize;}
    }
    if(out.image.isNull())return fail(errors.isEmpty()?"Shared ARCP pixels are unavailable.":errors.first());
    auto &s=state();QMutexLocker lock(&s.mutex);if(s.root==root&&s.bytes+out.image.sizeInBytes()<=qMin<qint64>(128*1024*1024,AssetMemory::bytes()/32)){s.bytes+=out.image.sizeInBytes();s.images[key]=out;}return out;
}
}
