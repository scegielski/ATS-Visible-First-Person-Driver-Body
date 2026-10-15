from pathlib import Path
import math,struct,json
root=Path(__file__).resolve().parents[2]/'tools/base'
model=root/'model/spf_driver_body';model.mkdir(parents=True,exist_ok=True)
# Read the verified PMG v21 attachment transform; pose is authored in interior space.
asset=Path(__file__).resolve().parents[3]/'extracted-ford/vehicle/car/ford_mustang_67/interior/standard.pmg'
b=asset.read_bytes()
assert struct.unpack_from('<I',b)[0]==0x506d6715
chars='\x000123456789abcdefghijklmnopqrstuvwxyz_'
def token(value):
 result=''
 while value: result+=chars[value%38];value//=38
 return result
count=struct.unpack_from('<I',b,20)[0];offset=struct.unpack_from('<I',b,80)[0]
locators={}
for i in range(count):
 o=offset+i*44;name=token(struct.unpack_from('<Q',b,o)[0])
 locators[name]=struct.unpack_from('<8fI',b,o+8)
attachment=locators['carpet'];translation=attachment[:3];scale=attachment[3];q=attachment[4:8]
assert abs(scale-1)<1e-6
assert abs(sum(t*t for t in q)-1)<1e-5
def rotate(vector,quaternion):
 w,x,y,z=quaternion
 ux,uy,uz=vector
 tx=2*(y*uz-z*uy);ty=2*(z*ux-x*uz);tz=2*(x*uy-y*ux)
 return [ux+w*tx+y*tz-z*ty,uy+w*ty+z*tx-x*tz,uz+w*tz+x*ty-y*tx]
inverse=(q[0],-q[1],-q[2],-q[3])
def local_point(point):
 return [v/scale for v in rotate([point[i]-translation[i] for i in range(3)],inverse)]
wheel_x=locators['steering_w'][0]

pieces=[]
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def unit(v):
 length=math.sqrt(sum(x*x for x in v));return [x/length for x in v]
def append_mesh(material,pos,expected,tri):
 # Ensure every face points toward the supplied outward direction, then average
 # actual face normals for smooth shading (including tapered surfaces).
 normals=[[0.,0.,0.] for _ in pos];fixed=[]
 for a,b,c in tri:
  n=cross([pos[b][i]-pos[a][i] for i in range(3)],[pos[c][i]-pos[a][i] for i in range(3)])
  if sum(n[i]*(expected[a][i]+expected[b][i]+expected[c][i]) for i in range(3))<0:
   b,c=c,b;n=[-x for x in n]
  assert sum(x*x for x in n)>1e-18
  fixed.append((a,b,c))
  for vertex in [a,b,c]:
   for i in range(3):normals[vertex][i]+=n[i]
 normals=[unit(n) for n in normals]
 for a,b,c in fixed:
  n=cross([pos[b][i]-pos[a][i] for i in range(3)],[pos[c][i]-pos[a][i] for i in range(3)])
  assert sum(n[i]*(normals[a][i]+normals[b][i]+normals[c][i]) for i in range(3))>0
 pieces.append((material,pos,normals,fixed))
def tube(a,b,radius,material,end_radius=None):
 end_radius=radius*.72 if end_radius is None else end_radius
 d=unit([b[i]-a[i] for i in range(3)]);ref=[0,1,0] if abs(d[1])<.95 else [1,0,0]
 u=unit(cross(d,ref));v=cross(d,u);pos=[];expected=[];tri=[];segments=24
 for t,factor in [(0,.85),(.08,1),(.4,1),(.85,1),(1,.8)]:
  r=(radius*(1-t)+end_radius*t)*factor
  for k in range(segments):
   n=[math.cos(k*math.tau/segments)*u[i]+math.sin(k*math.tau/segments)*v[i] for i in range(3)]
   pos.append([a[i]+(b[i]-a[i])*t+r*n[i] for i in range(3)]);expected.append(n)
 for ring in range(4):
  for k in range(segments):
   j=(k+1)%segments;lo=ring*segments;hi=lo+segments
   tri.extend([(lo+k,lo+j,hi+k),(lo+j,hi+j,hi+k)])
 for ring,point,direction in [(0,a,[-x for x in d]),(4,b,d)]:
  center=len(pos);pos.append(list(point));expected.append(direction)
  for k in range(segments):tri.append((center,ring*segments+k,ring*segments+(k+1)%segments))
 append_mesh(material,pos,expected,tri)
