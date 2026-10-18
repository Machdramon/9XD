#pragma once
#include <QImage>
#include <QtEndian>
#include <atomic>

// Truevision TGA: raw/RLE truecolor, grayscale and color-mapped images.
inline QImage decodeTga(const QByteArray &bytes,QString &error,const std::atomic_bool &cancel){
    auto fail=[&](const QString &text){error=text;return QImage();};
    if(bytes.size()<18)return fail("Unsupported image or incomplete TGA header.");
    const auto *p=reinterpret_cast<const uchar *>(bytes.constData());
    auto word=[&](int at){return qFromLittleEndian<quint16>(p+at);};
    const int type=p[2],base=type>=8?type-8:type,depth=p[16],descriptor=p[17],attributes=descriptor&15;
    const bool rle=type>=8;const int width=word(12),height=word(14),first=word(3),colors=word(5),paletteDepth=p[7];
    if((base!=1&&base!=2&&base!=3)||p[1]!=(base==1?1:0)||(descriptor&0xc0))return fail("Unsupported image or TGA type (interleaved TGA is not supported).");
    if(width<1||height<1||width>8192||height>8192||qint64(width)*height>4*1024*1024)return fail("TGA dimensions exceed the preview limit.");
    if((base==1&&(depth!=8&&depth!=16))||(base==2&&depth!=15&&depth!=16&&depth!=24&&depth!=32)||(base==3&&depth!=8&&depth!=16))return fail("Unsupported TGA pixel depth.");
    if(base==1&&(colors<1||first+colors>65536||(paletteDepth!=15&&paletteDepth!=16&&paletteDepth!=24&&paletteDepth!=32)))return fail("Invalid TGA palette.");
    qsizetype at=18+p[0];
    auto color=[&](int bits,bool alpha)->QRgb{int size=(bits+7)/8;if(at>bytes.size()-size){error="Truncated TGA pixel data.";return 0;}const auto *v=p+at;at+=size;if(bits==15||bits==16){quint16 value=qFromLittleEndian<quint16>(v);return qRgba(((value>>10)&31)*255/31,((value>>5)&31)*255/31,(value&31)*255/31,alpha?(value&0x8000?255:0):255);}return qRgba(v[2],v[1],v[0],bits==32&&alpha?v[3]:255);};
    QVector<QRgb> palette;if(base==1){palette.resize(colors);for(int i=0;i<colors;++i){palette[i]=color(paletteDepth,paletteDepth==16||paletteDepth==32);if(!error.isEmpty())return {};}}
    auto pixel=[&]()->QRgb{
        if(base==2)return color(depth,attributes!=0);
        const int size=depth/8;if(at>bytes.size()-size){error="Truncated TGA pixel data.";return 0;}const auto *v=p+at;at+=size;
        if(base==3)return qRgba(v[0],v[0],v[0],depth==16?v[1]:255);
        int index=(depth==8?v[0]:qFromLittleEndian<quint16>(v))-first;if(index<0||index>=palette.size()){error="TGA palette index is out of range.";return 0;}return palette[index];
    };
    QImage image(width,height,QImage::Format_ARGB32);if(image.isNull())return fail("Could not allocate TGA image.");
    const int total=width*height;int position=0;
    while(position<total){if(cancel.load())return {};int count=1;bool repeat=false;if(rle){if(at>=bytes.size())return fail("Truncated TGA RLE packet.");auto packet=p[at++];count=(packet&127)+1;repeat=packet&128;if(count>total-position)return fail("TGA RLE packet exceeds image bounds.");}QRgb value=0;if(repeat)value=pixel();for(int i=0;i<count;++i){if(!repeat)value=pixel();if(!error.isEmpty())return {};const int x=position%width,y=position/width;image.setPixel(descriptor&16?width-1-x:x,descriptor&32?y:height-1-y,value);++position;}}
    return image;
}
