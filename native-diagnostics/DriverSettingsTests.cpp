#include "DriverSettings.hpp"
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
 std::filesystem::remove(legacy);std::filesystem::remove(bundle);REQUIRE(std::filesystem::remove(directory));std::cout<<"Unified driver settings regression passed\n";
}