def ellipsoid(center,radii,material):
 pos=[];expected=[];tri=[];segments=24;rings=12
 for ring in range(1,rings):
  theta=math.pi*ring/rings
  for k in range(segments):
   direction=[math.sin(theta)*math.cos(k*math.tau/segments),math.cos(theta),math.sin(theta)*math.sin(k*math.tau/segments)]
   pos.append([center[i]+radii[i]*direction[i] for i in range(3)])
   expected.append(unit([direction[i]/radii[i] for i in range(3)]))
 for ring in range(rings-2):
  for k in range(segments):
   j=(k+1)%segments;lo=ring*segments;hi=lo+segments
   tri.extend([(lo+k,lo+j,hi+k),(lo+j,hi+j,hi+k)])
 for y,ring in [(1,0),(-1,rings-2)]:
  idx=len(pos);pos.append([center[0],center[1]+y*radii[1],center[2]]);expected.append([0,y,0])
  for k in range(segments):tri.append((idx,ring*segments+k,ring*segments+(k+1)%segments))
 append_mesh(material,pos,expected,tri)
# Driver centered on the wheel; seat height/depth follows the stock seat locator.
seat_y=locators['codrv_seat'][1];seat_z=locators['codrv_seat'][2]
for x in [wheel_x-.12,wheel_x+.12]:
 tube((x,seat_y,seat_z),(x,-.43,-.44),.085,'denim')
 tube((x,-.43,-.44),(x,-.69,-.76),.065,'denim')
 ellipsoid((x,-.698,-.84),(.065,.047,.13),'shoe')
 ellipsoid((x,-.43,-.44),(.082,.074,.075),'denim')
ellipsoid((wheel_x,seat_y+.02,seat_z),(.205,.09,.12),'denim')
ellipsoid((wheel_x,-.225,seat_z+.035),(.205,.18,.115),'shirt')
for side in [-1,1]:
 shoulder=(wheel_x+side*.20,-.09,seat_z+.02)
 elbow=(wheel_x+side*.24,-.38,-.20)
 hand=(wheel_x+side*.15,-.28,-.51)
 tube(shoulder,elbow,.06,'shirt');tube(elbow,hand,.045,'skin')
 ellipsoid((hand[0],hand[1]+.018,hand[2]-.012),(.038,.055,.025),'skin')
 # Four curled fingers plus a distinct thumb, around the wheel rim.
 for finger in range(4):
  x=hand[0]+(finger-1.5)*.015
  tube((x,hand[1]+.035,hand[2]-.025),(x,hand[1]+.055,hand[2]-.045),.009,'skin',.008)
  tube((x,hand[1]+.055,hand[2]-.045),(x,hand[1]+.025,hand[2]-.055),.008,'skin',.007)
 tube((hand[0]-side*.032,hand[1],hand[2]),(hand[0]-side*.045,hand[1]+.028,hand[2]-.028),.013,'skin',.010)
interior_positions=[p for _,positions,_,_ in pieces for p in positions]
# Invert attachment rotation for positions and normals, preserving mesh winding.
pieces=[(mat,[local_point(p) for p in positions],[rotate(n,inverse) for n in normals],tri)
        for mat,positions,normals,tri in pieces]
for original,local in zip(interior_positions,[p for _,positions,_,_ in pieces for p in positions]):
 restored=[v*scale+translation[i] for i,v in enumerate(rotate(local,q))]
 assert max(abs(restored[i]-original[i]) for i in range(3))<1e-6
