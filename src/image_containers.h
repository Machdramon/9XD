#pragma once
#include "texture_decode.h"
#include <QMap>
inline QImage decodeGnf(const QByteArray &bytes,const ImageRecipe &selection,QString &error,const std::atomic_bool &cancel){
    auto fail=[&](QString text){error=text;return QImage();};if(bytes.size()<48||!bytes.startsWith("GNF "))return fail("Incomplete GNF header.");const auto *p=reinterpret_cast<const uchar *>(bytes.constData());auto u=[&](int n){return qFromLittleEndian<quint32>(p+n);};
    qint64 start=8+qint64(u(4));if(p[9]!=1||selection.surface||start<48||start>bytes.size()||u(12)>quint32(bytes.size()))return fail("Only single-texture GNF files are supported.");
    auto w1=u(20),w2=u(24),w3=u(28),w4=u(32),w5=u(36);int width=(w2&0x3fff)+1,height=((w2>>14)&0x3fff)+1,pitch=((w4>>13)&0x3fff)+1,tile=(w3>>20)&31,format=(w1>>20)&63,type=(w1>>26)&15;
    if((w3>>28)!=9||(w4&0x1fff)||w5&0x3ffffff||selection.mipLevel)return fail("GNF currently supports the base level of a single 2D texture (no arrays/cubemaps).");
    if(tile!=8&&tile!=13)return fail(QString("GNF tile mode %1 is not supported (available: linear-aligned and 1D thin).").arg(tile));
    if(!imageValidSize(width,height)||pitch<width||pitch>8192)return fail("Invalid GNF dimensions or pitch.");
    ImageRecipe r;r.width=pitch;r.height=height;r.layout=tile==13?"PS4 1D thin":"Linear";
    if(format>=35&&format<=41){r.format=QString("BC%1").arg(format-34);if(format==40)r.format="BC6H";if(type==1&&(format==38||format==39))r.format+=" SNORM";if(type==7&&format==40)r.format="BC6H signed";}
    else if(type==0||type==9){r.format=format==10?"RGBA8":format==1?"Gray8":"";}else return fail("Unsupported GNF numeric format.");
    if(r.format.isEmpty())return fail("Unsupported GNF surface format.");QString e;auto span=imageSpan(r,e);if(!e.isEmpty())return fail(e);if(span>bytes.size()-start)return fail("GNF texture data is truncated.");auto image=decodeTextureData(bytes.mid(start,span),{},r,error,cancel);if(image.isNull())return image;image=image.copy(0,0,width,height);
    for(int y=0;y<height;++y){if(cancel.load())return {};auto *row=reinterpret_cast<QRgb *>(image.scanLine(y));for(int x=0;x<width;++x){auto color=row[x];int channels[8]={0,255,0,0,qRed(color),qGreen(color),qBlue(color),qAlpha(color)};row[x]=qRgba(channels[w3&7],channels[(w3>>3)&7],channels[(w3>>6)&7],channels[(w3>>9)&7]);}}return image;
}
inline QImage decodeTim2(const QByteArray &bytes,const ImageRecipe &selection,QString &error,const std::atomic_bool &cancel){
    auto fail=[&](QString text){error=text;return QImage();};if(bytes.size()<16||!bytes.startsWith("TIM2"))return fail("Incomplete TIM2 header.");const auto *p=reinterpret_cast<const uchar *>(bytes.constData());auto u16=[&](int n){return qFromLittleEndian<quint16>(p+n);};auto u32=[&](int n){return qFromLittleEndian<quint32>(p+n);};
    if(p[5]>1||selection.surface>=u16(6))return fail("TIM2 image index or alignment is unsupported.");qint64 at=p[5]?128:16;
    for(int i=0;i<selection.surface;++i){if(at>bytes.size()-48||u32(at)<48||u32(at)>bytes.size()-at)return fail("Invalid TIM2 picture size.");at+=u32(at);}
    if(at>bytes.size()-48)return fail("Incomplete TIM2 picture.");qint64 total=u32(at),paletteSize=u32(at+4),imageSize=u32(at+8);int header=u16(at+12),levels=qMax(1,int(p[at+17])),type=p[at+19],clut=p[at+18]&63;
    if(total<48||total>bytes.size()-at||header<48||header+imageSize+paletteSize>total||levels>8||selection.mipLevel>=levels||p[at+16]!=0)return fail("Unsupported or invalid TIM2 picture layout.");
    ImageRecipe r;r.width=u16(at+20);r.height=u16(at+22);r.format=type==1?"PS2 RGBA16":type==2?"RGBX8":type==3?"PS2 RGBA8":type==4?"Indexed4":type==5?"Indexed8":"";r.lowNibbleFirst=true;r.paletteFormat=clut==1?"PS2 RGBA16":clut==2?"RGBX8":"PS2 RGBA8";if(r.format.isEmpty())return fail("Unsupported TIM2 color type.");
    qint64 imageOffset=at+header,available=imageSize;
    if(selection.mipLevel){if(header<64+levels*4)return fail("TIM2 mip size table is missing.");qint64 skipped=0;for(int i=0;i<selection.mipLevel;++i)skipped+=u32(at+64+i*4);available=u32(at+64+selection.mipLevel*4);if(skipped+available>imageSize)return fail("Invalid TIM2 mip sizes.");imageOffset+=skipped;r.width=qMax(1,r.width>>selection.mipLevel);r.height=qMax(1,r.height>>selection.mipLevel);}
    QByteArray palette;if(imageIndexed(r.format)){
        if(clut<1||clut>3)return fail("Unsupported TIM2 CLUT color type.");int count=imagePaletteCount(r),step=imagePixelBits(r.paletteFormat)/8;auto bank=qint64(selection.paletteBank)*count*step;if(bank+count*step>paletteSize)return fail("TIM2 palette bank is out of range.");palette=bytes.mid(at+header+imageSize+bank,count*step);
        // TIM2 CLUT bit 7 selects linear storage; CSM1 swaps index bits 3 and 4.
        r.ps2Clut=count==256&&!(p[at+18]&128);
    }
    return decodeTextureData(bytes.mid(imageOffset,available),palette,r,error,cancel);
}

