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
