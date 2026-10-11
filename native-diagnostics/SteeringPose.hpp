#pragma once
#include <cmath>
#include <algorithm>
inline bool IsSteeringClip(float duration,uint32_t frames) noexcept {
 return std::isfinite(duration) && std::fabs(duration-1.9166667f)<=0.0001f && frames==47;
}
inline float SteeringFraction(float steering) noexcept {
 if(!std::isfinite(steering))return 0.f;
 steering=std::clamp(steering,-1.f,1.f);
 return steering<=0.f ? -steering : 1.f-steering;
}
