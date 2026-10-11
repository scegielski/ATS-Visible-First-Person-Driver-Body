"""Mirror the validated left gesture onto the right rig, correcting wrist target with IK."""
from pathlib import Path
import struct,json,math
import numpy as np
root=Path(__file__).resolve().parent
scope={'__file__':str(root/'build_window_gesture.py')}
exec((root/'build_window_gesture.py').read_text().split('lines=')[0],scope)
fk=scope['fk'];rotation=scope['rotation'];quaternion=scope['quaternion'];between=scope['between']
neutral=scope['neutral'];bones=scope['bones'];mesh=scope['mesh'];mirror=np.diag([-1.,1.,1.])
reference_world=fk(neutral)
target=(reference_world[7][:3,3]+reference_world[28][:3,3])/2+np.array([.0762,.18,-.08])
bind=[np.array(struct.unpack_from('<14f',mesh,112+i*200+136)) for i in range(61)];bind_world=fk(bind)
selected=tuple(i+21 for i in scope['selected'])
assert selected==tuple(range(25,46))
def right_pose(left):
 left_world=fk(left);pose=[v.copy() for v in neutral]
 def mirrored_rotation(i):
  r=i+21
  return mirror@left_world[i][:3,:3]@bind_world[i][:3,:3].T@mirror@bind_world[r][:3,:3]
 # Map anatomical rotations through the stock bind frames, rather than copying quaternions.
 for i in scope['selected']:
  r=i+21;world=fk(pose);parent=bones[r]['parent'];desired=mirrored_rotation(i)
  pose[r][4:8]=quaternion(world[parent][:3,:3].T@desired,pose[r][4:8])
 # Retarget the right wrist to the same above-wheel location as the left gesture.
 world=fk(pose);shoulder,elbow,wrist=(world[i][:3,3] for i in (26,27,28))
 l1=np.linalg.norm(elbow-shoulder);l2=np.linalg.norm(wrist-elbow)
 direction=target-shoulder;distance=np.linalg.norm(direction);direction/=distance
 assert abs(l1-l2)<distance<l1+l2
 along=(l1*l1-l2*l2+distance*distance)/(2*distance)
 bend=elbow-shoulder-direction*np.dot(elbow-shoulder,direction);bend/=np.linalg.norm(bend)
 new_elbow=shoulder+direction*along+bend*math.sqrt(l1*l1-along*along)
 upper=rotation(between(elbow-shoulder,new_elbow-shoulder))@world[26][:3,:3]
 lower=rotation(between(wrist-elbow,target-new_elbow))@world[27][:3,:3]
 hand=world[28][:3,:3]
 pose[26][4:8]=quaternion(world[25][:3,:3].T@upper,pose[26][4:8])
 pose[27][4:8]=quaternion(upper.T@lower,pose[27][4:8])
 pose[28][4:8]=quaternion(lower.T@hand,pose[28][4:8])
 result=fk(pose)
 assert np.linalg.norm(result[28][:3,3]-target)<2e-5
 assert abs(np.linalg.norm(result[27][:3,3]-result[26][:3,3])-l1)<2e-5
 assert abs(np.linalg.norm(result[28][:3,3]-result[27][:3,3])-l2)<2e-5
 for i in range(61):
  assert np.isfinite(pose[i]).all() and abs(np.dot(pose[i][4:8],pose[i][4:8])-1)<1e-4
  if i not in selected:assert np.array_equal(pose[i],neutral[i])
 return pose
poses=[right_pose(scope[name]) for name in ('open_pose','intermediate','gesture')]
lines=['#pragma once','#include "WindowGestureTargets.hpp"','inline constexpr std::array<WindowGestureTarget,21> kRightGestureTargets{{']
file=['# Right-hand quaternion groups: reach/open, intermediate finger-down, final held.','SPF_GESTURE_POSE 2']
for i in selected:
 values=lambda p:','.join(f'{float(v):.9f}f' for v in p[i][4:8])
 lines.append(f' {{{i},{{{values(poses[0])}}},{{{values(poses[1])}}},{{{values(poses[2])}}}}},')
 file.extend(['# '+bones[i]['name'],'bone '+str(i)+' '+' '.join(f'{float(v):.9f}' for p in poses for v in p[i][4:8])])
lines.append('}};');(root/'native-diagnostics/RightGestureTargets.hpp').write_text('\n'.join(lines)+'\n')
(root/'releases/right_gesture_pose.txt').write_text('\n'.join(file)+'\n')
w=fk(poses[2]);middle=w[34][:3,3]-w[32][:3,3]
assert np.dot(middle,[0.,1.,0.])>0
w=fk(poses[1]);middle=w[34][:3,3]-w[32][:3,3]
assert np.dot(middle,[0.,1.,0.])<0
report={'gesture_bones':selected,'middle_bones':[32,33,34],'finger_down_intermediate':True,'raised_final':True,'wrist_target_m':target.tolist(),'rightward_hand_offset_m':.0762,'arm_lengths_preserved':True,'left_arm_and_pedals_unchanged':True,'default_hotkey':'F11 (configurable)','visual_test_pending':True}
(root/'right_gesture_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
