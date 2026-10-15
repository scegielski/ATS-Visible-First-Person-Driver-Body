from pathlib import Path
import ast,json,struct,math
import numpy as np
from signal_pinch import pinch_fingers
root=Path(__file__).resolve().parents[2]
bones=json.loads((root/'development/evidence/steering_integration_audit.json').read_text())['driver_bones']
tree=ast.parse((root/'development/scripts/build_brake_prototype.py').read_text());exec(compile(ast.Module(body=[n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name in ('rotation','between','quaternion','fk')],type_ignores=[]),'helpers','exec'))
source=(root/'stock-reference/ford_mustang_67_ani.pma').read_bytes();offset=struct.unpack_from('<I',source,72)[0]
neutral=[np.array(struct.unpack_from('<14f',source,offset+i*56)) for i in range(61)];world=fk(neutral)
mesh=(root/'stock-reference/driver.pmg').read_bytes();stock_hand_rotation=world[7][:3,:3].copy();target=world[7][:3,3]+np.array([-.0381,-.0635,-.0508])
# Small clavicle protraction supplies the extra reach without stretching the arm.
pivot=world[4][:3,3];old_root=world[5][:3,3];direction=old_root-pivot+np.array([0.,0.,-.04]);direction/=np.linalg.norm(direction)
new_root=pivot+direction*np.linalg.norm(old_root-pivot)
clavicle_rotation=rotation(between(old_root-pivot,new_root-pivot))@world[4][:3,:3]
seed=[v.copy() for v in neutral];seed[4][4:8]=quaternion(world[bones[4]['parent']][:3,:3].T@clavicle_rotation,neutral[4][4:8]);world=fk(seed)
pinch_quats,pinch_report=pinch_fingers(neutral,mesh,rotation,quaternion)
shoulder,elbow,wrist=(world[i][:3,3] for i in (5,6,7));l1=np.linalg.norm(elbow-shoulder);l2=np.linalg.norm(wrist-elbow)
def pose_at(point):
 pose=[v.copy() for v in seed];direction=point-shoulder;distance=np.linalg.norm(direction);direction/=distance
 assert abs(l1-l2)<distance<l1+l2
 along=(l1*l1-l2*l2+distance*distance)/(2*distance);bend=elbow-shoulder-direction*np.dot(elbow-shoulder,direction);bend/=np.linalg.norm(bend)
 next_elbow=shoulder+direction*along+bend*math.sqrt(l1*l1-along*along)
 upper=rotation(between(elbow-shoulder,next_elbow-shoulder))@world[5][:3,:3];lower=rotation(between(wrist-elbow,point-next_elbow))@world[6][:3,:3]
 pose[5][4:8]=quaternion(world[4][:3,:3].T@upper,pose[5][4:8]);pose[6][4:8]=quaternion(upper.T@lower,pose[6][4:8]);pose[7][4:8]=quaternion(lower.T@stock_hand_rotation,pose[7][4:8])
 for i,q in pinch_quats.items():pose[i][4:8]=q
 for i in range(14,20):assert np.array_equal(pose[i],neutral[i])
 result=fk(pose);assert np.linalg.norm(result[7][:3,3]-point)<2e-5
 assert abs(np.linalg.norm(result[6][:3,3]-result[5][:3,3])-l1)<2e-5
 assert abs(np.linalg.norm(result[7][:3,3]-result[6][:3,3])-l2)<2e-5
 for i in range(61):
  assert np.isfinite(pose[i]).all() and abs(np.dot(pose[i][4:8],pose[i][4:8])-1)<1e-4
  if i not in range(4,25):assert np.array_equal(pose[i],neutral[i])
 return pose
reach_pose=pose_at(target);waypoint=(fk(neutral)[7][:3,3]+target)/2+np.array([.00635,.02135,0.]);hover=pose_at(waypoint)
for i in range(8,23):
 flat_q=np.array(struct.unpack_from('<14f',mesh,112+i*200+136)[4:8]);q=hover[i][4:8]
 if np.dot(q,flat_q)<0:flat_q=-flat_q
 opened=q*.75+flat_q*.25;hover[i][4:8]=opened/np.linalg.norm(opened)
lines=['#pragma once','#include "WindowGestureTargets.hpp"']
for name,direction in [('Left',-1),('Right',1)]:
 pushed=pose_at(target+np.array([0.,direction*.025,0.]));lines+=['inline constexpr std::array<WindowGestureTarget,21> kSignal'+name+'Targets{{']
 file=['# Left hand turn-signal stalk: approach waypoint, reach, actuate.','SPF_GESTURE_POSE 2']
 for i in range(4,25):
  q=lambda p:','.join(f'{float(v):.9f}f' for v in p[i][4:8])
  lines.append(f' {{{i},{{{q(hover)}}},{{{q(reach_pose)}}},{{{q(pushed)}}}}},')
  file+=['# '+bones[i]['name'],'bone '+str(i)+' '+' '.join(f'{float(v):.9f}' for p in (hover,reach_pose,pushed) for v in p[i][4:8])]
 lines+=['}};'];(root/'releases'/('signal_'+name.lower()+'_pose.txt')).write_text('\n'.join(file)+'\n')
(root/'native-diagnostics/SignalTargets.hpp').write_text('\n'.join(lines)+'\n')
report={'stalk_wrist_target_m':target.tolist(),'leftward_adjustment_m':.0381,'forward_adjustment_m':.0508,'downward_adjustment_m':.0635,'finger_pose':'index and middle pinch with thumb','pinch_fit':pinch_report,'stroke_m':.025,'left_direction':'down','right_direction':'up','arm_lengths_preserved':True,'right_arm_and_pedals_unchanged':True,'approach_waypoint_m':waypoint.tolist(),'approved_finger_pose_retained':True,'midway_hand_open_blend':.25,'visual_test_pending':True}
(root/'development/evidence/signal_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))


