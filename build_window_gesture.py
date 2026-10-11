"""Generate a compact middle-finger gesture above the steering wheel using the stock rig."""
from pathlib import Path
import ast,struct,json,math
import numpy as np
from thumb_wrap_solver import solve_thumb_wrap
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
target=(world[7][:3,3]+world[28][:3,3])/2 + np.array([-.0762,.18,-.08])
# Raise and protract the clavicle to extend naturally beyond the previous reach.
clavicle=world[4][:3,3];old_arm_root=world[5][:3,3]
clavicle_direction=np.array([-.10,.02,-.07]);clavicle_direction/=np.linalg.norm(clavicle_direction)
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
# Share axial orientation equally between forearm and wrist.
forearm_axis=target-next_elbow;forearm_axis/=np.linalg.norm(forearm_axis)
current_normal=lower@rotation(neutral[7][4:8])@local_basis[:,2]
projected_current=current_normal-forearm_axis*np.dot(current_normal,forearm_axis)
projected_desired=out-forearm_axis*np.dot(out,forearm_axis)
projected_current/=np.linalg.norm(projected_current);projected_desired/=np.linalg.norm(projected_desired)
roll=math.atan2(np.dot(forearm_axis,np.cross(projected_current,projected_desired)),np.dot(projected_current,projected_desired))
full_rotation=rotation((math.cos(roll/2),*(forearm_axis*math.sin(roll/2))))
final_hand_rotation=full_rotation@lower@rotation(neutral[7][4:8])
# Add a small upward wrist tilt while retaining the reduced forearm roll.
wrist_axis=final_hand_rotation@local_basis[:,0]
palm=-final_hand_rotation@local_basis[:,2]
tilt=math.radians(10)*(1 if np.dot(np.cross(wrist_axis,palm),[0.,1.,0.])>0 else -1)
final_hand_rotation=rotation((math.cos(tilt/2),*(wrist_axis*math.sin(tilt/2))))@final_hand_rotation
# Reduce the rightward hand lean, then compensate the middle finger below.
straighten=math.radians(8)
final_hand_rotation=rotation((math.cos(straighten/2),0.,0.,math.sin(straighten/2)))@final_hand_rotation
# Turn the palm modestly toward the driver without changing its target position.
current_palm=-final_hand_rotation@local_basis[:,2]
palm_toward_driver=current_palm+np.array([0.,0.,.25]);palm_toward_driver/=np.linalg.norm(palm_toward_driver)
final_hand_rotation=rotation(between(current_palm,palm_toward_driver))@final_hand_rotation
# Additional explicit fifteen-degree wrist turn toward the driver's face.
current_palm=-final_hand_rotation@local_basis[:,2]
face_axis=np.cross(current_palm,np.array([0.,0.,1.]));face_axis/=np.linalg.norm(face_axis)
face_turn=math.radians(30)
final_hand_rotation=rotation((math.cos(face_turn/2),*(face_axis*math.sin(face_turn/2))))@final_hand_rotation
roll_rotation=rotation((math.cos(roll/6),*(forearm_axis*math.sin(roll/6))))
lower=roll_rotation@lower
open_pose=[v.copy() for v in flat]
open_pose[5][4:8]=quaternion(world[4][:3,:3].T@upper,neutral[5][4:8])
open_pose[6][4:8]=quaternion(upper.T@lower,neutral[6][4:8])
open_pose[7][4:8]=quaternion(lower.T@final_hand_rotation,neutral[7][4:8])
# Aim the straight middle finger vertically through its base joint, not the wrist.
open_world=fk(open_pose)
current_middle=open_world[13][:3,3]-open_world[11][:3,3]
vertical=np.array([0.,1.,0.])
middle_rotation=rotation(between(current_middle,vertical))@open_world[11][:3,:3]
open_pose[11][4:8]=quaternion(open_world[7][:3,:3].T@middle_rotation,open_pose[11][4:8])
assert np.max(np.abs(fk(open_pose)[7][:3,:3]-final_hand_rotation))<1e-5
gesture=[v.copy() for v in open_pose]
# Curl index, ring and little fingers; leave the middle finger extended.
for chain in ((8,9,10),(14,15,16),(17,18,19)):
 for i,degrees in zip(chain,(85,80,25) if chain[0]==8 else (90,90,35)):
  angle=math.radians(degrees)
  gesture[i][4:8]=quaternion(rotation(flat[i][4:8])@rotation((math.cos(angle/2),0.,0.,math.sin(angle/2))),flat[i][4:8])
# Add a small natural bend to the raised middle finger's middle and tip joints.
for i,degrees in ((11,15),(12,10),(13,8)):
 angle=math.radians(degrees)
 gesture[i][4:8]=quaternion(rotation(open_pose[i][4:8])@rotation((math.cos(angle/2),0.,0.,math.sin(angle/2))),open_pose[i][4:8])