inline QImage decodeGtf(const QByteArray &bytes,const ImageRecipe &selection,QString &error,const std::atomic_bool &cancel){
    auto fail=[&](QString text){error=text;return QImage();};if(bytes.size()<48)return fail("Incomplete GTF header.");const auto *p=reinterpret_cast<const uchar *>(bytes.constData());auto u16=[&](int n){return qFromBigEndian<quint16>(p+n);};auto u32=[&](int n){return qFromBigEndian<quint32>(p+n);};
    auto count=u32(8);if(count<1||count>4096||12+qint64(count)*36>bytes.size()||selection.surface>=int(count))return fail("Invalid GTF texture table.");int at=12+selection.surface*36,gcm=at+12;int format=p[gcm],levels=p[gcm+1];
    if(p[gcm+2]!=2||p[gcm+3]||u16(gcm+12)>1||selection.mipLevel>=levels||levels>14)return fail("Only GTF 2D textures are supported; index/mip is out of range.");
    ImageRecipe r;r.width=u16(gcm+8);r.height=u16(gcm+10);int base=format&~0x60;r.format=base==0x81?"Gray8":base==0x82?"ARGB1555":base==0x84?"RGB565":base==0x85?"ARGB8":base==0x86?"BC1":base==0x87?"BC2":base==0x88?"BC3":"";if(r.format.isEmpty())return fail("Unsupported GTF color format.");
    // RSX block-compressed data is block-linear; uncompressed swizzled textures use Morton order.
    r.layout=(format&32)||imageBlockCompressed(r.format)?"Linear":"Morton rectangular";r.bigEndian=true;
    qint64 offset=u32(at+4),available=u32(at+8);if(offset>bytes.size()-available)return fail("GTF texture data is truncated.");
    for(int mip=0;mip<selection.mipLevel;++mip){QString e;r.rowStride=mip==0&&(format&32)?u32(gcm+16):0;auto size=imageSpan(r,e);if(!e.isEmpty())return fail(e);offset+=size;available-=size;r.width=qMax(1,r.width/2);r.height=qMax(1,r.height/2);}
    r.rowStride=selection.mipLevel==0&&(format&32)&&!imageBlockCompressed(r.format)?u32(gcm+16):0;QString e;auto size=imageSpan(r,e);if(!e.isEmpty())return fail(e);if(size>available||offset>bytes.size()-size)return fail("GTF mip is truncated.");auto image=decodeTextureData(bytes.mid(offset,size),{},r,error,cancel);
    // Standard GCM channel remapping, including zero/one constants.
    auto remap=u32(gcm+4);if(!image.isNull())for(int y=0;y<image.height();++y){auto *row=reinterpret_cast<QRgb *>(image.scanLine(y));for(int x=0;x<image.width();++x){auto color=row[x];int inputs[4]={qAlpha(color),qRed(color),qGreen(color),qBlue(color)},out[4];for(int c=0;c<4;++c){int mode=(remap>>(8+c*2))&3;out[c]=mode==0?0:mode==1?255:inputs[(remap>>(c*2))&3];}row[x]=qRgba(out[1],out[2],out[3],out[0]);}}return image;
}
inline QImage decodeDds(const QByteArray &bytes,const ImageRecipe &selection,QString &error,const std::atomic_bool &cancel){
    auto fail=[&](QString text){error=text;return QImage();};
    if(bytes.size()<128||bytes.first(4)!="DDS ")return fail("Incomplete DDS header.");
    const auto *p=reinterpret_cast<const uchar *>(bytes.constData());auto u=[&](int n){return qFromLittleEndian<quint32>(p+n);};
    if(u(4)!=124||u(76)!=32)return fail("Invalid DDS header size.");
    int w=int(u(16)),h=int(u(12)),levels=int(qMax(1u,u(28))),surfaces=1;qsizetype start=128;QString format;bool forceOpaque=false,premultiplied=false;quint32 masks[4]={u(92),u(96),u(100),u(104)};int bits=int(u(88));
    if(!imageValidSize(w,h)||levels<1||levels>14||u(24)>1||(u(112)&0x200000))return fail("Unsupported DDS dimensions, volume texture or mip count.");
    const QByteArray fourcc=bytes.mid(84,4);
    auto alphaMode=[&](QImage image){if(image.isNull())return image;if(forceOpaque||premultiplied)for(int y=0;y<image.height();++y){auto *row=reinterpret_cast<QRgb *>(image.scanLine(y));for(int x=0;x<image.width();++x){auto color=row[x];int a=qAlpha(color);auto straight=[&](int channel){return a?qMin(255,(channel*255+a/2)/a):0;};row[x]=forceOpaque?color|0xff000000: qRgba(straight(qRed(color)),straight(qGreen(color)),straight(qBlue(color)),a);}}return image;};
    if(u(80)&4){
        if(fourcc=="DXT1")format="BC1";else if(fourcc=="DXT2"||fourcc=="DXT3"){format="BC2";premultiplied=fourcc=="DXT2";}else if(fourcc=="DXT4"||fourcc=="DXT5"){format="BC3";premultiplied=fourcc=="DXT4";}else if(fourcc=="ATI1"||fourcc=="BC4U")format="BC4";else if(fourcc=="BC4S")format="BC4 SNORM";else if(fourcc=="ATI2"||fourcc=="BC5U")format="BC5";else if(fourcc=="BC5S")format="BC5 SNORM";
        else if(fourcc=="DX10"){
            if(bytes.size()<148)return fail("Incomplete DDS DX10 header.");start=148;
            if(u(132)!=3||u(140)<1||u(140)>2048)return fail("Only DDS 2D textures, arrays and cubemaps are supported.");surfaces=int(u(140))*((u(136)&4)?6:1);
            premultiplied=(u(144)&7)==2;forceOpaque=(u(144)&7)==3;
            switch(u(128)){case 49:format="RG8";break;case 56:format="R16 UNORM";break;case 35:format="RG16 UNORM";break;case 11:format="RGBA16 UNORM";break;case 54:format="R16 FLOAT";break;case 34:format="RG16 FLOAT";break;case 10:format="RGBA16 FLOAT";break;case 41:format="R32 FLOAT";break;case 2:format="RGBA32 FLOAT";break;case 71:case 72:format="BC1";break;case 74:case 75:format="BC2";break;case 77:case 78:format="BC3";break;case 80:format="BC4";break;case 81:format="BC4 SNORM";break;case 83:format="BC5";break;case 84:format="BC5 SNORM";break;case 95:format="BC6H";break;case 96:format="BC6H signed";break;case 98:case 99:format="BC7";break;case 28:case 29:format="RGBA8";break;case 87:case 91:format="BGRA8";break;case 88:case 93:format="BGRA8";forceOpaque=true;break;case 61:format="Gray8";break;case 85:format="RGB565";break;case 86:format="ARGB1555";break;default:return fail("Unsupported DDS DXGI format.");}
        }else return fail("Unsupported DDS compression code.");
    }else if(!(u(80)&0x40)||(bits!=16&&bits!=24&&bits!=32))return fail("DDS requires RGB masks or a supported BC format.");
    if(fourcc!="DX10"&&(u(112)&0x200)){if((u(112)&0xfc00)!=0xfc00)return fail("Partial legacy DDS cubemaps are not supported.");surfaces=6;}
    if(selection.surface>=surfaces||selection.mipLevel>=levels)return fail("DDS surface or mip level is out of range.");
    qint64 chain=0,offset=0,selectedSpan=0,stride=0;int sw=w,sh=h;
    for(int level=0;level<levels;++level){int lw=qMax(1,w>>level),lh=qMax(1,h>>level);qint64 row;
        if(format.startsWith("BC"))row=qint64((lw+3)/4)*imageBlockBytes(format);
        else row=qint64(lw)*(format.isEmpty()?bits:imagePixelBits(format))/8;
        if(!format.startsWith("BC")&&level==0&&(u(8)&8)){if(u(20)<row||u(20)>32*1024*1024)return fail("Invalid DDS row pitch.");row=u(20);}
        qint64 size=row*(format.startsWith("BC")?(lh+3)/4:lh);
        if(level==selection.mipLevel){offset=chain;selectedSpan=size;stride=row;sw=lw;sh=lh;}chain+=size;
    }
    offset+=start+qint64(selection.surface)*chain;if(offset<0||selectedSpan>bytes.size()-offset)return fail("DDS surface data is truncated.");
    if(!format.isEmpty()){ImageRecipe r;r.width=sw;r.height=sh;r.format=format;r.rowStride=stride;return alphaMode(decodeTextureData(bytes.mid(offset,selectedSpan),{},r,error,cancel));}
    // Legacy DDS RGB masks need not use one of our named raw channel orders.
    quint32 used=0;for(auto mask:masks){if((mask&used)||(bits<32&&(mask>>bits)))return fail("Invalid DDS channel masks.");used|=mask;}
    if(!masks[0]||!masks[1]||!masks[2])return fail("DDS RGB masks are missing.");
    auto channel=[](quint32 value,quint32 mask,int fallback){if(!mask)return fallback;int shift=0;while(!(mask&1)){mask>>=1;++shift;}if(mask&(mask+1))return -1;return int(((quint64(value>>shift)&mask)*255+mask/2)/mask);};
    QImage image(sw,sh,QImage::Format_ARGB32);if(image.isNull())return fail("Could not allocate DDS image.");
    for(int y=0;y<sh;++y){if(cancel.load())return {};for(int x=0;x<sw;++x){const uchar *v=p+offset+y*stride+x*(bits/8);quint32 value=0;for(int b=0;b<bits/8;++b)value|=quint32(v[b])<<(b*8);int r=channel(value,masks[0],0),g=channel(value,masks[1],0),b=channel(value,masks[2],0),a=channel(value,masks[3],255);if(r<0||g<0||b<0||a<0)return fail("Non-contiguous DDS channel masks are unsupported.");image.setPixel(x,y,qRgba(r,g,b,a));}}return image;
}

