"""Add a staged brake reach and ankle press without changing verified gas/arm tracks."""
from pathlib import Path
import struct,json,math,zipfile
import numpy as np
root=Path(__file__).resolve().parent
BRAKE_REACH=.14
LEFT_REST_SHIFT=.06
source=(root/'stock-reference/ford_mustang_67_ani.pma').read_bytes()
bones=json.loads((root/'steering_integration_audit.json').read_text())['driver_bones']
frames,count=struct.unpack_from('<2I',source,4);assert (frames,count)==(47,61)
_,ids,transforms,_,_=struct.unpack_from('<5I',source,64)
mapping=source[ids:transforms];assert list(mapping)==list(range(61))
arms={4,25}
for bone in bones:
 if bone['parent'] in arms:arms.add(bone['index'])
def rotation(q):
 w,x,y,z=q
 return np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],[2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],[2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])
def between(a,b):
 a=a/np.linalg.norm(a);b=b/np.linalg.norm(b)
 q=np.array([1+np.dot(a,b),*np.cross(a,b)]);assert np.linalg.norm(q)>.01
 return q/np.linalg.norm(q)
def quaternion(matrix,reference):
 # Stable matrix conversion, quaternion order w,x,y,z.
 t=np.trace(matrix)
 if t>0:
  s=math.sqrt(t+1)*2;q=np.array([s/4,(matrix[2,1]-matrix[1,2])/s,(matrix[0,2]-matrix[2,0])/s,(matrix[1,0]-matrix[0,1])/s])
 else:
  i=int(np.argmax(np.diag(matrix)));j=(i+1)%3;k=(i+2)%3;s=math.sqrt(1+matrix[i,i]-matrix[j,j]-matrix[k,k])*2
  q=np.zeros(4);q[0]=(matrix[k,j]-matrix[j,k])/s;q[i+1]=s/4;q[j+1]=(matrix[j,i]+matrix[i,j])/s;q[k+1]=(matrix[k,i]+matrix[i,k])/s
 q/=np.linalg.norm(q)
 if np.dot(q,reference)<0:q=-q
 assert np.max(np.abs(rotation(q)-matrix))<1e-5
 return q
neutral=[np.array(struct.unpack_from('<14f',source,transforms+i*56)) for i in range(count)]
def fk(pose):
 matrices=[]
 for bone,values in zip(bones,pose):
  local=np.eye(4);local[:3,:3]=rotation(values[4:8])@np.diag(values[11:14]);local[:3,3]=values[8:11]
  matrices.append(local if bone['parent']==255 else matrices[bone['parent']]@local)
 return matrices
world=fk(neutral);hip=world[54][:3,3];knee=world[55][:3,3];ankle=world[57][:3,3]
assert [bones[i]['name'] for i in (54,55,57)]==['rgupleg','rgleg','rgfoot']
length1=np.linalg.norm(knee-hip);length2=np.linalg.norm(ankle-knee)
left=world[51][:3,3]-ankle;left[1]=0;left/=np.linalg.norm(left)
forward=world[58][:3,3]-ankle
hinge=np.cross(forward,[0.,1.,0.]);hinge/=np.linalg.norm(hinge)
axis=np.linalg.solve(world[57][:3,:3],hinge);axis/=np.linalg.norm(axis)
def left_rest_pose():
 pose=[v.copy() for v in neutral]
 h,k,a=(world[i][:3,3] for i in (48,49,51))
 l1=np.linalg.norm(k-h);l2=np.linalg.norm(a-k)
 target=a+left*LEFT_REST_SHIFT
 direction=target-h;distance=np.linalg.norm(direction);direction/=distance
 assert abs(l1-l2)<distance<l1+l2
 along=(l1*l1-l2*l2+distance*distance)/(2*distance)
 bend=k-h-direction*np.dot(k-h,direction);bend/=np.linalg.norm(bend)
 newknee=h+direction*along+bend*math.sqrt(max(0,l1*l1-along*along))
 thigh=rotation(between(k-h,newknee-h))@rotation(quaternion(world[48][:3,:3],neutral[48][4:8]))
 shin=rotation(between(a-k,target-newknee))@rotation(quaternion(world[49][:3,:3],neutral[49][4:8]))
 parent=rotation(quaternion(world[0][:3,:3],neutral[0][4:8]))
 foot=rotation(quaternion(world[51][:3,:3],neutral[51][4:8]))
 pose[48][4:8]=quaternion(parent.T@thigh,neutral[48][4:8])
 pose[49][4:8]=quaternion(thigh.T@shin,neutral[49][4:8])
 pose[51][4:8]=quaternion(shin.T@foot,neutral[51][4:8])
 actual=fk(pose)
 assert np.linalg.norm(actual[51][:3,3]-target)<2e-5
 assert abs(np.linalg.norm(actual[49][:3,3]-actual[48][:3,3])-l1)<2e-5
 assert abs(np.linalg.norm(actual[51][:3,3]-actual[49][:3,3])-l2)<2e-5
 return pose