assert seat_z>locators['steering_w'][2] # pelvis behind wheel; feet extend forward (-Z)
assert max(p[1] for p in interior_positions)<0
report={'attachment_position':translation,'attachment_quaternion_wxyz':q,
 'driver_center_x':wheel_x,'seat_position_yz':[seat_y,seat_z],
 'interior_bounds':[[min(p[i] for p in interior_positions) for i in range(3)],
                    [max(p[i] for p in interior_positions) for i in range(3)]],
 'outward_winding_and_unit_normals_verified':True, 'original_geometry_no_external_asset_license_required':True, 'roundtrip_transform_verified':True,'corrected_alignment_in_game_verified':False}
(Path(__file__).resolve().parents[2]/'development/evidence/alignment_validation.json').write_text(json.dumps(report,indent=2))
names=['denim','shoe','shirt','skin'];colors=[(48,63,91),(32,29,27),(55,94,78),(184,137,104)]
for name,rgb in zip(names,colors):
 header=struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,4,4,24,32);(model/(name+'.tga')).write_bytes(header+bytes([rgb[2],rgb[1],rgb[0]])*16)
 (model/(name+'.tobj')).write_text('map 2d '+name+'.tga\naddr clamp_to_edge clamp_to_edge\n')
head='Header { FormatVersion: 5 Source: "SPF independent prototype generator" Type: "Model" Name: "driver_body" }\n'
vc=sum(len(p[1]) for p in pieces);tc=sum(len(p[3]) for p in pieces)
s=head+f'Global {{ VertexCount: {vc} TriangleCount: {tc} MaterialCount: 4 PieceCount: {len(pieces)} PartCount: 1 BoneCount: 0 LocatorCount: 0 Skeleton: "" }}\n'
for n in names:s+=f'Material {{ Alias: "{n}" Effect: "eut2.dif" }}\n'
def stream(tag,fmt,data,extra=''):
 return f'Stream {{ Format: {fmt} Tag: "{tag}" {extra}\n'+''.join(str(i)+' ( '+' '.join(f'{t:.8f}' for t in row)+' )\n' for i,row in enumerate(data))+'}\n'
for i,(mat,pos,norm,tris) in enumerate(pieces):
 s+=f'Piece {{ Index: {i} Material: {names.index(mat)} VertexCount: {len(pos)} TriangleCount: {len(tris)} StreamCount: 4\n'
 s+=stream('_POSITION','FLOAT3',pos)+stream('_NORMAL','FLOAT3',norm)+stream('_UV0','FLOAT2',[(0.5,0.5)]*len(pos),'AliasCount: 1 Aliases: "_TEXCOORD0"')+stream('_RGBA','FLOAT4',[(1,1,1,1)]*len(pos))
 s+='Triangles {\n'+''.join(f'{j} ( {a} {b} {c} )\n' for j,(a,b,c) in enumerate(tris))+'}\n}\n'
s+=f'Part {{ Name: "body" PieceCount: {len(pieces)} LocatorCount: 0 Pieces: '+ ' '.join(str(i) for i in range(len(pieces)))+' Locators: }\n'
(model/'driver_body.pim').write_text(s)
s='Header { FormatVersion: 1 Source: "SPF independent prototype generator" Type: "Trait" Name: "driver_body" }\nGlobal { LookCount: 1 VariantCount: 1 PartCount: 1 MaterialCount: 4 }\nLook { Name: "default"\n'
for n in names:
 s+=f'Material {{ Alias: "{n}" Effect: "eut2.dif" Flags: 0 AttributeCount: 5 TextureCount: 1\n'
 for tag,fmt,vals in [('diffuse','FLOAT3','1.0 1.0 1.0'),('specular','FLOAT3','0.0 0.0 0.0'),('shininess','FLOAT','25.0'),('add_ambient','FLOAT','0.0'),('reflection','FLOAT','0.0')]:s+=f'Attribute {{ Format: {fmt} Tag: "{tag}" Value: ( {vals} ) }}\n'
 s+=f'Texture {{ Tag: "texture[0]:texture_base" Value: "/model/spf_driver_body/{n}" }} }}\n'
s+='}\nVariant { Name: "default" Part { Name: "body" AttributeCount: 1 Attribute { Format: INT Tag: "visible" Value: ( 1 ) } } }\n'
(model/'driver_body.pit').write_text(s)
print('Generated headless seated body:',vc,'vertices,',tc,'triangles')

