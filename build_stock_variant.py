"""Create the headless stock driver and engine-driven seated hold animation.
Run after converting tools/base/model/spf_stock_driver/mount.{pim,pit}.
Only private mod paths are written; stock game archives remain unchanged.
"""
from pathlib import Path
import struct,re,shutil,json
from stock_assets import Archive,hashpath
root=Path(__file__).parent;stage=root/'package';dest=stage/'model/spf_stock_driver';dest.mkdir(parents=True,exist_ok=True)
game=Path('C:/Program Files (x86)/Steam/steamapps/common/American Truck Simulator')
arc=Archive(game/'base_share.scs');base='asset/character/human/driver/man/'
original=arc.read(base+'driver.pmg');mesh=bytearray(original);removed=[];hands=[]
assert struct.unpack_from('<6I',mesh)==(0x506d6715,3,3,61,4,0)
for piece in range(3):
 o=12384+piece*100;ic,vc=struct.unpack_from('<2I',mesh,o);start=struct.unpack_from('<I',mesh,o+64)[0];io=struct.unpack_from('<I',mesh,o+96)[0];head=set();hand=set()
 for v in range(vc):
  ids=mesh[start+v*60+52:start+v*60+56];weights=mesh[start+v*60+56:start+v*60+60];assert sum(weights)==255
  if any(b==47 and w for b,w in zip(ids,weights)):head.add(v)
  if any((b==7 or b==28 or 8<=b<=22 or 29<=b<=43) and w for b,w in zip(ids,weights)):hand.add(v)
 count=hand_count=0
 for index in range(0,ic,3):
  face=struct.unpack_from('<3H',mesh,io+index*2);assert max(face)<vc
  if any(v in head for v in face):
   assert not any(v in hand for v in face);struct.pack_into('<3H',mesh,io+index*2,0,0,0);count+=1
  elif any(v in hand for v in face):hand_count+=1
 removed.append(count);hands.append(hand_count)
assert removed==[2258,46,0] and hands[0]==2114
assert mesh[112:12312]==original[112:12312] # skeleton untouched
(dest/'driver.pmg').write_bytes(mesh)
chars='\x000123456789abcdefghijklmnopqrstuvwxyz_'
def token(s):return sum(chars.index(c)*38**i for i,c in enumerate(s))
pmd=bytearray(arc.read(base+'driver.pmd'));struct.pack_into('<Q',pmd,64,token('default'));struct.pack_into('<Q',pmd,104,token('default'));(dest/'driver.pmd').write_bytes(pmd)
pma=arc.read(base+'ford_mustang_67_ani.pma');assert len(pma)==333+47*61*56
header=bytearray(pma[:84]);struct.pack_into('<I',header,4,2);struct.pack_into('<5I',header,64,84,92,153,6985,7009)
frame=pma[333:333+61*56];pose=bytes(header)+struct.pack('<2f',1.,1.)+pma[272:333]+frame*2;assert len(pose)==6985
assert struct.unpack_from('<5I',pose,64)==(84,92,153,6985,7009)
assert list(pose[92:153])==list(range(61))
assert pose[153:153+61*56]==frame and pose[153+61*56:]==frame
(dest/'seat_hold.pma').write_bytes(pose)
import runpy
runpy.run_path(str(root/'make_locator_only_mount.py'))
materials=[];textures=set()
for path in sorted(set(re.findall(rb'/automat/[a-z0-9/]+\.mat',bytes(pmd)))):
 path=path.decode().lstrip('/');data=arc.read(path);assert data;materials.append(path)
 for texture in re.findall(rb'"(/[^"\r\n]+\.tobj)"',data):
  path=texture.decode().lstrip('/');assert hashpath(path) in arc.entries; textures.add(path)
assert textures
report={'stock_skeleton_bones':61,'stock_animation_frames':47,'seated_hold_frames':2,'removed_head_triangles_by_piece':removed,'retained_hand_triangles_by_piece':hands,'stock_materials_verified':materials,'stock_texture_objects_verified':sorted(textures),'skeleton_preserved_byte_exact':True,'animation_architecture':'animated_hookup with neutral seated hold, not rigid baked geometry','steering_synchronization':False,'in_game_verified':False}
(root/'stock_driver_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
