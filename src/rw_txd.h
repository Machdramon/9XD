#pragma once
#include "rw_core.h"
#include "texture_decode.h"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <atomic>
// Bounded PS2 native TXD raster records. Never search another dictionary by a
// nearest-image heuristic: names are resolved only inside an explicit scope.
namespace RwTxd {
struct Entry {
    qint64 at=0,end=0,dictionary=0,dictionaryEnd=0,pixels=0,pixelSize=0,paletteSize=0;
    QString name,mask,error;int width=0,height=0,depth=0,version=0,format=0;quint64 tex0=0;quint32 addressing=0;QByteArray identity;
    bool supported()const{return error.isEmpty();}
};
inline QList<Entry> entries(const QByteArray &b,qint64 start,qint64 end,const std::atomic_bool &cancel){
    QList<Entry> out;if(start<0||end>b.size()||start>=end)return out;
    const auto *d=reinterpret_cast<const uint8_t *>(b.constData());RwCore::Reader reader{d,b.size(),{}};
    const QByteArray signature=QByteArray::fromHex("16000000");qint64 pos=start;
    while((pos=b.indexOf(signature,pos))>=0&&pos<end){
        if(cancel.load())return {};RwCore::Chunk dict;auto next=pos+4;if(!reader.chunk(pos,end,dict)||dict.type!=RwCore::TexDictionary){pos=next;continue;}
        auto children=reader.children(dict);const auto *count=reader.find(children,RwCore::Struct);int seen=0;
        if(!count||count->size<4){pos=next;continue;}int expected=RwCore::le16(d,count->data);if(expected>4096){pos=next;continue;}
        QList<Entry> batch;
        for(const auto &native:children){if(native.type!=RwCore::TextureNative)continue;++seen;if(cancel.load())return {};
            auto c=reader.children(native);const auto *header=reader.find(c,RwCore::Struct),*name=reader.find(c,RwCore::String),*mask=reader.find(c,RwCore::String,1),*raster=reader.find(c,RwCore::Struct,1);
            if(!header||header->size!=8||b.mid(header->data,4)!=QByteArray("PS2\0",4)||!name||name->size>256||!raster)continue;
            auto rc=reader.children(*raster);const auto *meta=reader.find(rc,RwCore::Struct),*data=reader.find(rc,RwCore::Struct,1);if(!meta||meta->size!=64||!data)continue;
            Entry e;e.at=native.at;e.end=native.end;e.dictionary=dict.at;e.dictionaryEnd=dict.end;e.name=QString::fromLatin1(reader.string(*name));if(mask)e.mask=QString::fromLatin1(reader.string(*mask));e.addressing=RwCore::le32(d,header->data+4);
            e.width=RwCore::le32(d,meta->data);e.height=RwCore::le32(d,meta->data+4);e.depth=RwCore::le32(d,meta->data+8);e.format=RwCore::le16(d,meta->data+12);e.version=RwCore::le16(d,meta->data+14);e.tex0=qFromLittleEndian<quint64>(b.constData()+meta->data+16);
            e.pixels=data->data;e.pixelSize=RwCore::le32(d,meta->data+48);e.paletteSize=RwCore::le32(d,meta->data+52);
            const int psm=(e.tex0>>20)&63,cpsm=(e.tex0>>51)&15;
            if(e.name.isEmpty()||!imageValidSize(e.width,e.height)||e.pixelSize>data->size||e.paletteSize>data->size-e.pixelSize||e.version>2)e.error="Unsupported or truncated PS2 raster.";
            else if(!((e.depth==8&&psm==19&&(cpsm==0||cpsm==2||cpsm==10))||(e.depth==4&&psm==20&&(cpsm==0||cpsm==2||cpsm==10))||(e.depth==32&&psm==0)||(e.depth==16&&(psm==2||psm==10))))e.error="Unsupported PS2 raster pixel/palette format.";
            if(e.version==1)e.error="Legacy swizzled raster variant is not decoded.";
            QCryptographicHash hash(QCryptographicHash::Sha256);hash.addData(b.mid(meta->data,16));QByteArray storage(12,0);qToLittleEndian<quint64>(e.tex0&~(quint64(0x3fff)|(quint64(0x3fff)<<37)),storage.data());qToLittleEndian(e.addressing,storage.data()+8);hash.addData(storage);hash.addData(b.mid(e.pixels,e.pixelSize+e.paletteSize));e.identity=hash.result();batch.append(e);
        }
        if(seen==expected){out+=batch;pos=dict.end;}else pos=next;
    }
    return out;
}
struct Transfer {qint64 data=0,length=0;int width=0,height=0;};
inline Transfer transfer(const QByteArray &b,qint64 at,qint64 end,QString &error){
    Transfer t;auto fail=[&]{error="Unsupported or truncated TXD GIF transfer.";return Transfer{};};
    if(at<0||end>b.size()||end-at<80)return fail();const auto *d=reinterpret_cast<const uint8_t *>(b.constData());
    if(RwCore::le32(d,at)!=3||RwCore::le32(d,at+4)!=0x10000000||RwCore::le32(d,at+8)!=14||RwCore::le32(d,at+24)!=0x51||RwCore::le32(d,at+40)!=0x52||RwCore::le32(d,at+56)!=0x53||RwCore::le32(d,at+48)!=0||RwCore::le32(d,at+52)!=0)return fail();
    // Base level must start at the local origin. Nonzero mip/palette destinations
    // are allowed only for palette payloads, whose host order is decoded directly.
    auto tag=qFromLittleEndian<quint64>(b.constData()+at+64);if(((tag>>58)&3)!=2)return fail();
    t.width=RwCore::le32(d,at+32);t.height=RwCore::le32(d,at+36);t.data=at+80;t.length=qint64(tag&32767)*16;
    if(t.width<1||t.height<1||t.width>8192||t.height>8192||!t.length||t.length>end-t.data)return fail();return t;
}
inline QImage decode(const QByteArray &b,const Entry &e,QString &error,const std::atomic_bool &cancel){
    error.clear();if(cancel.load())return {};if(!e.supported()){error=e.error;return {};}
    auto fail=[&](QString text){error=text;return QImage();};const int cpsm=(e.tex0>>51)&15;
    ImageRecipe r;r.width=e.width;r.height=e.height;r.format=e.depth==8?"Indexed8":e.depth==4?"Indexed4":e.depth==32?"PS2 RGBA8":"PS2 RGBA16";r.lowNibbleFirst=true;r.ps2Clut=e.depth==8;r.paletteFormat=cpsm==0?"PS2 RGBA8":"PS2 RGBA16";
    QByteArray raw,palette;auto bytes=qint64(e.width)*e.height*e.depth/8;
    if(e.version==2){
        auto tx=transfer(b,e.pixels,e.pixels+e.pixelSize,error);if(!error.isEmpty())return {};
        if(qFromLittleEndian<quint64>(b.constData()+e.pixels+16)!=0)return fail("Non-origin base-mip TXD transfer is not supported.");
        if(e.depth<=8){
            const int transferBits=e.depth==8?32:16,pitch=int((e.tex0>>14)&63)*64;
            if(!pitch||pitch<e.width||tx.width*2<e.width||tx.height*2<e.height||qint64(tx.width)*tx.height*transferBits/8!=tx.length)return fail("TXD transfer dimensions disagree with the raster.");
            QByteArray memory(4*1024*1024,0),written(memory.size(),0);raw=QByteArray(bytes,0);int step=transferBits/8;
            for(int y=0;y<tx.height;++y){if(cancel.load())return {};for(int x=0;x<tx.width;++x){auto dst=imageGsBitAddress(x,y,qMax(64,pitch/2),transferBits)/8,src=tx.data+qint64(y*tx.width+x)*step;if(dst<0||dst>memory.size()-step)return fail("TXD transfer exceeds GS storage.");for(int k=0;k<step;++k){memory[dst+k]=b[src+k];written[dst+k]=1;}}}
            for(int y=0;y<e.height;++y){if(cancel.load())return {};for(int x=0;x<e.width;++x){auto src=imageGsBitAddress(x,y,pitch,e.depth);if(src<0||src/8>=memory.size()||!written[src/8])return fail("TXD raster reads unwritten transfer storage.");auto dst=qint64(y*e.width+x)*e.depth;int value=(quint8(memory[src/8])>>(src%8))&((1<<e.depth)-1);raw[dst/8]=char(quint8(raw[dst/8])|(value<<(dst%8)));}}
        }else{if(tx.width!=e.width||tx.height!=e.height||tx.length<bytes)return fail("TXD direct-color dimensions disagree.");raw=b.mid(tx.data,bytes);}
        if(e.depth<=8){auto pal=transfer(b,e.pixels+e.pixelSize,e.pixels+e.pixelSize+e.paletteSize,error);if(!error.isEmpty())return {};auto size=(e.depth==8?256:16)*(cpsm==0?4:2);if(pal.length<size)return fail("TXD palette is truncated.");palette=b.mid(pal.data,size);}
    }else{
        if(e.pixelSize<bytes)return fail("TXD pixels are truncated.");raw=b.mid(e.pixels,bytes);if(e.depth<=8){auto size=(e.depth==8?256:16)*(cpsm==0?4:2);if(e.paletteSize<size)return fail("TXD palette is truncated.");palette=b.mid(e.pixels+e.pixelSize,size);}
    }
    return decodeTextureData(raw,palette,r,error,cancel);
}
inline QImage decodeNative(const QByteArray &bytes,QString &error,const std::atomic_bool &cancel){
    if(bytes.size()<12||qFromLittleEndian<quint32>(bytes.constData())!=RwCore::TextureNative){error="Not a RenderWare native texture.";return {};}
    qint64 length=12+qint64(qFromLittleEndian<quint32>(bytes.constData()+4));if(length>bytes.size()||length>32*1024*1024){error="Truncated or oversized native texture.";return {};}
    QByteArray wrapper(28,0);auto put=[&](int at,quint32 value){qToLittleEndian(value,wrapper.data()+at);};put(0,RwCore::TexDictionary);put(4,16+length);put(8,0x1c020037);put(12,RwCore::Struct);put(16,4);put(20,0x1c020037);qToLittleEndian<quint16>(1,wrapper.data()+24);wrapper+=bytes.first(length);
    auto list=entries(wrapper,0,wrapper.size(),cancel);if(list.size()!=1){error="Native TXD raster is invalid or unsupported.";return {};}return decode(wrapper,list.first(),error,cancel);
}
inline QJsonArray images(const QList<Entry> &entries){QJsonArray out;for(const auto &e:entries)if(e.supported())out.append(QJsonObject{{"name",e.name},{"basis","Bounded RenderWare PS2 native texture; base mip."},{"width",e.width},{"height",e.height},{"settings",QJsonObject{{"offset",QString::number(e.at)},{"format","Encoded image"},{"container","RenderWare TXD"},{"surface",0},{"width",1},{"height",1}}}});return out;}
}