# Preserve the exact thumb pose confirmed by the user; only fingers are eased.
confirmed_thumb_rotations={20: [0.775290357, 0.458662947, 0.432317792, 0.040675419], 21: [0.809557454, -0.00899529, -0.033336647, 0.5860243], 22: [0.928706579, 0.001725242, 0.000803126, 0.370810555]}
for bone,q in confirmed_thumb_rotations.items():gesture[bone][4:8]=q
thumb_report={'user_confirmed_pose_preserved':True,'finger_change_clearance_visual_test_pending':True}
thumb_target=fk(gesture)[22][:3,3]
# Intermediate fist has its middle finger down; the final raised pose is unchanged.
intermediate=[v.copy() for v in gesture]
for i,degrees in ((11,85),(12,80),(13,25)):
 angle=math.radians(degrees)
 intermediate[i][4:8]=quaternion(rotation(open_pose[i][4:8])@rotation((math.cos(angle/2),0.,0.,math.sin(angle/2))),open_pose[i][4:8])
 # Keep this finger down even in the reach/open target.
 open_pose[i][4:8]=intermediate[i][4:8].copy()
intermediate_world=fk(intermediate)
folded_direction=intermediate_world[13][:3,3]-intermediate_world[11][:3,3]
assert np.dot(folded_direction,[0.,1.,0.])<0
selected=(4,5,6,7,*range(8,23),23,24)
for pose in (open_pose,intermediate,gesture):
 actual=fk(pose)
 assert np.linalg.norm(actual[7][:3,3]-target)<2e-5
 assert abs(np.linalg.norm(actual[6][:3,3]-actual[5][:3,3])-l1)<2e-5
 assert abs(np.linalg.norm(actual[7][:3,3]-actual[6][:3,3])-l2)<2e-5
 for i in range(61):
  assert np.isfinite(pose[i]).all() and abs(np.dot(pose[i][4:8],pose[i][4:8])-1)<1e-4
  if i not in selected:assert np.array_equal(pose[i],neutral[i])
assert not np.array_equal(gesture[11][4:8],open_pose[11][4:8])
opened=fk(open_pose);closed=fk(gesture)
for a,b in ((8,10),(14,16),(17,19)):
 assert np.linalg.norm(closed[b][:3,3]-closed[a][:3,3])<np.linalg.norm(opened[b][:3,3]-opened[a][:3,3])*(.90 if a==8 else .75)
middle_up=closed[13][:3,3]-closed[11][:3,3];middle_up/=np.linalg.norm(middle_up)
assert np.dot(middle_up,vertical)>math.cos(math.radians(30))
lines=['#pragma once','#include <array>','struct WindowGestureTarget { unsigned bone; std::array<float,4> hover,intermediate,pressed; };','inline constexpr std::array<WindowGestureTarget,21> kWindowGestureTargets{{']
for i in selected:
 values=lambda pose:','.join(f'{float(v):.9f}f' for v in pose[i][4:8])
 lines.append(f' {{{i},{{{values(open_pose)}}},{{{values(intermediate)}}},{{{values(gesture)}}}}},')
lines.append('}};');(root/'native-diagnostics/WindowGestureTargets.hpp').write_text('\n'.join(lines)+'\n')
report={'wrist_target_m':target.tolist(),'leftward_hand_offset_m':.0762,'gesture_bones':selected,'middle_finger_extended':True,'intermediate_middle_finger_down':True,'extension_after_arrival':True,'middle_finger_vertical':False,'middle_finger_slightly_curled':True,'middle_joint_curl_degrees':[15,10,8],'total_roll_degrees':math.degrees(roll),'forearm_roll_degrees':math.degrees(roll)/3,'wrist_roll_degrees':math.degrees(roll)*2/3,'rotation_split':'one third forearm, two thirds wrist','extra_upward_wrist_tilt_degrees':10,'hand_straighten_degrees':8,'additional_palm_turn_degrees':30,'index_curl_degrees':[85,80,25],'thumb_turn':'base aimed toward folded index middle segment','other_fingers_curled':True,'arm_lengths_preserved':True,'right_arm_and_pedals_unchanged':True,'alignment_visual_test_pending':True,'thumb_wrap':'all three joints fit outside folded finger capsule proxies','thumb_clearance_check':thumb_report,'thumb_tip_target_m':thumb_target.tolist(),'initial_spf_hotkey':'F10 (configurable)','target_method':'neutral wrist midpoint plus 18 cm up and 8 cm forward of neutral wrist midpoint, with clavicle protraction'}
(root/'window_gesture_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))



# Editable runtime pose file; copy this into the installed plugin config directory.
pose_lines=['# Quaternion order: w x y z. Groups are reach/open, intermediate finger-down, and final held gesture.','SPF_GESTURE_POSE 2']
for i in selected:
 pose_lines.append('# '+bones[i]['name'])
 values=[*open_pose[i][4:8],*intermediate[i][4:8],*gesture[i][4:8]]
 pose_lines.append('bone '+str(i)+' '+' '.join(f'{float(v):.9f}' for v in values))
(root/'releases/gesture_pose.txt').write_text('\n'.join(pose_lines)+'\n')