inline QImage decodeTim(const QByteArray &bytes,const ImageRecipe &selection,QString &error,const std::atomic_bool &cancel){
    auto fail=[&](QString text){error=text;return QImage();};if(bytes.size()<20)return fail("Incomplete TIM header.");const auto *p=reinterpret_cast<const uchar *>(bytes.constData());auto u16=[&](int n){return qFromLittleEndian<quint16>(p+n);};auto u32=[&](int n){return qFromLittleEndian<quint32>(p+n);};
    if(u32(0)!=16||(u32(4)&~15u))return fail("Invalid TIM header.");int mode=u32(4)&7;if(mode>3)return fail("Unsupported TIM color mode.");int at=8;QByteArray palette;
    if(u32(4)&8){if(bytes.size()<at+12)return fail("Incomplete TIM CLUT header.");auto length=u32(at);if(length<12||length>quint32(bytes.size()-at)||qint64(u16(at+8))*u16(at+10)*2>length-12)return fail("Invalid TIM CLUT block.");int count=mode==0?16:256;auto bank=qint64(selection.paletteBank)*count*2;if(bank+count*2>length-12)return fail("TIM palette bank is out of range.");palette=bytes.mid(at+12+bank,count*2);at+=length;}
    if(at>bytes.size()-12)return fail("Incomplete TIM image block.");auto length=u32(at);int words=u16(at+8),height=u16(at+10);if(length<12||length>quint32(bytes.size()-at)||qint64(words)*height*2>length-12)return fail("Invalid TIM image block.");
    ImageRecipe r;r.width=mode==0?words*4:mode==1?words*2:mode==2?words:words*2/3;r.height=height;r.format=mode==0?"Indexed4":mode==1?"Indexed8":mode==2?"PS1 BGR555":"RGB8";r.rowStride=words*2;r.lowNibbleFirst=true;r.paletteFormat="PS1 BGR555";
    if(selection.mipLevel||selection.surface)return fail("TIM contains one image without mipmaps.");return decodeTextureData(bytes.mid(at+12,length-12),palette,r,error,cancel);
}

