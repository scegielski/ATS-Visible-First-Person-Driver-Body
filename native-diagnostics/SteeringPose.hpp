#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>
inline bool IsSteeringClip(float duration,uint32_t frames) noexcept {
 return std::isfinite(duration) && std::fabs(duration-1.9166667f)<=0.0001f && frames==47;
}
inline bool IsThrottleClip(float duration,uint32_t frames) noexcept {
 return std::isfinite(duration) && std::fabs(duration-64.5833333f)<=0.01f && frames==1551;
}
inline uint32_t ThrottleStartFrame(float throttle) noexcept {
 if(!std::isfinite(throttle))return 0;
 return static_cast<uint32_t>(std::lround(std::clamp(throttle,0.f,1.f)*32.f))*47;
}
inline float SteeringFraction(float steering) noexcept {
 if(!std::isfinite(steering))return 0.f;
 steering=std::clamp(steering,-1.f,1.f);
 return steering<=0.f ? -steering : 1.f-steering;
}
