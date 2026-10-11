"""Build a locator-only PMG/PMD, matching SCS v21/v4 empty-mount layout.
No third-party model is copied; locator transform and hookup are our own.
The official converter requires a Material block in an empty PIT, so this
minimal writer removes the dummy geometry after conversion.
"""
from pathlib import Path
import struct
root=Path(__file__).parent;dest=root/'package/model/spf_stock_driver'
chars='\x000123456789abcdefghijklmnopqrstuvwxyz_'
def token(s):return sum(chars.index(c)*38**i for i,c in enumerate(s))
header=bytearray(112)
struct.pack_into('<6I',header,0,0x506d6715,0,1,0,0,1)
# SCS empty-geometry bounds, also present in the installed animated passenger mount.
struct.pack_into('<6I',header,48,*([0x7f7fffff]*3+[0xff7fffff]*3))
text=b'spf.stock_driver\0';text+=b'\0'*((-len(text))%4)
struct.pack_into('<10I',header,72,112,112,136,180,180,len(text),180+len(text),0,180+len(text),0)
part=struct.pack('<Q4I',token('body'),0,0,1,0)
carpet=(.7947843670845032,-.7508626580238342,-.9037798643112183)
interior=(-.3847368657588959,1.0660456418991089,.1469421684741974)
position=(interior[0]+carpet[0],-interior[1]-carpet[1],interior[2]+carpet[2])
locator=struct.pack('<Q8fI',token('stock_body'),*position,1.,0.,0.,1.,0.,0)
mesh=bytes(header)+part+locator+text
assert len(mesh)==180+len(text);(dest/'mount.pmg').write_bytes(mesh)
# One default look/variant, one visible part, no materials or pieces.
header=struct.pack('<16I',4,0,1,0,1,1,1,4,0,64,72,80,104,88,108,108)
tables=struct.pack('<2Q2I',token('default'),token('default'),0,1)
attribute=struct.pack('<Q2I',token('visible'),0,0)
model=header+tables+attribute+struct.pack('<I',1)
assert len(model)==108;(dest/'mount.pmd').write_bytes(model)
print('Wrote locator-only mount:',len(mesh),'PMG bytes,',len(model),'PMD bytes')
