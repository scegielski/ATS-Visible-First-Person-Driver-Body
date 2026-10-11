#pragma once
#include "WindowGestureTargets.hpp"
#include "HornPose.hpp"
#include <sstream>
#include <string>
#include <memory>
#include <atomic>
using GesturePoseTargets=std::array<WindowGestureTarget,21>;
inline bool ParseGesturePose(const std::string& text,GesturePoseTargets& output,std::string& error) {
 if(text.size()>65536){error="file exceeds 64 KiB";return false;}
 std::istringstream lines(text);std::string line;bool header=false;std::array<bool,21> seen{};unsigned version=0;
 auto candidate=kWindowGestureTargets;
 while(std::getline(lines,line)) {
  if(auto pos=line.find('#');pos!=std::string::npos)line.erase(pos);
  std::istringstream row(line);std::string tag;if(!(row>>tag))continue;
  if(!header){std::string extra;if(tag!="SPF_GESTURE_POSE" || !(row>>version) || (version!=1 && version!=2) || (row>>extra)){error="expected SPF_GESTURE_POSE version 1 or 2 header";return false;}header=true;continue;}
  if(tag!="bone"){error="unknown row type";return false;}
  unsigned bone=0;if(!(row>>bone)){error="invalid bone index";return false;}
  size_t index=0;for(;index<candidate.size();++index)if(candidate[index].bone==bone)break;
  if(index==candidate.size() || seen[index]){error="unknown or duplicate bone";return false;}
  std::array<std::array<float,4>*,3> quats{{&candidate[index].hover,version==2?&candidate[index].intermediate:&candidate[index].pressed,&candidate[index].pressed}};
  for(unsigned group=0;group<(version==2?3u:2u);++group) {
   auto* q=quats[group];
   float norm=0;for(float& value:*q){if(!(row>>value) || !std::isfinite(value)){error="invalid quaternion";return false;}norm+=value*value;}
   if(norm<.81f || norm>1.21f){error="quaternion must be near unit length";return false;}
   for(float& value:*q)value/=std::sqrt(norm);
  }
  if(version==1)candidate[index].intermediate=candidate[index].pressed;
  std::string extra;if(row>>extra){error="unexpected extra values";return false;}
  seen[index]=true;
 }
 if(!header || std::find(seen.begin(),seen.end(),false)!=seen.end()){error="file must contain all 21 gesture bones";return false;}
 output=candidate;return true;
}
class LiveGesturePose {
 std::atomic<std::shared_ptr<const GesturePoseTargets>> current{std::make_shared<const GesturePoseTargets>(kWindowGestureTargets)};
 std::atomic<uint64_t> generation{0};
public:
 bool Replace(const std::string& text,std::string& error) {
  GesturePoseTargets candidate{};if(!ParseGesturePose(text,candidate,error))return false;
  current.store(std::make_shared<const GesturePoseTargets>(candidate));generation.fetch_add(1);return true;
 }
 auto Snapshot() const {return current.load();}
 uint64_t Generation() const {return generation.load();}
};
