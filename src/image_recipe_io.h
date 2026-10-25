#pragma once
#include "image_recipe.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <cmath>
inline QStringList imageLayouts(){return {"Linear","Morton Z-order","Morton rectangular","Tiled 8x8","PS2 GS pages","PS2 GS 16S","Xbox 360 tiled","PS4 1D thin"};}
inline QJsonObject encodeImageRecipe(const ImageRecipe &r,int count=1,qint64 step=0){
    return {{"version",1},{"offset",QString::number(r.offset)},{"rowStride",QString::number(r.rowStride)},{"paletteOffset",QString::number(r.paletteOffset)},{"width",r.width},{"height",r.height},{"mipLevel",r.mipLevel},{"surface",r.surface},{"paletteBank",r.paletteBank},{"paletteEntries",r.paletteEntries},{"format",r.format},{"layout",r.layout},{"paletteFormat",r.paletteFormat},{"byteSwap",r.byteSwap},{"container",r.container},{"bigEndian",r.bigEndian},{"flipX",r.flipX},{"flipY",r.flipY},{"lowNibbleFirst",r.lowNibbleFirst},{"ps2Clut",r.ps2Clut},{"ps2Alpha",r.ps2Alpha},{"count",count},{"step",QString::number(step)}};
}
inline bool decodeImageRecipe(const QJsonObject &json,ImageRecipe &result,int &count,qint64 &step,QString &error){
    ImageRecipe r;int n=1;qint64 spacing=0;auto fail=[&]{error="Invalid or unsupported image recipe.";return false;};
    if(json.value("version")!=1)return fail();
    auto integer=[&](const char *key,int low,int high,int &out){auto v=json.value(key);if(!v.isDouble()||!std::isfinite(v.toDouble())||v.toDouble()!=std::floor(v.toDouble())||v.toDouble()<low||v.toDouble()>high)return false;out=v.toInt();return true;};
    auto offset=[&](const char *key,qint64 &out){auto v=json.value(key);if(!v.isString())return false;bool ok=false;out=v.toString().toLongLong(&ok,10);return ok&&out>=0;};
    if(!offset("offset",r.offset)||!offset("rowStride",r.rowStride)||!offset("paletteOffset",r.paletteOffset)||!offset("step",spacing)||!integer("width",1,8192,r.width)||!integer("height",1,8192,r.height)||!integer("mipLevel",0,13,r.mipLevel)||!integer("surface",0,2047,r.surface)||!integer("paletteBank",0,4095,r.paletteBank)||!integer("paletteEntries",0,16384,r.paletteEntries)||!integer("count",1,128,n))return fail();
    r.format=json.value("format").toString();r.layout=json.value("layout").toString();r.paletteFormat=json.value("paletteFormat").toString();r.byteSwap=json.value("byteSwap").toString();r.container=json.value("container").toString();
    if(!imageFormats().contains(r.format)||!imageLayouts().contains(r.layout)||!imagePaletteFormats().contains(r.paletteFormat)||!QStringList{"None","Swap 8 in 16","Swap 8 in 32","Swap 16 in 32"}.contains(r.byteSwap)||!imageContainers().contains(r.container))return fail();
    for(auto pair:{qMakePair("bigEndian",&r.bigEndian),qMakePair("flipX",&r.flipX),qMakePair("flipY",&r.flipY),qMakePair("lowNibbleFirst",&r.lowNibbleFirst),qMakePair("ps2Clut",&r.ps2Clut),qMakePair("ps2Alpha",&r.ps2Alpha)}){if(!json.value(pair.first).isBool())return fail();*pair.second=json.value(pair.first).toBool();}
    QString validation;imageSpan(r,validation);if(!validation.isEmpty()){error=validation;return false;}result=r;count=n;step=spacing;return true;
}
struct ImagePreset{QString name;ImageRecipe recipe;};
inline QList<ImagePreset> imagePresets(){
    QList<ImagePreset> presets;auto add=[&](QString name,QString format,QString layout="Linear",bool big=false){ImageRecipe r;r.format=format;r.layout=layout;r.bigEndian=big;presets.append({name,r});};
    add("Standard / container image","Encoded image");add("PC / RGBA8","RGBA8");add("PC / BGRA8","BGRA8");
    add("PS1 / BGR555","PS1 BGR555");add("PS1 / Indexed4","Indexed4");presets.last().recipe.paletteFormat="PS1 BGR555";presets.last().recipe.lowNibbleFirst=true;
    add("PS2 / RGBA32 GS","PS2 RGBA8","PS2 GS pages");presets.last().recipe.ps2Alpha=true;
    add("PS2 / Indexed8 GS CSM1","Indexed8","PS2 GS pages");presets.last().recipe.paletteFormat="PS2 RGBA8";presets.last().recipe.ps2Clut=true;presets.last().recipe.ps2Alpha=true;
    add("PS3 / BC1 Morton","BC1","Morton rectangular");add("PS4 / BC7 1D thin","BC7","PS4 1D thin");
    add("Xbox / BGRA8 Morton","BGRA8","Morton rectangular");add("Xbox 360 / BC1 tiled","BC1","Xbox 360 tiled");presets.last().recipe.byteSwap="Swap 8 in 16";
    add("NES / CHR 2bpp","NES CHR2");presets.last().recipe.paletteFormat="RGB8";
    add("SNES / 4bpp","SNES 4bpp");presets.last().recipe.paletteFormat="RGB555";
    add("GameCube / CMPR","GC CMPR","Linear",true);add("Wii / RGBA8","GC RGBA8","Linear",true);return presets;
}
