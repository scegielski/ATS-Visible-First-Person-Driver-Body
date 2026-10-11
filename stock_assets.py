import struct,zlib
from pathlib import Path
M=(1<<64)-1;K0=0xc3a5c85c97cb3127;K1=0xb492b66fbe98f273;K2=0x9ae16a3b2f90404f;K3=0xc949d7c7509e6557
u=lambda x:x&M
r=lambda x,n:u((u(x)>>n)|(u(x)<<(64-n))) if n else u(x)
mix=lambda x:u(x)^(u(x)>>47)
def h16(a,b):
 a=u((u(a)^u(b))*0x9ddfea08eb382d69);a^=a>>47;b=u((u(b)^a)*0x9ddfea08eb382d69);b^=b>>47;return u(b*0x9ddfea08eb382d69)
def hashpath(path):
 s=path.encode();n=len(s);f=lambda i:struct.unpack_from('<Q',s,i)[0]
 if n<=16:
  if n>8:return h16(f(0),r(f(n-8)+n,n))^f(n-8)
  if n>=4:return h16(n+(struct.unpack_from('<I',s)[0]<<3),struct.unpack_from('<I',s,n-4)[0])
  if n:return u(mix(u((s[0]+(s[n>>1]<<8))*K2)^u((n+(s[-1]<<2))*K3))*K2)
  return K2
 if n<=32:
  a=u(f(0)*K1);b=f(8);c=u(f(n-8)*K2);d=u(f(n-16)*K0)
  return h16(r(a-b,43)+r(c,30)+d,a+r(b^K3,20)-c+n)
 if n<=64:
  z=f(24);a=u(f(0)+u((n+f(n-16))*K0));b=r(a+z,52);c=r(a,37);a=u(a+f(8));c=u(c+r(a,7));a=u(a+f(16));vf=u(a+z);vs=u(b+r(a,31)+c)
  a=u(f(16)+f(n-32));z=f(n-8);b=r(a+z,52);c=r(a,37);a=u(a+f(n-24));c=u(c+r(a,7));a=u(a+f(n-16));wf=u(a+z);ws=u(b+r(a,31)+c)
  return u(mix(u(mix(u(u(vf+ws)*K2+u(wf+vs)*K0))*K0+vs))*K2)
 def weak(o,a,b):
  w,x,y,z=[f(o+i) for i in [0,8,16,24]];a=u(a+w);b=r(b+a+z,21);c=a;a=u(a+x+y);b=u(b+r(a,44));return u(a+z),u(b+c)
 x=f(n-40);y=u(f(n-16)+f(n-56));z=h16(f(n-48)+n,f(n-24));v=weak(n-64,n,z);w=weak(n-32,y+K1,x);x=u(x*K1+f(0))
 for o in range(0,(n-1)&~63,64):
  x=u(r(x+y+v[0]+f(o+8),37)*K1);y=u(r(y+v[1]+f(o+48),42)*K1);x^=w[1];y=u(y+v[0]+f(o+40));z=u(r(z+w[0],33)*K1);v=weak(o,u(v[1]*K1),x+w[0]);w=weak(o+32,z+w[1],y+f(o+16));z,x=x,z
 return h16(h16(v[0],w[0])+u(mix(y)*K1)+z,h16(v[1],w[1])+x)
class Archive:
 def __init__(self,path):
  self.file=Path(path).open('rb');h=self.file.read(52);count,es,words,ms=struct.unpack_from('<4I',h,12);eo,mo=struct.unpack_from('<2Q',h,28)
  self.file.seek(eo);entries=zlib.decompress(self.file.read(es));self.file.seek(mo);self.meta=zlib.decompress(self.file.read(ms));self.entries={}
  for i in range(count):
   key,mi,parts,flags=struct.unpack_from('<QIHH',entries,i*16);self.entries[key]=(mi,parts,flags)
 def read(self,path):
  entry=self.entries.get(hashpath(path))
  if entry is None:return None
  mi,parts,flags=entry
  for i in range(parts):
   desc=struct.unpack_from('<I',self.meta,(mi+i)*4)[0]
   if desc>>24 & 128:
    a,b,_,block=struct.unpack_from('<4I',self.meta,(desc&0xffffff)*4);self.file.seek(block*16);data=self.file.read(a&0xffffff);codec=(a>>24)&0xf0
    if codec not in [0,16]:return None
    data=zlib.decompress(data) if codec==16 else data;assert len(data)==(b&0xffffff);return data
  return None
if __name__=='__main__':
 game=Path('C:/Program Files (x86)/Steam/steamapps/common/American Truck Simulator')
 for name in ['base.scs','base_share.scs','base_vehicle.scs','dlc_rt_ford.scs','def.scs']:
  arc=Archive(game/name)
  for path in ['asset/character/human/driver','asset/character/human','def/vehicle']:
   data=arc.read(path)
   if data:print(name,path,repr(data[:6000]))
