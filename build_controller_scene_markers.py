"""Build solid controller axes as a separate private animated accessory."""
from pathlib import Path
import struct,zipfile,json,re
root=Path(__file__).resolve().parent
source=Path('C:/Users/scegi/OneDrive/Documents/American Truck Simulator/mod/ATS-VisibleDriverBody-Mustang-0.1.0.scs')
destination=root/'releases/ATS-VisibleDriverBody-Mustang-0.8.1-colored-scene-markers-test.scs'
chars='\x000123456789abcdefghijklmnopqrstuvwxyz_'
def token(s):return sum(chars.index(c)*38**i for i,c in enumerate(s))
with zipfile.ZipFile(source) as z:
 mesh=bytearray(z.read('model/spf_stock_driver/driver.pmg'));descriptor=z.read('model/spf_stock_driver/driver.pmd')
 for bone in (7,28):struct.pack_into('<16f',mesh,112+bone*200+72,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)
 for axis in range(3):
  o=12384+axis*100;ic,vc=struct.unpack_from('<2I',mesh,o);vo=struct.unpack_from('<I',mesh,o+64)[0];io=struct.unpack_from('<I',mesh,o+96)[0];assert vc>=16 and ic>=72
  mesh[io:io+ic*2]=bytes(ic*2)
  for vertex in range(vc):struct.pack_into('<3f',mesh,vo+vertex*60,0,0,0);mesh[vo+vertex*60+52:vo+vertex*60+60]=bytes([7,0,0,0,255,0,0,0])
  indices=[]
  for side,bone in enumerate((7,28)):
   for corner in range(8):
    point=[.003 if corner&(1<<d) else -.003 for d in range(3)];point[axis]=.12 if corner&(1<<axis) else 0;vertex=side*8+corner
    struct.pack_into('<3f',mesh,vo+vertex*60,*point);struct.pack_into('<3f',mesh,vo+vertex*60+12,0,1,0);mesh[vo+vertex*60+52:vo+vertex*60+60]=bytes([bone,0,0,0,255,0,0,0])
   indices.extend(side*8+i for i in [0,2,3,0,3,1,4,5,7,4,7,6,0,1,5,0,5,4,2,6,7,2,7,3,0,4,6,0,6,2,1,3,7,1,7,5])
  struct.pack_into('<72H',mesh,io,*indices)
 struct.pack_into('<6f',mesh,48,-2,-2,-2,2,2,2)
 old=z.read('model/spf_stock_driver/mount.pmg');position=struct.unpack_from('<8f',old,144)
 text1=b'spf.stock_driver\0';text2=b'spf.ctrl_markers\0';texts=text1+text2;texts+=bytes((-len(texts))%4)
 h=bytearray(112);struct.pack_into('<6I',h,0,0x506d6715,0,1,0,0,2);struct.pack_into('<6I',h,48,*([0x7f7fffff]*3+[0xff7fffff]*3));struct.pack_into('<10I',h,72,112,112,136,224,224,len(texts),224+len(texts),0,224+len(texts),0)
 mount=bytes(h)+struct.pack('<Q4I',token('body'),0,0,2,0)+b''.join(struct.pack('<Q8fI',token(name),*position,offset) for name,offset in [('stock_body',0),('ctrl_marker',len(text1))])+texts
 assert len(mount)==224+len(texts)
 additions={'model/spf_controller_markers/driver.pmg':bytes(mesh),'model/spf_controller_markers/driver.pmd':descriptor,'unit/hookup/vehicle/spf_controller_markers.sii':b'SiiNunit\n{\nanimated_hookup : spf.ctrl_markers {\n model: "/model/spf_controller_markers/driver.pmd"\n animation: "/model/spf_stock_driver/seat_hold.pma"\n}\n}\n'}
 # Replace every material reference, including alternate looks, without changing PMD offsets.
 matches=list(re.finditer(rb'/automat/[^\x00]+?\.mat',descriptor))
 edited=bytearray(descriptor)
 for i,match in enumerate(matches):
  axis=i%3;name=f'/automat/cf/{axis+1:016x}.mat'.encode();assert len(name)==len(match.group());edited[match.start():match.end()]=name
 additions['model/spf_controller_markers/driver.pmd']=bytes(edited)
 for axis,rgb in enumerate([(1,0,0),(0,1,0),(0,0,1)]):
  name=f'automat/cf/{axis+1:016x}.mat'
  additions[name]=(f'effect : "eut2.dif" {{\n diffuse : {{ {rgb[0]}, {rgb[1]}, {rgb[2]} }}\n additional_ambient : 0.5\n texture : "texture_base" {{ source : "/model/spf_controller_markers/white.tobj" }}\n}}\n').encode()
 path=b'/model/spf_controller_markers/white.dds'
 tobj=bytearray((root/'package/model/spf_driver_body/skin.tobj').read_bytes()[:48]);struct.pack_into('<I',tobj,40,len(path));additions['model/spf_controller_markers/white.tobj']=bytes(tobj)+path
 # A 4x4 opaque white RGBA DDS keeps these materials independent of stock textures.
 header=[124,0x100f,4,4,16,0,0]+[0]*11+[32,0x41,0,32,0xff,0xff00,0xff0000,0xff000000,0x1000,0,0,0,0]
 assert len(header)==31
 additions['model/spf_controller_markers/white.dds']=b'DDS '+struct.pack('<31I',*header)+bytes([255]*64)
 with zipfile.ZipFile(destination,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as out:
  for name in z.namelist():
   if name not in additions:out.writestr(name,mount if name=='model/spf_stock_driver/mount.pmg' else z.read(name))
  for name,data in additions.items():out.writestr(name,data)
 with zipfile.ZipFile(destination) as result:assert result.read('model/spf_stock_driver/driver.pmg')==z.read('model/spf_stock_driver/driver.pmg')
print(destination)
