#pragma once
#include "ControllerSceneFrame.hpp"
#include "InteractionBoxEditor.hpp"
#include "ControllerModelData.hpp"
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
 using namespace DriverIK;auto q=label?frame.neutralQ:Product(frame.neutralQ,box.rotation);V center=box.center;
 if(label)center=Add(center,Rotate(box.rotation,{0,box.half[1]*box.scale*box.stretch[1]+.045f,0}));
 center=Add(frame.neutral,Rotate(frame.neutralQ,Sub(center,frame.camera)));ControllerMeshPose pose;pose.visible=true;
 for(unsigned col=0;col<3;++col){V axis{};axis[col]=label?(col==0?.10f:col==1?.014f:1.f):box.half[col]*box.scale*box.stretch[col];auto v=Rotate(q,axis);for(unsigned row=0;row<3;++row)pose.matrix[row*4+col]=v[row];}
 for(unsigned row=0;row<3;++row)pose.matrix[row*4+3]=center[row];return pose;
}

// Three crossed stacks sample a feathered 3D density field, avoiding visible cube faces.
inline ControllerMeshData InteractionHoverFillMesh(){
 constexpr unsigned slices=9,tile=64,atlas=tile*3;ControllerMeshData mesh;mesh.width=mesh.height=atlas;mesh.mips=1;mesh.texture.resize(atlas*atlas*4);
 auto density=[](float v){float t=std::max(0.f,1-v*v);return t*t;};
 for(unsigned slice=0;slice<slices;++slice){float z=(float(slice)+.5f)/slices*2-1;for(unsigned y=0;y<tile;++y)for(unsigned x=0;x<tile;++x){float u=float(x)/(tile-1)*2-1,v=float(y)/(tile-1)*2-1;unsigned pixel=(((slice/3)*tile+y)*atlas+(slice%3)*tile+x)*4;mesh.texture[pixel]=90;mesh.texture[pixel+1]=210;mesh.texture[pixel+2]=255;mesh.texture[pixel+3]=static_cast<uint8_t>(std::lround(7*density(u)*density(v)*density(z)));}
  for(unsigned axis=0;axis<3;++axis){unsigned base=static_cast<unsigned>(mesh.vertices.size());for(unsigned corner=0;corner<4;++corner){ControllerMeshVertex vertex{};vertex.normal[2]=1;vertex.position[axis]=z;vertex.position[(axis+1)%3]=(corner&1)?1.f:-1.f;vertex.position[(axis+2)%3]=(corner&2)?1.f:-1.f;vertex.uv[0]=((slice%3)*tile+((corner&1)?tile-.5f:.5f))/atlas;vertex.uv[1]=((slice/3)*tile+((corner&2)?tile-.5f:.5f))/atlas;mesh.vertices.push_back(vertex);}for(unsigned index:{0u,1u,2u,1u,3u,2u})mesh.indices.push_back(static_cast<uint16_t>(base+index));}
 }
 return mesh;
}
inline std::array<ControllerMeshPose,4> InteractionHoverPoses(const InteractionBoxEditor& editor,const ControllerSceneFrame& frame,const int (&targets)[2],bool active,bool jointsAvailable){
 std::array<ControllerMeshPose,4> poses{};if(!active||editor.enabled)return poses;for(unsigned side=0;side<2;++side){int box=targets[side];if(box<0||box>=static_cast<int>(InteractionBoxCount)||(side&&box==targets[0])||(!jointsAvailable&&(box==12||box==13||box>=16)))continue;auto pose=SetupBoxTrackingPose(editor.boxes[box],frame);for(float v:pose.matrix)if(!std::isfinite(v)||std::abs(v)>10)pose.visible=false;poses[side*2]=pose;}return poses;
}
