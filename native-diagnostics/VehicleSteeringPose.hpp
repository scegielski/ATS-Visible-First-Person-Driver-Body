#pragma once
#include "VehicleBoxProfiles.hpp"
#include "RuntimeArmIK.hpp"
#include <set>

struct VehicleSteeringPose {
 DriverIK::V offset[2]{};
 bool headVisible=true;
 float headBackshift=0;
};
inline std::filesystem::path VehiclePosePath(const std::filesystem::path& config,const VehicleBoxIdentity& id){
 if(!id.Valid())return {};auto name=id.Filename();name.replace(name.size()-10,10,"_poses.ini");return config/"vehicles"/name;
}
inline bool ParseVehicleSteeringPose(const std::string& text,const VehicleBoxIdentity& id,VehicleSteeringPose& output){
 DriverSettingSections sections;if(!id.Valid()||!ParseDriverSettings(text,sections)||!id.MatchesMetadata(sections["vehicle"]))return false;
 auto found=sections.find("steering_grips");if(found==sections.end())return false;
 VehicleSteeringPose candidate;std::set<std::string> seen;
 std::istringstream input(found->second);std::string line;
 while(std::getline(input,line)){
  auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos||line[first]=='#'||line[first]==';')continue;
  std::istringstream row(line);std::string key,extra;row>>key;if(!seen.insert(key).second)return false;
  unsigned side=key=="left_offset"?0:key=="right_offset"?1:2;if(side>1)return false;
  auto& offset=candidate.offset[side];if(!(row>>offset[0]>>offset[1]>>offset[2])||(row>>extra))return false;
  for(float value:offset)if(!std::isfinite(value)||std::abs(value)>.5f)return false;
  if(DriverIK::Length(offset)>.5f)return false;
 }
 if(seen.size()!=2)return false;
 found=sections.find("head");if(found!=sections.end()){
  seen.clear();std::istringstream head(found->second);
  while(std::getline(head,line)){
   auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos||line[first]=='#'||line[first]==';')continue;
   std::istringstream row(line);std::string key,extra;row>>key;if(!seen.insert(key).second)return false;
   if(key=="visible"){int value=-1;if(!(row>>value)||(value!=0&&value!=1))return false;candidate.headVisible=value==1;}
   else if(key=="backshift_m"){if(!(row>>candidate.headBackshift)||!std::isfinite(candidate.headBackshift)||std::abs(candidate.headBackshift)>.5f)return false;}
   else return false;if(row>>extra)return false;
  }
  if(seen.size()!=2)return false;
 }
 output=candidate;return true;
}
inline std::string DefaultVehicleSteeringPose(const VehicleBoxIdentity& id){
 return SerializeDriverSettings({{"vehicle",id.Metadata()},{"steering_grips","# Metres in vehicle axes: +X right, +Y up, +Z backward.\nleft_offset 0 0 0\nright_offset 0 0 0\n"},{"head","visible 1\nbackshift_m 0\n"}});
}
struct LiveVehicleSteeringPose {
 VehicleBoxIdentity identity;VehicleSteeringPose settings;std::string prior;
 bool Poll(const std::filesystem::path& config,const VehicleBoxIdentity& id){
  if(id!=identity){identity=id;settings={};prior.clear();}
  auto path=VehiclePosePath(config,id);if(path.empty())return false;
  std::error_code error;std::filesystem::create_directories(path.parent_path(),error);if(error)return false;
  if(!std::filesystem::exists(path,error)){
   auto text=DefaultVehicleSteeringPose(id);auto file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
   if(file!=INVALID_HANDLE_VALUE){DWORD written=0;bool okay=WriteFile(file,text.data(),static_cast<DWORD>(text.size()),&written,nullptr)&&written==text.size();CloseHandle(file);if(!okay)return false;}
   else if(GetLastError()!=ERROR_FILE_EXISTS)return false;
  }
  std::string text;if(!ReadDriverSettingsFile(path,text))return false;if(text==prior)return true;
  VehicleSteeringPose parsed;if(!ParseVehicleSteeringPose(text,id,parsed))return false;settings=parsed;prior=text;return true;
 }
};
inline unsigned ApplyVehicleSteeringGrips(DriverIK::Pose& pose,const DriverIK::Positions& positions,const VehicleSteeringPose& settings){
 using namespace DriverIK;unsigned mask=0;
 for(unsigned side=0;side<2;++side){
  if(Length(settings.offset[side])<1e-7f)continue;
  unsigned upper=side?26:5,lower=upper+1,wrist=upper+2;auto before=FK(pose,positions);
  V target=Add(before.p[wrist],settings.offset[side]),ray=Sub(target,before.p[upper]);float distance=Length(ray);
  float minimum=std::abs(Length(positions[lower])-Length(positions[wrist]))+.002f,maximum=Length(positions[lower])+Length(positions[wrist])-.002f;
  if(distance<1e-6f||maximum<=minimum)continue;
  target=Add(before.p[upper],Mul(ray,std::clamp(distance,minimum,maximum)/distance));
  auto candidate=pose;if(Solve(candidate,positions,upper,lower,wrist,target,before.q[wrist],before.p[lower])){pose=candidate;mask|=1u<<side;}
 }
 return mask;
}
