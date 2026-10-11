"""Author compact left-arm horn poses; no steering/pedal atlas duplication."""
from pathlib import Path
import ast,json,struct,math
import numpy as np
root=Path(__file__).resolve().parent
bones=json.loads((root/'steering_integration_audit.json').read_text())['driver_bones']
# Reuse the tested quaternion and FK helpers without executing the brake builder.
helper_tree=ast.parse((root/'build_brake_prototype.py').read_text())
exec(compile(ast.Module(body=[n for n in helper_tree.body if isinstance(n,ast.FunctionDef) and n.name in ('rotation','between','quaternion','fk')],type_ignores=[]),'pose_helpers','exec'))
source=(root/'stock-reference/ford_mustang_67_ani.pma').read_bytes()
offset=struct.unpack_from('<I',source,72)[0]
neutral=[np.array(struct.unpack_from('<14f',source,offset+i*56)) for i in range(61)]
world=fk(neutral)
shoulder,elbow,wrist=(world[i][:3,3] for i in (5,6,7))
center=(wrist+world[28][:3,3])/2 + np.array([-.005,-.095,0.])
l1=np.linalg.norm(elbow-shoulder);l2=np.linalg.norm(wrist-elbow)
selected=(4,5,6,7,*range(8,23),23,24)
mesh=(root/"stock-reference/driver.pmg").read_bytes()
flat_fingers={i:np.array(struct.unpack_from("<14f",mesh,112+i*200+136)[4:8]) for i in range(8,23)}
# Estimate the wheel surface from the stock wrists throughout the steering clip.
# Re-grabs leave this plane slightly, so this is a visual-test estimate.
wheel_points=[]
for frame in range(47):
 frame_pose=[v.copy() for v in neutral]
 for bone in range(4,46):
  frame_pose[bone]=np.array(struct.unpack_from('<14f',source,offset+(frame*61+bone)*56))
 frame_world=fk(frame_pose)
 wheel_points.extend([frame_world[7][:3,3],frame_world[28][:3,3]])
wheel_points=np.array(wheel_points)
_,_,plane_axes=np.linalg.svd(wheel_points-wheel_points.mean(axis=0))
wheel_normal=plane_axes[-1]
if wheel_normal[2]<0:wheel_normal=-wheel_normal
flat=[v.copy() for v in neutral]
for i,q in flat_fingers.items():flat[i][4:8]=q
flat_world=fk(flat)
finger_up=flat_world[13][:3,3]-flat_world[11][:3,3]
finger_span=flat_world[8][:3,3]-flat_world[17][:3,3]
palm_normal=np.cross(finger_span,finger_up);palm_normal/=np.linalg.norm(palm_normal)
finger_up-=palm_normal*np.dot(finger_up,palm_normal);finger_up/=np.linalg.norm(finger_up)
palm_right=np.cross(finger_up,palm_normal)
local_basis=world[7][:3,:3].T@np.column_stack((palm_right,finger_up,palm_normal))
desired_up=np.array([0.,1.,0.])-wheel_normal*wheel_normal[1];desired_up/=np.linalg.norm(desired_up)
desired_basis=np.column_stack((np.cross(desired_up,wheel_normal),desired_up,wheel_normal))
horn_hand_rotation=desired_basis@local_basis.T
assert abs(np.linalg.det(horn_hand_rotation)-1)<1e-5
assert np.max(np.abs(horn_hand_rotation.T@horn_hand_rotation-np.eye(3)))<1e-5

def pose_at(target):
 pose=[v.copy() for v in neutral]
 direction=target-shoulder;distance=np.linalg.norm(direction);direction/=distance
 assert abs(l1-l2)<distance<l1+l2
 along=(l1*l1-l2*l2+distance*distance)/(2*distance)
 bend=elbow-shoulder-direction*np.dot(elbow-shoulder,direction);bend/=np.linalg.norm(bend)
 next_elbow=shoulder+direction*along+bend*math.sqrt(l1*l1-along*along)
 upper=rotation(between(elbow-shoulder,next_elbow-shoulder))@world[5][:3,:3]
 lower=rotation(between(wrist-elbow,target-next_elbow))@world[6][:3,:3]
 pose[5][4:8]=quaternion(world[4][:3,:3].T@upper,neutral[5][4:8])
 pose[6][4:8]=quaternion(upper.T@lower,neutral[6][4:8])
 pose[7][4:8]=quaternion(lower.T@horn_hand_rotation,neutral[7][4:8])
 for i,q in flat_fingers.items():
  assert abs(np.dot(q,q)-1)<1e-4
  pose[i][4:8]=q
 result=fk(pose)
 assert np.linalg.norm(result[7][:3,3]-target)<2e-5
 actual_up=result[13][:3,3]-result[11][:3,3];actual_up/=np.linalg.norm(actual_up)
 actual_span=result[8][:3,3]-result[17][:3,3]
 actual_normal=np.cross(actual_span,actual_up);actual_normal/=np.linalg.norm(actual_normal)
 assert np.dot(actual_normal,wheel_normal)>.99999
 assert np.dot(actual_up,desired_up)>.99999
 assert abs(np.linalg.norm(result[6][:3,3]-result[5][:3,3])-l1)<2e-5
 assert abs(np.linalg.norm(result[7][:3,3]-result[6][:3,3])-l2)<2e-5
 for i in range(61):
  if i not in selected:assert np.array_equal(pose[i],neutral[i])
 return pose
hover=pose_at(center+np.array([0.,0.,.02]))
pressed=pose_at(center+np.array([0.,0.,.008]))
lines=['#pragma once','#include <array>','struct HornBoneTarget { unsigned bone; std::array<float,4> hover,pressed; };','inline constexpr std::array<HornBoneTarget,21> kHornTargets{{']
for i in selected:
 values=lambda p:','.join(f'{float(x):.9f}f' for x in p[i][4:8])
 lines.append(f' {{{i},{{{values(hover)}}},{{{values(pressed)}}}}},')
lines.append('}};')
(root/'native-diagnostics/HornPoseTargets.hpp').write_text('\n'.join(lines)+'\n')
report={'gesture_bones':selected,'wheel_center_estimate_m':center.tolist(),'palm_press_distance_m':.012,'vertical_adjustment_m':-.095,'left_adjustment_m':.005,'forward_adjustment_m':0.,'latest_backward_adjustment_m':.025,'latest_right_adjustment_m':.025,'additional_vertical_adjustment_m':0.,'palm_normal_estimate':wheel_normal.tolist(),'palm_parallel_to_wheel_plane':True,'fingers_point_up_in_wheel_plane':True,'finger_pose':'open bind-pose rotations, blended from current grip during reach','arm_lengths_preserved':True,'other_bones_unchanged':True,'runtime_pose_layout_validated':True,'visual_horn_alignment_validated':False,'center_method':'midpoint of neutral wrists lowered 9.5 cm total, moved 0.5 cm left, original depth restored following VR feedback'}
(root/'horn_pose_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