inline QImage decodeTpl(const QByteArray &bytes,const ImageRecipe &selection,QString &error,const std::atomic_bool &cancel){
    auto fail=[&](QString text){error=text;return QImage();};if(bytes.size()<12)return fail("Incomplete TPL header.");const auto *p=reinterpret_cast<const uchar *>(bytes.constData());auto u16=[&](int n){return qFromBigEndian<quint16>(p+n);};auto u32=[&](int n){return qFromBigEndian<quint32>(p+n);};
    const qint64 count=u32(4),table=u32(8);if(u32(0)!=0x0020af30||count<1||count>4096||table>bytes.size()-count*8||selection.surface>=count)return fail("Invalid TPL table or image index.");
    const qint64 header=u32(table+selection.surface*8),palHeader=u32(table+selection.surface*8+4);if(header>bytes.size()-36)return fail("Invalid TPL image header.");
    QMap<quint32,QString> formats{{0,"GC I4"},{1,"GC I8"},{2,"GC IA4"},{3,"GC IA8"},{4,"GC RGB565"},{5,"GC RGB5A3"},{6,"GC RGBA8"},{8,"GC CI4"},{9,"GC CI8"},{10,"GC CI14X2"},{14,"GC CMPR"}};
    ImageRecipe r;r.width=u16(header+2);r.height=u16(header);r.format=formats.value(u32(header+4));qint64 offset=u32(header+8);if(r.format.isEmpty())return fail("Unsupported TPL texture format.");int maxMip=p[header+34];if(selection.mipLevel>maxMip||selection.mipLevel>13)return fail("TPL mip level is out of range.");
    for(int level=0;level<selection.mipLevel;++level){QString e;auto size=imageSpan(r,e);if(!e.isEmpty())return fail(e);offset+=size;r.width=qMax(1,r.width/2);r.height=qMax(1,r.height/2);}
    QString e;auto size=imageSpan(r,e);if(!e.isEmpty())return fail(e);if(offset>bytes.size()-size)return fail("TPL image data is truncated.");QByteArray palette;
    if(imageIndexed(r.format)){if(!palHeader||palHeader>bytes.size()-12)return fail("Missing TPL palette header.");int entries=u16(palHeader);auto pf=u32(palHeader+4);qint64 po=u32(palHeader+8);if(pf>2||entries<1||po>bytes.size()-qint64(entries)*2)return fail("Invalid TPL palette.");r.paletteFormat=pf==0?"GC IA8":pf==1?"RGB565":"GC RGB5A3";r.bigEndian=true;r.paletteEntries=entries;palette=bytes.mid(po,entries*2);}
    return decodeTextureData(bytes.mid(offset,size),palette,r,error,cancel);
}
