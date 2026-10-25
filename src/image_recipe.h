#pragma once
#include <QStringList>
#include <QtGlobal>
struct ImageRecipe {
    qint64 offset=0,rowStride=0,paletteOffset=0;
    int width=256,height=256,mipLevel=0,surface=0,paletteBank=0,paletteEntries=0;
    QString format="RGBA8",layout="Linear",paletteFormat="RGBA8",byteSwap="None",container="Auto";
    bool bigEndian=false,flipX=false,flipY=false,lowNibbleFirst=false,ps2Clut=false,ps2Alpha=false;
};
inline QStringList imageContainers(){return {"Auto","GTF","Genki image bank"};}
inline QStringList imageFormats(){return {"RG8","R16 UNORM","RG16 UNORM","RGBA16 UNORM","R16 FLOAT","RG16 FLOAT","RGBA16 FLOAT","R32 FLOAT","RGBA32 FLOAT","RGBA8","BGRA8","ARGB8","ABGR8","RGB8","BGR8","Gray8","Gray4","IA8","IA4","RGB565","RGB555","RGBA5551","ARGB1555","RGBA4444","PS1 BGR555","PS2 RGBA8","PS2 RGBA16","PS2 Indexed8H","PS2 Indexed4HL","PS2 Indexed4HH","RGBX8","Indexed8","Indexed4","BC1","BC2","BC3","BC4","BC4 SNORM","BC5","BC5 SNORM","BC6H","BC6H signed","BC7","NES CHR2","SNES 2bpp","SNES 4bpp","SNES 8bpp","SNES Mode7","GC I4","GC I8","GC IA4","GC IA8","GC RGB565","GC RGB5A3","GC RGBA8","GC CI4","GC CI8","GC CI14X2","GC CMPR","Encoded image"};}
inline QStringList imagePaletteFormats(){return {"RGBA8","BGRA8","RGB8","BGR8","RGB565","RGB555","PS1 BGR555","PS2 RGBA8","PS2 RGBA16","RGBX8","GC RGB5A3","GC IA8"};}
inline bool imageBlockCompressed(const QString &f){return f.startsWith("BC");}
inline bool imagePlanar(const QString &f){return f=="NES CHR2"||f.startsWith("SNES");}
inline bool imageIndexed(const QString &f){return f.startsWith("Indexed")||f.startsWith("GC CI")||f.startsWith("PS2 Indexed")||imagePlanar(f);}
inline int imagePixelBits(const QString &f){
    if(f=="RGBA32 FLOAT")return 128;
    if(f=="RGBA16 UNORM"||f=="RGBA16 FLOAT")return 64;
    if(f=="RG16 UNORM"||f=="RG16 FLOAT"||f=="R32 FLOAT")return 32;
    if(f=="RG8"||f=="R16 UNORM"||f=="R16 FLOAT")return 16;
    if(f=="NES CHR2"||f=="SNES 2bpp")return 2;
    if(f=="Indexed4"||f=="Gray4"||f=="SNES 4bpp"||f=="GC I4"||f=="GC CI4"||f=="BC1"||f=="BC4"||f=="BC4 SNORM"||f=="GC CMPR")return 4;
    if(imageBlockCompressed(f)||f=="Gray8"||f=="IA4"||f=="Indexed8"||f=="SNES 8bpp"||f=="SNES Mode7"||f=="GC I8"||f=="GC IA4"||f=="GC CI8")return 8;
    if(f=="RGB8"||f=="BGR8")return 24;
    if(f.startsWith("PS2 Indexed"))return 32;
    if(f=="RGBA8"||f=="BGRA8"||f=="ARGB8"||f=="ABGR8"||f=="PS2 RGBA8"||f=="RGBX8"||f=="GC RGBA8")return 32;
    return 16;
}
inline int imagePaletteCount(const ImageRecipe &r){if(r.paletteEntries>0&&r.paletteEntries<=16384)return r.paletteEntries;if(r.format=="NES CHR2"||r.format=="SNES 2bpp")return 4;if(r.format=="Indexed4"||r.format.startsWith("PS2 Indexed4")||r.format=="SNES 4bpp"||r.format=="GC CI4")return 16;if(r.format=="GC CI14X2")return 16384;return 256;}
inline int imageBlockBytes(const QString &f){return f=="BC1"||f=="BC4"||f=="BC4 SNORM"?8:16;}
inline bool imageValidSize(int w,int h){return w>0&&h>0&&w<=8192&&h<=8192&&qint64(w)*h<=4*1024*1024;}
inline qint64 imageMorton(int x,int y,int width,int height){qint64 result=0;int bit=0;for(int mask=1;mask<width||mask<height;mask<<=1){if(mask<width)result|=qint64(bool(x&mask))<<bit++;if(mask<height)result|=qint64(bool(y&mask))<<bit++;}return result;}
// Xenos address bit assignment for a 2D texture, in bytes (32-block aligned pitch).
inline qint64 imageXenosAddress(int x,int y,int pitch,int bytes){
    qint64 group=((qint64(y>>5)*(pitch>>5)+(x>>5))*64+(((y>>1)&7)*8+(x&7)))*bytes;
    int pipe=((x>>3)&3)^(((y>>3)&1)<<1);
    return ((y&1)<<4)|(pipe<<6)|(((y>>4)&1)<<11)|(group&15)|((group&16)<<1)|((group&224)<<3)|((group>>8)<<12);
}
// GS local-memory page/block/column address, expressed in bits. Storage pitch is rounded to pages.
inline qint64 imageGsBitAddress(int x,int y,int width,int bits,bool alternate16=false){
    int pw=bits<=8?128:64,ph=bits==4?128:bits==8||bits==16?64:32;
    int bw=bits==4?32:bits==8||bits==16?16:8,bh=bits<=8?16:8;
    int bx=(x%pw)/bw,by=(y%ph)/bh;int block;
    if(alternate16)block=(by&1)|((bx&1)<<1)|((by&2)<<2)|((bx&2)<<3)|(by&4);
    else if(bits==4||bits==16)block=(by&1)|((bx&1)<<1)|((by&2)<<1)|((bx&2)<<2)|((by&4)<<2);
    else block=(bx&1)|((by&1)<<1)|((bx&2)<<1)|((by&2)<<2)|((bx&4)<<2);
    int xx=x%bw,yy=y%bh,column;
    if(bits==32)column=(xx&1)|((yy&1)<<1)|((xx&6)<<1)|((yy&6)<<3);
    else if(bits==16)column=((xx&8)>>3)|((xx&1)<<1)|((yy&1)<<2)|((xx&6)<<2)|((yy&6)<<4);
    else if(bits==8)column=((yy&2)>>1)|((xx&8)>>2)|((xx&1)<<2)|((yy&1)<<3)|((xx&2)<<3)|((((xx>>2)^(yy>>1)^(yy>>2))&1)<<5)|((yy&12)<<4);
    else column=((yy&2)>>1)|((xx&8)>>2)|((xx&16)>>2)|((xx&1)<<3)|((yy&1)<<4)|((xx&2)<<4)|((((xx>>2)^(yy>>1)^(yy>>2))&1)<<6)|((yy&12)<<5);
    return (qint64(y/ph)*((width+pw-1)/pw)+x/pw)*8192*8+block*256*8+column*bits;
}
inline qint64 imageSpan(const ImageRecipe &r,QString &error){
    if(!imageFormats().contains(r.format)){error="Unsupported pixel format.";return 0;}
    if(r.offset<0||r.paletteOffset<0||r.rowStride<0||r.mipLevel<0||r.surface<0||r.paletteBank<0){error="Offsets and indexes must be nonnegative.";return 0;}
    if(r.format=="Encoded image")return 0;
    if(!imageValidSize(r.width,r.height)){error="Use dimensions up to 8192 and at most 4 million pixels.";return 0;}
    const auto f=r.format;const int bits=imagePixelBits(f);qint64 span=0;
    if(f.startsWith("GC ")){int bw=bits==4?8:bits==8?8:4,bh=bits==4?8:bits==8?4:4;if(f=="GC CMPR")bw=bh=8;span=qint64((r.width+bw-1)/bw)*((r.height+bh-1)/bh)*(f=="GC RGBA8"?64:32);}
    else if(imagePlanar(f)){if(r.width%8||r.height%8){error="NES/SNES tile dimensions must be multiples of 8.";return 0;}span=qint64(r.width)*r.height*bits/8;}
    else{
        int w=imageBlockCompressed(f)?(r.width+3)/4:r.width,h=imageBlockCompressed(f)?(r.height+3)/4:r.height,unit=imageBlockCompressed(f)?imageBlockBytes(f):bits/8;
        if(r.layout=="PS2 GS pages"||r.layout=="PS2 GS 16S"){
            if(r.layout=="PS2 GS 16S"&&bits!=16){error="GS 16S layout requires 16-bit pixels.";return 0;}
            if(imageBlockCompressed(f)||(bits!=4&&bits!=8&&bits!=16&&bits!=32)){error="GS layout requires 4/8/16/32-bit raw pixels.";return 0;}int pw=bits<=8?128:64,ph=bits==4?128:bits==8||bits==16?64:32;span=qint64((w+pw-1)/pw)*((h+ph-1)/ph)*8192;
        }else if(r.layout=="Xbox 360 tiled"){
            if(unit!=1&&unit!=2&&unit!=4&&unit!=8&&unit!=16){error="Xenos tiling needs 1/2/4/8/16-byte pixels or blocks.";return 0;}
            int pitch=(w+31)&~31;span=0;for(int y=0;y<h;++y)for(int x=0;x<w;++x)span=qMax(span,imageXenosAddress(x,y,pitch,unit)+unit);
        }else if(r.layout=="Morton Z-order"||r.layout=="Morton rectangular"){
            if((w&(w-1))||(h&(h-1))||(r.layout=="Morton Z-order"&&w!=h)){error="Morton layout requires power-of-two dimensions (square for Z-order).";return 0;}span=(qint64(w)*h*(imageBlockCompressed(f)?unit*8:bits)+7)/8;
        }else if(r.layout=="PS4 1D thin"){
            if(unit!=1&&unit!=2&&unit!=4&&unit!=8&&unit!=16){error="PS4 thin layout needs 1/2/4/8/16-byte pixels or blocks.";return 0;}
            span=qint64((w+7)/8)*((h+7)/8)*64*unit;
        }else if(r.layout=="Tiled 8x8"){
            if(w%8||h%8){error="Tiled 8x8 requires pixel/block dimensions divisible by 8.";return 0;}span=qint64(w)*h*(imageBlockCompressed(f)?unit*8:bits)/8;
        }else if(r.layout=="Linear"){
            const qint64 row=imageBlockCompressed(f)?qint64(w)*unit:(qint64(w)*bits+7)/8,stride=r.rowStride?r.rowStride:row;if(stride<row||stride>32*1024*1024){error="Row stride must fit a complete row and be at most 32 MiB.";return 0;}span=(h-1)*stride+row;
        }else{error="Unsupported memory layout.";return 0;}
    }
    if(r.layout!="Linear"&&r.rowStride){error="Swizzled layouts require row stride 0.";return 0;}
    if((f.startsWith("GC ")||imagePlanar(f))&&r.layout!="Linear"){error="This format already defines its tile layout; choose Linear.";return 0;}
    if(span>32*1024*1024){error="The image data exceeds 32 MiB.";return 0;}return span;
}
