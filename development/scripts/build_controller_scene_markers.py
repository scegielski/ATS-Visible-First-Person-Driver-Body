"""Build solid controller axes as a separate private animated accessory."""
from pathlib import Path
import struct,zipfile,json,re
root=Path(__file__).resolve().parents[2]
source=Path('C:/Users/scegi/OneDrive/Documents/American Truck Simulator/mod/ATS-VisibleDriverBody-Mustang-0.1.0.scs')
destination=root/'releases/ATS-VisibleDriverBody-Mustang-0.18.0-radio-interaction-box-test.scs'
chars='\x000123456789abcdefghijklmnopqrstuvwxyz_'
def token(s):return sum(chars.index(c)*38**i for i,c in enumerate(s))
with zipfile.ZipFile(source) as z:
 mesh=bytearray(z.read('model/spf_stock_driver/driver.pmg'));descriptor=z.read('model/spf_stock_driver/driver.pmd')
 for bone in (7,28,44,45,48,49,50,51,52,53,54,55,56,57,58,59,60,46):struct.pack_into('<16f',mesh,112+bone*200+72,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)
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
 # Visible ignition and right-hand collision wireframes use private marker bones.
 usedVertices={};usedIndices={}
 for piece,bone,half in [(0,44,(.04,.04,.04)),(1,49,(.06,.05,.05)),(2,48,(.04,.035,.055)),(2,45,(.04,.035,.055)),(0,50,(.07,.07,.05)),(1,51,(.07,.05,.05)),(2,52,(.07,.07,.05)),(1,55,(.06,.06,.06)),(0,56,(.06,.06,.06)),(2,57,(.06,.06,.06)),(1,58,(.06,.06,.06)),(0,59,(.04,.04,.04)),(1,60,(.04,.04,.04)),(1,46,(.07,.04,.04))]:
  o=12384+piece*100;ic,vc=struct.unpack_from('<2I',mesh,o);vo=struct.unpack_from('<I',mesh,o+64)[0];io=struct.unpack_from('<I',mesh,o+96)[0];vertices=[];faces=[];vertexStart=usedVertices.get(piece,16);indexStart=usedIndices.get(piece,72)
  for axis in range(3):
   others=[d for d in range(3) if d!=axis]
   for signs in range(4):
    center=[0.,0.,0.]
    for k,d in enumerate(others):center[d]=half[d]*(1 if signs&(1<<k) else -1)
    base=vertexStart+len(vertices)
    for corner in range(8):
     point=[center[d]+(.0015 if corner&(1<<d) else -.0015) for d in range(3)];point[axis]=half[axis]*(1 if corner&(1<<axis) else -1);vertices.append(point)
    faces.extend(base+i for n,i in enumerate([0,2,3,0,3,1,4,5,7,4,7,6,0,1,5,0,5,4,2,6,7,2,7,3,0,4,6,0,6,2,1,3,7,1,7,5]) if n//12!=2-axis)
  assert vc>=vertexStart+len(vertices) and ic>=indexStart+len(faces), (piece,vc,vertexStart+len(vertices),ic,indexStart+len(faces))
  for k,point in enumerate(vertices):v=vertexStart+k;struct.pack_into('<3f',mesh,vo+v*60,*point);struct.pack_into('<3f',mesh,vo+v*60+12,0,1,0);mesh[vo+v*60+52:vo+v*60+60]=bytes([bone,0,0,0,255,0,0,0])
  struct.pack_into('<'+str(len(faces))+'H',mesh,io+indexStart*2,*faces)
  usedVertices[piece]=vertexStart+len(vertices);usedIndices[piece]=indexStart+len(faces)
 # Solid beams occupy separate marker bones and point down controller local -Z.
 for piece,bone in [(0,53),(2,54)]:
  o=12384+piece*100;ic,vc=struct.unpack_from('<2I',mesh,o);vo=struct.unpack_from('<I',mesh,o+64)[0];io=struct.unpack_from('<I',mesh,o+96)[0];base=usedVertices[piece];indexStart=usedIndices[piece]
  assert base+8<=vc and indexStart+36<=ic
  for corner in range(8):
   point=[(.0015 if corner&(1<<d) else -.0015) if d<2 else (.5 if corner&4 else -.5) for d in range(3)];v=base+corner;struct.pack_into('<3f',mesh,vo+v*60,*point);struct.pack_into('<3f',mesh,vo+v*60+12,0,1,0);mesh[vo+v*60+52:vo+v*60+60]=bytes([bone,0,0,0,255,0,0,0])
  faces=[base+i for i in [0,2,3,0,3,1,4,5,7,4,7,6,0,1,5,0,5,4,2,6,7,2,7,3,0,4,6,0,6,2,1,3,7,1,7,5]];struct.pack_into('<36H',mesh,io+indexStart*2,*faces);usedVertices[piece]=base+8;usedIndices[piece]=indexStart+36
 # Geometry labels share each target bone, so visibility and transforms stay identical.
 font={
 'H':['10001','10001','10001','11111','10001','10001','10001'],
 'O':['01110','10001','10001','10001','10001','10001','01110'],
 'R':['11110','10001','10001','11110','10100','10010','10001'],
 'N':['10001','11001','11001','10101','10011','10011','10001'],
 'I':['11111','00100','00100','00100','00100','00100','11111'],
 'G':['01110','10001','10000','10111','10001','10001','01110'],
 'D':['11110','10001','10001','10001','10001','10001','11110'],
 'P':['11110','10001','10001','11110','10000','10000','10000'],
 'W':['10001','10001','10001','10101','10101','11011','10001'],
 'S':['01111','10000','10000','01110','00001','00001','11110'],
 'A':['01110','10001','10001','11111','10001','10001','10001'],
 'L':['10000','10000','10000','10000','10000','10000','11111'],
 'E':['11111','10000','10000','11110','10000','10000','11111'],
 'F':['11111','10000','10000','11110','10000','10000','10000'],
 'C':['01111','10000','10000','10000','10000','10000','01111'],
 'M':['10001','11011','10101','10101','10001','10001','10001'],
 ' ':['00000']*7,
 'T':['11111','00100','00100','00100','00100','00100','00100']}
 for piece,bone,word,halfY,halfZ in [(0,44,'IGNITION',.04,.04),(1,49,'HORN',.05,.05),(0,50,'LEFT',.07,.05),(1,51,'CENTER',.05,.05),(2,52,'RIGHT',.07,.05),(1,55,'SEAT',.06,.06),(0,56,'L WIN',.06,.06),(2,57,'R WIN',.06,.06),(1,58,'WHEEL',.06,.06),(0,59,'LIGHTS',.04,.04),(1,60,'WIPERS',.04,.04),(1,46,'RADIO',.04,.04)]:
  o=12384+piece*100;ic,vc=struct.unpack_from('<2I',mesh,o);vo=struct.unpack_from('<I',mesh,o+64)[0];io=struct.unpack_from('<I',mesh,o+96)[0];vertexStart=usedVertices[piece];indexStart=usedIndices[piece];vertices=[];faces=[];pixel=.0035
  for letter,char in enumerate(word):
   glyph=font[char];taken=set()
   for row,line in enumerate(glyph):
    for col,value in enumerate(line):
     if value!='1' or (row,col) in taken:continue
     width=1
     while col+width<5 and line[col+width]=='1' and (row,col+width) not in taken:width+=1
     height=1
     while row+height<7 and all(glyph[row+height][c]=='1' and (row+height,c) not in taken for c in range(col,col+width)):height+=1
     taken.update((r,c) for r in range(row,row+height) for c in range(col,col+width))
     x=(letter*6+col-len(word)*3)*pixel;y=halfY+.015+(7-row-height)*pixel;labelZ=halfZ+.002;start=vertexStart+len(vertices);vertices.extend([(x,y,labelZ),(x+pixel*(width-.1),y,labelZ),(x+pixel*(width-.1),y+pixel*(height-.1),labelZ),(x,y+pixel*(height-.1),labelZ)]);faces.extend(start+i for i in [0,1,2,0,2,3,2,1,0,3,2,0])
  assert vc>=vertexStart+len(vertices) and ic>=indexStart+len(faces), (piece,vc,vertexStart+len(vertices),ic,indexStart+len(faces))
  for k,point in enumerate(vertices):v=vertexStart+k;struct.pack_into('<3f',mesh,vo+v*60,*point);struct.pack_into('<3f',mesh,vo+v*60+12,0,0,1);mesh[vo+v*60+52:vo+v*60+60]=bytes([bone,0,0,0,255,0,0,0])
  struct.pack_into('<'+str(len(faces))+'H',mesh,io+indexStart*2,*faces)
  usedVertices[piece]=vertexStart+len(vertices);usedIndices[piece]=indexStart+len(faces)
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


