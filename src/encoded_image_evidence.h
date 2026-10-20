#pragma once
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QtEndian>
#include <atomic>
// Signature + bounded header/chunk checks. The image decoder remains the authority on pixels.
inline QJsonArray encodedImageEvidence(const QByteArray &b,qint64 sourceSize,const std::atomic_bool &cancel){
    QJsonArray out;auto le32=[&](qint64 p){return qFromLittleEndian<quint32>(b.constData()+p);};auto be32=[&](qint64 p){return qFromBigEndian<quint32>(b.constData()+p);};auto be16=[&](qint64 p){return qFromBigEndian<quint16>(b.constData()+p);};
    struct Magic{QByteArray bytes;QString name;};
    for(const auto &magic:QVector<Magic>{{QByteArray::fromHex("89504e470d0a1a0a"),"PNG"},{QByteArray::fromHex("ffd8ff"),"JPEG"},{"BM","BMP"},{"DDS ","DDS"},{"GIF8","GIF"}}){
        qsizetype search=0;for(int hit=0;hit<64&&out.size()<32;++hit){if(cancel.load())return out;auto at=b.indexOf(magic.bytes,search);if(at<0)break;search=at+1;qint64 start=at,width=0,height=0;bool valid=false;
            if(magic.name=="PNG"&&start+33<=b.size()&&be32(start+8)==13&&b.mid(start+12,4)=="IHDR"){width=be32(start+16);height=be32(start+20);qint64 p=start+8;for(int chunks=0;chunks<4096&&p+12<=b.size();++chunks){auto length=be32(p);if(length>quint64(b.size()-p-12))break;auto type=b.mid(p+4,4);p+=12+length;if(type=="IEND"){valid=length==0;break;}}}
            else if(magic.name=="BMP"&&start+54<=b.size()&&le32(start+14)>=40){auto total=le32(start+2),pixels=le32(start+10);width=qint32(le32(start+18));height=qAbs(qint64(qint32(le32(start+22))));valid=total>=54&&pixels<total&&start+total<=sourceSize;}
            else if(magic.name=="DDS"&&start+128<=b.size()&&le32(start+4)==124&&le32(start+76)==32){width=le32(start+16);height=le32(start+12);valid=true;}
            else if(magic.name=="GIF"&&start+13<=b.size()&&(b.mid(start,6)=="GIF87a"||b.mid(start,6)=="GIF89a")){width=qFromLittleEndian<quint16>(b.constData()+start+6);height=qFromLittleEndian<quint16>(b.constData()+start+8);valid=true;}
            else if(magic.name=="JPEG"){qint64 p=start+2;for(int markers=0;markers<256&&p+4<=b.size();++markers){if(quint8(b[p++])!=255)break;while(p<b.size()&&quint8(b[p])==255)++p;if(p>=b.size())break;int marker=quint8(b[p++]);if(marker==0xda||marker==0xd9)break;if(marker==0x01||(marker>=0xd0&&marker<=0xd7))continue;if(p+2>b.size())break;auto length=be16(p);if(length<2||p+length>b.size())break;if(((marker>=0xc0&&marker<=0xc3)||(marker>=0xc5&&marker<=0xc7)||(marker>=0xc9&&marker<=0xcb)||(marker>=0xcd&&marker<=0xcf))&&length>=8){height=be16(p+3);width=be16(p+5);valid=true;break;}p+=length;}}
            if(!valid||width<1||height<1||width>8192||height>8192||width*height>4194304)continue;
            out.append(QJsonObject{{"basis",magic.name+" signature and bounded header; decode to verify pixels"},{"width",width},{"height",height},{"settings",QJsonObject{{"offset",QString::number(start)},{"width",1},{"height",1},{"format","Encoded image"},{"surface",0}}}});
        }
    }return out;
}
