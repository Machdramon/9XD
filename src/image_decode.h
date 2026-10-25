#pragma once
#include "binary_document.h"
#include <QImage>
#include <QImageReader>
#include <QBuffer>
#include <QtEndian>
#include <atomic>
#include "tga_decode.h"

#include "image_containers.h"
struct DecodedImage {QImage image;QString error;bool cancelled=false;};
// All source I/O uses the same bounded reader as Hex Viewer, including RAM snapshots.
inline DecodedImage decodeBinaryImage(ReadRequest source,const PatchList &patches,const ImageRecipe &recipe,const std::atomic_bool &cancel){
    DecodedImage out;QString error;const auto span=imageSpan(recipe,error);if(!error.isEmpty()){out.error=error;return out;}
    auto read=[&](qint64 offset,qint64 length){
        QByteArray bytes;
        if(offset<0||length<0||offset>source.expectedSize||length>source.expectedSize-offset){out.error="The image or palette extends outside the source.";return bytes;}
        for(qint64 done=0;done<length;){
            if(cancel.load()){out.cancelled=true;return QByteArray();}
            auto request=source;request.offset=offset+done;request.length=qMin(qint64(1024*1024),length-done);auto result=readPage(request);
            if(!result.error.isEmpty()||result.bytes.size()!=request.length){out.error=result.error.isEmpty()?"Incomplete image data.":result.error;return QByteArray();}
            applyPatches(result.bytes,request.offset,patches);bytes+=result.bytes;done+=request.length;
        }return bytes;
    };
    if(recipe.format=="Encoded image"){
        const auto remaining=source.expectedSize-recipe.offset;
        if(remaining<=0||remaining>32*1024*1024){out.error="Encoded image input must be between 1 byte and 32 MiB.";return out;}
        auto bytes=read(recipe.offset,remaining);if(!out.error.isEmpty()||out.cancelled)return out;
        if(bytes.startsWith("GNF "))out.image=decodeGnf(bytes,recipe,out.error,cancel);
        else if(bytes.startsWith("TIM2"))out.image=decodeTim2(bytes,recipe,out.error,cancel);
        else if(recipe.container=="Genki image bank"){
            auto bank=GenkiTextureBank::headerless(bytes,0,bytes.size(),&cancel);auto adapted=GenkiTextureBank::pictureTim2(bytes,bank,recipe.surface,out.error);auto selected=recipe;selected.surface=0;if(!adapted.isEmpty())out.image=decodeTim2(adapted,selected,out.error,cancel);
        }
        else if(recipe.container=="GTF"||source.path.endsWith(".gtf",Qt::CaseInsensitive))out.image=decodeGtf(bytes,recipe,out.error,cancel);
        else if(bytes.startsWith("DDS "))out.image=decodeDds(bytes,recipe,out.error,cancel);
        else if(bytes.startsWith(QByteArray::fromHex("10000000")))out.image=decodeTim(bytes,recipe,out.error,cancel);
        else if(bytes.startsWith(QByteArray::fromHex("0020af30")))out.image=decodeTpl(bytes,recipe,out.error,cancel);
        else{
            QBuffer buffer(&bytes);buffer.open(QIODevice::ReadOnly);QImageReader reader(&buffer);reader.setDecideFormatFromContent(true);
            if(recipe.surface&&!reader.jumpToImage(recipe.surface)){out.error="Image frame/page is out of range.";return out;}
            if(recipe.mipLevel){out.error="This image does not contain selectable mipmaps.";return out;}
            const auto size=reader.size();
            if(imageValidSize(size.width(),size.height()))out.image=reader.read();
            // Our TGA decoder additionally supports RLE and indexed variants.
            if(out.image.isNull()){out.image=decodeTga(bytes,out.error,cancel);if(recipe.surface){out.image={};out.error="TGA contains a single image.";}}
        }
    }else{
        auto bytes=read(recipe.offset,span);if(!out.error.isEmpty()||out.cancelled)return out;
        QByteArray palette;
        if(imageIndexed(recipe.format)){
            int step=imagePixelBits(recipe.paletteFormat)/8,count=imagePaletteCount(recipe);
            if(!imagePaletteFormats().contains(recipe.paletteFormat)||step<1){out.error="Unsupported palette format.";return out;}
            auto bank=qint64(recipe.paletteBank)*count*step;
            if(recipe.paletteOffset>source.expectedSize||bank>source.expectedSize-recipe.paletteOffset){out.error="Palette bank is outside the source.";return out;}
            palette=read(recipe.paletteOffset+bank,count*step);if(!out.error.isEmpty()||out.cancelled)return out;
        }
        out.image=decodeTextureData(bytes,palette,recipe,out.error,cancel);
    }
    if(cancel.load()){out.image={};out.cancelled=true;return out;}
    if(recipe.flipX||recipe.flipY)out.image=out.image.flipped((recipe.flipX?Qt::Horizontal:Qt::Orientations())|(recipe.flipY?Qt::Vertical:Qt::Orientations()));
    // Revalidate metadata after decoding as well as during each block read.
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);if(!final.error.isEmpty()){out.image={};out.error=final.error;}return out;
}
