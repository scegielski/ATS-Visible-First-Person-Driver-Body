#pragma once
#include "RuntimeArmIK.hpp"
#include <string_view>
#include <vector>
#include <sstream>
#include <set>
inline constexpr std::array<std::string_view,25> JointPoseNames{{"all","rest","left_hand_open","left_hand_closed","left_thumb_open","left_thumb_closed","right_hand_open","right_hand_closed","right_thumb_open","right_thumb_closed","left_foot","right_foot","accelerator","brake","clutch","horn","left_window","right_window","ignition_hover","ignition_turn","signal_left","signal_right","head","steering_left","steering_right"}};
using JointPoseWeights=std::array<float,JointPoseNames.size()>;
struct JointPoseEdit{unsigned pose=0,bone=0;DriverIK::V degrees{};};
struct JointPoseEditBank{std::array<JointPoseEdit,1800> values{};unsigned count=0;bool empty()const{return count==0;}auto begin()const{return values.begin();}auto end()const{return values.begin()+count;}};
inline bool ParseJointPoseEdits(const std::string& text,std::vector<JointPoseEdit>& output){
 std::vector<JointPoseEdit> candidate;std::set<std::pair<unsigned,unsigned>> seen;std::istringstream input(text);std::string line;
 while(std::getline(input,line)){auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos||line[first]=='#'||line[first]==';')continue;
  std::istringstream row(line);std::string name,extra;JointPoseEdit edit;if(!(row>>name>>edit.bone>>edit.degrees[0]>>edit.degrees[1]>>edit.degrees[2])||(row>>extra)||edit.bone>=72)return false;
  auto found=std::find(JointPoseNames.begin(),JointPoseNames.end(),name);if(found==JointPoseNames.end())return false;edit.pose=static_cast<unsigned>(found-JointPoseNames.begin());for(float v:edit.degrees)if(!std::isfinite(v)||std::abs(v)>360)return false;if(!seen.insert({edit.pose,edit.bone}).second)return false;candidate.push_back(edit);
 }
 std::sort(candidate.begin(),candidate.end(),[](const auto& a,const auto& b){return std::pair{a.pose,a.bone}<std::pair{b.pose,b.bone};});output=std::move(candidate);return true;
}
inline bool ParseJointPoseEdits(const std::string& text,JointPoseEditBank& output){std::vector<JointPoseEdit> parsed;if(!ParseJointPoseEdits(text,parsed)||parsed.size()>output.values.size())return false;JointPoseEditBank candidate;candidate.count=static_cast<unsigned>(parsed.size());std::copy(parsed.begin(),parsed.end(),candidate.values.begin());output=candidate;return true;}
inline DriverIK::Q JointEulerOffset(DriverIK::V degrees,float weight){using namespace DriverIK;Q result{1,0,0,0};for(unsigned axis=0;axis<3;++axis){float half=degrees[axis]*.00872664626f*std::clamp(weight,0.f,1.f);Q turn{std::cos(half),0,0,0};turn[axis+1]=std::sin(half);result=Product(result,turn);}return result;}
template<size_t N,typename Edits> inline void ApplyJointPoseEdits(std::array<DriverIK::Q,N>& pose,const Edits& edits,const JointPoseWeights& weights){for(const auto& edit:edits)if(edit.bone<N&&edit.pose<weights.size()&&std::isfinite(weights[edit.pose])&&weights[edit.pose]>0)pose[edit.bone]=DriverIK::Product(pose[edit.bone],JointEulerOffset(edit.degrees,weights[edit.pose]));}
