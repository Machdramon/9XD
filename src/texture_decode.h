#pragma once
#include "image_recipe.h"
#include <QImage>
#include <QtEndian>
#include <atomic>
#include <cmath>
#include <cstring>
#define BCDEC_STATIC
#define BCDEC_BC4BC5_PRECISE
#define BCDEC_IMPLEMENTATION
#include "bcdec.h"
#undef BCDEC_IMPLEMENTATION

inline QRgb imagePackedColor(const uchar *p,const QString &f,bool bigEndian=false){
    int r=0,g=0,b=0,a=255;auto word=[&]{return bigEndian?qFromBigEndian<quint16>(p):qFromLittleEndian<quint16>(p);};
    if(f=="RGB565"||f=="GC RGB565"){auto v=word();r=((v>>11)&31)*255/31;g=((v>>5)&63)*255/63;b=(v&31)*255/31;}
    else if(f=="GC RGB5A3"){auto v=word();if(v&0x8000){r=((v>>10)&31)*255/31;g=((v>>5)&31)*255/31;b=(v&31)*255/31;}else{a=((v>>12)&7)*255/7;r=((v>>8)&15)*17;g=((v>>4)&15)*17;b=(v&15)*17;}}
    else if(f=="PS1 BGR555"||f=="RGB555"){auto v=word();r=(v&31)*255/31;g=((v>>5)&31)*255/31;b=((v>>10)&31)*255/31;if(f=="PS1 BGR555")a=v==0?0:v&0x8000?128:255;}
    else if(f=="PS2 RGBA16"){auto v=word();r=(v&31)*255/31;g=((v>>5)&31)*255/31;b=((v>>10)&31)*255/31;a=v&0x8000?255:0;}
    else if(f=="RGBA5551"){auto v=word();r=((v>>11)&31)*255/31;g=((v>>6)&31)*255/31;b=((v>>1)&31)*255/31;a=v&1?255:0;}
    else if(f=="ARGB1555"){auto v=word();r=((v>>10)&31)*255/31;g=((v>>5)&31)*255/31;b=(v&31)*255/31;a=v&0x8000?255:0;}
    else if(f=="RGBA4444"){auto v=word();r=((v>>12)&15)*17;g=((v>>8)&15)*17;b=((v>>4)&15)*17;a=(v&15)*17;}
    else if(f=="Gray8"||f=="GC I8")r=g=b=p[0];
    else if(f=="IA4"){r=g=b=(p[0]>>4)*17;a=(p[0]&15)*17;}
    else if(f=="GC IA4"){r=g=b=(p[0]&15)*17;a=(p[0]>>4)*17;}
    else if(f=="GC IA8"){a=p[0];r=g=b=p[1];}
    else if(f=="IA8"){r=g=b=p[0];a=p[1];}
    else if(f=="ARGB8"){a=p[0];r=p[1];g=p[2];b=p[3];}
    else if(f=="ABGR8"){a=p[0];b=p[1];g=p[2];r=p[3];}
    else{const bool reverse=f.startsWith("BGR");r=p[reverse?2:0];g=p[1];b=p[reverse?0:2];if(f=="RGBA8"||f=="BGRA8"||f=="PS2 RGBA8")a=f=="PS2 RGBA8"?qMin(255,int(p[3])*255/128):p[3];}
    return qRgba(r,g,b,a);
}
inline qint64 imageStorageBit(const ImageRecipe &r,int x,int y,int width,int height,int bits){
    if(r.layout=="PS2 GS pages"||r.layout=="PS2 GS 16S")return imageGsBitAddress(x,y,width,bits,r.layout=="PS2 GS 16S");
    if(r.layout=="Xbox 360 tiled")return imageXenosAddress(x,y,(width+31)&~31,bits/8)*8;
    if(r.layout=="PS4 1D thin")return (qint64(y/8)*((width+7)/8)*64+(x/8)*64+imageMorton(x%8,y%8,8,8))*bits;
    if(r.layout.startsWith("Morton"))return imageMorton(x,y,width,height)*bits;
    if(r.layout=="Tiled 8x8")return (qint64(y/8)*(width/8)*64+(x/8)*64+(y%8)*8+x%8)*bits;
    return qint64(y)*(r.rowStride?r.rowStride:(qint64(width)*bits+7)/8)*8+x*bits;
}
inline void imageDecodeBcBlock(const uchar *source,const QString &format,QRgb *pixels){
    alignas(8) uchar compressed[16]={};std::memcpy(compressed,source,imageBlockBytes(format));uchar rgba[64]={};
    if(format=="BC1")bcdec_bc1(compressed,rgba,16);
    else if(format=="BC2")bcdec_bc2(compressed,rgba,16);
    else if(format=="BC3")bcdec_bc3(compressed,rgba,16);
    else if(format=="BC7")bcdec_bc7(compressed,rgba,16);
    else if(format.startsWith("BC6")){float rgb[48]={};bcdec_bc6h_float(compressed,rgb,12,format.contains("signed"));for(int i=0;i<16;++i){auto map=[](float value){if(!std::isfinite(value)||value<=0)return 0;return qBound(0,int(std::pow(value/(1+value),1/2.2f)*255+.5f),255);};pixels[i]=qRgb(map(rgb[i*3]),map(rgb[i*3+1]),map(rgb[i*3+2]));}return;}
    else{const bool two=format.startsWith("BC5"),sign=format.contains("SNORM");float values[32]={};if(two)bcdec_bc5_float(compressed,values,8,sign);else bcdec_bc4_float(compressed,values,4,sign);for(int i=0;i<16;++i){auto map=[&](float value){return qBound(0,int((sign?value*.5f+.5f:value)*255+.5f),255);};int r=map(values[i*(two?2:1)]),g=two?map(values[i*2+1]):r;pixels[i]=qRgb(r,g,two?0:r);}return;}
    for(int i=0;i<16;++i)pixels[i]=qRgba(rgba[i*4],rgba[i*4+1],rgba[i*4+2],rgba[i*4+3]);
}
inline QImage decodeTextureData(QByteArray bytes,const QByteArray &palette,const ImageRecipe &r,QString &error,const std::atomic_bool &cancel){
    QString validation;const auto span=imageSpan(r,validation);if(!validation.isEmpty()||bytes.size()<span){error=validation.isEmpty()?"Incomplete texture data.":validation;return {};}
    if(r.byteSwap!="None"){int group=r.byteSwap=="Swap 8 in 16"?2:r.byteSwap=="Swap 8 in 32"||r.byteSwap=="Swap 16 in 32"?4:0;if(!group||bytes.size()%group){error="Byte swap requires complete 16/32-bit groups.";return {};}for(qsizetype i=0;i<bytes.size();i+=group){if(r.byteSwap=="Swap 16 in 32"){std::swap(bytes[i],bytes[i+2]);std::swap(bytes[i+1],bytes[i+3]);}else for(int j=0;j<group/2;++j)std::swap(bytes[i+j],bytes[i+group-1-j]);}}
    QImage image(r.width,r.height,QImage::Format_ARGB32);if(image.isNull()){error="Could not allocate the texture.";return {};}
    const auto *data=reinterpret_cast<const uchar *>(bytes.constData());const auto f=r.format;const int bits=imagePixelBits(f);
    QVector<QRgb> colors;
    if(imageIndexed(f)){int count=imagePaletteCount(r),step=imagePixelBits(r.paletteFormat)/8;if(!imagePaletteFormats().contains(r.paletteFormat)||step<1||palette.size()<count*step){error="Incomplete or unsupported palette.";return {};}const auto *p=reinterpret_cast<const uchar *>(palette.constData());colors.resize(count);for(int i=0;i<count;++i){int index=r.ps2Clut&&count==256?((i&~24)|((i&8)<<1)|((i&16)>>1)):i;auto color=imagePackedColor(p+index*step,r.paletteFormat,r.bigEndian||r.paletteFormat.startsWith("GC "));if(r.ps2Alpha&&r.paletteFormat!="PS2 RGBA8")color=qRgba(qRed(color),qGreen(color),qBlue(color),qMin(255,qAlpha(color)*255/128));colors[i]=color;}}
    auto paletteColor=[&](int index){if(index<0||index>=colors.size()){error="Palette index is outside the supplied palette.";return QRgb(0);}return colors[index];};
    if(imageBlockCompressed(f)){
        int width=(r.width+3)/4,height=(r.height+3)/4;for(int y=0;y<height;++y){if(cancel.load())return {};for(int x=0;x<width;++x){QRgb block[16];auto at=imageStorageBit(r,x,y,width,height,imageBlockBytes(f)*8)/8;if(at<0||at>bytes.size()-imageBlockBytes(f)){error="Texture block address is outside the source.";return {};}imageDecodeBcBlock(data+at,f,block);for(int dy=0;dy<4&&y*4+dy<r.height;++dy)for(int dx=0;dx<4&&x*4+dx<r.width;++dx)image.setPixel(x*4+dx,y*4+dy,block[dy*4+dx]);}}
    }else if(f.startsWith("GC ")){
        int bw=bits==4?8:bits==8?8:4,bh=bits==4?8:bits==8?4:4;if(f=="GC CMPR")bw=bh=8;
        for(int y=0;y<r.height;++y){if(cancel.load())return {};for(int x=0;x<r.width;++x){auto tile=(qint64(y/bh)*((r.width+bw-1)/bw)+x/bw)*(f=="GC RGBA8"?64:32);int index=(y%bh)*bw+x%bw;const auto *p=data+tile;QRgb color;
            if(f=="GC CMPR"){int local=(y%8)/4*2+(x%8)/4;auto *b=p+local*8;quint16 c0=qFromBigEndian<quint16>(b),c1=qFromBigEndian<quint16>(b+2);QRgb c[4]={imagePackedColor(b,"RGB565",true),imagePackedColor(b+2,"RGB565",true),0,0};auto mix=[](QRgb a,QRgb b,int n,int d){return qRgb((qRed(a)*n+qRed(b)*(d-n))/d,(qGreen(a)*n+qGreen(b)*(d-n))/d,(qBlue(a)*n+qBlue(b)*(d-n))/d);};if(c0>c1){c[2]=mix(c[0],c[1],2,3);c[3]=mix(c[0],c[1],1,3);}else{c[2]=mix(c[0],c[1],1,2);c[3]=0;}color=c[(b[4+y%4]>>(6-2*(x%4)))&3];}
            else if(f=="GC RGBA8")color=qRgba(p[index*2+1],p[32+index*2],p[33+index*2],p[index*2]);
            else if(f=="GC I4"){int v=((p[index/2]>>(index%2?0:4))&15)*17;color=qRgb(v,v,v);}
            else if(f=="GC CI4")color=paletteColor((p[index/2]>>(index%2?0:4))&15);
            else if(f=="GC CI8")color=paletteColor(p[index]);
            else if(f=="GC CI14X2")color=paletteColor(qFromBigEndian<quint16>(p+index*2)&16383);
            else color=imagePackedColor(p+index*(bits/8),f,true);image.setPixel(x,y,color);
        }}
    }else if(imagePlanar(f)){
        for(int y=0;y<r.height;++y){if(cancel.load())return {};for(int x=0;x<r.width;++x){qint64 tile=(qint64(y/8)*(r.width/8)+x/8)*(bits*8);int index=0;if(f=="SNES Mode7")index=data[tile+(y%8)*8+x%8];else for(int plane=0;plane<bits;++plane){qint64 at=tile+(f=="NES CHR2"?plane*8+y%8:(plane/2)*16+(y%8)*2+plane%2);index|=((data[at]>>(7-x%8))&1)<<plane;}image.setPixel(x,y,paletteColor(index));}}
    }else{
        for(int y=0;y<r.height;++y){if(cancel.load())return {};auto *row=reinterpret_cast<QRgb *>(image.scanLine(y));for(int x=0;x<r.width;++x){auto bit=imageStorageBit(r,x,y,r.width,r.height,bits);if(bit<0||bit/8>bytes.size()-(bits+7)/8){error="Pixel address is outside the texture.";return {};}const auto *p=data+bit/8;if(f=="Indexed4"||f=="Gray4"){int shift=(bit%8==0)!=(r.lowNibbleFirst||r.layout=="PS2 GS pages")?4:0;int index=(*p>>shift)&15;row[x]=f=="Gray4"?qRgb(index*17,index*17,index*17):paletteColor(index);}else if(f=="Indexed8")row[x]=paletteColor(*p);else if(f.startsWith("PS2 Indexed"))row[x]=paletteColor(f=="PS2 Indexed8H"?p[3]:f=="PS2 Indexed4HL"?p[3]&15:p[3]>>4);else row[x]=imagePackedColor(p,f,r.bigEndian);}}
    }
    if(!error.isEmpty())return {};return image;
}
