"""Export an executable code window with literal and direct-call annotations."""
from pathlib import Path
import sys,struct
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'work/discovery-deps'))
import pefile
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
from capstone.x86 import X86_OP_MEM,X86_OP_IMM,X86_REG_RIP
from discover_animation_hook import DEFAULT
pe=pefile.PE(str(DEFAULT));base=pe.OPTIONAL_HEADER.ImageBase
start=int(sys.argv[1],0);size=int(sys.argv[2],0)
md=Cs(CS_ARCH_X86,CS_MODE_64);md.detail=True
lines=[]
for ins in md.disasm(pe.get_data(start-base,size),start):
    line=f'{ins.address:#x} {ins.mnemonic:8} {ins.op_str}'
    for op in ins.operands:
        if op.type==X86_OP_MEM and op.mem.base==X86_REG_RIP:
            address=ins.address+ins.size+op.mem.disp
            try:
                raw=pe.get_data(address-base,180).split(b'\0')[0]
                if len(raw)>3 and all(32<=v<127 for v in raw):line+=' ; '+raw.decode()
            except Exception:pass
    lines.append(line)
target=ROOT/'work'/f'code_{start:x}.txt';target.write_text('\n'.join(lines))
print(f'Exported {len(lines)} instructions')
print('\n'.join(line for line in lines if ' ; ' in line))
