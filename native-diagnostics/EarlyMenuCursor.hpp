#pragma once
#include <mutex>
#include <cmath>
#include <cstdint>
struct EarlyMenuCursor {
 std::mutex mutex;float u=0,v=0;uint64_t tick=0;
 bool Publish(float x,float y,uint64_t now){if(!std::isfinite(x)||!std::isfinite(y)||x<0||x>1||y<0||y>1)return false;std::lock_guard lock(mutex);u=x;v=y;tick=now;return true;}
 bool Take(float& x,float& y,uint64_t now){std::unique_lock lock(mutex,std::try_to_lock);if(!lock.owns_lock()||!tick)return false;bool fresh=now>=tick && now-tick<=100;tick=0;if(!fresh)return false;x=u;y=v;return true;}
};
