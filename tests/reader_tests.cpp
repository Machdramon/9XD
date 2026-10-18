#include "compare_data.h"
#include "data_inspector.h"
#include <QCheckBox>
#include "binary_reader.h"
#include "mesh_data.h"
#include "mesh_debug.h"
#include "mesh_scene.h"
#include "model_export.h"
#include "recent_files.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QtEndian>
#include <bit>
#include <cmath>
#include "hex_view.h"
#include "image_decode.h"
#include <QLineEdit>
#include <QTableWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QClipboard>
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QSignalSpy>
#include <QTimer>
#include <atomic>
#include <QScrollBar>
#include <QSemaphore>
#include <QtConcurrentRun>
#include <climits>
#include <QWheelEvent>
#ifdef Q_OS_WIN
#include <windows.h>
#include <winioctl.h>
#include <io.h>
#endif

// QFile::resize alone does not mark a file sparse on NTFS.
static bool resizeSparseFile(QFile &file, qint64 size) {
#ifdef Q_OS_WIN
    const auto handle=reinterpret_cast<HANDLE>(_get_osfhandle(file.handle()));
    DWORD returned=0;
    if(handle==INVALID_HANDLE_VALUE || !DeviceIoControl(handle,FSCTL_SET_SPARSE,
        nullptr,0,nullptr,0,&returned,nullptr))return false;
#endif
    return file.resize(size);
}

