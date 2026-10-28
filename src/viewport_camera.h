#pragma once
#include <algorithm>
#include <cmath>
namespace ViewportCamera {
struct ClipRange {float nearPlane,farPlane;};
inline ClipRange clipping(bool orthographic,bool automatic,double distance,double targetLength,double extent,double worldRadius,double nearWorld,double farWorld){
    if(!automatic){double radius=std::max(1e-20,worldRadius);return {float(nearWorld/radius),float(farWorld/radius)};}
    double far=std::max(.01,distance+targetLength+std::max(1.,extent)*1.1);
    double near=orthographic?-far:std::max(1e-9,distance*.001);
    return {float(near),float(far)};
}
// Free-fly can approach a wall while orbit distance stays large. Keep the plane before that wall.
inline float closeNear(float nearPlane,double hitDistance){return hitDistance>=0?float(std::max(1e-9,std::min(double(nearPlane),hitDistance*.05))):nearPlane;}
inline int clipSlider(double units){return int(std::clamp(std::lround((std::log10(std::max(1e-6,units))+6)*100),0l,1800l));}
inline double clipUnits(int position){return std::pow(10.,std::clamp(position,0,1800)/100.-6);}
inline float zoom(float distance,double wheelDelta,bool fine){return float(std::clamp(double(distance)*std::exp(-wheelDelta*.001*(fine?.2:1.)),1e-7,1e5));}
}
