"""Build an experimental private arm clip; leave the seated-hold package intact."""
from pathlib import Path
import json,struct,zipfile
root=Path(__file__).resolve().parent
source=(root/'stock-reference/ford_mustang_67_ani.pma').read_bytes()
bones=json.loads((root/'steering_integration_audit.json').read_text())['driver_bones']
arms={4,25}
for bone in bones:
 if bone['parent'] in arms:arms.add(bone['index'])
assert {5,6,7,26,27,28}.issubset(arms) and not {0,1,2,3,47}.intersection(arms)
frames,count=struct.unpack_from('<2I',source,4)
times,ids,transforms,end,expanded=struct.unpack_from('<5I',source,64)
clip=bytearray(source)
for frame in range(frames):
 for track,bone in enumerate(source[ids:transforms]):
  if bone not in arms:
   dest=transforms+(frame*count+track)*56
   start=transforms+track*56
   clip[dest:dest+56]=source[start:start+56]
assert clip[transforms:transforms+count*56]==source[transforms:transforms+count*56]
assert clip!=source and len(clip)==len(source)
archive=root/'releases/ATS-VisibleDriverBody-Mustang-0.3.0-steering-test.scs'
with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_STORED) as out:
 for path in sorted((root/'package').rglob('*')):
  if not path.is_file():continue
  name=path.relative_to(root/'package').as_posix();data=path.read_bytes()
  if name=='unit/hookup/vehicle/spf_stock_driver.sii':data=data.replace(b'/seat_hold.pma',b'/steering_arms.pma')
  if name=='manifest.sii':data=data.replace(b'0.2.4',b'0.3.0 steering test')
  out.writestr(name,data)
 out.writestr('model/spf_stock_driver/steering_arms.pma',clip)
report={'frames':frames,'bones':count,'animated_arm_bones':sorted(arms),'neutral_frame_preserved':True,'other_tracks_frozen_at_neutral':True,'runtime_validated':False}
(root/'steering_prototype_validation.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report));print(archive.name)
