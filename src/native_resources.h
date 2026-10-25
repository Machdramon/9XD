#pragma once
#include "binary_document.h"
#include "memory_budget.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include <QtEndian>
#include <QScopeGuard>
#include "pgr_resources.h"
#include "pgr1_compression.h"
#ifdef WORKBENCH_QT_ZLIB
#include <QtZlib/zlib.h>
#else
#include <zlib.h>
#endif

// Container adapters use the document reader, including its snapshot checks and patches.
// Entries remain virtual until the user opens one. No paths from an archive are written.
namespace NativeResources {
inline constexpr qint64 MaxOutput=128*1024*1024, MaxPacked=128*1024*1024;
inline constexpr int MaxEntries=10000, MaxDepth=4;
struct Output {QByteArray bytes;QString error;};
inline bool range(qint64 at,qint64 length,qint64 size){return at>=0&&length>=0&&at<=size&&length<=size-at;}
inline QByteArray read(ReadRequest source,const PatchList &patches,qint64 at,qint64 size,const std::atomic_bool &cancel,QString &error,qint64 readLimit=MaxPacked){
    if(!range(at,size,source.expectedSize)||size>readLimit){error="Resource range exceeds the source or read budget.";return {};}
    QByteArray bytes;bytes.reserve(size);for(qint64 n=0;n<size;){if(cancel.load()){error="Resource operation canceled.";return {};}auto r=source;r.offset=at+n;r.length=qMin(qint64(1024*1024),size-n);auto result=readPage(r);if(!result.error.isEmpty()||result.bytes.size()!=r.length){error=result.error.isEmpty()?"Incomplete resource read.":result.error;return {};}applyPatches(result.bytes,at+n,patches);bytes+=result.bytes;n+=result.bytes.size();}
    auto check=source;check.offset=0;check.length=0;auto result=readPage(check);if(!result.error.isEmpty()){error=result.error;return {};}return bytes;
}
inline QJsonObject entry(QString name,qint64 offset,qint64 packed,qint64 decoded,QString codec,QString format){return {{"name",name},{"offset",QString::number(offset)},{"packedBytes",QString::number(packed)},{"decodedBytes",QString::number(decoded)},{"codec",codec},{"container",format},{"supported",codec=="stored"||codec=="gzip"||codec=="deflate"||codec=="zlib"}};}
inline QJsonObject inspect(ReadRequest source,const PatchList &patches,const std::atomic_bool &cancel){
    QString error;auto head=read(source,patches,0,qMin(qint64(32),source.expectedSize),cancel,error);if(!error.isEmpty())return {{"error",error}};
    QJsonArray entries;QString format;auto fail=[&](QString text){return QJsonObject{{"format",format},{"error",text},{"entries",QJsonArray()}};};
    auto u16=[](const QByteArray &b,qint64 p){return qFromLittleEndian<quint16>(b.constData()+p);};auto u32=[](const QByteArray &b,qint64 p){return qFromLittleEndian<quint32>(b.constData()+p);};auto be=[](const QByteArray &b,qint64 p){return qFromBigEndian<quint32>(b.constData()+p);};
    if(head.startsWith(QByteArray::fromHex("50616b1b"))){
        format="MSR Pak";if(head.size()<8)return fail("Incomplete MSR Pak header.");qint64 decoded=u32(head,4);if(decoded<1||decoded>MaxOutput||source.expectedSize-8>MaxPacked)return fail("MSR Pak size exceeds the resource budget.");auto name=QFileInfo(source.path).fileName();if(name.isEmpty())name="MSR resource";auto row=entry(name+".decoded",8,source.expectedSize-8,decoded,"msr-bizarre",format);row["supported"]=true;entries.append(row);
    }else if(head.startsWith(QByteArray::fromHex("474f5448504f4b1a"))){
        format="PGR1 GOTHPOK";
        if(head.size()<20)return fail("Incomplete GOTHPOK header.");
        qint64 decoded=u32(head,8),packed=source.expectedSize-20;
        const auto flags=u32(head,12);
        if((flags&0xffff)!=0&&(flags&0xffff)!=1)return fail("Unsupported GOTHPOK flags.");
        bool compressed=flags&1;
        if(decoded>MaxOutput||packed>MaxPacked||(!compressed&&packed!=decoded))return fail("Invalid GOTHPOK size or resource budget exceeded.");
        auto name=QFileInfo(source.path).fileName();if(name.isEmpty())name="Resource";
        auto row=entry(name+".decoded",20,packed,decoded,compressed?"pgr1-lz":"stored",format);row["supported"]=true;entries.append(row);
    }else if(head.startsWith("ENDI")||head.startsWith("ENSW")||head.startsWith("INDX")){
        auto bytes=read(source,patches,0,source.expectedSize,cancel,error);if(!error.isEmpty())return {{"format","PGR chunk resources"},{"error",error},{"entries",QJsonArray()}};return PgrResources::inspect(bytes,cancel);
    }else if(head.size()>=2&&quint8(head[0])==0x78&&((quint8(head[0])*256+quint8(head[1]))%31)==0&&!(quint8(head[1])&0x20)){
        format="Zlib";auto bytes=read(source,patches,0,source.expectedSize,cancel,error,AssetMemory::bytes()/4);if(!error.isEmpty())return fail(error);qint64 p=0,total=0;auto name=QFileInfo(source.path).fileName();if(name.isEmpty())name="Resource";
        while(p<bytes.size()){
            if(entries.size()>=MaxEntries)return fail("Zlib member limit exceeded.");
            z_stream stream{};if(inflateInit2(&stream,15)!=Z_OK)return fail("Could not initialize zlib inspection.");auto cleanup=qScopeGuard([&]{inflateEnd(&stream);});stream.next_in=reinterpret_cast<Bytef *>(bytes.data()+p);stream.avail_in=uInt(bytes.size()-p);int code=Z_OK;
            while(code==Z_OK){if(cancel.load())return fail("Resource operation canceled.");char buffer[65536];stream.next_out=reinterpret_cast<Bytef *>(buffer);stream.avail_out=sizeof(buffer);auto before=stream.total_in;code=inflate(&stream,Z_NO_FLUSH);if(stream.total_out>MaxOutput||total+stream.total_out>quint64(AssetMemory::bytes()/4))return fail("Zlib inspection output budget exceeded.");if(code==Z_OK&&before==stream.total_in&&stream.avail_out==sizeof(buffer))return fail("Incomplete zlib stream.");}
            if(code!=Z_STREAM_END)return fail("Corrupt or incomplete zlib member.");auto consumed=qint64(stream.total_in),decoded=qint64(stream.total_out);entries.append(entry(QString("%1 / block %2").arg(name).arg(entries.size()),p,consumed,decoded,"zlib",format));total+=decoded;p+=consumed;
            if(p<bytes.size()){
                // FM1 XB2 chunks are independently compressed and padded to 2048 bytes.
                auto aligned=(p+2047)&~qint64(2047);if(aligned>bytes.size())aligned=bytes.size();for(;p<aligned;++p)if(bytes[p]!=0)return fail("Unsupported bytes after zlib stream.");
            }
        }
        if(entries.size()>1){auto members=entries;qint64 decoded=0;for(auto v:members)decoded+=v.toObject()["decodedBytes"].toString().toLongLong();auto combined=entry(name+" / combined decoded blocks",0,source.expectedSize,decoded,"zlib-blocks",format);combined["supported"]=decoded<=AssetMemory::bytes()/4;combined["members"]=members;entries.prepend(combined);}
    }else if(head.startsWith(QByteArray::fromHex("aaaac0c0"))){
        format="Forza Motorsport CAB";if(head.size()<32||u32(head,4)!=15)return fail("Unsupported Forza CAB version.");qint64 count=u32(head,8),names=0x132+count*24;if(count<1||count>1000000||names>source.expectedSize)return fail("Invalid Forza CAB table.");
        auto table=read(source,patches,0x132,count*24,cancel,error);if(!error.isEmpty())return fail(error);qint64 dataStart=source.expectedSize;
        for(int i=0;i<count;++i){qint64 at=u32(table,i*24),packed=u32(table,i*24+4);if(at<names||!range(at,packed,source.expectedSize))return fail("Forza CAB payload range exceeds the source.");dataStart=qMin(dataStart,at);}
        auto strings=read(source,patches,names,dataStart-names,cancel,error,8*1024*1024);if(!error.isEmpty())return fail(error);
        for(int i=0;i<count;++i){auto p=i*24;qint64 at=u32(table,p),packed=u32(table,p+4),decoded=u32(table,p+8),nameAt=u32(table,p+12);if(nameAt>=strings.size())return fail("Invalid Forza CAB name offset.");auto end=strings.indexOf('\0',nameAt);if(end<0||end-nameAt>4096)return fail("Unterminated Forza CAB name.");auto prefix=read(source,patches,at,qMin(qint64(2),packed),cancel,error);if(!error.isEmpty())return fail(error);bool zipped=prefix.size()==2&&quint8(prefix[0])==0x78&&((quint8(prefix[0])*256+quint8(prefix[1]))%31)==0;auto item=entry(QString::fromUtf8(strings.mid(nameAt,end-nameAt)),at,packed,decoded,zipped?"zlib":packed==decoded?"stored":"unsupported",format);if(!item["supported"].toBool())item["reason"]="Unrecognized Forza CAB member compression.";entries.append(item);}
    }else if(head.startsWith(QByteArray("AFS\0",4))){
        format="AFS";if(head.size()<8)return fail("Incomplete AFS header.");qint64 count=u32(head,4);if(count<1||count>MaxEntries||!range(8,count*8,source.expectedSize))return fail("Invalid AFS entry table or 10,000-entry limit exceeded.");auto table=read(source,patches,8,count*8,cancel,error);if(!error.isEmpty())return fail(error);
        for(int i=0;i<count;++i){qint64 offset=u32(table,i*8),length=u32(table,i*8+4);if(!range(offset,length,source.expectedSize)||(length&&offset<8+count*8))return fail("AFS entry points outside its data area.");entries.append(entry(QString("Entry %1").arg(i,5,10,QChar('0')),offset,length,length,"stored",format));}
    }else if(head.startsWith("FArC")||head.startsWith("FArc")){
        bool stored=head.startsWith("FArc");format=stored?"FArc (stored entries)":"FArC (gzip entries)";if(head.size()<12)return fail("Incomplete FArc header.");qint64 end=8+qint64(be(head,4));if(end<12||end>1024*1024||end>source.expectedSize||be(head,8)!=1)return fail("Unsupported FArc header variant.");auto table=read(source,patches,0,end,cancel,error);if(!error.isEmpty())return fail(error);
        qint64 p=12,recordBytes=stored?9:13;while(p<end){if(cancel.load())return fail("Resource operation canceled.");if(table[p]==0){for(auto c:table.mid(p))if(c)return fail("Malformed FArc padding.");break;}auto zero=table.indexOf('\0',p);if(zero<0||zero-p>1024||zero+recordBytes>end||entries.size()>=MaxEntries)return fail("Invalid FArc name/table.");auto name=QString::fromUtf8(table.mid(p,zero-p));qint64 offset=be(table,zero+1),packed=be(table,zero+5),decoded=stored?packed:be(table,zero+9);if(offset<end||!range(offset,packed,source.expectedSize))return fail("FArc entry range exceeds the source.");entries.append(entry(name,offset,packed,decoded,stored?"stored":"gzip",format));p=zero+recordBytes;}
    }else if(head.startsWith(QByteArray("\x1f\x8b",2))){
        format="Gzip";if(head.size()<10||quint8(head[2])!=8||(quint8(head[3])&0xe0))return fail("Unsupported gzip header.");auto name=QFileInfo(source.path).fileName();if(name.endsWith(".gz",Qt::CaseInsensitive))name.chop(3);else name+=".decoded";if(name.isEmpty())name="Decoded resource";entries.append(entry(name,0,source.expectedSize,-1,"gzip",format));
    }else if(head.startsWith(QByteArray("PK\x03\x04",4))||head.startsWith(QByteArray("PK\x05\x06",4))){
        format="ZIP";auto tailStart=qMax(qint64(0),source.expectedSize-65557);auto tail=read(source,patches,tailStart,source.expectedSize-tailStart,cancel,error);if(!error.isEmpty())return fail(error);qint64 eocd=-1;
        for(qint64 p=tail.size()-22;p>=0;--p)if(tail.mid(p,4)==QByteArray("PK\x05\x06",4)&&p+22+u16(tail,p+20)==tail.size()){eocd=p;break;}
        if(eocd<0)return fail("ZIP directory footer not found.");auto count=u16(tail,eocd+10);qint64 tableSize=u32(tail,eocd+12),tableAt=u32(tail,eocd+16);if(u16(tail,eocd+4)||u16(tail,eocd+6)||u16(tail,eocd+8)!=count||count==65535||count>MaxEntries||tableSize>8*1024*1024||!range(tableAt,tableSize,tailStart+eocd))return fail("Unsupported split/ZIP64 archive or invalid ZIP directory.");auto table=read(source,patches,tableAt,tableSize,cancel,error);if(!error.isEmpty())return fail(error);qint64 p=0;
        for(int i=0;i<count;++i){if(cancel.load())return fail("Resource operation canceled.");if(!range(p,46,table.size())||table.mid(p,4)!=QByteArray("PK\x01\x02",4))return fail("Invalid ZIP directory record.");auto flags=u16(table,p+8),method=u16(table,p+10),nameSize=u16(table,p+28),extra=u16(table,p+30),comment=u16(table,p+32);qint64 packed=u32(table,p+20),decoded=u32(table,p+24),offset=u32(table,p+42);if(!range(p+46,qint64(nameSize)+extra+comment,table.size())||u16(table,p+34)||packed==0xffffffff||decoded==0xffffffff||offset==0xffffffff)return fail("Unsupported ZIP64/split record or truncated ZIP name.");auto raw=table.mid(p+46,nameSize);auto name=(flags&0x800)?QString::fromUtf8(raw):QString::fromLatin1(raw);auto row=entry(name,offset,packed,decoded,method==0?"stored":method==8?"deflate":"unsupported",format);row["zipLocal"]=true;row["zipFlags"]=flags;row["zipMethod"]=method;row["zipRawName"]=QString::fromLatin1(raw.toHex());row["crc32"]=QString::number(u32(table,p+16));if(flags&1){row["supported"]=false;row["reason"]="Encrypted ZIP entry.";}if(name.endsWith('/'))row["supported"]=false;if(!range(offset,30,tableAt))return fail("ZIP local record exceeds the data area.");row["zipDirectoryOffset"]=QString::number(tableAt);entries.append(row);p+=46+nameSize+extra+comment;}
        if(p!=table.size())return fail("Unrecognized trailing ZIP directory data.");
    }
    return {{"format",format},{"entries",entries},{"stage",format.isEmpty()?"unrecognized":"entries listed"}};
}
inline Output inflateBytes(const QByteArray &packed,int window,qint64 expected,const std::atomic_bool &cancel,qint64 limit=MaxOutput,bool pgrSync=false){
    Output out;if(expected>limit){out.error="Decoded resource exceeds the 128 MiB limit.";return out;}z_stream stream{};if(inflateInit2(&stream,window)!=Z_OK){out.error="Could not initialize native decompression.";return out;}auto cleanup=qScopeGuard([&]{inflateEnd(&stream);});stream.next_in=reinterpret_cast<Bytef *>(const_cast<char *>(packed.constData()));stream.avail_in=uInt(packed.size());
    while(true){if(cancel.load()){out.error="Resource operation canceled.";break;}char buffer[65536];stream.next_out=reinterpret_cast<Bytef *>(buffer);stream.avail_out=sizeof(buffer);auto before=stream.total_in;int code=inflate(&stream,Z_NO_FLUSH);auto produced=qsizetype(sizeof(buffer)-stream.avail_out);if(out.bytes.size()+produced>limit){out.error="Decoded resource exceeds the output limit.";break;}out.bytes.append(buffer,produced);if(code==Z_STREAM_END){if(stream.avail_in){out.error="Trailing bytes or multiple compressed members are not supported.";break;}if(expected>=0&&out.bytes.size()!=expected)out.error="Decoded size does not match its entry table.";return out;}if(pgrSync&&code==Z_OK&&stream.avail_in==0&&(stream.data_type&128)&&packed.endsWith(QByteArray::fromHex("0000ffff"))){return out;}if(code!=Z_OK||(before==stream.total_in&&produced==0)){out.error="Compressed stream is corrupt or incomplete (checksum validation failed).";break;}}
    out.bytes.clear();return out;
}
inline Output extract(ReadRequest source,const PatchList &patches,QJsonObject row,const std::atomic_bool &cancel){
    if(row["codec"]=="zlib-blocks"){
        Output out;auto members=row["members"].toArray();if(members.isEmpty()||members.size()>MaxEntries){out.error="Invalid zlib block list.";return out;}auto expected=row["decodedBytes"].toString().toLongLong();if(expected<0||expected>AssetMemory::bytes()/4){out.error="Combined zlib resource exceeds the output budget.";return out;}
        for(auto value:members){auto item=value.toObject();if(item["codec"]!="zlib"){out.bytes.clear();out.error="Invalid nested zlib member.";return out;}auto part=extract(source,patches,item,cancel);if(!part.error.isEmpty()||part.bytes.size()>AssetMemory::bytes()/4-out.bytes.size()){out.bytes.clear();out.error=part.error.isEmpty()?"Combined zlib resource exceeds the output budget.":part.error;return out;}out.bytes+=part.bytes;}
        if(out.bytes.size()!=expected){out.bytes.clear();out.error="Combined zlib size mismatch.";}return out;
    }
    Output out;bool ok=false;auto offset=row["offset"].toString().toLongLong(&ok);if(!ok){out.error="Invalid resource offset.";return out;}auto packed=row["packedBytes"].toString().toLongLong(&ok);if(!ok||packed<0||packed>MaxPacked){out.error="Packed resource exceeds the 128 MiB limit.";return out;}auto expected=row["decodedBytes"].toString().toLongLong(&ok);if(!ok||expected< -1||expected>MaxOutput){out.error="Invalid decoded resource size or 128 MiB limit exceeded.";return out;}if(!row["supported"].toBool()){out.error=row["reason"].toString("Unsupported resource codec.");return out;}
    if(row["zipLocal"].toBool()){
        auto header=read(source,patches,offset,30,cancel,out.error);if(!out.error.isEmpty())return out;auto u16=[&](int p){return qFromLittleEndian<quint16>(header.constData()+p);};if(!header.startsWith(QByteArray("PK\x03\x04",4))||u16(6)!=row["zipFlags"].toInt()||u16(8)!=row["zipMethod"].toInt()){out.error="ZIP local header disagrees with the directory.";return out;}auto name=read(source,patches,offset+30,u16(26),cancel,out.error);if(!out.error.isEmpty())return out;if(name.toHex()!=row["zipRawName"].toString().toLatin1()){out.error="ZIP local name disagrees with the directory.";return out;}offset+=30+u16(26)+u16(28);if(!range(offset,packed,row["zipDirectoryOffset"].toString().toLongLong())){out.error="ZIP payload exceeds the data area.";return out;}
    }
    auto bytes=read(source,patches,offset,packed,cancel,out.error);if(!out.error.isEmpty())return out;auto codec=row["codec"].toString();if(codec=="stored"){if(expected!=packed){out.error="Stored entry sizes disagree.";return out;}out.bytes=std::move(bytes);}else if(codec=="msr-bizarre"){auto result=Pgr1Compression::decode(bytes,expected,cancel,MaxOutput,true);out.bytes=std::move(result.bytes);out.error=result.error;out.error.replace("GOTHPOK","MSR Pak");}else if(codec=="pgr1-lz"){auto result=Pgr1Compression::decode(bytes,expected,cancel,MaxOutput);out.bytes=std::move(result.bytes);out.error=result.error;}else if(codec=="pgr-zlib-sync"){out=inflateBytes(bytes,15,-1,cancel,MaxOutput,true);bool valid=false;auto capacity=row["pgrDeclaredCapacity"].toString().toLongLong(&valid);if(out.error.isEmpty()&&(!valid||capacity<out.bytes.size()||capacity>MaxOutput)){out.bytes.clear();out.error="Invalid PGR2 declared output capacity.";}}else if(codec=="gzip"||codec=="deflate"||codec=="zlib")out=inflateBytes(bytes,codec=="gzip"?31:codec=="zlib"?15:-15,expected,cancel);else out.error="Unsupported resource codec.";
    if(out.error.isEmpty()&&row.contains("crc32")){auto actual=crc32(0,reinterpret_cast<const Bytef *>(out.bytes.constData()),uInt(out.bytes.size()));if(quint64(actual)!=row["crc32"].toString().toULongLong()){out.bytes.clear();out.error="ZIP entry CRC32 mismatch.";}}
    return out;
}
}
