"""Combine steering with 33 throttle poses in a private, independently seekable clip."""
from pathlib import Path
import struct,json,math,zipfile
import numpy as np
root=Path(__file__).resolve().parents[2]
source=(root/'stock-reference/ford_mustang_67_ani.pma').read_bytes()
bones=json.loads((root/'development/evidence/steering_integration_audit.json').read_text())['driver_bones']
frames,count=struct.unpack_from('<2I',source,4)
assert (frames,count)==(47,61)
_,ids,transforms,_,_=struct.unpack_from('<5I',source,64)
mapping=list(source[ids:transforms]);assert mapping==list(range(61))
arms={4,25}
for bone in bones:
 if bone['parent'] in arms:arms.add(bone['index'])
def rotation(q):
 w,x,y,z=q
 return np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],[2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],[2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])
def multiply(a,b):
 w,x,y,z=a;v,i,j,k=b
 return (w*v-x*i-y*j-z*k,w*i+x*v+y*k-z*j,w*j-x*k+y*v+z*i,w*k+x*j-y*i+z*v)
neutral=[struct.unpack_from('<14f',source,transforms+i*56) for i in range(count)]
world=[]
for bone,values in zip(bones,neutral):
 matrix=np.eye(4);matrix[:3,:3]=rotation(values[4:8])@np.diag(values[11:14]);matrix[:3,3]=values[8:11]
 world.append(matrix if bone['parent']==255 else world[bone['parent']]@matrix)
assert bones[57]['name']=='rgfoot' and bones[58]['parent']==57
forward=world[58][:3,3]-world[57][:3,3]
axis_world=np.cross(forward,np.array([0.,1.,0.]));axis_world/=np.linalg.norm(axis_world)
axis=np.linalg.solve(world[57][:3,:3],axis_world);axis/=np.linalg.norm(axis)
levels=33;total=frames*levels;header=bytearray(source[:84]);struct.pack_into('<I',header,4,total)
struct.pack_into('<f',header,60,(total-1)/24)
times=84;newids=times+total*4;newtransforms=newids+count;end=newtransforms+total*count*56
struct.pack_into('<5I',header,64,times,newids,newtransforms,end,end+total*12)
data=bytearray(header+struct.pack('<%df'%total,*([1/24]*total))+bytes(mapping))
min_height=world[58][1,3]
for level in range(levels):
 angle=-math.radians(12)*level/(levels-1)
 delta=(math.cos(angle/2),*(axis*math.sin(angle/2)))
 foot=list(neutral[57]);foot[4:8]=multiply(foot[4:8],delta)
 for frame in range(frames):
  for bone in range(count):
   start=transforms+(frame*count+bone)*56 if bone in arms else transforms+bone*56
   block=source[start:start+56]
   if bone==57 and level:block=struct.pack('<14f',*foot)
   data.extend(block)
assert len(data)==end
assert abs(struct.unpack_from('<f',data,60)[0]-(total-1)/24)<0.00001
assert struct.unpack_from('<5I',data,64)==(84,84+total*4,84+total*4+count,end,end+total*12)
# Every throttle level retains identical arms and all other frozen bones.
for level in range(levels):
 for frame in range(frames):
  for bone in range(count):
   offset=newtransforms+((level*frames+frame)*count+bone)*56
   actual=data[offset:offset+56]
   reference=source[transforms+((frame if bone in arms else 0)*count+bone)*56:transforms+((frame if bone in arms else 0)*count+bone+1)*56]
   if bone!=57 or level==0:assert actual==reference
   values=struct.unpack('<14f',actual);assert all(math.isfinite(v) for v in values)
   assert abs(sum(v*v for v in values[4:8])-1)<1e-4
end_foot=rotation(multiply(neutral[57][4:8],(math.cos(-math.radians(12)/2),*(axis*math.sin(-math.radians(12)/2)))))
toe=world[bones[57]['parent']]@np.block([[end_foot@np.diag(neutral[57][11:14]),np.array(neutral[57][8:11])[:,None]],[np.zeros((1,3)),np.ones((1,1))]])
pressed_toe=(toe@np.array([*neutral[58][8:11],1]))[:3]
assert pressed_toe[1]<world[58][1,3],'Positive gas must lower toes in avatar coordinates'
archive=root/'releases/ATS-VisibleDriverBody-Mustang-0.4.0-throttle-test.scs'
with zipfile.ZipFile(root/'releases/ATS-VisibleDriverBody-Mustang-0.3.0-steering-test.scs') as before,zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_STORED) as out:
 for name in before.namelist():
  if name=='model/spf_stock_driver/steering_arms.pma':continue
  block=before.read(name)
  if name=='unit/hookup/vehicle/spf_stock_driver.sii':block=block.replace(b'steering_arms.pma',b'steering_throttle.pma')
  if name=='manifest.sii':block=block.replace(b'0.3.0 steering test',b'0.4.0 throttle test')
  out.writestr(name,block)
 out.writestr('model/spf_stock_driver/steering_throttle.pma',data)
report={'frames':total,'steering_frames_per_level':frames,'throttle_levels':levels,'ankle_degrees':12,'bone_modified':57,'toe_height_delta_m':float(pressed_toe[1]-world[58][1,3]),'arm_tracks_preserved_all_levels':True,'other_tracks_preserved':True,'duration_seconds':(total-1)/24,'runtime_validated':False}
(root/'development/evidence/throttle_prototype_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
