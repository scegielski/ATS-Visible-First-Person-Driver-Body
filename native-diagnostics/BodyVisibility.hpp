#pragma once
#include <array>
#include <cmath>
using DriverPalette=std::array<std::array<float,16>,61>;
// The game's matrices are row-major affine matrices: translation is 3,7,11.
// The assembly routine also uses these entries as each bone's origin.
inline bool CollapseDriverPalette(DriverPalette& palette) noexcept {
 for(const auto& matrix:palette)for(float v:matrix)if(!std::isfinite(v))return false;
 const auto root=palette[0];
 for(auto& matrix:palette){matrix={};matrix[3]=root[3];matrix[7]=root[7];matrix[11]=root[11];matrix[15]=1;}
 return true;
}
inline bool HideInteriorBody(bool cameraValid,int cameraType) noexcept {
 return cameraValid && cameraType>=0 && cameraType<=9 && cameraType!=2;
}
inline bool CollapseDriverHead(DriverPalette& palette,float backshiftMeters=0.f) noexcept {
 if(!std::isfinite(backshiftMeters))return false;
 for(const auto& matrix:palette)for(float v:matrix)if(!std::isfinite(v))return false;
 // These are world bone transforms at this hook, before inverse-bind multiplication.
 // Use the neck origin directly; transforming its bind position would apply it twice.
 const auto neck=palette[46];const float z=neck[11]+backshiftMeters;if(!std::isfinite(z))return false;
 for(unsigned bone:{46u,47u}){palette[bone]={};palette[bone][3]=neck[3];palette[bone][7]=neck[7];palette[bone][11]=z;palette[bone][15]=1;}
 return true;
}
inline bool OffsetDriverHead(DriverPalette& palette,float backshiftMeters) noexcept {
 if(!std::isfinite(backshiftMeters))return false;
 for(const auto& matrix:palette)for(float x:matrix)if(!std::isfinite(x))return false;
 auto candidate=palette;
 for(unsigned bone:{46u,47u}){candidate[bone][11]+=backshiftMeters;if(!std::isfinite(candidate[bone][11]))return false;}
 palette=candidate;return true;
}
