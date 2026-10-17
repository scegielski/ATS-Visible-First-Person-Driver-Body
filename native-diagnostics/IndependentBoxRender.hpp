#pragma once
#include "ControllerSceneFrame.hpp"
#include "InteractionBoxEditor.hpp"
#include "ControllerModelData.hpp"
#include <cstring>
inline ControllerMeshData SetupBoxWireMesh(){
 ControllerMeshData mesh;mesh.width=mesh.height=mesh.mips=1;mesh.texture={255,255,255,255};
 constexpr uint16_t faces[]{0,2,1,1,2,3,4,5,6,5,7,6,0,1,4,1,5,4,2,6,3,3,6,7,0,4,2,2,4,6,1,3,5,3,7,5};
 for(unsigned axis=0;axis<3;++axis)for(unsigned signs=0;signs<4;++signs){unsigned base=static_cast<unsigned>(mesh.vertices.size());
  for(unsigned corner=0;corner<8;++corner){ControllerMeshVertex vertex{};vertex.normal[2]=1;unsigned cross=0;for(unsigned d=0;d<3;++d){float direction=(corner&(1u<<d))?1.f:-1.f;vertex.position[d]=d==axis?direction:(((signs&(1u<<cross++))?1.f:-1.f)+direction*.012f);}mesh.vertices.push_back(vertex);}
  for(auto index:faces)mesh.indices.push_back(static_cast<uint16_t>(base+index));
 }
 return mesh;
}
inline ControllerMeshPose SetupBoxTrackingPose(const InteractionBox& box,const ControllerSceneFrame& frame,bool label=false){
 using namespace DriverIK;auto basis=frame.CabinToTracking();auto q=label?frame.neutralQ:Product(basis,box.rotation);V center=box.center;
 if(label)center=Add(center,Rotate(box.rotation,{0,box.half[1]*box.scale*box.stretch[1]+.045f,0}));
 center=frame.CabinPointToTracking(center);ControllerMeshPose pose;pose.visible=true;
 for(unsigned col=0;col<3;++col){V axis{};axis[col]=label?(col==0?.10f:col==1?.014f:1.f):box.half[col]*box.scale*box.stretch[col];auto v=Rotate(q,axis);for(unsigned row=0;row<3;++row)pose.matrix[row*4+col]=v[row];}
 for(unsigned row=0;row<3;++row)pose.matrix[row*4+3]=center[row];return pose;
}

// Higher-precision density samples preserve faint feathering between layers.
inline uint16_t HoverUnitHalf(float value){
 if(!std::isfinite(value)||value<=0)return 0;value=std::min(value,1.f);if(value<std::ldexp(1.f,-14))return static_cast<uint16_t>(std::lround(std::ldexp(value,24)));int exponent=0;float fraction=std::frexp(value,&exponent);unsigned mantissa=static_cast<unsigned>(std::lround((fraction*2-1)*1024));if(mantissa==1024){mantissa=0;++exponent;}return static_cast<uint16_t>(((exponent+14)<<10)|mantissa);
}
inline constexpr unsigned HoverVolumeSlices=33,HoverVolumeTile=96,HoverVolumeGrid=6;
inline ControllerMeshData InteractionHoverFillMesh(std::array<uint8_t,3> color={90,210,255},float strength=1.f){
 constexpr unsigned slices=HoverVolumeSlices,tile=HoverVolumeTile,grid=HoverVolumeGrid,atlas=tile*grid;ControllerMeshData mesh;mesh.width=mesh.height=atlas;mesh.mips=1;mesh.format=5;mesh.texture.resize(atlas*atlas*8);
 auto density=[](float v){float t=std::max(0.f,1-v*v);return t*t;};uint16_t channels[3]{};for(unsigned c=0;c<3;++c){float v=color[c]/255.f;channels[c]=HoverUnitHalf(v<=.04045f?v/12.92f:std::pow((v+.055f)/1.055f,2.4f));}
 for(unsigned slice=0;slice<slices;++slice){float z=(float(slice)+.5f)/slices*2-1;for(unsigned y=0;y<tile;++y)for(unsigned x=0;x<tile;++x){float u=float(x)/(tile-1)*2-1,v=float(y)/(tile-1)*2-1;unsigned pixel=(((slice/grid)*tile+y)*atlas+(slice%grid)*tile+x)*8;uint16_t sample[]{channels[0],channels[1],channels[2],HoverUnitHalf((7.f/255.f)*(9.f/slices)*std::clamp(strength,0.f,2.f)*density(u)*density(v)*density(z))};std::memcpy(mesh.texture.data()+pixel,sample,sizeof(sample));}
  for(unsigned axis=0;axis<3;++axis){unsigned base=static_cast<unsigned>(mesh.vertices.size());for(unsigned corner=0;corner<4;++corner){ControllerMeshVertex vertex{};vertex.normal[2]=1;vertex.position[axis]=z;vertex.position[(axis+1)%3]=(corner&1)?1.f:-1.f;vertex.position[(axis+2)%3]=(corner&2)?1.f:-1.f;vertex.uv[0]=((slice%grid)*tile+((corner&1)?tile-.5f:.5f))/atlas;vertex.uv[1]=((slice/grid)*tile+((corner&2)?tile-.5f:.5f))/atlas;mesh.vertices.push_back(vertex);}for(unsigned index:{0u,1u,2u,1u,3u,2u})mesh.indices.push_back(static_cast<uint16_t>(base+index));}
 }
 return mesh;
}
inline std::array<ControllerMeshPose,4> InteractionHoverPoses(const InteractionBoxEditor& editor,const ControllerSceneFrame& frame,const int (&targets)[2],bool active,bool jointsAvailable){
 std::array<ControllerMeshPose,4> poses{};if(!active||editor.enabled)return poses;for(unsigned side=0;side<2;++side){int box=targets[side];if(box<0||box>=static_cast<int>(InteractionBoxCount)||(side&&box==targets[0])||(!jointsAvailable&&(box==12||box==13||box>=16)))continue;auto pose=SetupBoxTrackingPose(editor.boxes[box],frame);for(float v:pose.matrix)if(!std::isfinite(v)||std::abs(v)>10)pose.visible=false;poses[side*2]=pose;}return poses;
}
