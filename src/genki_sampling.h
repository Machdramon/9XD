#pragma once
#include <QJsonObject>
// Convert a contiguous GS region to an extracted standalone image. Region-repeat
// The normalized source extent uses the power-of-two envelope of the stored region.
// Noncontiguous masks cannot use this affine mapping. Region-repeat
// stores a bit mask in MIN and an OR origin in MAX, not a clamp rectangle.
namespace GenkiSampling {
inline QJsonObject region(int mode,int lo,int hi,int pixels){
    if(pixels<1||pixels>1024||lo<0||hi<0||lo>1023||hi>1023)return {};
    if(mode==0)return {{"scale",1.},{"offset",0.},{"wrap","Repeat"}};
    if(mode==1)return {{"scale",1.},{"offset",0.},{"wrap","Clamp"}};
    int origin=lo,extent=hi-lo+1;
    if(mode==3){if(lo!=pixels-1||(pixels&(pixels-1))||(hi&lo))return {};origin=hi;extent=pixels;}
    else if(mode!=2||lo>hi)return {};
    if(mode==2&&hi<pixels)return {{"scale",1.},{"offset",0.},{"wrap","Clamp"}};
    if(extent!=pixels)return {};
    int source=1;while(source<origin+extent)source*=2;
    return {{"scale",double(source)/pixels},{"offset",-double(origin)/pixels},{"wrap",mode==3?"Repeat":"Clamp"}};
}
inline bool apply(QJsonObject &m,int width,int height){
    auto u=region(m["clampModeU"].toInt(-1),m["clampMinU"].toInt(),m["clampMaxU"].toInt(),width);
    auto v=region(m["clampModeV"].toInt(-1),m["clampMinV"].toInt(),m["clampMaxV"].toInt(),height);
    if(u.isEmpty()||v.isEmpty())return false;
    m["uvScaleU"]=u["scale"];m["uvScaleV"]=v["scale"];m["uvOffsetU"]=u["offset"];m["uvOffsetV"]=v["offset"];m["textureWrapU"]=u["wrap"];m["textureWrapV"]=v["wrap"];m["regionSampling"]=true;return true;
}
}
