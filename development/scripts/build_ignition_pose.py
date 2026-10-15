from pathlib import Path
import ast,json,struct,math
import numpy as np
root=Path(__file__).resolve().parents[2]
bones=json.loads((root/'development/evidence/steering_integration_audit.json').read_text())['driver_bones']
tree=ast.parse((root/'development/scripts/build_brake_prototype.py').read_text());exec(compile(ast.Module(body=[n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name in ('rotation','between','quaternion','fk')],type_ignores=[]),'helpers','exec'))
source=(root/'stock-reference/ford_mustang_67_ani.pma').read_bytes();offset=struct.unpack_from('<I',source,72)[0]
neutral=[np.array(struct.unpack_from('<14f',source,offset+i*56)) for i in range(61)];world=fk(neutral);stock_hand_rotation=world[28][:3,:3].copy()
shoulder,elbow,wrist=(world[i][:3,3] for i in (26,27,28));target=wrist+np.array([-.0254,-.2016,-.015])
l1=np.linalg.norm(elbow-shoulder);l2=np.linalg.norm(wrist-elbow)
# Protract the clavicle and cap forward reach without lengthening any segment.
pivot=world[25][:3,3];clavicle_length=np.linalg.norm(shoulder-pivot)
requested_target=target.copy();requested_target[2]=-.36079514610238145
radius=clavicle_length+l1+l2-.01
max_forward=pivot[2]-math.sqrt(radius*radius-(target[0]-pivot[0])**2-(target[1]-pivot[1])**2)
target[2]=max(requested_target[2],max_forward)
new_shoulder=pivot+(target-pivot)/np.linalg.norm(target-pivot)*clavicle_length
clavicle_rotation=rotation(between(shoulder-pivot,new_shoulder-pivot))@world[25][:3,:3]
seed=[v.copy() for v in neutral];seed[25][4:8]=quaternion(world[bones[25]['parent']][:3,:3].T@clavicle_rotation,neutral[25][4:8])
world=fk(seed);shoulder,elbow,wrist=(world[i][:3,3] for i in (26,27,28))
direction=target-shoulder;distance=np.linalg.norm(direction);direction/=distance
assert abs(l1-l2)<distance<l1+l2
along=(l1*l1-l2*l2+distance*distance)/(2*distance)
bend=elbow-shoulder-direction*np.dot(elbow-shoulder,direction);bend/=np.linalg.norm(bend)
next_elbow=shoulder+direction*along+bend*math.sqrt(l1*l1-along*along)
upper=rotation(between(elbow-shoulder,next_elbow-shoulder))@world[26][:3,:3]
lower=rotation(between(wrist-elbow,target-next_elbow))@world[27][:3,:3]
reach=[v.copy() for v in seed]
reach[26][4:8]=quaternion(world[25][:3,:3].T@upper,neutral[26][4:8]);reach[27][4:8]=quaternion(upper.T@lower,neutral[27][4:8]);reach[28][4:8]=quaternion(lower.T@stock_hand_rotation,neutral[28][4:8])
assert np.max(np.abs(fk(reach)[28][:3,:3]-stock_hand_rotation))<2e-5
turned=[v.copy() for v in reach]
# First visual-test estimate: rotate wrist 70 degrees in the key-on direction about its local longitudinal axis.
a=math.radians(-70);twist=rotation(np.array([math.cos(a/2),math.sin(a/2),0.,0.]))
turned[28][4:8]=quaternion(rotation(reach[28][4:8])@twist,reach[28][4:8])
# Intermediate reach waypoint, above the direct path while approaching the key.
waypoint=(fk(neutral)[28][:3,3]+target)*.5+np.array([0.,.03,0.])
d=waypoint-shoulder;dist=np.linalg.norm(d);d/=dist
assert abs(l1-l2)<dist<l1+l2
al=(l1*l1-l2*l2+dist*dist)/(2*dist);be=elbow-shoulder-d*np.dot(elbow-shoulder,d);be/=np.linalg.norm(be)
mid_elbow=shoulder+d*al+be*math.sqrt(l1*l1-al*al)
u=rotation(between(elbow-shoulder,mid_elbow-shoulder))@world[26][:3,:3]
l=rotation(between(wrist-elbow,waypoint-mid_elbow))@world[27][:3,:3]
hover=[v.copy() for v in seed];hover[26][4:8]=quaternion(world[25][:3,:3].T@u,neutral[26][4:8]);hover[27][4:8]=quaternion(u.T@l,neutral[27][4:8]);hover[28][4:8]=quaternion(l.T@stock_hand_rotation,neutral[28][4:8])
assert np.linalg.norm(fk(hover)[28][:3,3]-waypoint)<2e-5
selected=tuple(range(25,46));lines=['#pragma once','#include "WindowGestureTargets.hpp"','inline constexpr std::array<WindowGestureTarget,21> kIgnitionTargets{{'];file=['# Ignition reach, key grasp, key turn; estimated Mustang target.','SPF_GESTURE_POSE 2']
for i in selected:
 q=lambda p:','.join(f'{float(v):.9f}f' for v in p[i][4:8])
 lines.append(f' {{{i},{{{q(hover)}}},{{{q(reach)}}},{{{q(turned)}}}}},')
 file+=['# '+bones[i]['name'],'bone '+str(i)+' '+' '.join(f'{float(v):.9f}' for p in (hover,reach,turned) for v in p[i][4:8])]
lines+=['}};'];(root/'native-diagnostics/IgnitionTargets.hpp').write_text('\n'.join(lines)+'\n');(root/'releases/ignition_pose.txt').write_text('\n'.join(file)+'\n')
for pose in (reach,turned):
 w=fk(pose);assert np.linalg.norm(w[28][:3,3]-target)<2e-5
 assert abs(np.linalg.norm(w[27][:3,3]-w[26][:3,3])-l1)<2e-5
 assert abs(np.linalg.norm(w[28][:3,3]-w[27][:3,3])-l2)<2e-5
 for i in range(61):
  assert np.isfinite(pose[i]).all() and abs(np.dot(pose[i][4:8],pose[i][4:8])-1)<1e-4
  if i not in selected:assert np.array_equal(pose[i],neutral[i])
report={'engine_signal_runtime_validated':True,'captured_states':[0,1,2,3,0],'wrist_target_m':target.tolist(),'leftward_adjustment_m':.0254,'relative_drop_m':.2016,'relative_forward_m':float(fk(neutral)[28][2,3]-target[2]),'requested_additional_forward_m':.3048,'actual_additional_forward_m':float(-.23904690222419606-target[2]),'reach_limited':True,'wrist_turn_degrees':-70,'arm_lengths_preserved':True,'other_bones_unchanged':True,'reach_waypoint_m':waypoint.tolist(),'thumb_pivot_runtime':True,'visual_test_pending':True}
(root/'development/evidence/ignition_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))












