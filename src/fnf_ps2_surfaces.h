#pragma once
#include "faf_geometry.h"
#include "ps2_gs_address.h"
// Retail ARCP images are GS host-to-local upload packets, not encoded PNGs.
namespace FnfPs2Surfaces {
using FafGeometry::range;using FafGeometry::u32;
inline quint64 u64(const QByteArray &b,qint64 at){return qFromLittleEndian<quint64>(b.constData()+at);}
struct Transfer {qint64 at=0,bytes=0;int dbp=0,dbw=0,psm=0,x=0,y=0,width=0,height=0;};
struct Texture {QString name,error;qint64 at=0,size=0;quint32 id=0;int psm=0,width=0,height=0,bufferWidth=0,paletteBase=0,levels=0;bool external=false;QVector<Transfer> transfers;};
struct Material {QString name,error;int image=-1,wrapU=0,wrapV=0,mode=0;quint32 color=0;QByteArray state;};
inline Texture texture(const QByteArray &b,qint64 at,qint64 size,QString name,quint32 id,bool external=false){
    Texture t;t.at=at;t.size=size;t.name=name;t.id=id;t.external=external;auto fail=[&](QString e){t.error=e;return t;};
    if(external)return fail("External ARCP image reference.");if(!range(at,size,b.size())||size<14)return fail("Incomplete ARCP GS texture header.");t.psm=quint8(b[at]);int tw=quint8(b[at+1])&15,th=quint8(b[at+2])&15;t.bufferWidth=int(quint8(b[at+3]))*64;
    if(tw>12||th>12||t.bufferWidth<64||t.bufferWidth>4096||(t.psm!=0&&t.psm!=19&&t.psm!=20))return fail("Unsupported ARCP GS texture format/dimensions.");t.width=1<<tw;t.height=1<<th;t.paletteBase=qFromLittleEndian<quint16>(b.constData()+at+8)&16383;
    if(size==14){t.external=true;return fail("ARCP image has a reference-only header; no upload pixels.");}if(size<46||(size-30)%16)return fail("Invalid ARCP GS upload size.");auto dma=u32(b,at+14);if((dma>>28)!=7||qint64(dma&65535)*16!=size-30)return fail("Unsupported ARCP image DMA tag.");
    qint64 p=at+30,end=at+size;quint64 bitblt=0,pos=0,dim=0,dir=3;int packets=0;
    while(p<end){if(!range(p,16,end)||++packets>4096)return fail("Invalid ARCP GIF packet boundary.");auto tag=u64(b,p),regs=u64(b,p+8);int loops=tag&0x7fff,flg=(tag>>58)&3,nreg=(tag>>60)&15;if(!nreg)nreg=16;p+=16;
        if(flg==0){if(nreg!=1||(regs&15)!=14||!range(p,qint64(loops)*16,end))return fail("Unsupported ARCP GIF register packet.");for(int i=0;i<loops;++i){auto value=u64(b,p),reg=u64(b,p+8);p+=16;if(reg==0x50)bitblt=value;else if(reg==0x51)pos=value;else if(reg==0x52)dim=value;else if(reg==0x53)dir=value;else if(reg!=0x61)return fail("Unsupported ARCP upload GS register.");}}
        else if(flg==2){Transfer x;x.at=p;x.bytes=qint64(loops)*16;x.dbp=(bitblt>>32)&16383;x.dbw=(bitblt>>48)&63;x.psm=(bitblt>>56)&63;x.x=(pos>>32)&2047;x.y=(pos>>48)&2047;x.width=dim&4095;x.height=(dim>>32)&4095;
            if(dir!=0||x.psm!=0||x.dbw<1||x.width<1||x.height<1||x.width>4096||x.height>4096||qint64(x.width)*x.height*4!=x.bytes||!range(p,x.bytes,end))return fail("Unsupported/incomplete ARCP host-to-local transfer.");t.transfers.append(x);p+=x.bytes;
        }else return fail("Unsupported ARCP GIF image mode.");
    }
    int first=t.psm==0?0:1;if(t.transfers.size()<=first)return fail("Incomplete ARCP texture uploads.");if(first){auto palette=t.transfers[0];int side=t.psm==19?16:8;if(palette.width<side||palette.height<side||palette.width>256||palette.height>256||palette.bytes<side*side*4)return fail("Unsupported ARCP palette upload.");}t.levels=t.transfers.size()-first;auto image=t.transfers[first];if(image.x||image.y)return fail("Unsupported ARCP base-mip origin.");return t;
}
inline QImage decode(const QByteArray &b,const Texture &t,QString &error,const std::atomic_bool &cancel){
    auto fail=[&](QString e){error=e;return QImage();};if(!t.error.isEmpty())return fail(t.error);if(t.transfers.isEmpty()||!imageValidSize(t.width,t.height))return fail("Invalid ARCP texture descriptor.");
    QByteArray memory(4*1024*1024,0),written(memory.size(),0);
    for(const auto &x:t.transfers){if(!range(x.at,x.bytes,b.size()))return fail("ARCP upload changed.");for(int y=0;y<x.height;++y){if(cancel.load())return fail("ARCP texture decoding canceled.");for(int u=0;u<x.width;++u){qint64 target=qint64(x.dbp)*256+Ps2Gs::bitAddress(u+x.x,y+x.y,x.dbw*64,32)/8,source=x.at+(qint64(y)*x.width+u)*4;if(!range(target,4,memory.size()))return fail("ARCP upload exceeds GS local memory.");std::memcpy(memory.data()+target,b.constData()+source,4);std::memset(written.data()+target,1,4);}}}
    auto palette=t.transfers.first();int count=t.psm==0?0:t.psm==19?256:16;QVector<QRgb> colors(count);for(int i=0;i<count;++i){int source=(i&~24)|((i&8)<<1)|((i&16)>>1);auto at=qint64(t.paletteBase)*256+Ps2Gs::bitAddress(source%16,source/16,64,32)/8;if(!range(at,4,memory.size())||!written[at]||!written[at+1]||!written[at+2]||!written[at+3])return fail("ARCP palette reads unwritten GS local memory.");colors[i]=qRgba(quint8(memory[at]),quint8(memory[at+1]),quint8(memory[at+2]),qMin(255,int(quint8(memory[at+3]))*255/128));}
    QImage image(t.width,t.height,QImage::Format_ARGB32);if(image.isNull())return fail("Could not allocate ARCP texture.");int bits=t.psm==0?32:t.psm==19?8:4;auto base=t.transfers[t.psm==0?0:1].dbp;
    for(int y=0;y<t.height;++y){if(cancel.load())return fail("ARCP texture decoding canceled.");auto *row=reinterpret_cast<QRgb *>(image.scanLine(y));for(int x=0;x<t.width;++x){auto bit=qint64(base)*256*8+Ps2Gs::bitAddress(x,y,t.bufferWidth,bits);if(bit<0||bit/8>=memory.size()||!written[bit/8])return fail("ARCP image reads unwritten GS local memory.");if(bits==32){auto p=bit/8;if(!range(p,4,memory.size())||!written[p+1]||!written[p+2]||!written[p+3])return fail("ARCP RGBA image reads unwritten GS storage.");row[x]=qRgba(quint8(memory[p]),quint8(memory[p+1]),quint8(memory[p+2]),qMin(255,int(quint8(memory[p+3]))*255/128));}else{int index=(quint8(memory[bit/8])>>(bit%8))&((1<<bits)-1);row[x]=colors[index];}}}
    return image;
}
inline Material material(const QByteArray &b,qint64 at,qint64 size,QString name,int imageCount){
    Material m;m.name=name;if(size<40||!range(at,size,b.size())){m.error="Unsupported ARCP material state.";return m;}m.image=int(u32(b,at));m.color=u32(b,at+4);auto clamp=u64(b,at+16);m.wrapU=clamp&3;m.wrapV=(clamp>>2)&3;m.mode=quint8(b[at+37]);m.state=b.mid(at,40);if(m.image<0||m.image>=imageCount)m.error="ARCP material image index exceeds the source table.";if(m.wrapU>1||m.wrapV>1)m.error="ARCP region clamp/repeat needs additional shader support.";return m;
}
inline QString wrap(int mode){return mode==1?"Clamp":"Repeat";}
}
