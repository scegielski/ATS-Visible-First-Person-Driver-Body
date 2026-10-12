#pragma once
#include <array>
#include <cmath>
using DriverPalette=std::array<std::array<float,16>,61>;
// A render-only degenerate palette: all weighted vertices meet at one finite point.
// The engine regenerates the full palette next call; animation inputs are untouched.
inline bool CollapseDriverPalette(DriverPalette& palette) noexcept {
 for(const auto& matrix:palette)for(float v:matrix)if(!std::isfinite(v))return false;
 const auto root=palette[0];
 for(auto& matrix:palette){matrix={};matrix[12]=root[12];matrix[13]=root[13];matrix[14]=root[14];matrix[15]=1;}
 return true;
}
inline bool HideInteriorBody(bool cameraValid,int cameraType) noexcept {
 return cameraValid && cameraType>=0 && cameraType<=9 && cameraType!=2;
}

inline bool CollapseDriverHead(DriverPalette& palette) noexcept {
 for(const auto& matrix:palette)for(float v:matrix)if(!std::isfinite(v))return false;
 // Original driver neck bind position from its unchanged skeleton.
 constexpr std::array<float,3> bind{.000026522655f,1.548001528f,-.080883108f};
 const auto neck=palette[46];std::array<float,3> anchor{};
 for(unsigned row=0;row<3;++row)anchor[row]=neck[row]*bind[0]+neck[4+row]*bind[1]+neck[8+row]*bind[2]+neck[12+row];
 for(unsigned bone:{46u,47u}){palette[bone]={};palette[bone][12]=anchor[0];palette[bone][13]=anchor[1];palette[bone][14]=anchor[2];palette[bone][15]=1;}
 return true;
}
