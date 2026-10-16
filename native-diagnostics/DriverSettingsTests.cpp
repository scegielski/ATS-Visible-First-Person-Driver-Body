#include "DriverSettings.hpp"
#include "VehicleBoxProfiles.hpp"
#include "VehicleSteeringPose.hpp"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#define REQUIRE(x) do{if(!(x)){std::cerr<<"FAIL line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
void Put(const std::filesystem::path& p,const std::string& text){std::ofstream f(p,std::ios::binary|std::ios::trunc);f<<text;REQUIRE(f.good());}
int main(){
 auto directory=std::filesystem::temp_directory_path()/("spf-driver-settings-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));REQUIRE(std::filesystem::create_directory(directory));
 auto legacy=directory/"tracking.txt";auto bundle=directory/"driver_body.ini";Put(legacy,"legacy calibration\n");std::string text;REQUIRE(ReadDriverSetting(legacy,text)&&text=="legacy calibration\n");
 DriverSettingSections sections{{"tracking","new calibration\n"},{"interaction_boxes","box pose\n"},{"custom_future_setting","keep this\n"}};Put(bundle,SerializeDriverSettings(sections));REQUIRE(ReadDriverSetting(legacy,text)&&text.starts_with("new calibration"));
 Put(bundle,"[tracking]\nbad\n[tracking]\nduplicate\n");REQUIRE(ReadDriverSetting(legacy,text)&&text.starts_with("new calibration"));REQUIRE(!WriteDriverSetting(directory,"interaction_boxes","must not replace malformed user edit"));
 Put(bundle,SerializeDriverSettings(sections));REQUIRE(WriteDriverSetting(directory,"interaction_boxes","edited box\n"));std::string saved;REQUIRE(ReadDriverSettingsFile(bundle,saved));DriverSettingSections parsed;REQUIRE(ParseDriverSettings(saved,parsed));REQUIRE(parsed["tracking"].starts_with("new calibration")&&parsed["custom_future_setting"].starts_with("keep this")&&parsed["interaction_boxes"].starts_with("edited box"));
 auto size=saved.size();for(unsigned i=0;i<100;++i)REQUIRE(WriteDriverSetting(directory,"interaction_boxes","edited box\n"));REQUIRE(ReadDriverSettingsFile(bundle,saved)&&saved.size()==size);
 auto mustang=directory/"mustang_boxes.ini";Put(mustang,SerializeDriverSettings({{"interaction_boxes","Mustang saved placement\n"}}));
 REQUIRE(ReadDriverSetting(directory/"interaction_boxes.txt",text)&&text.starts_with("Mustang saved placement"));
 REQUIRE(WriteDriverSetting(directory,"interaction_boxes","new Mustang placement\n"));
 REQUIRE(ReadDriverSetting(directory/"interaction_boxes.txt",text)&&text.starts_with("new Mustang placement"));
 REQUIRE(ReadDriverSettingsFile(bundle,text)&&text==saved);std::filesystem::remove(mustang);
 auto poses=directory/"mustang_poses.ini";Put(poses,SerializeDriverSettings({{"ignition_pose","Mustang ignition\n"}}));
 REQUIRE(ReadDriverSetting(directory/"ignition_pose.txt",text)&&text.starts_with("Mustang ignition"));
 REQUIRE(WriteDriverSetting(directory,"ignition_pose","edited Mustang ignition\n"));
 REQUIRE(ReadDriverSetting(directory/"ignition_pose.txt",text)&&text.starts_with("edited Mustang ignition"));
 REQUIRE(ReadDriverSettingsFile(bundle,text)&&text==saved);std::filesystem::remove(poses);
 REQUIRE(!ParseDriverSettings("[invalid-name]\n1\n",parsed));REQUIRE(!ParseDriverSettings("outside a section\n[x]\n1\n",parsed));REQUIRE(!ParseDriverSettings(std::string(262145,'x'),parsed));REQUIRE(ParseDriverSettings("\xef\xbb\xbf[tracking]\n1\n",parsed));
 std::string defaults;defaults=kDriverDefaults;REQUIRE(ParseDriverSettings(defaults,parsed)&&parsed.size()==16);
 for(const auto& [key,value]:parsed){std::string old;REQUIRE(ReadDriverSettingsFile(std::filesystem::path("../../releases")/(key+".txt"),old));auto trim=[](std::string s){s.erase(std::remove(s.begin(),s.end(),'\r'),s.end());auto n=s.find_last_not_of("\r\n");return n==std::string::npos?std::string{}:s.substr(0,n+1);};REQUIRE(trim(value)==trim(old));}
 REQUIRE(ReadDriverSetting(directory/"signal_activation_delay.txt",text)&&text.find("0.22")!=std::string::npos);
 Put(bundle,"[signal_activation_delay]\n9.99\n");REQUIRE(ReadDriverSetting(directory/"signal_activation_delay.txt",text)&&text.find("0.22")!=std::string::npos);

 VehicleBoxIdentity ford{"ford","vehicle.mustang"},other{"kenworth","w900"},similar{"ford","vehicle/mustang"};
 REQUIRE(!VehicleBoxIdentity{}.Valid());REQUIRE(VehicleBoxPath(directory,{}).empty());
 REQUIRE(ford.Filename()!=similar.Filename());REQUIRE(ford.Filename()==ford.Filename());
 VehicleBoxIdentity traversal{"../brand","../../model"};REQUIRE(traversal.Filename().find('/')==std::string::npos);
 InteractionBoxEditor custom;custom.boxes[0].center={1.25f,2,3};
 Put(mustang,SerializeDriverSettings({{"interaction_boxes",SerializeInteractionBoxes(custom)}}));
 std::string original;REQUIRE(ReadDriverSettingsFile(mustang,original));
 InteractionBoxEditor loaded;std::string placement;bool migrated=false;
 REQUIRE(InitializeVehicleBoxes(directory,ford,loaded,placement,migrated)&&migrated);REQUIRE(loaded.boxes[0].center[0]==1.25f);
 REQUIRE(InitializeVehicleBoxes(directory,other,loaded,placement,migrated)&&!migrated);REQUIRE(loaded.boxes[0].center[0]!=1.25f);
 for(unsigned i=0;i<12;++i){REQUIRE(std::abs(loaded.boxes[i].center[2]+.4f)<1e-5f);REQUIRE(std::abs(loaded.boxes[i].center[1]-(1.6f-.24f*static_cast<float>(i/4)))<1e-5f);for(unsigned axis=0;axis<3;++axis)REQUIRE(std::abs(loaded.boxes[i].half[axis]*loaded.boxes[i].scale*loaded.boxes[i].stretch[axis]-.05f)<1e-6f);}
 REQUIRE(loaded.boxes[12].center==InteractionBoxEditor{}.boxes[12].center&&loaded.boxes[13].center==InteractionBoxEditor{}.boxes[13].center);

 loaded.boxes[0].center[0]=9;REQUIRE(WriteVehicleBoxes(VehicleBoxPath(directory,other),other,SerializeInteractionBoxes(loaded)));
 REQUIRE(ReadVehicleBoxes(VehicleBoxPath(directory,ford),ford,loaded,placement)&&loaded.boxes[0].center[0]==1.25f);
 REQUIRE(ReadVehicleBoxes(VehicleBoxPath(directory,other),other,loaded,placement)&&loaded.boxes[0].center[0]==9);
 REQUIRE(!ReadVehicleBoxes(VehicleBoxPath(directory,ford),other,loaded,placement));
 auto vehicleFile=VehicleBoxPath(directory,ford);REQUIRE(ReadDriverSettingsFile(vehicleFile,text));REQUIRE(ParseDriverSettings(text,parsed));parsed["zz_custom"]="preserve me\n";Put(vehicleFile,SerializeDriverSettings(parsed));
 REQUIRE(ReadVehicleBoxes(vehicleFile,ford,loaded,placement));REQUIRE(WriteVehicleBoxes(vehicleFile,ford,placement));REQUIRE(ReadDriverSettingsFile(vehicleFile,text)&&text.find("preserve me")!=std::string::npos);
 Put(vehicleFile,"malformed user edit");REQUIRE(!InitializeVehicleBoxes(directory,ford,loaded,placement,migrated));REQUIRE(!WriteVehicleBoxes(vehicleFile,ford,placement));REQUIRE(ReadDriverSettingsFile(vehicleFile,text)&&text=="malformed user edit");
 VehicleBoxIdentity oldSeed{"ford","vehicle.unused"};auto seedPath=VehicleBoxPath(directory,oldSeed);REQUIRE(WriteVehicleBoxes(seedPath,oldSeed,SerializeInteractionBoxes(InteractionBoxEditor{})));REQUIRE(InitializeVehicleBoxes(directory,oldSeed,loaded,placement,migrated));REQUIRE(SerializeInteractionBoxes(loaded)==SerializeInteractionBoxes(GenericVehicleBoxes()));loaded.boxes[0].center[1]=1.77f;REQUIRE(WriteVehicleBoxes(seedPath,oldSeed,SerializeInteractionBoxes(loaded)));REQUIRE(InitializeVehicleBoxes(directory,oldSeed,loaded,placement,migrated)&&loaded.boxes[0].center[1]==1.77f);std::filesystem::remove(seedPath);

 VehicleSteeringPose grip;REQUIRE(ParseVehicleSteeringPose(DefaultVehicleSteeringPose(other),other,grip)&&grip.headVisible);
 REQUIRE(!ParseVehicleSteeringPose(DefaultVehicleSteeringPose(other),ford,grip));
 LiveVehicleSteeringPose liveGrips;REQUIRE(liveGrips.Poll(directory,other));auto gripFile=VehiclePosePath(directory,other);REQUIRE(gripFile!=VehicleBoxPath(directory,other));
 REQUIRE(ReadDriverSettingsFile(gripFile,text));REQUIRE(ParseDriverSettings(text,parsed));parsed["steering_grips"]="left_offset 0 0 0.02\nright_offset 0 0 0.03\n";parsed["head"]="visible 1\nbackshift_m 0.01\n";Put(gripFile,SerializeDriverSettings(parsed));REQUIRE(liveGrips.Poll(directory,other)&&liveGrips.settings.offset[0][2]==.02f&&liveGrips.settings.offset[1][2]==.03f&&liveGrips.settings.headBackshift==.01f);
 Put(gripFile,"broken edit");REQUIRE(!liveGrips.Poll(directory,other)&&liveGrips.settings.offset[0][2]==.02f);REQUIRE(liveGrips.Poll(directory,ford)&&liveGrips.settings.offset[0][2]==0);std::filesystem::remove(gripFile);std::filesystem::remove(VehiclePosePath(directory,ford));
 REQUIRE(ReadDriverSettingsFile(mustang,text)&&text==original);
 std::filesystem::remove(vehicleFile);std::filesystem::remove(VehicleBoxPath(directory,other));std::filesystem::remove(directory/"vehicles");std::filesystem::remove(mustang);
 std::filesystem::remove(legacy);std::filesystem::remove(bundle);REQUIRE(std::filesystem::remove(directory));std::cout<<"Unified driver settings regression passed\n";
}