class ReaderTests : public QObject {
    Q_OBJECT
private slots:
    void imageContainersDdsTimTim2TplGtfGnf(){
        std::atomic_bool cancel{false};QString error;ImageRecipe selection;
        auto le32=[](QByteArray &b,int at,quint32 v){qToLittleEndian(v,reinterpret_cast<uchar *>(b.data()+at));};auto le16=[](QByteArray &b,int at,quint16 v){qToLittleEndian(v,reinterpret_cast<uchar *>(b.data()+at));};auto be32=[](QByteArray &b,int at,quint32 v){qToBigEndian(v,reinterpret_cast<uchar *>(b.data()+at));};auto be16=[](QByteArray &b,int at,quint16 v){qToBigEndian(v,reinterpret_cast<uchar *>(b.data()+at));};
        QByteArray dds(128,0);dds.replace(0,4,"DDS ");le32(dds,4,124);le32(dds,12,4);le32(dds,16,4);le32(dds,28,2);le32(dds,76,32);le32(dds,80,4);dds.replace(84,4,"DXT1");dds+=QByteArray::fromHex("00f8000000000000e007000000000000");auto image=decodeDds(dds,selection,error,cancel);QVERIFY2(error.isEmpty(),qPrintable(error));QCOMPARE(image.pixelColor(3,3),QColor(Qt::red));selection.mipLevel=1;image=decodeDds(dds,selection,error,cancel);QCOMPARE(image.size(),QSize(2,2));QCOMPARE(image.pixelColor(1,1),QColor(Qt::green));selection.mipLevel=2;image=decodeDds(dds,selection,error,cancel);QVERIFY(!error.isEmpty());selection={};error.clear();
        // TIM 4bpp, two banks of 16 BGR555 entries, one 4-pixel row.
        QByteArray tim(8+12+64+12+2,0);le32(tim,0,16);le32(tim,4,8);le32(tim,8,76);le16(tim,16,16);le16(tim,18,2);le16(tim,20,31);le16(tim,52,0x3e0);le32(tim,84,14);le16(tim,92,1);le16(tim,94,1);image=decodeTim(tim,selection,error,cancel);QVERIFY2(error.isEmpty(),qPrintable(error));QCOMPARE(image.size(),QSize(4,1));QCOMPARE(image.pixelColor(0,0),QColor(Qt::red));selection.paletteBank=1;image=decodeTim(tim,selection,error,cancel);QCOMPARE(image.pixelColor(0,0),QColor(Qt::green));selection={};
        QByteArray tm2(16+48+8,0);tm2.replace(0,4,"TIM2");tm2[4]=4;le16(tm2,6,1);le32(tm2,16,56);le32(tm2,24,8);le16(tm2,28,48);tm2[33]=1;tm2[35]=3;le16(tm2,36,2);le16(tm2,38,1);tm2.replace(64,8,QByteArray::fromHex("ff00008000ff0080"));image=decodeTim2(tm2,selection,error,cancel);QVERIFY2(error.isEmpty(),qPrintable(error));QCOMPARE(image.pixelColor(1,0),QColor(Qt::green));
        QByteArray tpl(56+32,0);be32(tpl,0,0x20af30);be32(tpl,4,1);be32(tpl,8,12);be32(tpl,12,20);be16(tpl,20,4);be16(tpl,22,4);be32(tpl,24,4);be32(tpl,28,56);for(int i=0;i<16;++i)be16(tpl,56+i*2,0xf800);image=decodeTpl(tpl,selection,error,cancel);QVERIFY2(error.isEmpty(),qPrintable(error));QCOMPARE(image.pixelColor(3,3),QColor(Qt::red));
        QByteArray gtf(48+8,0);be32(gtf,0,0x02020000);be32(gtf,4,8);be32(gtf,8,1);be32(gtf,16,48);be32(gtf,20,8);gtf[24]=char(0xa5);gtf[25]=1;gtf[26]=2;be32(gtf,28,0xaae4);be16(gtf,32,2);be16(gtf,34,1);be16(gtf,36,1);be32(gtf,40,8);gtf.replace(48,8,QByteArray::fromHex("ffff0000ff00ff00"));image=decodeGtf(gtf,selection,error,cancel);QVERIFY2(error.isEmpty(),qPrintable(error));QCOMPARE(image.pixelColor(0,0),QColor(Qt::red));QCOMPARE(image.pixelColor(1,0),QColor(Qt::green));
        QByteArray gnf(64,0);gnf.replace(0,4,"GNF ");le32(gnf,4,40);gnf[8]=2;gnf[9]=1;le32(gnf,12,64);le32(gnf,20,10u<<20);le32(gnf,24,1|(1<<14));le32(gnf,28,(9u<<28)|(8u<<20)|4|(5<<3)|(6<<6)|(7<<9));le32(gnf,32,1<<13);for(int i=0;i<4;++i)gnf.replace(48+i*4,4,QByteArray::fromHex("ff0000ff"));image=decodeGnf(gnf,selection,error,cancel);QVERIFY2(error.isEmpty(),qPrintable(error));QCOMPARE(image.pixelColor(1,1),QColor(Qt::red));le32(gnf,28,(9u<<28)|(14u<<20));image=decodeGnf(gnf,selection,error,cancel);QVERIFY(error.contains("tile mode"));
        // Every parser rejects short inputs without inspecting outside the header.
        for(int n=0;n<48;++n){QByteArray shortData(n,0);error.clear();QVERIFY(decodeDds(shortData,selection,error,cancel).isNull());error.clear();QVERIFY(decodeTim(shortData.left(qMin(n,19)),selection,error,cancel).isNull());error.clear();QVERIFY(decodeTpl(shortData,selection,error,cancel).isNull());error.clear();QVERIFY(decodeGtf(shortData,selection,error,cancel).isNull());error.clear();QVERIFY(decodeGnf(shortData,selection,error,cancel).isNull());}
    }
    void expandedPackedAndClassicConsoleFormats(){
        std::atomic_bool cancel{false};QString error;ImageRecipe r;r.width=r.height=1;
        auto check=[&](QString format,QByteArray data,QColor expected){r.format=format;error.clear();auto image=decodeTextureData(data,{},r,error,cancel);QVERIFY2(error.isEmpty(),qPrintable(error));QCOMPARE(image.pixelColor(0,0),expected);};
        check("ARGB8",QByteArray::fromHex("80ff0000"),QColor(255,0,0,128));check("ABGR8",QByteArray::fromHex("800000ff"),QColor(255,0,0,128));
        check("RGBA5551",QByteArray::fromHex("01f8"),QColor(Qt::red));check("ARGB1555",QByteArray::fromHex("00fc"),QColor(Qt::red));check("PS2 RGBA16",QByteArray::fromHex("1f80"),QColor(Qt::red));check("PS2 RGBA8",QByteArray::fromHex("ff000080"),QColor(Qt::red));
        check("RGBA4444",QByteArray::fromHex("0ff0"),QColor(255,0,0,255));check("PS1 BGR555",QByteArray::fromHex("0000"),QColor(0,0,0,0));check("PS1 BGR555",QByteArray::fromHex("1f80"),QColor(255,0,0,128));
        QByteArray palette=QByteArray::fromHex("000000ffff0000ff00ff00ff0000ffff");r.width=r.height=8;r.format="NES CHR2";QByteArray data(16,0);data[0]=char(0x80);data[8]=char(0x40);error.clear();auto image=decodeTextureData(data,palette,r,error,cancel);QVERIFY(error.isEmpty());QCOMPARE(image.pixelColor(0,0),QColor(Qt::red));QCOMPARE(image.pixelColor(1,0),QColor(Qt::green));QCOMPARE(image.pixelColor(2,0),QColor(Qt::black));
        r.format="SNES 2bpp";data.fill(0);data[0]=char(0x80);data[1]=char(0x40);image=decodeTextureData(data,palette,r,error,cancel);QCOMPARE(image.pixelColor(0,0),QColor(Qt::red));QCOMPARE(image.pixelColor(1,0),QColor(Qt::green));
        palette.resize(1024);for(int i=0;i<256;++i){palette[i*4]=char(i);palette[i*4+1]=0;palette[i*4+2]=0;palette[i*4+3]=char(255);}for(auto format:{"SNES 4bpp","SNES 8bpp"}){r.format=format;data=QByteArray(imagePixelBits(format)*8,0);data[0]=char(0x80);data[16]=char(0x80);image=decodeTextureData(data,palette,r,error,cancel);QCOMPARE(image.pixelColor(0,0).red(),5);}
        r.format="SNES Mode7";data=QByteArray(64,char(7));image=decodeTextureData(data,palette,r,error,cancel);QCOMPARE(image.pixelColor(7,7).red(),7);
    }
    void gamecubeFormatsAndCompressedBlocks(){
        std::atomic_bool cancel{false};QString error;ImageRecipe r;r.width=r.height=4;r.format="GC RGB565";QByteArray bytes;for(int i=0;i<16;++i)bytes+=QByteArray::fromHex("f800");auto image=decodeTextureData(bytes,{},r,error,cancel);QVERIFY(error.isEmpty());QCOMPARE(image.pixelColor(3,3),QColor(Qt::red));
        r.format="GC RGB5A3";bytes.fill(0);for(int i=0;i<16;++i){bytes[i*2]=char(0x3f);bytes[i*2+1]=0;}image=decodeTextureData(bytes,{},r,error,cancel);QCOMPARE(image.pixelColor(0,0),QColor(255,0,0,109));
        r.format="GC RGBA8";bytes=QByteArray(64,0);for(int i=0;i<16;++i){bytes[i*2]=char(255);bytes[i*2+1]=char(255);}image=decodeTextureData(bytes,{},r,error,cancel);QCOMPARE(image.pixelColor(3,3),QColor(Qt::red));
        r.format="GC IA4";bytes=QByteArray(32,char(0x8f));image=decodeTextureData(bytes,{},r,error,cancel);QCOMPARE(image.pixelColor(0,0),QColor(255,255,255,136));
        r.width=r.height=8;r.format="GC CMPR";bytes.clear();for(int i=0;i<4;++i)bytes+=QByteArray::fromHex("f800000000000000");image=decodeTextureData(bytes,{},r,error,cancel);QVERIFY(error.isEmpty());QCOMPARE(image.pixelColor(7,7),QColor(Qt::red));
        r.width=3;r.height=2;r.format="BC1";bytes=QByteArray::fromHex("00f8000000000000");image=decodeTextureData(bytes,{},r,error,cancel);QVERIFY(error.isEmpty());QCOMPARE(image.size(),QSize(3,2));QCOMPARE(image.pixelColor(2,1),QColor(Qt::red));
        r.format="BC2";bytes=QByteArray(8,char(255))+QByteArray::fromHex("00f8000000000000");image=decodeTextureData(bytes,{},r,error,cancel);QCOMPARE(image.pixelColor(2,1),QColor(Qt::red));
        r.format="BC3";bytes=QByteArray::fromHex("ff0000000000000000f8000000000000");image=decodeTextureData(bytes,{},r,error,cancel);QCOMPARE(image.pixelColor(2,1),QColor(Qt::red));
        r.format="BC4";bytes=QByteArray::fromHex("ff00000000000000");image=decodeTextureData(bytes,{},r,error,cancel);QCOMPARE(image.pixelColor(2,1),QColor(Qt::white));
        r.format="BC5";bytes+=QByteArray(8,0);image=decodeTextureData(bytes,{},r,error,cancel);QCOMPARE(image.pixelColor(2,1),QColor(Qt::red));
        r.format="BC7";bytes=QByteArray(16,0);int bit=0;auto put=[&](int value,int count){for(int i=0;i<count;++i,++bit)if(value&(1<<i))bytes[bit/8]=char(quint8(bytes[bit/8])|(1<<(bit%8)));};put(64,7);for(int c=0;c<4;++c){put(c==0||c==3?127:0,7);put(c==0||c==3?127:0,7);}put(0,2);image=decodeTextureData(bytes,{},r,error,cancel);QVERIFY(error.isEmpty());QCOMPARE(image.pixelColor(2,1),QColor(254,0,0,254));
        for(auto format:{"BC6H","BC6H signed"}){r.format=format;image=decodeTextureData(QByteArray(16,0),{},r,error,cancel);QVERIFY(error.isEmpty());QCOMPARE(image.pixelColor(2,1),QColor(Qt::black));}
        r.format="BC1";image=decodeTextureData(QByteArray(7,0),{},r,error,cancel);QVERIFY(!error.isEmpty());
    }
    void consoleMemoryAddressingAndEndian(){
        std::atomic_bool cancel{false};QString error;ImageRecipe r;r.width=r.height=4;r.format="Gray8";r.layout="PS2 GS pages";QByteArray gs(8192,0);const int offsets[4][4]={{0,4,16,20},{8,12,24,28},{33,37,49,53},{41,45,57,61}};for(int y=0;y<4;++y)for(int x=0;x<4;++x)gs[offsets[y][x]]=char(y*4+x);auto image=decodeTextureData(gs,{},r,error,cancel);QVERIFY(error.isEmpty());for(int y=0;y<4;++y)for(int x=0;x<4;++x)QCOMPARE(image.pixelColor(x,y).red(),y*4+x);
        // Check that a complete GS page is a bijection for all supported PSM bit depths.
        for(int bits:{4,8,16,32}){QSet<qint64> addresses;int width=bits<=8?128:64,height=bits==4?128:bits==8||bits==16?64:32;for(int y=0;y<height;++y)for(int x=0;x<width;++x){auto address=imageGsBitAddress(x,y,width,bits);QVERIFY(address>=0&&address<8192*8);QVERIFY(!addresses.contains(address));addresses.insert(address);}}
        r.width=32;r.height=32;r.layout="Xbox 360 tiled";QString e;auto span=imageSpan(r,e);QByteArray tiled(span,0);QSet<qint64> addresses;for(int y=0;y<32;++y)for(int x=0;x<32;++x){auto address=imageXenosAddress(x,y,32,1);QVERIFY(!addresses.contains(address));addresses.insert(address);tiled[address]=char((y*32+x)%251);}image=decodeTextureData(tiled,{},r,error,cancel);QVERIFY(error.isEmpty());for(int y=0;y<32;++y)for(int x=0;x<32;++x)QCOMPARE(image.pixelColor(x,y).red(),(y*32+x)%251);
        QCOMPARE(imageXenosAddress(0,1,32,4),qint64(16));QCOMPARE(imageXenosAddress(8,0,32,4),qint64(64));QCOMPARE(imageXenosAddress(0,16,32,4),qint64(2048));
        r.width=8;r.height=4;r.layout="Morton rectangular";QByteArray morton(32,0);for(int y=0;y<4;++y)for(int x=0;x<8;++x)morton[imageMorton(x,y,8,4)]=char(y*8+x);image=decodeTextureData(morton,{},r,error,cancel);QCOMPARE(image.pixelColor(7,3).red(),31);
        r.width=r.height=1;r.format="RGBA8";r.layout="Linear";r.byteSwap="Swap 8 in 32";image=decodeTextureData(QByteArray::fromHex("ff0000ff"),{},r,error,cancel);QCOMPARE(image.pixelColor(0,0),QColor(Qt::red));
    }
    void encodedImagesTgaAndUnswizzle(){
        std::atomic_bool cancel{false};QImage original(2,2,QImage::Format_ARGB32);original.fill(Qt::red);
        auto decode=[&](const QByteArray &bytes,ImageRecipe recipe){auto memory=std::make_shared<MemoryData>();memory->blocks={bytes};memory->size=bytes.size();ReadRequest source;source.memory=memory;source.expectedSize=bytes.size();return decodeBinaryImage(source,{},recipe,cancel);};
        ImageRecipe recipe;recipe.format="Encoded image";
        for(const auto &format:{"PNG","BMP","JPG","TIFF","WEBP"}){QByteArray bytes;QBuffer buffer(&bytes);QVERIFY(buffer.open(QIODevice::WriteOnly));QVERIFY2(original.save(&buffer,format),format);auto result=decode(bytes,recipe);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.image.size(),original.size());QVERIFY(result.image.pixelColor(0,0).red()>240);}
        // 2x1, top origin, 24-bit uncompressed BGR: red then green.
        QByteArray tga=QByteArray::fromHex("0000020000000000000000000200010018200000ff00ff00");
        auto result=decode(tga,recipe);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.image.pixelColor(0,0),QColor(Qt::red));QCOMPARE(result.image.pixelColor(1,0),QColor(Qt::green));
        tga[2]=10;tga.replace(18,6,QByteArray::fromHex("810000ff"));result=decode(tga,recipe);QVERIFY(result.error.isEmpty());QCOMPARE(result.image.pixelColor(1,0),QColor(Qt::red));
        tga[18]=char(0x82);result=decode(tga,recipe);QVERIFY(!result.error.isEmpty());tga.truncate(19);result=decode(tga,recipe);QVERIFY(!result.error.isEmpty());
        // Indexed 8-bit with a palette starting at index 5, a 24-bit green entry.
        tga=QByteArray::fromHex("00010105000100180000000001000100082000ff0005");result=decode(tga,recipe);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.image.pixelColor(0,0),QColor(Qt::green));
        // Grayscale with alpha.
        tga=QByteArray::fromHex("0000030000000000000000000100010010288040");result=decode(tga,recipe);QVERIFY(result.error.isEmpty());QCOMPARE(result.image.pixelColor(0,0),QColor(128,128,128,64));
        recipe.format="Gray8";recipe.width=4;recipe.height=4;recipe.layout="Morton Z-order";QByteArray morton(16,0);
        for(int y=0;y<4;++y)for(int x=0;x<4;++x){int index=(x&1)|((y&1)<<1)|((x&2)<<1)|((y&2)<<2);morton[index]=char(y*4+x);}
        result=decode(morton,recipe);QVERIFY(result.error.isEmpty());for(int y=0;y<4;++y)for(int x=0;x<4;++x)QCOMPARE(result.image.pixelColor(x,y).red(),y*4+x);
        recipe.width=3;result=decode(morton,recipe);QVERIFY(!result.error.isEmpty());
        recipe.width=16;recipe.height=8;recipe.layout="Tiled 8x8";QByteArray tiled(128,0);for(int y=0;y<8;++y)for(int x=0;x<16;++x)tiled[(x/8)*64+y*8+x%8]=char(y*16+x);
        result=decode(tiled,recipe);QVERIFY(result.error.isEmpty());for(int y=0;y<8;++y)for(int x=0;x<16;++x)QCOMPARE(result.image.pixelColor(x,y).red(),y*16+x);
        recipe.rowStride=16;result=decode(tiled,recipe);QVERIFY(!result.error.isEmpty());
    }
    void rawImageFormatsRangesAndSnapshots(){
        std::atomic_bool cancel{false};auto memory=std::make_shared<MemoryData>();auto data=QByteArray::fromHex("ff0000ff00ff00ff");memory->blocks={data};memory->size=data.size();ReadRequest source;source.memory=memory;source.expectedSize=memory->size;
        ImageRecipe recipe;recipe.width=2;recipe.height=1;
        auto result=decodeBinaryImage(source,{},recipe,cancel);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.image.pixelColor(0,0),QColor(Qt::red));QCOMPARE(result.image.pixelColor(1,0),QColor(Qt::green));
        recipe.flipX=true;result=decodeBinaryImage(source,{},recipe,cancel);QCOMPARE(result.image.pixelColor(0,0),QColor(Qt::green));recipe.flipX=false;
        recipe.format="BGRA8";result=decodeBinaryImage(source,{},recipe,cancel);QCOMPARE(result.image.pixelColor(0,0),QColor(Qt::blue));
        recipe.format="RGB565";recipe.width=1;recipe.bigEndian=true;PatchList patches{{0,QByteArray::fromHex("f800")}};result=decodeBinaryImage(source,patches,recipe,cancel);QCOMPARE(result.image.pixelColor(0,0),QColor(Qt::red));
        recipe.bigEndian=false;patches={{0,QByteArray::fromHex("00f8")}};result=decodeBinaryImage(source,patches,recipe,cancel);QCOMPARE(result.image.pixelColor(0,0),QColor(Qt::red));
        recipe.format="Gray8";recipe.width=2;recipe.height=2;recipe.rowStride=4;result=decodeBinaryImage(source,{},recipe,cancel);QCOMPARE(result.image.pixelColor(0,1),QColor(Qt::black));QCOMPARE(result.image.pixelColor(1,1),QColor(Qt::white));
        recipe.offset=7;result=decodeBinaryImage(source,{},recipe,cancel);QVERIFY(!result.error.isEmpty());recipe.offset=0;recipe.rowStride=1;result=decodeBinaryImage(source,{},recipe,cancel);QVERIFY(!result.error.isEmpty());
        recipe.rowStride=0;cancel=true;result=decodeBinaryImage(source,{},recipe,cancel);QVERIFY(result.cancelled);cancel=false;
        QByteArray indexed(66,0);indexed[0]=char(0x12);indexed.replace(6,4,QByteArray::fromHex("ff0000ff"));indexed.replace(10,4,QByteArray::fromHex("00ff00ff"));memory->blocks={indexed};memory->size=indexed.size();source.expectedSize=memory->size;recipe.width=2;recipe.height=1;recipe.format="Indexed4";recipe.paletteOffset=2;result=decodeBinaryImage(source,{},recipe,cancel);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.image.pixelColor(0,0),QColor(Qt::red));QCOMPARE(result.image.pixelColor(1,0),QColor(Qt::green));
    }
    void checksumKnownVectors(){
        MemoryData empty;auto memory=empty.splice(0,0,"123456789");ReadRequest source{"",0,9,{},0,memory};
        ChecksumSpec spec;spec.length=9;spec.algorithms=checksumAlgorithms();std::atomic<bool> cancel{false};std::atomic<qint64> progress{0};
        const auto result=calculateChecksums(source,spec,cancel,progress);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));
        QCOMPARE(result.checksums.size(),8);QCOMPARE(progress.load(),qint64(9));
        QCOMPARE(result.checksums.value("Checksum-8"),QString("DD"));QCOMPARE(result.checksums.value("Checksum-16"),QString("01DD"));
        QCOMPARE(result.checksums.value("Checksum-32"),QString("000001DD"));QCOMPARE(result.checksums.value("CRC-32"),QString("CBF43926"));
        QCOMPARE(result.checksums.value("MD5"),QString("25F9E794323B453885F5181F1B624D0B"));
        QCOMPARE(result.checksums.value("SHA-1"),QString("F7C3BC1D808E04732ADF679965CCC34CA7AE3441"));
        QCOMPARE(result.checksums.value("SHA-256"),QString("15E2B0D3C33891EBB0F1EF609EC419420C20E320CE94C65FBC8C3312448EB225"));
        QCOMPARE(result.checksums.value("SHA-512"),QString("D9E6762DD1C8EAF6D61B3C6192FC408D4D6D5F1176D0C29169BC24E71C3F274AD27FCD5811B313D681F7E55EC02D73D499C95455B6B5BB503ACF574FBA8FFE85"));
        spec.length=0;auto zero=calculateChecksums(source,spec,cancel,progress);QVERIFY(zero.error.isEmpty());
        QCOMPARE(zero.checksums.value("CRC-32"),QString("00000000"));QCOMPARE(zero.checksums.value("MD5"),QString("D41D8CD98F00B204E9800998ECF8427E"));
        cancel.store(true);QVERIFY(calculateChecksums(source,spec,cancel,progress).cancelled);
        cancel.store(false);spec.begin=10;QVERIFY(!calculateChecksums(source,spec,cancel,progress).error.isEmpty());
    }
    void checksumDiskPatchesAndBlockBoundaries(){
        QTemporaryDir directory;QVERIFY(directory.isValid());QFile file(directory.filePath("checksum.bin"));
        const QByteArray bytes=QByteArray(1048576,'A')+QByteArray("BC");QVERIFY(file.open(QIODevice::WriteOnly));QCOMPARE(file.write(bytes),qint64(bytes.size()));file.close();
        auto metadata=readPage({file.fileName(),0,-1,{},0});QVERIFY(metadata.error.isEmpty());
        ReadRequest source{file.fileName(),0,metadata.size,metadata.modified,0};ChecksumSpec spec;spec.length=bytes.size();spec.algorithms={"SHA-256","CRC-32"};
        spec.patches={{1048575,QByteArray("XYZ")}};std::atomic<bool> cancel{false};std::atomic<qint64> progress{0};
        auto patched=bytes;patched.replace(1048575,3,"XYZ");auto result=calculateChecksums(source,spec,cancel,progress);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));
        QCOMPARE(result.checksums.value("SHA-256"),QString::fromLatin1(QCryptographicHash::hash(patched,QCryptographicHash::Sha256).toHex().toUpper()));
        spec.begin=1048575;spec.length=3;result=calculateChecksums(source,spec,cancel,progress);QVERIFY(result.error.isEmpty());
        QCOMPARE(result.checksums.value("SHA-256"),QString::fromLatin1(QCryptographicHash::hash("XYZ",QCryptographicHash::Sha256).toHex().toUpper()));
        source.expectedModified=source.expectedModified.addSecs(-1);QVERIFY(!calculateChecksums(source,spec,cancel,progress).error.isEmpty());
        cancel.store(true);QVERIFY(calculateChecksums(source,spec,cancel,progress).cancelled);
    }
    void recentFilesPersistence() {
        QTemporaryDir dir;QString settings=dir.filePath("prefs/recent.json");RecentFiles recent(settings);QStringList files;
        for(int i=0;i<14;++i){QString path=dir.filePath(QString("file%1.bin").arg(i));QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));f.write("test");f.close();files.append(QFileInfo(path).canonicalFilePath());QVERIFY(recent.add(path));}
        QCOMPARE(recent.paths().size(),12);QCOMPARE(recent.paths().first(),files.last());QVERIFY(recent.add(files[4]));QCOMPARE(recent.paths().first(),files[4]);QCOMPARE(recent.paths().count(files[4]),1);RecentFiles reloaded(settings);QCOMPARE(reloaded.paths(),recent.paths());QVERIFY(reloaded.add(dir.filePath("does-not-exist.bin")));QCOMPARE(reloaded.paths(),recent.paths());QVERIFY(reloaded.clear());QVERIFY(RecentFiles(settings).paths().isEmpty());
    }
    void modelExportFormatsAndTransforms() {
        QTemporaryDir dir;auto mesh=std::make_shared<MeshData>();mesh->vertices={{{0,0,0},{0,0,1},{0,0}},{{1,0,0},{0,0,1},{1,0}},{{0,1,0},{0,0,1},{0,1}}};mesh->triangles={0,1,2};auto plain=std::make_shared<MeshData>(*mesh);plain->hasUvs=false;
        auto read=[](const QString &path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return QByteArray();return f.readAll();};auto u32=[](const QByteArray &b,int at){return qFromLittleEndian<quint32>(b.constData()+at);};
        ModelExportOptions o;o.format=ModelFormat::Obj;QString obj=dir.filePath("test.obj");auto result=exportModel(obj,{{"one","",mesh},{"two","",plain}},o);QVERIFY2(result.ok,qPrintable(result.error));auto text=read(obj);QVERIFY(text.contains("f 1/1/1 2/2/2 3/3/3"));QVERIFY(text.contains("f 4//4 5//5 6//6"));QCOMPARE(result.faces,quint64(2));
        o.axes={1,2,0};o.scale=2;QVERIFY(exportModel(obj,{{"one","",mesh}},o).ok);text=read(obj);QVERIFY(text.contains("v 0 0 2"));QVERIFY(text.contains("v 2 0 0"));QVERIFY(text.contains("vn 0 1 0"));
        o={};o.format=ModelFormat::Obj;o.negate[0]=true;QVERIFY(exportModel(obj,{{"one","",mesh}},o).ok);text=read(obj);QVERIFY(text.contains("v -1 0 0"));QVERIFY(text.contains("f 1/1/1 3/3/3 2/2/2"));o.reverseFaces=true;QVERIFY(exportModel(obj,{{"one","",mesh}},o).ok);QVERIFY(read(obj).contains("f 1/1/1 2/2/2 3/3/3"));QCOMPARE(mesh->vertices[1].position[0],1.f);QCOMPARE(mesh->triangles,QVector<quint32>({0,1,2}));
        o={};o.format=ModelFormat::Glb;o.material=true;o.flipV=true;QString glb=dir.filePath("test.glb");QVERIFY(exportModel(glb,{{"one","",mesh},{"two","",plain}},o).ok);auto bytes=read(glb);QCOMPARE(u32(bytes,0),quint32(0x46546c67));QCOMPARE(u32(bytes,4),quint32(2));QCOMPARE(u32(bytes,8),quint32(bytes.size()));quint32 jsonSize=u32(bytes,12);QCOMPARE(jsonSize%4,quint32(0));QCOMPARE(u32(bytes,16),quint32(0x4e4f534a));auto json=QJsonDocument::fromJson(bytes.mid(20,jsonSize)).object();QCOMPARE(json["meshes"].toArray().size(),2);QCOMPARE(json["nodes"].toArray().size(),2);QCOMPARE(json["materials"].toArray().size(),1);auto first=json["meshes"].toArray()[0].toObject()["primitives"].toArray()[0].toObject();auto second=json["meshes"].toArray()[1].toObject()["primitives"].toArray()[0].toObject();QVERIFY(!second["attributes"].toObject().contains("TEXCOORD_0"));auto accessors=json["accessors"].toArray(),views=json["bufferViews"].toArray();int binStart=28+jsonSize;QCOMPARE(u32(bytes,24+jsonSize),quint32(0x004e4942));
        for(auto value:views){auto v=value.toObject();QCOMPARE(v["byteOffset"].toInt()%4,0);QVERIFY(binStart+v["byteOffset"].toInt()+v["byteLength"].toInt()<=bytes.size());}
        int uvId=first["attributes"].toObject()["TEXCOORD_0"].toInt();int uvOffset=views[accessors[uvId].toObject()["bufferView"].toInt()].toObject()["byteOffset"].toInt();QCOMPARE(std::bit_cast<float>(u32(bytes,binStart+uvOffset+4)),1.f);
        int indexId=first["indices"].toInt();auto indexAccessor=accessors[indexId].toObject();QCOMPARE(indexAccessor["count"].toInt(),3);int indexOffset=views[indexAccessor["bufferView"].toInt()].toObject()["byteOffset"].toInt();QCOMPARE(u32(bytes,binStart+indexOffset+8),quint32(2));
        o.format=ModelFormat::Stl;QString stl=dir.filePath("test.stl");QVERIFY(exportModel(stl,{{"one","",mesh},{"two","",plain}},o).ok);bytes=read(stl);QCOMPARE(bytes.size(),qsizetype(184));QCOMPARE(u32(bytes,80),quint32(2));QCOMPARE(std::bit_cast<float>(u32(bytes,92)),1.f);
        QFile existing(obj);QVERIFY(existing.open(QIODevice::WriteOnly));existing.write("KEEP");existing.close();o.format=ModelFormat::Obj;o.axes={0,0,2};QVERIFY(!exportModel(obj,{{"one","",mesh}},o).ok);QCOMPARE(read(obj),QByteArray("KEEP"));o={};o.format=ModelFormat::Obj;auto cancel=std::make_shared<ExportProgress>();cancel->cancel=true;QVERIFY(exportModel(obj,{{"one","",mesh}},o,cancel).cancelled);QCOMPARE(read(obj),QByteArray("KEEP"));o.protectedPaths={obj};QVERIFY(!exportModel(obj,{{"one","",mesh}},o).ok);QCOMPARE(read(obj),QByteArray("KEEP"));
        o={};o.format=ModelFormat::Obj;auto invalid=std::make_shared<MeshData>(*mesh);invalid->triangles[1]=9;QVERIFY(!exportModel(obj,{{"invalid","",invalid}},o).ok);QCOMPARE(read(obj),QByteArray("KEEP"));invalid=std::make_shared<MeshData>(*mesh);for(auto &v:invalid->vertices)for(auto &n:v.normal)n=0;QVERIFY(!exportModel(obj,{{"invalid","",invalid}},o).ok);o.normals=ExportNormals::Recalculate;QVERIFY(exportModel(obj,{{"fixed","",invalid}},o).ok);QVERIFY(read(obj).contains("vn 0 0 1"));
    }
    void exportSphereFixture() {
        QString source=QFINDTESTDATA("../examples/sphere128.bin");auto meta=readPage({source,0,-1,{},0});auto mesh=readMesh({source,0,meta.size,meta.modified,0},{},MeshSpec{});QVERIFY(mesh.error.isEmpty());QTemporaryDir temporary;QString directory=qEnvironmentVariable("WORKBENCH_EXPORT_DIR",temporary.path());QVERIFY(QDir().mkpath(directory));
        for(auto format:{ModelFormat::Obj,ModelFormat::Glb,ModelFormat::Stl}){ModelExportOptions o;o.format=format;QString extension=format==ModelFormat::Obj?"obj":format==ModelFormat::Glb?"glb":"stl";auto result=exportModel(directory+"/sphere128."+extension,{{"Sphere128",source,mesh.mesh}},o);QVERIFY2(result.ok,qPrintable(result.error));QCOMPARE(result.vertices,quint64(128));QCOMPARE(result.faces,quint64(252));}
    }
    void meshCollectionIdentityAndOcclusion() {
        auto front=std::make_shared<MeshData>();for(auto point:{QVector3D(-1,-1,1),QVector3D(1,-1,1),QVector3D(0,1,1)}){MeshVertex v{};for(int j=0;j<3;++j)v.position[j]=point[j];front->vertices.append(v);}front->triangles={0,1,2};auto back=std::make_shared<MeshData>(*front);for(auto &v:back->vertices)v.position[2]=-1;
        MeshScene scene;int a=scene.add(front,"Frente","example.bin"),b=scene.add(back,"Background","example.bin");auto all=scene.assemble();QCOMPARE(all.data->triangles,QVector<quint32>({0,1,2,3,4,5}));QCOMPARE(all.spans[1].id,b);MeshDebug debug(*all.data);QVERIFY(!debug.visible({0,0,3},debug.positions[3]));
        scene.setVisible(a,false);auto visible=scene.assemble();MeshDebug exposed(*visible.data,all.data.get());QCOMPARE(exposed.positions[0],debug.positions[3]);QVERIFY(exposed.visible({0,0,3},exposed.positions[0]));scene.select(b,MeshElement::Vertex,2);scene.remove(a);QCOMPARE(scene.selection().mesh,b);QCOMPARE(scene.selection().index,2);
        int c=scene.add(front);QVERIFY(c>b);scene.markStale();QVERIFY(scene.entry(b)->stale);QVERIFY(!scene.entry(c)->stale);scene.replace(b,front,"new.bin");QVERIFY(!scene.entry(b)->stale);QCOMPARE(scene.entry(b)->id,b);scene.setVisible(b,false);scene.select(b,MeshElement::Vertex,0);QCOMPARE(scene.selection().index,-1);scene.clear();QVERIFY(scene.entries().isEmpty());QCOMPARE(scene.active(),-1);
    }
    void meshLabelOcclusionAndPicking() {
        MeshData mesh;for(float z:{.5f,-.5f})for(auto p:{QVector3D(-1,-1,z),QVector3D(1,-1,z),QVector3D(0,1,z)}){MeshVertex v{};for(int j=0;j<3;++j)v.position[j]=p[j];mesh.vertices.append(v);}mesh.triangles={0,1,2,3,4,5};MeshDebug debug(mesh);
        QVector3D eye(0,0,3);QMatrix4x4 projection;projection.perspective(45,1,.01f,100);QMatrix4x4 correction;correction(1,1)=-1;correction(2,2)=.5f;correction(2,3)=.5f;QMatrix4x4 camera;camera.lookAt(eye,{0,0,0},{0,1,0});auto mvp=correction*projection*camera;
        QVERIFY(debug.visible(eye,debug.positions[0]));QVERIFY(!debug.visible(eye,debug.positions[3]));
        QCOMPARE(debug.hit(eye,{0,0,-1}),0);QCOMPARE(debug.hit({0,0,-3},{0,0,1}),1);
        auto labels=debug.labels(mvp,eye,{500,500},MeshElement::Vertex,0,6);QCOMPARE(labels.size(),3);for(auto l:labels)QVERIFY(l.id<3);
        QCOMPARE(debug.labels(mvp,eye,{500,500},MeshElement::Face,0,2).size(),1);
        QCOMPARE(debug.labels(mvp,eye,{500,500},MeshElement::Vertex,3,3).size(),0);
        QCOMPARE(debug.pick(mvp,eye,{500,500},{250,250},MeshElement::Face),0);QCOMPARE(debug.pick(mvp,eye,{500,500},{250,250},MeshElement::Mesh),0);
        QCOMPARE(debug.pick(mvp,eye,{500,500},labels.first().pixel,MeshElement::Vertex),0);
        QCOMPARE(debug.pick(mvp,eye,{500,500},{1,1},MeshElement::Face),-1);
        auto geometry=labelGeometry(labels,{500,500},MeshElement::Vertex,1.5f,0);QVERIFY(!geometry.isEmpty());QCOMPARE(geometry.size()%6,0);for(auto v:geometry){QVERIFY(std::abs(v.position[0])<=1);QVERIFY(std::abs(v.position[1])<=1);}
        QCOMPARE(indexColor(0,3,{Qt::red,Qt::green,Qt::blue}),QColor(Qt::red));QCOMPARE(indexColor(2,3,{Qt::red,Qt::green,Qt::blue}),QColor(Qt::blue));QCOMPARE(indexColor(1,3,{Qt::red,Qt::green,Qt::blue}),QColor(Qt::green));
        QMatrix4x4 behind;behind.lookAt({0,0,-3},{0,0,-4},{0,1,0});QVERIFY(debug.labels(correction*projection*behind,{0,0,-3},{500,500},MeshElement::Vertex,0,6).isEmpty());
    }
    void tabularInspector() {
        DataInspector inspector;inspector.resize(440,850);inspector.show();
        auto value=[&](QString type){auto *table=inspector.findChild<QTableWidget *>("inspectorValues");for(int i=0;i<table->rowCount();++i)if(table->item(i,0)->text()==type)return table->item(i,1)->text();return QString();};
        inspector.setData(QByteArray::fromHex("0102030405060708"),{0,7,16,false},8,"ASCII",false);
        QCOMPARE(value("UInt16"),QString("513"));QCOMPARE(value("Binary (8 bits)"),QString("00000001"));
        inspector.findChild<QComboBox *>("inspectorEndian")->setCurrentIndex(1);QCOMPARE(value("UInt16"),QString("258"));
        inspector.findChild<QCheckBox *>("inspectorHex")->setChecked(true);QCOMPARE(value("UInt64"),QString("0x0102030405060708"));
        inspector.findChild<QCheckBox *>("inspectorHex")->setChecked(false);
        inspector.setData(QByteArray(8,char(0xff)),{0,0,16,false},8,"ASCII",false);QCOMPARE(value("UInt64"),QString("18446744073709551615"));QCOMPARE(value("Int64"),QString("-1"));
        inspector.setData(QByteArray(8,0),{0,0,16,false},8,"ASCII",false);QCOMPARE(value("Unix Int64"),QString("1970-01-01 00:00:00 UTC"));QCOMPARE(value("FILETIME"),QString("1601-01-01 00:00:00 UTC"));
        inspector.setData(QByteArray(1,'A'),{0,0,16,false},1,"ASCII",false);QVERIFY(value("UInt16").contains("insufficient"));QCOMPARE(value("ASCII"),QString("A"));
        inspector.setData(QByteArray::fromHex("73cc2a0000000000"),{0,7,16,false},8,"ASCII",false);
        if(qEnvironmentVariableIsSet("WORKBENCH_INSPECTOR_SCREENSHOT"))inspector.grab().save(qEnvironmentVariable("WORKBENCH_INSPECTOR_SCREENSHOT"));
    }
    void sphereMeshFixture() {
        QString path=QFINDTESTDATA("../examples/sphere128.bin");QVERIFY(!path.isEmpty());auto meta=readPage({path,0,-1,{},128});QCOMPARE(meta.bytes.left(4),QByteArray("BWS1"));QCOMPARE(meta.size,qint64(5296));ReadRequest source{path,0,meta.size,meta.modified,0};MeshSpec spec;
        auto result=readMesh(source,{},spec);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.mesh->vertices.size(),128);QCOMPARE(result.mesh->triangles.size(),756);QCOMPARE(result.mesh->vertexOffsets.size(),128);QCOMPARE(result.mesh->vertexOffsets[127],qint64(128+127*12));QCOMPARE(result.mesh->faceSources.size(),252);
        QFile original(path);QVERIFY(original.open(QIODevice::ReadOnly));auto raw=original.readAll();for(int face=0;face<252;++face)for(int j=0;j<3;++j){auto source=result.mesh->faceSources[face];QCOMPARE(source.offsets[j],qint64(1664+source.indexSlots[j]*2));quint16 value=quint8(raw[source.offsets[j]])|(quint16(quint8(raw[source.offsets[j]+1]))<<8);QCOMPARE(quint32(value),result.mesh->triangles[face*3+j]);}

        for(auto v:result.mesh->vertices){double length=std::hypot(v.position[0],v.position[1],v.position[2]);QVERIFY(std::abs(length-1)<.0001);QVERIFY(v.uv[0]>=0&&v.uv[0]<=1);QVERIFY(v.uv[1]>=0&&v.uv[1]<=1);QVERIFY(std::abs(std::hypot(v.normal[0],v.normal[1],v.normal[2])-1)<.00001);}
        QMap<QPair<quint32,quint32>,int> edges;for(int i=0;i<756;i+=3){auto &t=result.mesh->triangles;for(int j=0;j<3;++j){auto a=t[i+j],b=t[i+(j+1)%3];edges[{qMin(a,b),qMax(a,b)}]++;}}for(auto count:edges)QCOMPARE(count,2);
        spec.normalOffset=4528;spec.normalStride=6;spec.normalType="SNorm16";auto ints=readMesh(source,{},spec);QVERIFY(ints.error.isEmpty());for(int i=0;i<128;++i)for(int j=0;j<3;++j)QVERIFY(std::abs(result.mesh->vertices[i].normal[j]-ints.mesh->vertices[i].normal[j])<.0001);
        spec.normals=false;QVERIFY(readMesh(source,{},spec).error.isEmpty());spec.vertexCount=100001;QVERIFY(!readMesh(source,{},spec).error.isEmpty());spec=MeshSpec{};spec.vertexOffset=meta.size-1;QVERIFY(!readMesh(source,{},spec).error.isEmpty());spec=MeshSpec{};
        QVERIFY(!readMesh(source,{{1664,QByteArray::fromHex("feff")}},spec).error.isEmpty());auto edit=readMesh(source,{{128,QByteArray::fromHex("10270000")}},spec);QVERIFY(edit.error.isEmpty());QCOMPARE(edit.mesh->vertices[0].position[0],1.f);
        spec.bigEndian=true;QVERIFY(!readMesh(source,{},spec).error.isEmpty());
    }
    void nativeShiftDigitBroadcast() {
#ifdef Q_OS_MACOS
        QTemporaryDir dir;auto path=dir.filePath("native-keys.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(QByteArray::fromHex("abcd"));file.close();
        HexView view;view.resize(1200,500);view.show();view.openFile(path);QTRY_VERIFY(!view.busy());view.selectMatch(0,2);auto *table=view.findChild<QTableWidget *>("bytes");
        QKeyEvent first(QEvent::KeyPress,Qt::Key_ParenRight,Qt::ShiftModifier,0,29,0,")");QApplication::sendEvent(table,&first);QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("0B"));QCOMPARE(table->item(0,2)->text(),QString("0D"));
        QKeyEvent second(QEvent::KeyPress,Qt::Key_Equal,Qt::ShiftModifier,0,29,0,"=");QApplication::sendEvent(table,&second);QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("00"));QCOMPARE(table->item(0,2)->text(),QString("00"));QCOMPARE(view.selection().count(2),qint64(2));view.shutdown();
