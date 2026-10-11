"""Export selected candidate functions for manual control-flow/ABI review."""
from pathlib import Path
import sys,json
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'work/discovery-deps'))
import pefile
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
from discover_animation_hook import DEFAULT
pe=pefile.PE(str(DEFAULT)); base=pe.OPTIONAL_HEADER.ImageBase
report=json.loads((ROOT/'work/animation_hook_candidates.json').read_text())
selected=[r for r in report['references'] if '/asset/character/human/driver/' in r['anchor'] or 'Failed to load animation %s onto' in r['anchor'] or 'Model animation' in r['anchor']]
decoder=Cs(CS_ARCH_X86,CS_MODE_64)
lines=['STATIC CANDIDATES ONLY: unwind entries may describe split function fragments. No ABI is inferred.']
seen=set()
for item in selected:
    if not item['function_start'] or item['function_start'] in seen:continue
    seen.add(item['function_start']); start=int(item['function_start'],16); end=int(item['function_end'],16)
    lines.extend(['',item['anchor'],f'Candidate range {start:#x}..{end:#x}'])
    for ins in decoder.disasm(pe.get_data(start-base,end-start),start):
        lines.append(f'{ins.address:#x} {ins.mnemonic:8} {ins.op_str}')
(ROOT/'work/animation_candidate_disassembly.txt').write_text('\n'.join(lines))
print(f'Exported {len(seen)} candidate ranges for review')
