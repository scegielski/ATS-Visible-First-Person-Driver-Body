"""Generate a compact middle-finger gesture above the steering wheel using the stock rig."""
from pathlib import Path
import ast,struct,json,math
import numpy as np
root=Path(__file__).resolve().parent
bones=json.loads((root/'steering_integration_audit.json').read_text())['driver_bones']
helpers=ast.parse((root/'build_brake_prototype.py').read_text())
exec(compile(ast.Module(body=[n for n in helpers.body if isinstance(n,ast.FunctionDef) and n.name in ('rotation','between','quaternion','fk')],type_ignores=[]),'pose_helpers','exec'))
source=(root/'stock-reference/ford_mustang_67_ani.pma').read_bytes();offset=struct.unpack_from('<I',source,72)[0]
neutral=[np.array(struct.unpack_from('<14f',source,offset+i*56)) for i in range(61)]
mesh=(root/'stock-reference/driver.pmg').read_bytes()
flat=[v.copy() for v in neutral]
for i in range(8,23):flat[i][4:8]=struct.unpack_from('<14f',mesh,112+i*200+136)[4:8]
world=fk(neutral);flat_world=fk(flat)
up=flat_world[13][:3,3]-flat_world[11][:3,3]
span=flat_world[8][:3,3]-flat_world[17][:3,3]
normal=np.cross(span,up);normal/=np.linalg.norm(normal)
up-=normal*np.dot(up,normal);up/=np.linalg.norm(up)
local_basis=world[7][:3,:3].T@np.column_stack((np.cross(up,normal),up,normal))
# Palm faces the driver; back of hand faces the windshield, finger points up.
out=np.array([0.,-1.,-1.]);out/=np.linalg.norm(out)
desired_up=np.array([0.,1.,0.])-out*out[1];desired_up/=np.linalg.norm(desired_up)
hand_rotation=np.column_stack((np.cross(desired_up,out),desired_up,out))@local_basis.T
target=(world[7][:3,3]+world[28][:3,3])/2 + np.array([0.,.18,-.08])
# Raise and protract the clavicle to extend naturally beyond the previous reach.
clavicle=world[4][:3,3];old_arm_root=world[5][:3,3]
clavicle_direction=np.array([-.08,.02,-.09]);clavicle_direction/=np.linalg.norm(clavicle_direction)
new_arm_root=clavicle+clavicle_direction*np.linalg.norm(old_arm_root-clavicle)
shoulder_rotation=rotation(between(old_arm_root-clavicle,new_arm_root-clavicle))@world[4][:3,:3]
flat[4][4:8]=quaternion(world[3][:3,:3].T@shoulder_rotation,flat[4][4:8])
world=fk(flat)
shoulder,elbow,wrist=(world[i][:3,3] for i in (5,6,7))
l1=np.linalg.norm(elbow-shoulder);l2=np.linalg.norm(wrist-elbow)
direction=target-shoulder;distance=np.linalg.norm(direction);direction/=distance
assert abs(l1-l2)<distance<l1+l2
along=(l1*l1-l2*l2+distance*distance)/(2*distance)
bend=elbow-shoulder-direction*np.dot(elbow-shoulder,direction);bend/=np.linalg.norm(bend)
next_elbow=shoulder+direction*along+bend*math.sqrt(l1*l1-along*along)
upper=rotation(between(elbow-shoulder,next_elbow-shoulder))@world[5][:3,:3]
lower=rotation(between(wrist-elbow,target-next_elbow))@world[6][:3,:3]
# Roll the forearm about its own axis, keeping the wrist at its stock local angle.
forearm_axis=target-next_elbow;forearm_axis/=np.linalg.norm(forearm_axis)
current_normal=lower@rotation(neutral[7][4:8])@local_basis[:,2]
projected_current=current_normal-forearm_axis*np.dot(current_normal,forearm_axis)
projected_desired=out-forearm_axis*np.dot(out,forearm_axis)
projected_current/=np.linalg.norm(projected_current);projected_desired/=np.linalg.norm(projected_desired)
roll=math.atan2(np.dot(forearm_axis,np.cross(projected_current,projected_desired)),np.dot(projected_current,projected_desired))
roll_rotation=rotation((math.cos(roll/2),*(forearm_axis*math.sin(roll/2))))
lower=roll_rotation@lower
open_pose=[v.copy() for v in flat]
open_pose[5][4:8]=quaternion(world[4][:3,:3].T@upper,neutral[5][4:8])
open_pose[6][4:8]=quaternion(upper.T@lower,neutral[6][4:8])
open_pose[7][4:8]=neutral[7][4:8].copy()
# Aim the straight middle finger vertically through its base joint, not the wrist.
open_world=fk(open_pose)
current_middle=open_world[13][:3,3]-open_world[11][:3,3]
vertical=np.array([0.,1.,0.])
middle_rotation=rotation(between(current_middle,vertical))@open_world[11][:3,:3]
open_pose[11][4:8]=quaternion(open_world[7][:3,:3].T@middle_rotation,open_pose[11][4:8])
assert np.array_equal(open_pose[7][4:8],neutral[7][4:8])
gesture=[v.copy() for v in open_pose]
# Curl index, ring and little fingers; leave the middle finger extended.
for chain in ((8,9,10),(14,15,16),(17,18,19)):
 for i,degrees in zip(chain,(95,115,85)):
  angle=math.radians(degrees)
  gesture[i][4:8]=quaternion(rotation(flat[i][4:8])@rotation((math.cos(angle/2),0.,0.,math.sin(angle/2))),flat[i][4:8])
