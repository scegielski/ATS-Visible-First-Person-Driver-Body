"""PMA v5 regression: validate header pointers against physical arrays."""
from pathlib import Path
import struct,math
root=Path(__file__).parent
def validate(data):
 assert struct.unpack_from('<I',data)[0]==5
 frames,bones=struct.unpack_from('<2I',data,4)
 times,ids,transforms,end,expanded=struct.unpack_from('<5I',data,64)
 assert times==84 and ids==times+frames*4 and transforms==ids+bones
 assert end==transforms+frames*bones*56 and end==len(data)
 assert expanded==end+frames*12
 indices=list(data[ids:transforms]);assert len(set(indices))==bones and max(indices)<61
 for frame in range(frames):
  for bone in range(bones):
   values=struct.unpack_from('<14f',data,transforms+(frame*bones+bone)*56)
   assert all(math.isfinite(v) for v in values)
   assert abs(sum(v*v for v in values[4:8])-1)<1e-4
 return frames,bones,transforms
original=(root/'stock-reference/ford_mustang_67_ani.pma').read_bytes()
pose=(root/'package/model/spf_stock_driver/seat_hold.pma').read_bytes()
assert validate(original)==(47,61,333)
assert validate(pose)==(2,61,153)
assert pose[153:153+61*56]==original[333:333+61*56]
assert pose[153+61*56:]==pose[153:153+61*56]
bad=bytearray(pose);struct.pack_into('<5I',bad,64,84,272,84,92,153)
try:validate(bad)
except AssertionError:pass
else:raise AssertionError('Original bad header was not rejected')
print('PASS: original/corrected clip pointers, rig mapping, neutral frames, invalid-header regression')