#endif
    }
    void directEditingTransforms() {
        QTemporaryDir dir;auto path=dir.filePath("direct.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(QByteArray::fromHex("abcd12347856"));file.close();
        auto meta=readPage({path,0,-1,{},0});ReadRequest source{path,0,meta.size,meta.modified,6};DirectEditSpec spec;spec.ranges={{0,2},{4,2}};spec.character="0";spec.broadcast=true;
        auto r=directEdit(source,spec);QVERIFY(r.error.isEmpty());QCOMPARE(r.bytes,QByteArray::fromHex("0b0d0806"));QCOMPARE(r.matches.size(),2);
        spec.patches={{0,r.bytes.left(2)},{4,r.bytes.mid(2)}};spec.digit=1;r=directEdit(source,spec);QCOMPARE(r.bytes,QByteArray(4,0));spec.character="G";QVERIFY(!directEdit(source,spec).error.isEmpty());
        spec={};spec.hex=false;spec.type="UInt16";spec.ranges={{2,2}};spec.character="1";source.offset=2;source.length=2;r=directEdit(source,spec);QVERIFY(r.error.isEmpty());QCOMPARE(decodeNumber(r.bytes,"UInt16",false),QString("13330"));
        spec.type="UInt8";spec.ranges={{2,1}};spec.character="9";source.length=1;r=directEdit(source,spec);QVERIFY(r.error.isEmpty());QCOMPARE(quint8(r.bytes[0]),98);spec.digit=1;spec.drafts=r.directDrafts;spec.character="0";r=directEdit(source,spec);QCOMPARE(quint8(r.bytes[0]),90);
        spec.type="Int8";spec.character="9";spec.digit=0;spec.drafts={{2,"127"}};QVERIFY(!directEdit(source,spec).error.isEmpty());
        spec.type="Text";spec.drafts.clear();spec.encoding="ASCII";spec.character="Q";r=directEdit(source,spec);QCOMPARE(r.bytes,QByteArray("Q"));spec.character=QString::fromUtf8("日");QVERIFY(!directEdit(source,spec).error.isEmpty());
        spec.encoding="UTF-8";QVERIFY(!directEdit(source,spec).error.isEmpty());
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write(QByteArray::fromHex("00004841"));file.close();meta=readPage({path,0,-1,{},0});source={path,0,meta.size,meta.modified,4};spec={};spec.hex=false;spec.type="Float32";spec.ranges={{0,4}};spec.character="2";r=directEdit(source,spec);QVERIFY(r.error.isEmpty());QCOMPARE(decodeNumber(r.bytes,"Float32",false),QString("22.5"));
    }
    void directKeyboardAndBroadcast() {
        QTemporaryDir dir;auto path=dir.filePath("keys.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(QByteArray::fromHex("abcd12345678"));file.close();HexView view;view.resize(1400,600);view.show();view.openFile(path);QTRY_VERIFY(!view.busy());auto *table=view.findChild<QTableWidget *>("bytes");
        view.selectMatch(0,2);QTest::keyClick(table,Qt::Key_0,Qt::ShiftModifier);QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("0B"));QCOMPARE(table->item(0,2)->text(),QString("0D"));QCOMPARE(view.selection().count(6),qint64(2));
        QTest::keyClick(table,Qt::Key_0,Qt::ShiftModifier);QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("00"));QCOMPARE(table->item(0,2)->text(),QString("00"));view.undo();QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,2)->text(),QString("0D"));view.undo();QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("AB"));
        view.selectMatch(0,1);QTest::keyClicks(table,"1234");QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("12"));QCOMPARE(table->item(0,2)->text(),QString("34"));QCOMPARE(view.selection().anchor,qint64(2));
        QTest::keyClicks(table,"g");QCOMPARE(table->item(0,3)->text(),QString("12"));
        auto *type=view.findChild<QComboBox *>("interpretation");type->setCurrentText("UInt8");QTRY_VERIFY(!view.busy());auto rect=table->visualItemRect(table->item(0,17));QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,rect.topLeft()+QPoint(8,10));QTest::keyClicks(table,"09");QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("09"));QCOMPARE(view.selection().anchor,qint64(1));
        type->setCurrentText("Text");QTRY_VERIFY(!view.busy());rect=table->visualItemRect(table->item(0,17));QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,rect.topLeft()+QPoint(8,10));QTest::keyClicks(table,"Z");QTRY_VERIFY(!view.busy());QCOMPARE(table->item(0,1)->text(),QString("5A"));
        view.shutdown();
    }
    void entropyKnownDistributions() {
        QTemporaryDir dir;auto path=dir.filePath("entropy.bin");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));QByteArray bytes(1024,0);for(int i=0;i<1024;++i)bytes.append(char(i%256));bytes.append("tail");file.write(bytes);file.close();
        auto meta=readPage({path,0,-1,{},0});ReadRequest source{path,0,meta.size,meta.modified,0};EntropySpec spec{0,bytes.size(),1024,{}};std::atomic<bool> cancel{false};std::atomic<qint64> progress{0};
        auto r=analyzeEntropy(source,spec,cancel,progress);QVERIFY(r.error.isEmpty());QCOMPARE(r.entropy.size(),3);QCOMPARE(r.entropy[0].entropy,0.0);QCOMPARE(r.entropy[0].zeros,100.0);QCOMPARE(r.entropy[1].entropy,8.0);QCOMPARE(r.entropy[1].printable,100.0*95/256);QCOMPARE(r.entropy[2].length,qint64(4));
        spec.begin=1024;spec.length=1024;spec.patches={{1024,QByteArray(1024,0)}};r=analyzeEntropy(source,spec,cancel,progress);QCOMPARE(r.entropy[0].offset,qint64(1024));QCOMPARE(r.entropy[0].entropy,0.0);
        cancel=true;QVERIFY(analyzeEntropy(source,spec,cancel,progress).cancelled);cancel=false;source.expectedSize++;QVERIFY(!analyzeEntropy(source,spec,cancel,progress).error.isEmpty());source.expectedSize--;
        spec.length=0;QVERIFY(!analyzeEntropy(source,spec,cancel,progress).error.isEmpty());
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write(QByteArray(3*1024*1024,0));file.close();meta=readPage({path,0,-1,{},0});source={path,0,meta.size,meta.modified,0};spec={0,meta.size,256,{}};r=analyzeEntropy(source,spec,cancel,progress);QVERIFY(r.entropy.size()<=8192);QCOMPARE(r.entropy.last().offset+r.entropy.last().length,meta.size);QCOMPARE(r.entropy.last().entropy,0.0);
    }
    void magicLibraryAndChunkBoundaries() {
        QTemporaryDir dir;auto path=dir.filePath("magic.bin");QByteArray bytes(1024*1024+20,'Z');
        bytes.replace(1024*1024-2,4,QByteArray::fromHex("1234abef"));
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(bytes);file.close();
        auto meta=readPage({path,0,-1,{},0});ReadRequest source{path,0,meta.size,meta.modified,0};
        MagicSpec spec;spec.signatures={{"Test","12 34 A? ?F","",2,true}};
        std::atomic<bool> cancel{false};std::atomic<qint64> progress{0};
        auto r=scanMagic(source,spec,cancel,progress);QVERIFY2(r.error.isEmpty(),qPrintable(r.error));QCOMPARE(r.matches.size(),1);QCOMPARE(r.matches[0].first,qint64(1024*1024-2));QCOMPARE(r.matchTypes[0],0);
        spec.anchored=true;spec.regions={{1024*1024-4,6}};r=scanMagic(source,spec,cancel,progress);QCOMPARE(r.matches.size(),1);
        spec.regions[0].length=5;QVERIFY(scanMagic(source,spec,cancel,progress).matches.isEmpty());
        spec.anchored=false;spec.patches={{12,QByteArray::fromHex("1234acff")}};QCOMPARE(scanMagic(source,spec,cancel,progress).matches.size(),2);
        spec.signatures[0].enabled=false;QVERIFY(!scanMagic(source,spec,cancel,progress).error.isEmpty());spec.signatures[0].enabled=true;
        cancel=true;QVERIFY(scanMagic(source,spec,cancel,progress).cancelled);cancel=false;
        spec.signatures[0].pattern="5?";r=scanMagic(source,spec,cancel,progress);QCOMPARE(r.matches.size(),10000);QVERIFY(r.truncated);
        source.expectedSize++;QVERIFY(!scanMagic(source,spec,cancel,progress).error.isEmpty());
        QString error;QList<MagicSignature> decoded;QVERIFY(decodeMagicLibrary(encodeMagicLibrary(spec.signatures).toJson(),decoded,error));QCOMPARE(decoded[0].relativeOffset,qint64(2));
        QVERIFY(!decodeMagicLibrary("{}",decoded,error));QCOMPARE(decoded.size(),1);
        auto bad=decoded[0];bad.pattern="??";QVERIFY(!validateMagic(bad,error));bad.pattern="A";QVERIFY(!validateMagic(bad,error));
    }
    void advancedSearchFilters(){
        auto memory=std::make_shared<MemoryData>();memory->size=6;memory->blocks={QByteArray("aaaaAb")};ReadRequest source{"",0,6,{},0,memory};
        std::atomic<bool> cancel{false};std::atomic<qint64> progress{0};PatternSpec spec;spec.text=true;spec.expression="AA";spec.length=6;spec.caseSensitive=false;spec.overlap=true;
        auto result=searchPattern(source,spec,cancel,progress);QVERIFY(result.error.isEmpty());QCOMPARE(result.matches.size(),4);
        spec.alignment=2;result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),2);QCOMPARE(result.matches[1].first,qint64(2));
        spec.maxMatches=1;result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),1);QVERIFY(result.truncated);
        spec={};spec.expression="61 61";spec.length=6;spec.overlap=false;result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),2);QCOMPARE(result.matches[1].first,qint64(2));
    }
    void compareSnapshotsAcrossBlocksAndLengths(){
        QTemporaryDir dir;auto path=dir.filePath("reference.bin");QByteArray bytes(1024*1024+8,'A'),reference=bytes;reference.replace(1024*1024-2,5,QByteArray(5,'B'));reference.chop(2);
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(reference);file.close();auto meta=readPage({path,0,-1,{},0});ReadRequest right{path,0,meta.size,meta.modified,0};
        auto memory=std::make_shared<MemoryData>();memory->size=bytes.size();for(int i=0;i<bytes.size();i+=MemoryData::BlockSize)memory->blocks.append(bytes.mid(i,MemoryData::BlockSize));ReadRequest left{"",0,bytes.size(),{},0,memory};CompareState state;
        auto result=compareDocuments(left,right,{{4,"C"}},state);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.differentBytes,qint64(8));QCOMPARE(result.ranges.size(),3);QCOMPARE(result.ranges[1].offset,qint64(1024*1024-2));QCOMPARE(result.ranges[1].length,qint64(5));
        auto equal=compareDocuments(left,left,{},state);QVERIFY(equal.ranges.isEmpty());QVERIFY(equal.error.isEmpty());
        state.cancel=true;QVERIFY(compareDocuments(left,right,{},state).cancelled);state.cancel=false;
        auto reverse=compareDocuments(right,left,{},state);QCOMPARE(reverse.differentBytes,qint64(7));
    }
    void patternSearchAcrossBlocksAndTextOffsets() {
        QTemporaryDir dir;const auto path=dir.filePath("search.bin");
        QByteArray bytes(1024*1024+30,'Z');bytes.replace(1024*1024-1,4,QByteArray::fromHex("488b01af"));
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(bytes);file.close();
        auto meta=readPage({path,0,-1,{},0});ReadRequest source{path,0,meta.size,meta.modified,0};
        std::atomic<bool> cancel{false};std::atomic<qint64> progress{0};
        PatternSpec spec;spec.expression="48 8B ?? A?";spec.length=bytes.size();
        auto result=searchPattern(source,spec,cancel,progress);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));
        QCOMPARE(result.matches.size(),1);QCOMPARE(result.matches[0].first,qint64(1024*1024-1));QCOMPARE(result.matches[0].second,qint64(4));
        spec.patches={ {10,QByteArray::fromHex("488b03a1")} };result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),2);
        cancel.store(true);QVERIFY(searchPattern(source,spec,cancel,progress).cancelled);cancel.store(false);
        spec.expression="??";result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),10000);QVERIFY(result.truncated);
        spec.regex=true;spec.expression="\\x48.\\x01[\\xA0-\\xAF]";spec.begin=1024*1024-5;spec.length=20;spec.patches.clear();
        result=searchPattern(source,spec,cancel,progress);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.matches.size(),1);QCOMPARE(result.matches[0].first,qint64(1024*1024-1));
        spec.length=1024*1024+1;spec.begin=0;QVERIFY(!searchPattern(source,spec,cancel,progress).error.isEmpty());
        spec.length=20;spec.expression="(";QVERIFY(!searchPattern(source,spec,cancel,progress).error.isEmpty());
        spec.expression="Z*";spec.length=20;result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),1);
        const auto text=QString::fromUtf8("AA 日本42 😀 BBB").toUtf8();
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write(text);file.close();
        meta=readPage({path,0,-1,{},0});source={path,0,meta.size,meta.modified,0};
        spec={};spec.text=true;spec.encoding="UTF-8";spec.expression=QString::fromUtf8("日本42");spec.length=text.size();
        result=searchPattern(source,spec,cancel,progress);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.matches.size(),1);QCOMPARE(result.matches[0].first,qint64(3));QCOMPARE(result.matches[0].second,qint64(8));
        spec.regex=true;spec.expression=QString::fromUtf8("日本[0-9]+");result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),1);QCOMPARE(result.matches[0].second,qint64(8));
        FormatRule r;r.name="Match";for(auto m:result.matches)r.matchRanges.append({m.first,m.second});
        QVERIFY(r.matches(3));QVERIFY(r.matches(10));QVERIFY(!r.matches(11));FormatRule restored;QVERIFY(decodeRule(encodeRule(r),restored));QCOMPARE(restored.matchRanges[0].length,qint64(8));
        spec.expression="not found";QVERIFY(searchPattern(source,spec,cancel,progress).matches.isEmpty());
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write(QByteArray::fromHex("410042006500"));file.close();
        meta=readPage({path,0,-1,{},0});source={path,0,meta.size,meta.modified,0};spec.length=meta.size;spec.encoding="UTF-16LE";spec.expression="B.";
        result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),1);QCOMPARE(result.matches[0].first,qint64(2));QCOMPARE(result.matches[0].second,qint64(4));
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write(QByteArray::fromHex("4193fa967b42"));file.close();
        meta=readPage({path,0,-1,{},0});source={path,0,meta.size,meta.modified,0};spec.length=meta.size;spec.encoding="CP932";spec.expression=QString::fromUtf8("日本");
        result=searchPattern(source,spec,cancel,progress);QCOMPARE(result.matches.size(),1);QCOMPARE(result.matches[0].first,qint64(1));QCOMPARE(result.matches[0].second,qint64(4));
    }
    void formatRulesAndMultibyteMapping() {
        FormatRule repeated;repeated.name="Vectors";repeated.start=10LL*1024*1024*1024;
        repeated.length=12;repeated.step=16;repeated.repetitions=1000000;
        repeated.style.background=QColor("#f6d365");repeated.style.border=Qt::red;
        QVERIFY(repeated.matches(repeated.start));QVERIFY(repeated.matches(repeated.start+11));
        QVERIFY(!repeated.matches(repeated.start+12));QVERIFY(repeated.matches(repeated.start+16));
        QVERIFY(!repeated.matches(repeated.start+16*1000000));
        FormatRule restored;QVERIFY(decodeRule(encodeRule(repeated),restored));
        QCOMPARE(restored.start,repeated.start);QCOMPARE(restored.style.background,repeated.style.background);
        auto invalid=encodeRule(repeated);invalid["step"]="0";QVERIFY(!decodeRule(invalid,restored));
        invalid=encodeRule(repeated);invalid["repetitions"]="9223372036854775807";QVERIFY(!decodeRule(invalid,restored));
        FormatRule rectangle;rectangle.selectionRule=true;rectangle.selection={1,19,8,true};
        QVERIFY(rectangle.matches(9));QVERIFY(!rectangle.matches(12));
        QVERIFY(decodeRule(encodeRule(rectangle),restored));QVERIFY(restored.matches(17));
        auto utf8=decodeTokens(QString::fromUtf8("A日😀").toUtf8(),100,"UTF-8","Text",false);
        QCOMPARE(utf8.size(),3);QCOMPARE(utf8[1].offset,qint64(101));QCOMPARE(utf8[1].length,qint64(3));QCOMPARE(utf8[2].length,qint64(4));
        auto utf16=decodeTokens(QByteArray::fromHex("41003dd800de"),0,"UTF-16LE","Text",false);
        QCOMPARE(utf16.size(),2);QCOMPARE(utf16[1].length,qint64(4));
        auto japanese=decodeTokens(QByteArray::fromHex("93fa967b"),0,"CP932","Text",false);
        QCOMPARE(japanese.size(),2);QCOMPARE(japanese[0].text,QString::fromUtf8("日"));QCOMPARE(japanese[1].offset,qint64(2));
        auto floats=decodeTokens(QByteArray::fromHex("0000803f00000040"),0,"ASCII","Float32",false);
        QCOMPARE(floats.size(),2);QCOMPARE(floats[0].length,qint64(4));QCOMPARE(floats[1].text,QString("2"));
    }
    void formattingSurvivesNavigationAndDoesNotEditBytes() {
        QTemporaryDir dir;const auto path=dir.filePath("format.bin");
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(QByteArray(8192,'A'));file.close();
        HexView view;view.resize(1100,700);view.show();view.openFile(path);QTRY_VERIFY(!view.busy());
        FormatRule r;r.name="Header";r.start=1;r.length=2;r.step=4;r.repetitions=2000;
        r.style.background=QColor("#123456");r.style.border=Qt::red;r.style.bold=true;
        view.setFormatRules({r});QVERIFY(!view.dirty());
        QCOMPARE(view.formatAt(1).style.background,QColor("#123456"));QVERIFY(!view.formatAt(3).style.background.isValid());
        // A decoded number/character adopts a rule touching any constituent byte.
        QCOMPARE(view.formatAt(0,4).style.background,QColor("#123456"));
        FormatRule override;override.selectionRule=true;override.selection={2,2,16,false};override.style.background=Qt::green;
        view.setFormatRules({r,override});QCOMPARE(view.formatAt(0,4).style.background,QColor(Qt::green));
        override.enabled=false;view.setFormatRules({r,override});QCOMPARE(view.formatAt(0,4).style.background,QColor("#123456"));
        view.jumpTo(4097);QTRY_VERIFY(!view.busy());QCOMPARE(view.formatAt(4097).style.background,QColor("#123456"));
        view.findChild<QSpinBox *>("stride")->setValue(8);QTRY_VERIFY(!view.busy());
        QCOMPARE(view.formatAt(4097).style.background,QColor("#123456"));
        auto *table=view.findChild<QTableWidget *>();
        const auto hex=table->item(0,2)->data(ByteFormatDelegate::TokensRole).toList().first().toMap();
        const auto decoded=table->item(0,9)->data(ByteFormatDelegate::TokensRole).toList()[1].toMap();
        QCOMPARE(hex["offset"],decoded["offset"]);
        QSignalSpy saved(&view,&HexView::saveFinished);QVERIFY(view.saveFile(path));QTRY_COMPARE(saved.count(),1);QTRY_VERIFY(!view.busy());
        QCOMPARE(view.formatRules().size(),2);QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray(8192,'A'));file.close();
        view.openFile(path);QTRY_VERIFY(!view.busy());QVERIFY(view.formatRules().isEmpty());
    }
    void editHistoryAndSafeStreamingSave() {
        QTemporaryDir dir;
        const auto path=dir.filePath("source.bin"), target=dir.filePath("copy.bin");
        QByteArray original(2*1024*1024+7,'A');
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(original),original.size());file.close();
        const auto metadata=readPage({path,0,-1,{},0});
        const ReadRequest source{path,0,metadata.size,metadata.modified,metadata.size};
        EditHistory edits;QString error;
        QVERIFY(edits.push({{1024*1024-1,QByteArray("XYZ")}},error));
        QVERIFY(edits.push({{1024*1024,QByteArray("12")}},error));
        auto buffer=original;applyPatches(buffer,0,edits.patches());
        QCOMPARE(buffer.mid(1024*1024-1,3),QByteArray("X12"));
        edits.undo();buffer=original;applyPatches(buffer,0,edits.patches());
        QCOMPARE(buffer.mid(1024*1024-1,3),QByteArray("XYZ"));
        edits.redo();
        std::atomic<qint64> progress{0};
        auto saved=savePatchedFile(source,target,edits.patches(),&progress);
        QVERIFY2(saved.error.isEmpty(),qPrintable(saved.error));
        QCOMPARE(progress.load(),qint64(original.size()));
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),original);file.close();
        QFile copy(target);QVERIFY(copy.open(QIODevice::ReadOnly));auto expected=original;
        expected.replace(1024*1024-1,3,"X12");QCOMPARE(copy.readAll(),expected);copy.close();
        saved=savePatchedFile(source,path,edits.patches());
        QVERIFY2(saved.error.isEmpty(),qPrintable(saved.error));
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),expected);file.close();
        // Failure must leave an existing destination intact.
        auto stale=source;stale.expectedSize++;
        QVERIFY(!savePatchedFile(stale,target,edits.patches()).error.isEmpty());
        QVERIFY(copy.open(QIODevice::ReadOnly));QCOMPARE(copy.readAll(),expected);copy.close();
        const auto fresh=readPage({path,0,-1,{},0});
        ReadRequest current{path,0,fresh.size,fresh.modified,fresh.size};
        QVERIFY(!savePatchedFile(current,dir.filePath("missing/copy.bin"),{}).error.isEmpty());
        QVERIFY(!savePatchedFile(current,target,{{fresh.size,QByteArray("!")}}).error.isEmpty());
        QVERIFY(copy.open(QIODevice::ReadOnly));QCOMPARE(copy.readAll(),expected);
        // Replacement addressing remains 64-bit without allocating the file in RAM.
        QByteArray high("abcd");applyPatches(high,10LL*1024*1024*1024,{{10LL*1024*1024*1024+2,"Z"}});
        QCOMPARE(high,QByteArray("abZd"));
        edits.undo();QVERIFY(edits.push({{0,"B"}},error));QVERIFY(!edits.canRedo());
        QVERIFY(!edits.push({{0,QByteArray(EditHistory::MaxBytes+1,'C')}},error));
    }
    void editingUpdatesViewInspectorCopyAndSave() {
        QTemporaryDir dir;const auto path=dir.filePath("edit.bin");
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));QCOMPARE(file.write(QByteArray(8192,'A')),8192);file.close();
        HexView view;view.resize(1100,700);view.show();view.openFile(path);
        QTRY_VERIFY(!view.busy());
        auto *table=view.findChild<QTableWidget *>();QString error;
        QVERIFY(view.replaceSelection(QByteArray::fromHex("4243"),error));
        QVERIFY(view.dirty());QCOMPARE(table->item(0,1)->text(),QString("42"));
        QTRY_VERIFY(!view.busy());
        QVERIFY(view.findChild<DataInspector *>("inspector")->toPlainText().contains("UInt8: 66"));
        view.copySelection();QTRY_VERIFY(!view.busy());
        QCOMPARE(QApplication::clipboard()->text(),QString("42"));
        view.undo();QCOMPARE(table->item(0,1)->text(),QString("41"));QVERIFY(!view.dirty());
        view.redo();QCOMPARE(table->item(0,1)->text(),QString("42"));
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.read(2),QByteArray("AA"));file.close();
        QSignalSpy saved(&view,&HexView::saveFinished);
        QVERIFY(view.saveFile(path));QVERIFY(view.saving());
        QVERIFY(!view.replaceSelection("D",error));
        QTRY_COMPARE(saved.count(),1);QVERIFY(saved.first().first().toBool());
        QTRY_VERIFY(!view.busy());QVERIFY(!view.dirty());QVERIFY(!view.canRedo());
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.read(2),QByteArray("BC"));file.close();
        QCOMPARE(table->item(0,1)->text(),QString("42"));
        // Rectangular replacement touches only selected columns, and undo restores them.
        auto center=[table](int row,int col){return table->visualItemRect(table->item(row,col)).center();};
        QTest::mousePress(table->viewport(),Qt::LeftButton,Qt::AltModifier,center(0,2));
        QTest::mouseMove(table->viewport(),center(1,3));
        QTest::mouseRelease(table->viewport(),Qt::LeftButton,Qt::AltModifier,center(1,3));
        QCOMPARE(view.selection().count(8192),qint64(4));
        QVERIFY(view.replaceSelection("WXYZ",error));
        QCOMPARE(table->item(0,2)->text(),QString("57"));
        QCOMPARE(table->item(1,2)->text(),QString("59"));
        QCOMPARE(table->item(0,4)->text(),QString("41"));
        QVERIFY(!view.replaceSelection("X",error));
        view.undo();QCOMPARE(table->item(0,2)->text(),QString("43"));
        view.redo();
        QVERIFY(view.saveFile(dir.filePath("missing/failure.bin")));
        QTRY_COMPARE(saved.count(),2);QVERIFY(!saved.last().first().toBool());
        QVERIFY(view.dirty());
        QTRY_VERIFY(!view.busy());
    }
    void activityTracksSupersededWork() {
        QSemaphore gate;
        BinaryReader reader(nullptr, [&](const ReadRequest &request) {
            gate.acquire();
            ReadResult result; result.request = request;
            if (request.offset == 3) result.error = "Test error";
            return result;
        });
        reader.request({"test", 1});
        reader.request({"test", 2});
        reader.request({"test", 3});
        // Release before assertions so a failed assertion cannot block destruction.
        gate.release(2);
        auto activity = reader.activity();
        QVERIFY(activity.running); QVERIFY(activity.pending);
        QCOMPARE(activity.started, quint64(1));
        QCOMPARE(activity.replaced, quint64(1));
        QTRY_VERIFY(!reader.busy());
        activity = reader.activity();
        QVERIFY(!activity.pending);
        QCOMPARE(activity.started, quint64(2));
        QCOMPARE(activity.completed, quint64(2));
        QCOMPARE(activity.discarded, quint64(1));
        QCOMPARE(activity.errors, quint64(1));
        QCOMPARE(activity.elapsedMs, qint64(0));
        reader.shutdown();
        QVERIFY(reader.activity().stopping);
    }
    void largeSparseFile() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const auto path = dir.filePath("large.bin");
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadWrite));
        constexpr qint64 size = 10LL * 1024 * 1024 * 1024;
        QVERIFY(resizeSparseFile(file,size));
        QVERIFY(file.seek(size - 3));
        QCOMPARE(file.write("XYZ", 3), 3);
        file.close();
        auto first = readPage({path});
        QVERIFY2(first.error.isEmpty(), qPrintable(first.error));
        QCOMPARE(first.size, size);
        QCOMPARE(first.bytes.size(), PageBytes);
        auto tail = readPage({path, size - 3, size, first.modified});
        QVERIFY(tail.error.isEmpty());
        QCOMPARE(tail.bytes, QByteArray("XYZ"));
        HexView view;view.resize(1100,700);view.show();
        view.openFile(path);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(), 3000);
        auto *table = view.findChild<QTableWidget *>();
        QVERIFY(table->rowCount()>0); QVERIFY(table->rowCount()<100);
        auto *address = view.findChild<QLineEdit *>("offsetHeaderEditor");
        address->setText("0x" + QString::number(size - 3, 16));
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(), 3000);
        const int last=table->rowCount()-1;
        QCOMPARE(table->item(last, 14)->text(), QString("58"));
        QCOMPARE(table->item(last, 15)->text(), QString("59"));
        QCOMPARE(table->item(last, 16)->text(), QString("5A"));
        QCOMPARE(view.selection().anchor, size-3);
        QVERIFY(view.findChild<DataInspector *>("inspector")->toPlainText().contains("XYZ"));
        QVERIFY(!readPage({path, size + 1}).error.isEmpty());
        QVERIFY(!readPage({path, -1}).error.isEmpty());
        QVERIFY(readPage({path, size}).bytes.isEmpty());
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("short", 5), 5);
        file.close();
        QVERIFY(!readPage({path, 0, size, first.modified}).error.isEmpty());
    }
    void emptyAndMissing() {
        QTemporaryDir dir;
        const auto path = dir.filePath("empty.bin");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly)); file.close();
        const auto result = readPage({path});
        QVERIFY(result.error.isEmpty()); QCOMPARE(result.size, 0); QVERIFY(result.bytes.isEmpty());
        QVERIFY(!readPage({dir.filePath("missing")}).error.isEmpty());
        QVERIFY(!readPage({dir.path()}).error.isEmpty());
    }
    void latestRequestWinsWithoutBlocking() {
        std::atomic<int> jobs{0};
        BinaryReader reader(nullptr, [&](const ReadRequest &request) {
            ++jobs;
            QThread::msleep(80);
            ReadResult result; result.request = request; return result;
        });
        QStringList delivered;
        connect(&reader, &BinaryReader::ready, this, [&](const ReadResult &r) { delivered << r.request.path; });
        int ticks = 0;
        QTimer timer; timer.setInterval(5);
        connect(&timer, &QTimer::timeout, this, [&] { ++ticks; }); timer.start();
        reader.request({"old"});
        for (int i = 0; i < 1000; ++i) reader.request({QString::number(i)});
        QTRY_VERIFY_WITH_TIMEOUT(!reader.busy(), 3000);
        QCOMPARE(jobs.load(), 2);
        QCOMPARE(delivered, QStringList{"999"});
        QVERIFY(ticks > 2);
    }
    void shutdownDiscardsPendingAndResults() {
        BinaryReader reader(nullptr, [](const ReadRequest &request) {
            QThread::msleep(60);
            ReadResult r; r.request = request; return r;
        });
        int results = 0;
        connect(&reader, &BinaryReader::ready, this, [&](const ReadResult &) { ++results; });
        reader.request({"active"}); reader.request({"pending"});
        reader.shutdown();
        QTRY_VERIFY_WITH_TIMEOUT(!reader.busy(), 3000);
        QCOMPARE(results, 0);
    }
    void chunkCacheBoundAndReuse() {
        QTemporaryDir dir;const auto path=dir.filePath("cache.bin");QFile file(path);
        QVERIFY(file.open(QIODevice::ReadWrite));QVERIFY(file.resize(25*ChunkCache::ChunkBytes));
        QVERIFY(file.seek(ChunkCache::ChunkBytes-2));QCOMPARE(file.write("ABCD",4),4);file.close();
        int reads=0;
        ChunkCache cache([&](const ReadRequest &r){if(r.length>0)++reads;return readPage(r);});
        cache.reset(path);
        auto crossing=cache.read({path,ChunkCache::ChunkBytes-2,-1,{},4});
        QVERIFY(crossing.error.isEmpty());QCOMPARE(crossing.bytes,QByteArray("ABCD"));QCOMPARE(reads,2);
        QCOMPARE(cache.read({path,ChunkCache::ChunkBytes-2,-1,{},4}).bytes,QByteArray("ABCD"));QCOMPARE(reads,2);
        for(int i=2;i<20;++i){QVERIFY(cache.read({path,i*ChunkCache::ChunkBytes,-1,{},8}).error.isEmpty());}
        QCOMPARE(cache.chunkCount(),20);QCOMPARE(cache.retainedBytes(),20*ChunkCache::ChunkBytes);
        QVERIFY(cache.read({path,0,-1,{},8}).error.isEmpty());
        QVERIFY(cache.read({path,20*ChunkCache::ChunkBytes,-1,{},8}).error.isEmpty());
        QVERIFY(cache.contains(0));QVERIFY(!cache.contains(ChunkCache::ChunkBytes));
        QCOMPARE(cache.chunkCount(),20);QCOMPARE(cache.retainedBytes(),20*ChunkCache::ChunkBytes);
        QVERIFY(file.open(QIODevice::WriteOnly));QCOMPARE(file.write("NEW",3),3);file.close();
        QVERIFY(!cache.read({path,0,crossing.size,crossing.modified,3}).error.isEmpty());
        QCOMPARE(cache.chunkCount(),0);
        QCOMPARE(cache.read({path,0,-1,{},3}).bytes,QByteArray("NEW"));
    }
    void staleChunksNotRetainedAfterReset() {
        QSemaphore started,release;
        ChunkCache cache([&](const ReadRequest &request){
            ReadResult r;r.request=request;r.size=ChunkCache::ChunkBytes;
            if(request.length){started.release();release.acquire();r.bytes=QByteArray(ChunkCache::ChunkBytes,'x');}
            return r;
        });
        cache.reset("old");
        auto future=QtConcurrent::run([&]{return cache.read({"old",0,-1,{},1});});
        const bool waiting=started.tryAcquire(1,3000);
        cache.reset("new");release.release();future.waitForFinished();
        QVERIFY(waiting);QCOMPARE(cache.chunkCount(),0);QCOMPARE(future.result().bytes,QByteArray("x"));
    }
    void continuousScrollAndHugeOffsets() {
        // This checks 64 GiB sparse-file navigation correctness, not a latency target.
        constexpr int idleTimeout=10000;
        QTemporaryDir dir;const auto path=dir.filePath("scroll.bin");QFile file(path);
        QVERIFY(file.open(QIODevice::ReadWrite));
        constexpr qint64 size=64LL*1024*1024*1024;
        QVERIFY(resizeSparseFile(file,size));QVERIFY(file.seek(size-1));QCOMPARE(file.write("Z",1),1);file.close();
        HexView view;view.resize(1100,700);view.show();view.openFile(path);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),idleTimeout);
        auto *scroll=view.findChild<QScrollBar *>("fileScroll");auto *table=view.findChild<QTableWidget *>();
        QCOMPARE(scroll->maximum(),INT_MAX);
        scroll->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),idleTimeout);QCOMPARE(view.firstVisibleOffset(),16);
        QWheelEvent wheel(QPointF(80,80),QPointF(80,80),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(table->viewport(),&wheel);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),idleTimeout);QCOMPARE(view.firstVisibleOffset(),64);
        scroll->setValue(scroll->maximum());
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),idleTimeout);
        QCOMPARE(view.firstVisibleOffset()+qint64(table->rowCount())*16,size);
        QCOMPARE(table->item(table->rowCount()-1,16)->text(),QString("5A"));
        const auto end=view.firstVisibleOffset();
        scroll->triggerAction(QAbstractSlider::SliderSingleStepSub);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),idleTimeout);QCOMPARE(view.firstVisibleOffset(),end-16);
        for(int i=0;i<1000;++i)scroll->setValue(i*1000000);
        scroll->setValue(0);QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),idleTimeout);
        QCOMPARE(view.firstVisibleOffset(),0);QVERIFY(view.cachedBytes()<=20*ChunkCache::ChunkBytes);
        QVERIFY(table->rowCount()<100);
        auto *stride=view.findChild<QSpinBox *>("stride");stride->setValue(3);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),idleTimeout);
        scroll->setValue(scroll->maximum());QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),idleTimeout);
        const auto row=table->rowCount()-1;
        const int tail=int((size-1)%3)+1;QCOMPARE(table->item(row,tail)->text(),QString("5A"));
        view.shutdown();QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),idleTimeout);
    }
    void interpretationsAndCalculator() {
        QCOMPARE(decodeText(QByteArray::fromHex("e697a5e69cac"),"UTF-8"),QString::fromUtf8("日本"));
        QCOMPARE(decodeText(QByteArray::fromHex("e5652c67"),"UTF-16LE"),QString::fromUtf8("日本"));
        QCOMPARE(decodeText(QByteArray::fromHex("93fa967b"),"CP932"),QString::fromUtf8("日本"));
        QCOMPARE(decodeNumber(QByteArray::fromHex("0000803f"),"Float32",false),QString("1"));
        QCOMPARE(decodeNumber(QByteArray::fromHex("3ff0000000000000"),"Float64",true),QString("1"));
        QCOMPARE(decodeNumber(QByteArray::fromHex("ffff"),"Int16",false),QString("-1"));
        QCOMPARE(decodeNumber(QByteArray::fromHex("0102"),"UInt16",false),QString("513"));
        QCOMPARE(decodeNumber(QByteArray::fromHex("0102"),"UInt16",true),QString("258"));
        QVERIFY(decodeNumber(QByteArray(3,'\0'),"Float32",false).startsWith("Missing"));
        QVERIFY(calculateInteger("0x100","+","16").contains("272"));
        QCOMPARE(calculateInteger("1","÷","0"),QString("Division by zero"));
        QCOMPARE(calculateInteger("18446744073709551615","+","1"),QString("Overflow"));
    }
    void selectionGeometry() {
        ByteSelection sequential{14,18,16,false};
        QCOMPARE(sequential.count(100),5);QVERIFY(sequential.contains(16));
        ByteSelection rect{14,34,16,true};
        QCOMPARE(rect.count(100),39);QVERIFY(!rect.contains(16));QVERIFY(rect.contains(18));
        auto ranges=rect.ranges(100,100);
        QCOMPARE(ranges.size(),3);QCOMPARE(ranges[0].offset,2);QCOMPARE(ranges[0].length,13);
        ByteSelection tail{14,34,16,true};QCOMPARE(tail.count(35),27);
        QVERIFY(tail.ranges(35,10).isEmpty());
        ByteSelection reversed{18,14,16,false};QCOMPARE(reversed.count(100),5);
    }
    void strideAndMouseSelection() {
        QTemporaryDir dir;const auto path=dir.filePath("selection.bin");QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));QByteArray data;for(int i=0;i<5000;++i)data.append(char(i%256));QCOMPARE(file.write(data),data.size());file.close();
        HexView view; view.resize(1100,700);view.show();view.openFile(path);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        auto *table=view.findChild<QTableWidget *>();auto *stride=view.findChild<QSpinBox *>("stride");
        stride->setValue(8);QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        QCOMPARE(table->columnCount(),10);QVERIFY(table->rowCount()>3);QVERIFY(table->rowCount()<100);
        auto center=[&](int row,int col){return table->visualItemRect(table->item(row,col)).center();};
        QTest::mousePress(table->viewport(),Qt::LeftButton,Qt::NoModifier,center(0,7));
        QTest::mouseMove(table->viewport(),center(1,3));
        QTest::mouseRelease(table->viewport(),Qt::LeftButton,Qt::NoModifier,center(1,3));
        QCOMPARE(view.selection().anchor,6);QCOMPARE(view.selection().cursor,10);QCOMPARE(view.selection().count(5000),5);
        QTest::mousePress(table->viewport(),Qt::LeftButton,Qt::AltModifier,center(0,2));
        QTest::mouseMove(table->viewport(),center(2,4));
        QTest::mouseRelease(table->viewport(),Qt::LeftButton,Qt::AltModifier,center(2,4));
        QVERIFY(view.selection().rectangular);QCOMPARE(view.selection().count(5000),9);
        view.copySelection();QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        QCOMPARE(QApplication::clipboard()->text(),QString("01 02 03\n09 0A 0B\n11 12 13"));
        auto *address=view.findChild<QLineEdit *>("offsetHeaderEditor");address->setText("0d4094");QTest::keyClick(address,Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        QCOMPARE(view.selection().anchor,4094);
        auto *type=view.findChild<QComboBox *>("interpretation");type->setCurrentText("UInt32");
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        QVERIFY(view.findChild<DataInspector *>("inspector")->toPlainText().contains("16842750"));
        stride->setValue(3);QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
        QCOMPARE(view.selection().anchor,4094);QCOMPARE(table->columnCount(),5);
        view.shutdown();QTRY_VERIFY_WITH_TIMEOUT(!view.busy(),3000);
    }
};
QTEST_MAIN(ReaderTests)
#include "reader_tests.moc"
