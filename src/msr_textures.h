#pragma once
#include "native_resources.h"
#include "mesh_surface.h"
#include <QMutex>
#include <QMutexLocker>
namespace MsrTextures {
struct Texture {QString name;int width=0,height=0;quint32 flags=0;qint64 data=0,span=0;};
inline bool parse(const QByteArray &b,qint64 at,int count,QVector<Texture> &textures,QString &error){
    textures.clear();if(count<0||count>10000){error="Invalid MSR texture count.";return false;}
    for(int i=0;i<count;++i){if(!NativeResources::range(at,44,b.size())){error="Incomplete MSR texture header.";return false;}auto p=b.constData()+at;auto size=qFromLittleEndian<quint32>(p);auto name=QByteArray(p+4,32);int end=name.indexOf('\0');Texture t;t.name=QString::fromLatin1(name.left(end));t.width=qFromLittleEndian<quint16>(p+36);t.height=qFromLittleEndian<quint16>(p+38);t.flags=qFromLittleEndian<quint32>(p+40);t.data=at+44;t.span=size-44;
        if(size<44||!NativeResources::range(at,size,b.size())||end<0||t.width<2||t.height<2||t.width>2048||t.height>2048||(t.width&(t.width-1))||(t.height&(t.height-1))){error="Invalid MSR texture range/dimensions.";return false;}qint64 body=t.flags&4?qint64(t.width)*t.height*2:2048+qint64(t.width)*t.height/4;auto mask=t.flags&0x4000?(t.flags&4?qint64(t.width)*t.height:1024):0;if(body+mask>t.span){error="Incomplete MSR texture pixels/mask.";return false;}textures.append(t);at+=size;
    }return true;
}
inline qint64 twiddle(int x,int y,int w,int h){qint64 n=0;int bit=0;for(int mask=1;mask<w||mask<h;mask<<=1){if(mask<h)n|=qint64(bool(y&mask))<<bit++;if(mask<w)n|=qint64(bool(x&mask))<<bit++;}return n;}
inline QImage decode(const QByteArray &b,const Texture &t,const std::atomic_bool &cancel,QString &error){
    qint64 body=t.flags&4?qint64(t.width)*t.height*2:2048+qint64(t.width)*t.height/4;if(t.width<2||t.height<2||t.width>2048||t.height>2048||(t.width&(t.width-1))||(t.height&(t.height-1))||!NativeResources::range(t.data,body,b.size())){error="Invalid MSR texture payload.";return {};}
    if(t.flags&~quint32(0x5c004)){error="Unrecognized MSR texture flags.";return {};}
    QImage image(t.width,t.height,QImage::Format_RGBA8888);if(image.isNull()){error="Could not allocate MSR texture.";return {};}
    for(int y=0;y<t.height;++y){if(cancel.load()){error="MSR texture decoding canceled.";return {};}auto row=image.scanLine(y);for(int x=0;x<t.width;++x){qint64 offset;if(t.flags&4)offset=t.data+twiddle(x,y,t.width,t.height)*2;else{int code=quint8(b[t.data+2048+twiddle(x/2,y/2,t.width/2,t.height/2)]);offset=t.data+code*8+((x&1)*2+(y&1))*2;}auto v=qFromLittleEndian<quint16>(b.constData()+offset);int r,g,blue,a=255;if(t.flags&0x40000){a=((v>>12)&15)*17;r=((v>>8)&15)*17;g=((v>>4)&15)*17;blue=(v&15)*17;}else if(t.flags&0x10000){a=(v>>15)*255;r=((v>>10)&31)*255/31;g=((v>>5)&31)*255/31;blue=(v&31)*255/31;}else{r=((v>>11)&31)*255/31;g=((v>>5)&63)*255/63;blue=(v&31)*255/31;}row[x*4]=r;row[x*4+1]=g;row[x*4+2]=blue;row[x*4+3]=a;}}
    return image;
}
}
