#pragma once
// Include in SPF_AdjustmentHotkeys/MirrorControls.cpp inside its anonymous namespace.
// A short lease prevents movement surviving tracking loss or plugin unloading.
struct ControllerMirrorLease {
 std::mutex mutex;int slot=-1;unsigned char mask=0;uint64_t tick=0;
 void Put(int value,float x,float y,uint64_t now){std::lock_guard lock(mutex);slot=value;mask=static_cast<unsigned char>((x<-.2f?1:0)|(x>.2f?2:0)|(y>.2f?4:0)|(y<-.2f?8:0));tick=now;}
 bool Get(uint64_t now,int& value,unsigned char& directions){std::lock_guard lock(mutex);if(slot<0 || !tick || now-tick>150)return false;value=slot;directions=mask;return true;}
} controllerMirrorLease;