# Keep the thumb base open beside the fist, curling mainly its distal tip.
# The previous whole-thumb IK wrap crossed the curled fingers in VR.
for i,degrees in zip((20,21,22),(10,15,65)):
 angle=math.radians(degrees)
 gesture[i][4:8]=quaternion(rotation(flat[i][4:8])@rotation((math.cos(angle/2),0.,0.,math.sin(angle/2))),flat[i][4:8])
thumb_target=fk(gesture)[22][:3,3]
selected=(4,5,6,7,*range(8,23),23,24)
for pose in (open_pose,gesture):
 actual=fk(pose)
 assert np.linalg.norm(actual[7][:3,3]-target)<2e-5
 assert abs(np.linalg.norm(actual[6][:3,3]-actual[5][:3,3])-l1)<2e-5
 assert abs(np.linalg.norm(actual[7][:3,3]-actual[6][:3,3])-l2)<2e-5
 for i in range(61):
  assert np.isfinite(pose[i]).all() and abs(np.dot(pose[i][4:8],pose[i][4:8])-1)<1e-4
  if i not in selected:assert np.array_equal(pose[i],neutral[i])
for i in (11,12,13):assert np.array_equal(gesture[i],open_pose[i])
opened=fk(open_pose);closed=fk(gesture)
for a,b in ((8,10),(14,16),(17,19)):
 assert np.linalg.norm(closed[b][:3,3]-closed[a][:3,3])<np.linalg.norm(opened[b][:3,3]-opened[a][:3,3])*.75
middle_up=closed[13][:3,3]-closed[11][:3,3];middle_up/=np.linalg.norm(middle_up)
assert np.dot(middle_up,vertical)>.99999
lines=['#pragma once','#include "HornPoseTargets.hpp"','inline constexpr std::array<HornBoneTarget,21> kWindowGestureTargets{{']
for i in selected:
 values=lambda pose:','.join(f'{float(v):.9f}f' for v in pose[i][4:8])
 lines.append(f' {{{i},{{{values(open_pose)}}},{{{values(gesture)}}}}},')
lines.append('}};');(root/'native-diagnostics/WindowGestureTargets.hpp').write_text('\n'.join(lines)+'\n')
report={'wrist_target_m':target.tolist(),'gesture_bones':selected,'middle_finger_extended':True,'middle_finger_vertical':True,'forearm_roll_degrees':math.degrees(roll),'wrist_local_rotation_unchanged':True,'other_fingers_curled':True,'arm_lengths_preserved':True,'right_arm_and_pedals_unchanged':True,'alignment_visual_test_pending':True,'thumb_wrap':'base 10 degrees, middle 15 degrees, tip 65 degrees; visual clearance pending','thumb_tip_target_m':thumb_target.tolist(),'initial_spf_hotkey':'F10 (configurable)','target_method':'neutral wrist midpoint plus 18 cm up and 8 cm forward of neutral wrist midpoint, with clavicle protraction'}
(root/'window_gesture_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))

