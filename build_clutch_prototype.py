"""Add independent left-ankle clutch poses to the adjusted right pedal/steering clip."""
from pathlib import Path
import struct,math,json,zipfile
import numpy as np
root=Path(__file__).resolve().parent
bones=json.loads((root/'steering_integration_audit.json').read_text())['driver_bones']
before_path=root/'releases/ATS-VisibleDriverBody-Mustang-0.5.3-brake-reach-test.scs'
with zipfile.ZipFile(before_path) as package:base=package.read('model/spf_stock_driver/steering_pedals.pma')
frames,count=struct.unpack_from('<2I',base,4);assert (frames,count)==(3901,61)
_,ids,transforms,_,_=struct.unpack_from('<5I',base,64)
mapping=base[ids:transforms];assert list(mapping)==list(range(61))
def rotation(q):
 w,x,y,z=q
 return np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],[2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],[2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])
def multiply(a,b):
 w,x,y,z=a;v,i,j,k=b
 return (w*v-x*i-y*j-z*k,w*i+x*v+y*k-z*j,w*j-x*k+y*v+z*i,w*k+x*j-y*i+z*v)
pose=[list(struct.unpack_from('<14f',base,transforms+i*56)) for i in range(count)]
def fk(values):
 world=[]
 for bone,v in zip(bones,values):
  local=np.eye(4);local[:3,:3]=rotation(v[4:8])@np.diag(v[11:14]);local[:3,3]=v[8:11]
  world.append(local if bone['parent']==255 else world[bone['parent']]@local)
 return world
world=fk(pose);assert bones[51]['name']=='lffoot' and bones[52]['parent']==51
forward=world[52][:3,3]-world[51][:3,3]
hinge=np.cross(forward,[0.,1.,0.]);hinge/=np.linalg.norm(hinge)
axis=np.linalg.solve(world[51][:3,:3],hinge);axis/=np.linalg.norm(axis)
levels=9;total=frames*levels;header=bytearray(base[:84]);struct.pack_into('<I',header,4,total);struct.pack_into('<f',header,60,(total-1)/24)
newids=84+total*4;newtransforms=newids+count;end=newtransforms+total*count*56
struct.pack_into('<5I',header,64,84,newids,newtransforms,end,end+total*12)
data=bytearray(header+struct.pack('<%df'%total,*([1/24]*total))+mapping)
original_tracks=base[transforms:]
pressed=pose
for level in range(levels):
 segment=bytearray(original_tracks)
 ankle=list(pose[51]);angle=-math.radians(12)*level/(levels-1)
 ankle[4:8]=multiply(ankle[4:8],(math.cos(angle/2),*(axis*math.sin(angle/2))))
 assert abs(sum(v*v for v in ankle[4:8])-1)<1e-4
 if level:
  track=struct.pack('<14f',*ankle)
  for frame in range(frames):
   offset=(frame*count+51)*56;segment[offset:offset+56]=track
 else:assert segment==original_tracks
 # No other bone or right pedal/steering pose changes at any clutch level.
 for frame in range(frames):
  offset=(frame*count+51)*56
  assert segment[frame*count*56:offset]==original_tracks[frame*count*56:offset]
  assert segment[offset+56:(frame+1)*count*56]==original_tracks[offset+56:(frame+1)*count*56]
 data.extend(segment)
 if level==levels-1:pressed=[list(v) for v in pose];pressed[51]=ankle
assert len(data)==end and total<65536
assert abs(struct.unpack_from('<f',data,60)[0]-(total-1)/24)<.0001
pressed_world=fk(pressed)
assert np.linalg.norm(pressed_world[51][:3,3]-world[51][:3,3])<1e-6
assert pressed_world[52][1,3]<world[52][1,3]
archive=root/'releases/ATS-VisibleDriverBody-Mustang-0.6.1-pedal-alignment-test.scs'
with zipfile.ZipFile(before_path) as before,zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as out:
 for name in before.namelist():
  if name=='model/spf_stock_driver/steering_pedals.pma':continue
  block=before.read(name)
  if name=='unit/hookup/vehicle/spf_stock_driver.sii':block=block.replace(b'steering_pedals.pma',b'steering_all_pedals.pma')
  if name=='manifest.sii':block=block.replace(b'0.5.3 brake reach test',b'0.6.1 pedal alignment test')
  out.writestr(name,block)
 out.writestr('model/spf_stock_driver/steering_all_pedals.pma',data)
with zipfile.ZipFile(archive) as verify:assert verify.read('model/spf_stock_driver/steering_all_pedals.pma')==data
report={'frames':total,'right_pedal_blocks':83,'clutch_levels':levels,'duration_seconds':(total-1)/24,'brake_reach_left_m':.14,'left_rest_shift_m':.06,'clutch_ankle_degrees':12,'clutch_bone':51,'left_ankle_position_fixed':True,'steering_and_right_pedals_byte_exact_all_clutch_levels':True,'released_clutch_matches_previous_clip':True,'clutch_toe_height_delta_m':float(pressed_world[52][1,3]-world[52][1,3]),'uncompressed_clip_bytes':len(data),'archive_bytes':archive.stat().st_size,'runtime_validated':False}
(root/'clutch_alignment_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
