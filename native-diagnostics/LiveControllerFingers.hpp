#pragma once
#include "ControllerFingerTargets.hpp"
#include <sstream>
#include <string>
#include <atomic>
#include <memory>
using ControllerFingerPose=std::array<ControllerFingerTarget,15>;
inline bool ParseControllerFingerPose(const std::string& text,bool right,ControllerFingerPose& output,std::string& error){
 if(text.size()>16384){error="file exceeds 16 KiB";return false;}
 auto candidate=right?kControllerRightFingers:kControllerLeftFingers;std::array<bool,15> seen{};bool header=false,routing=false;std::istringstream lines(text);std::string line;
 while(std::getline(lines,line)){
  if(auto comment=line.find('#');comment!=std::string::npos)line.erase(comment);std::istringstream row(line);std::string tag;if(!(row>>tag))continue;
  if(!header){unsigned version=0;std::string extra;if(tag!="SPF_CONTROLLER_FINGERS" || !(row>>version) || version!=1 || row>>extra){error="expected SPF_CONTROLLER_FINGERS 1";return false;}header=true;continue;}
  if(tag=="trigger_finger"){
   std::string finger,extra;if(routing || !(row>>finger) || row>>extra){error="invalid or duplicate trigger routing";return false;}const std::array<std::string,5> names{"index","middle","ring","little","thumb"};auto it=std::find(names.begin(),names.end(),finger);if(it==names.end()){error="unknown trigger finger";return false;}unsigned start=(right?29u:8u)+static_cast<unsigned>(it-names.begin())*3;
   for(auto& target:candidate)target.triggerControlled=target.bone>=start && target.bone<start+3;routing=true;continue;
  }
  unsigned bone=0;if(tag!="bone" || !(row>>bone)){error="invalid bone row";return false;}size_t i=0;for(;i<candidate.size();++i)if(candidate[i].bone==bone)break;
  if(i==candidate.size() || seen[i]){error="unknown, opposite-hand or duplicate bone";return false;}
  for(auto q:{&candidate[i].open,&candidate[i].closed}){float norm=0;for(float& x:*q){if(!(row>>x) || !std::isfinite(x)){error="invalid quaternion";return false;}norm+=x*x;}if(norm<.81f || norm>1.21f){error="quaternion must be near unit length";return false;}for(float& x:*q)x/=std::sqrt(norm);}
  std::string extra;if(row>>extra){error="unexpected extra values";return false;}seen[i]=true;
 }
 if(!header || std::find(seen.begin(),seen.end(),false)!=seen.end()){error="file must contain all 15 finger bones";return false;}output=candidate;return true;
}
class LiveControllerFingerPose {
 const bool right;std::atomic<std::shared_ptr<const ControllerFingerPose>> current;std::atomic<uint64_t> revision{0},rejected{0};
public:
 explicit LiveControllerFingerPose(bool side):right(side),current(std::make_shared<const ControllerFingerPose>(side?kControllerRightFingers:kControllerLeftFingers)){}
 bool Replace(const std::string& text,std::string& error){ControllerFingerPose candidate;if(!ParseControllerFingerPose(text,right,candidate,error)){rejected.fetch_add(1);return false;}current.store(std::make_shared<const ControllerFingerPose>(candidate));revision.fetch_add(1);return true;}
 auto Snapshot() const{return current.load();}
 uint64_t Revision() const{return revision.load();}
 uint64_t Rejected() const{return rejected.load();}
};
