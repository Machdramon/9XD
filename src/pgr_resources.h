#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QtEndian>
#include <atomic>
#include <functional>

namespace PgrResources {
inline QJsonObject inspect(const QByteArray &b,const std::atomic_bool &cancel){
    if(b.size()<24)return {};
    auto le=[&](qint64 p){return qFromLittleEndian<quint32>(b.constData()+p);};
    auto be=[&](qint64 p){return qFromBigEndian<quint32>(b.constData()+p);};
    auto fits=[&](qint64 p,qint64 n,qint64 end){return p>=0&&n>=0&&p<=end&&n<=end-p;};
    // PGR2 retail indexed tables use 16-byte entries; offsets are relative to +12.
    if(b.startsWith("INDX")&&le(4)==0&&le(8)==quint32(b.size()-24)){
        qint64 count=le(12),tableEnd=16+count*16,previous=tableEnd;QJsonArray rows;auto fail=[&](QString e){return QJsonObject{{"format","PGR2 indexed resources"},{"error",e},{"entries",QJsonArray()}};};if(count<1||count>10000||tableEnd>b.size())return fail("Invalid PGR2 index table.");
        for(int i=0;i<count;++i){if(cancel.load())return fail("PGR2 inspection canceled.");qint64 p=16+i*16,at=qint64(le(p+12))+12;auto tag=b.mid(p,4);auto raw=le(p+8);qint64 size=raw&0x3ffffff;if(at<previous||!fits(at,size,b.size()))return fail("PGR2 section overlaps or exceeds its container.");previous=at+size;if(tag==QByteArray("END\0",4)){if(size||i+1!=count||!fits(at,12,b.size())||b.mid(at,4)!=tag)return fail("Invalid PGR2 terminator.");continue;}for(auto ch:tag)if(ch&&(quint8(ch)<32||quint8(ch)>126))return fail("Invalid PGR2 section tag.");bool compressed=raw&0x80000000;qint64 capacity=-1;if(compressed){if(size<10)return fail("Incomplete PGR2 compressed frame.");capacity=le(at);if(quint8(b[at+4])!=0x78||((quint8(b[at+4])*256+quint8(b[at+5]))%31)||b.mid(at+size-4,4)!=QByteArray::fromHex("0000ffff"))return fail("Unsupported PGR2 compression frame.");at+=4;size-=4;}
            rows.append(QJsonObject{{"name",QString::fromLatin1(tag).remove(QChar('\0'))},{"offset",QString::number(at)},{"packedBytes",QString::number(size)},{"decodedBytes",QString::number(compressed?-1:size)},{"codec",compressed?"pgr-zlib-sync":"stored"},{"pgrDeclaredCapacity",QString::number(capacity)},{"container","PGR2 indexed resources"},{"supported",true}});
        }return {{"format","PGR2 indexed resources"},{"entries",rows},{"stage","entries listed"}};
    }
    bool wrapped=b.startsWith("ENDI")||b.startsWith("ENSW");
    if(!wrapped&&!b.startsWith("INDX"))return {};
    if(wrapped&&(le(8)!=0||(b.startsWith("ENDI")?le(4)!=1:le(4)!=0)))return {};
    QString format="PGR chunk resources";QJsonArray entries;
    auto failure=[&](QString error){return QJsonObject{{"format",format},{"error",error},{"entries",QJsonArray()}};};
    auto add=[&](QString name,qint64 at,qint64 packed,qint64 decoded,bool compressed){entries.append(QJsonObject{{"name",name},{"offset",QString::number(at)},{"packedBytes",QString::number(packed)},{"decodedBytes",QString::number(decoded)},{"codec",compressed?"zlib":"stored"},{"container",format},{"supported",true}});};
    std::function<bool(qint64,qint64,int)> walk=[&](qint64 p,qint64 end,int depth){
        if(depth>4)return false;
        while(p<end){
            if(cancel.load()||entries.size()>=10000||!fits(p,12,end))return false;
            auto tag=b.mid(p,4);for(auto ch:tag)if(ch&& (quint8(ch)<32||quint8(ch)>126))return false;
            quint32 raw=le(p+8);bool compressed=raw&0x80000000;qint64 size=raw&0x7fffffff,at=p+12;
            if(!fits(at,size,end)||compressed&&size<6)return false;
            auto name=QString::fromLatin1(tag).remove(QChar('\0'))+QString(" at 0x%1").arg(p,0,16);
            if(tag=="INDX"&&!compressed){if(size<4)return false;add(name+" (inner layout unresolved)",at,size,size,false);}
            else {
                if(compressed&&(quint8(b[at])!=0x78||((quint8(b[at])*256+quint8(b[at+1]))%31)))return false;
                add(name,at,size,compressed?-1:size,compressed);
                // Observed SCNE embedded GPU blocks carry a strict six-word BE descriptor.
                // Expose individual streams as well as the containing section; no geometry guessed.
                if(tag=="SCNE"&&!compressed){
                    for(qint64 q=at;q+30<=at+size;++q){
                        if(cancel.load())return false;
                        if(quint8(b[q])!=0x80||be(q)!=0x80000000||be(q+4)!=1||be(q+12)!=0)continue;
                        qint64 decoded=be(q+8),packed=be(q+20);auto data=q+24;
                        if(decoded<1||decoded!=be(q+16)||decoded>128*1024*1024||packed<6||packed>128*1024*1024||!fits(data,packed,at+size)||quint8(b[data])!=0x78||((quint8(b[data])*256+quint8(b[data+1]))%31))continue;
                        if(entries.size()>=10000)return false;
                        add(QString("SCNE GPU block at 0x%1").arg(q,0,16),data,packed,decoded,true);q=data+packed-1;
                    }
                }
            }
            p=at+size;
        }
        return p==end;
    };
    if(!walk(wrapped?12:0,b.size(),0))return failure(cancel.load()?"PGR inspection canceled.":"Unsupported or incomplete PGR chunk tree; no partial extraction offered.");
    return {{"format",format},{"entries",entries},{"stage","entries listed"}};
}
}