rest=left_rest_pose()
def brake_pose(reach,press):
 if reach==0 and press==0:return [v.copy() for v in rest]
 pose=[v.copy() for v in rest]
 target=ankle+left*(BRAKE_REACH*reach)+np.array([0.,.005*reach+.015*math.sin(math.pi*reach),0.])
 direction=target-hip;distance=np.linalg.norm(direction);direction/=distance
 assert abs(length1-length2)<distance<length1+length2
 along=(length1*length1-length2*length2+distance*distance)/(2*distance)
 bend=knee-hip-direction*np.dot(knee-hip,direction);bend/=np.linalg.norm(bend)
 newknee=hip+direction*along+bend*math.sqrt(max(0,length1*length1-along*along))
 thigh=rotation(between(knee-hip,newknee-hip))@rotation(quaternion(world[54][:3,:3],neutral[54][4:8]))
 shin=rotation(between(ankle-knee,target-newknee))@rotation(quaternion(world[55][:3,:3],neutral[55][4:8]))
 parent=rotation(quaternion(world[0][:3,:3],neutral[0][4:8]))
 angle=-math.radians(12)*press
 ankle_rotation=rotation((math.cos(angle/2),*(axis*math.sin(angle/2))))
 foot=rotation(quaternion(world[57][:3,:3],neutral[57][4:8]))@ankle_rotation
 pose[54][4:8]=quaternion(parent.T@thigh,neutral[54][4:8])
 pose[55][4:8]=quaternion(thigh.T@shin,neutral[55][4:8])
 pose[57][4:8]=quaternion(shin.T@foot,neutral[57][4:8])
 actual=fk(pose)
 assert np.linalg.norm(actual[57][:3,3]-target)<2e-5
 assert abs(np.linalg.norm(actual[55][:3,3]-actual[54][:3,3])-length1)<2e-5
 assert abs(np.linalg.norm(actual[57][:3,3]-actual[55][:3,3])-length2)<2e-5
 return pose
blocks=83;total=blocks*frames;header=bytearray(source[:84]);struct.pack_into('<I',header,4,total);struct.pack_into('<f',header,60,(total-1)/24)
newids=84+total*4;newtransforms=newids+count;end=newtransforms+total*count*56
struct.pack_into('<5I',header,64,84,newids,newtransforms,end,end+total*12)
with zipfile.ZipFile(root/'releases/ATS-VisibleDriverBody-Mustang-0.4.0-throttle-test.scs') as previous:
 gas=previous.read('model/spf_stock_driver/steering_throttle.pma')
gas_offset=struct.unpack_from('<I',gas,72)[0]
trackdata=bytearray(gas[gas_offset:])
# Keep gas/steering exact; only the left resting leg is changed in these blocks.
for frame in range(1551):
 for bone in (48,49,51):
  offset=(frame*count+bone)*56
  trackdata[offset:offset+56]=struct.pack('<14f',*rest[bone])
poses=[brake_pose(step/16,0) for step in range(17)]+[brake_pose(1,step/32) for step in range(33)]
assert np.array_equal(poses[0],rest)
for pose in poses:
 for frame in range(frames):
  for bone in range(count):
   start=transforms+((frame if bone in arms else 0)*count+bone)*56
   track=source[start:start+56]
   if bone in (48,49,51,54,55,57):track=struct.pack('<14f',*pose[bone])
   trackdata.extend(track)
data=header+struct.pack('<%df'%total,*([1/24]*total))+mapping+trackdata
assert len(data)==end and abs(struct.unpack_from('<f',data,60)[0]-(total-1)/24)<1e-5
for frame in range(1551):
 for bone in range(count):
  if bone in (48,49,51):continue
  offset=(frame*count+bone)*56
  assert data[newtransforms+offset:newtransforms+offset+56]==gas[gas_offset+offset:gas_offset+offset+56]
for block in range(blocks):
 for frame in range(frames):
  for bone in range(count):
   start=newtransforms+((block*frames+frame)*count+bone)*56;track=data[start:start+56]
   values=struct.unpack('<14f',track);assert all(math.isfinite(v) for v in values) and abs(sum(v*v for v in values[4:8])-1)<1e-4
   if bone in (48,49,51):assert track==struct.pack('<14f',*rest[bone])
   if bone not in (48,49,51,54,55,57):
    ref=transforms+((frame if bone in arms else 0)*count+bone)*56;assert track==source[ref:ref+56]
hover=fk(poses[17]);pressed=fk(poses[-1]);assert pressed[58][1,3]<hover[58][1,3]
archive=root/'releases/ATS-VisibleDriverBody-Mustang-0.5.3-brake-reach-test.scs'
with zipfile.ZipFile(root/'releases/ATS-VisibleDriverBody-Mustang-0.4.0-throttle-test.scs') as before,zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_STORED) as out:
 for name in before.namelist():
  if name=='model/spf_stock_driver/steering_throttle.pma':continue
  block=before.read(name)
  if name=='unit/hookup/vehicle/spf_stock_driver.sii':block=block.replace(b'steering_throttle.pma',b'steering_pedals.pma')
  if name=='manifest.sii':block=block.replace(b'0.4.0 throttle test',b'0.5.3 brake reach test')
  out.writestr(name,block)
 out.writestr('model/spf_stock_driver/steering_pedals.pma',data)
report={'frames':total,'blocks':blocks,'duration_seconds':(total-1)/24,'brake_reach_left_m':BRAKE_REACH,'left_rest_shift_m':LEFT_REST_SHIFT,'left_leg_static_all_frames':True,'brake_hover_lift_m':.005,'transition_peak_lift_m':.0175,'ankle_press_degrees':12,'modified_brake_bones':[54,55,57],'modified_static_left_bones':[48,49,51],'gas_except_static_left_tracks_byte_exact':True,'steering_tracks_byte_exact':True,'other_bones_unchanged':True,'leg_lengths_preserved':True,'ankle_target_error_under_m':.00002,'brake_toe_height_delta_m':float(pressed[58][1,3]-hover[58][1,3]),'runtime_validated':False}
(root/'brake_reach_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
