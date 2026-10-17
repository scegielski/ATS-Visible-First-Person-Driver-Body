#pragma once
#include "VehicleBoxProfiles.hpp"
#include "RuntimeArmIK.hpp"
#include <set>
#include "LiveJointPoseEdits.hpp"

struct VehicleSteeringPose {
 JointPoseEditBank jointPoseEdits;
 DriverIK::V offset[4]{};
 DriverIK::Q footRotation[2]{{1,0,0,0},{1,0,0,0}};
 DriverIK::V cameraAngles{};bool cameraAnglesConfigured=false;
 bool cameraConfigured=false;DriverIK::V cameraSeat{};
 DriverIK::V cameraDefaultSeat{};
 bool driverVisible=true;
 bool headVisible=true;
 float headBackshift=0;
 float thumbSpreadDegrees=20.f,thumbCurlStrength=.60f,thumbBaseXDegrees=0.f,thumbBaseYDegrees=0.f,thumbBaseZDegrees=0.f,thumbActivationYDegrees=55.f,restingFingerCurl=-1.f;
};
inline std::filesystem::path VehiclePosePath(const std::filesystem::path& config,const VehicleBoxIdentity& id){
 if(!id.Valid())return {};auto name=id.Filename();name.replace(name.size()-10,10,"_poses.ini");return config/"vehicles"/name;
}
inline bool ParseVehicleSteeringPose(const std::string& text,const VehicleBoxIdentity& id,VehicleSteeringPose& output){
 DriverSettingSections sections;if(!id.Valid()||!ParseDriverSettings(text,sections)||!id.MatchesMetadata(sections["vehicle"]))return false;
 auto found=sections.find("steering_grips");if(found==sections.end())return false;
 VehicleSteeringPose candidate;if(id.brand=="westernstar"&&id.model=="vehicle.westernstar.5700xe"){candidate.cameraDefaultSeat={0,.2f,-.1f};candidate.cameraSeat=candidate.cameraDefaultSeat;candidate.cameraConfigured=true;candidate.thumbBaseXDegrees=30;candidate.thumbBaseYDegrees=-10;candidate.restingFingerCurl=.35f;ParseJointPoseEdits(kWesternStarJointPoseDefaults,candidate.jointPoseEdits);}std::set<std::string> seen;
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
 found=sections.find("feet");if(found!=sections.end()){
  seen.clear();std::istringstream feet(found->second);while(std::getline(feet,line)){auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos||line[first]=='#'||line[first]==';')continue;std::istringstream row(line);std::string key,extra;row>>key;if(!seen.insert(key).second)return false;unsigned side=key=="left_offset"?2:key=="right_offset"?3:4;if(side>3)return false;for(float& v:candidate.offset[side])if(!(row>>v)||!std::isfinite(v))return false;if(row>>extra)return false;}if(seen.size()!=2)return false;
 }
 found=sections.find("foot_rotations");if(found!=sections.end()){seen.clear();std::istringstream rows(found->second);while(std::getline(rows,line)){auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos||line[first]=='#'||line[first]==';')continue;std::istringstream row(line);std::string key,extra;row>>key;unsigned side=key=="left"?0:key=="right"?1:2;if(side>1||!seen.insert(key).second)return false;float norm=0;for(float& v:candidate.footRotation[side]){if(!(row>>v)||!std::isfinite(v))return false;norm+=v*v;}if((row>>extra)||std::abs(norm-1.f)>.01f)return false;for(float& v:candidate.footRotation[side])v/=std::sqrt(norm);}if(seen.size()!=2)return false;}
 found=sections.find("camera_angles");if(found!=sections.end()){std::istringstream row(found->second);std::string key,extra;if(!(row>>key>>candidate.cameraAngles[0]>>candidate.cameraAngles[1])||key!="yaw_pitch"||(row>>extra)||!std::isfinite(candidate.cameraAngles[0])||!std::isfinite(candidate.cameraAngles[1]))return false;candidate.cameraAnglesConfigured=true;}
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
 found=sections.find("camera");if(found!=sections.end()){seen.clear();std::istringstream camera(found->second);while(std::getline(camera,line)){auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos||line[first]=='#'||line[first]==';')continue;std::istringstream row(line);std::string key,extra;row>>key;if(!seen.insert(key).second)return false;if(key=="enabled"){int value;if(!(row>>value)||(value!=0&&value!=1))return false;candidate.cameraConfigured=value!=0;}else if(key=="seat_offset"){for(float& v:candidate.cameraSeat)if(!(row>>v)||!std::isfinite(v))return false;if(DriverIK::Length(candidate.cameraSeat)>2.00001f)return false;}else return false;if(row>>extra)return false;}if(seen.size()!=2)return false;}
 found=sections.find("camera_default");if(found!=sections.end()){std::istringstream row(found->second);std::string key,extra;if(!(row>>key>>candidate.cameraDefaultSeat[0]>>candidate.cameraDefaultSeat[1]>>candidate.cameraDefaultSeat[2])||key!="seat_offset"||(row>>extra))return false;for(float v:candidate.cameraDefaultSeat)if(!std::isfinite(v))return false;if(DriverIK::Length(candidate.cameraDefaultSeat)>2.00001f)return false;}
 found=sections.find("truck_thumbs");if(found!=sections.end()){seen.clear();std::istringstream rows(found->second);while(std::getline(rows,line)){auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos||line[first]=='#')continue;std::istringstream row(line);std::string key,extra;float value;if(!(row>>key>>value)||!std::isfinite(value)||(row>>extra)||!seen.insert(key).second)return false;if(key=="spread_degrees"){if(value<0||value>60)return false;candidate.thumbSpreadDegrees=value;}else if(key=="base_x_degrees"){if(std::abs(value)>180)return false;candidate.thumbBaseXDegrees=value;}else if(key=="base_y_degrees"){if(std::abs(value)>180)return false;candidate.thumbBaseYDegrees=value;}else if(key=="base_z_degrees"){if(std::abs(value)>180)return false;candidate.thumbBaseZDegrees=value;}else if(key=="activation_y_degrees"){if(std::abs(value)>180)return false;candidate.thumbActivationYDegrees=value;}else if(key=="curl_strength"){if(value<0||value>1.5f)return false;candidate.thumbCurlStrength=value;}else return false;}if(!seen.count("spread_degrees")||!seen.count("curl_strength"))return false;}
 found=sections.find("truck_fingers");if(found!=sections.end()){std::istringstream row(found->second);std::string key,extra;if(!(row>>key>>candidate.restingFingerCurl)||key!="resting_curl"||(row>>extra)||!std::isfinite(candidate.restingFingerCurl)||candidate.restingFingerCurl< -1||candidate.restingFingerCurl>1)return false;}
 found=sections.find("joint_pose_edits");if(found!=sections.end()&&!ParseJointPoseEdits(found->second,candidate.jointPoseEdits))return false;
 found=sections.find("body");if(found!=sections.end()){std::istringstream body(found->second);std::string key,extra;int visible;if(!(body>>key>>visible)||key!="visible"||(visible!=0&&visible!=1)||(body>>extra))return false;candidate.driverVisible=visible!=0;}
 output=candidate;return true;
}
inline std::string DefaultVehicleSteeringPose(const VehicleBoxIdentity& id){
 return SerializeDriverSettings({{"vehicle",id.Metadata()},{"steering_grips","# Metres in vehicle axes: +X right, +Y up, +Z backward.\nleft_offset 0 0 0\nright_offset 0 0 0\n"},{"head","visible 1\nbackshift_m 0\n"}});
}
struct LiveVehicleSteeringPose {
 VehicleBoxIdentity identity;VehicleSteeringPose settings;std::string prior;
 bool Save(const std::filesystem::path& config){
  auto path=VehiclePosePath(config,identity);std::string before;DriverSettingSections sections;if(path.empty()||!ReadDriverSettingsFile(path,before)||before!=prior||!ParseDriverSettings(before,sections)||!identity.MatchesMetadata(sections["vehicle"]))return false;
  std::ostringstream grips;grips<<std::setprecision(9);for(unsigned side=0;side<2;++side){grips<<(side?"right_offset":"left_offset");for(float v:settings.offset[side])grips<<' '<<v;grips<<'\n';}sections["steering_grips"]=grips.str();std::ostringstream feet;feet<<std::setprecision(9);for(unsigned side=2;side<4;++side){feet<<(side==2?"left_offset":"right_offset");for(float v:settings.offset[side])feet<<' '<<v;feet<<'\n';}sections["feet"]=feet.str();std::ostringstream rotations;rotations<<std::setprecision(9);for(unsigned side=0;side<2;++side){rotations<<(side?"right":"left");for(float v:settings.footRotation[side])rotations<<' '<<v;rotations<<'\n';}sections["foot_rotations"]=rotations.str();std::ostringstream camera;camera<<std::setprecision(9)<<"enabled "<<(settings.cameraConfigured?1:0)<<"\nseat_offset";for(float v:settings.cameraSeat)camera<<' '<<v;camera<<'\n';sections["camera"]=camera.str();if(settings.cameraAnglesConfigured){std::ostringstream angles;angles<<std::setprecision(9)<<"yaw_pitch "<<settings.cameraAngles[0]<<' '<<settings.cameraAngles[1]<<'\n';sections["camera_angles"]=angles.str();}else sections.erase("camera_angles");std::ostringstream head;head<<std::setprecision(9)<<"visible "<<(settings.headVisible?1:0)<<"\nbackshift_m "<<settings.headBackshift<<'\n';sections["head"]=head.str();sections["body"]="visible "+std::to_string(settings.driverVisible?1:0)+"\n";auto text=SerializeDriverSettings(sections);VehicleSteeringPose checked;if(!ParseVehicleSteeringPose(text,identity,checked))return false;
  auto temporary=path;temporary+=L".pose.tmp";{std::ofstream file(temporary,std::ios::binary|std::ios::trunc);if(!file)return false;file<<text;file.flush();if(!file.good())return false;}std::string latest;std::error_code ec;if(!ReadDriverSettingsFile(path,latest)||latest!=before){std::filesystem::remove(temporary,ec);return false;}if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return false;prior=text;return true;
 }
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

inline unsigned ApplyVehicleFootOffsets(DriverIK::Pose& pose,const DriverIK::Positions& positions,const VehicleSteeringPose& settings){
 using namespace DriverIK;unsigned mask=0;for(unsigned side=0;side<2;++side){if(Length(settings.offset[side+2])<1e-7f&&settings.footRotation[side]==Q{1,0,0,0})continue;unsigned upper=side?54:48,lower=upper+1,foot=upper+3;auto before=FK(pose,positions);V target=Add(before.p[foot],settings.offset[side+2]),ray=Sub(target,before.p[upper]);float distance=Length(ray),minimum=std::abs(Length(positions[lower])-Length(positions[foot]))+.002f,maximum=Length(positions[lower])+Length(positions[foot])-.002f;if(distance<1e-6f||maximum<=minimum)continue;target=Add(before.p[upper],Mul(ray,std::clamp(distance,minimum,maximum)/distance));auto candidate=pose;if(Solve(candidate,positions,upper,lower,foot,target,Product(settings.footRotation[side],before.q[foot]),before.p[lower])){pose=candidate;mask|=1u<<side;}}return mask;
}
