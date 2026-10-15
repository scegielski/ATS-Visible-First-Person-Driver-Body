from pathlib import Path
import struct,numpy as np,json
root=Path(__file__).resolve().parents[2];mesh=(root/'stock-reference/driver.pmg').read_bytes();anim=(root/'stock-reference/ford_mustang_67_ani.pma').read_bytes()
def quat(q):
 w,x,y,z=q
 return np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],[2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],[2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])
def matrix(values,mode):
 a=quat(values[:4]);b=quat(values[4:8]);m=np.eye(4)
 m[:3,:3]={'b':b,'ab':a@b,'aba':a@b@a.T,'ba':b@a}[mode]@np.diag(values[11:14]);m[:3,3]=values[8:11];return m
bones=[]
for i in range(61):
 o=112+i*200;bones.append((np.array(struct.unpack_from('<16f',mesh,o+8)).reshape(4,4),np.array(struct.unpack_from('<16f',mesh,o+72)).reshape(4,4),struct.unpack_from('<14f',mesh,o+136),struct.unpack_from('<I',mesh,o+196)[0]))
for mode in ['b','ab','aba','ba']:
 world=[]
 for bind,inv,values,parent in bones:
  m=matrix(values,mode);world.append(m if parent==255 else world[parent]@m)
 print(mode,'bind reconstruction max error',max(float(np.max(np.abs(world[i]-bones[i][0]))) for i in range(61)))

assert max(float(np.max(np.abs((matrix(values,'b') if parent==255 else bones[parent][0]@matrix(values,'b'))-bind))) for bind,inv,values,parent in bones)<1e-5
frame=23;world=[]
for i,(_,_,_,parent) in enumerate(bones):
 values=struct.unpack_from('<14f',anim,333+(frame*61+i)*56);m=matrix(values,'b');world.append(m if parent==255 else world[parent]@m)
for i in [0,7,28,47,51,57]:print('posed joint',i,world[i][:3,3])
