"""Offline candidate discovery. Never patches the executable or attaches to ATS."""
from pathlib import Path
import sys, re, struct, hashlib, json, bisect
ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'work/discovery-deps'))
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

DEFAULT = Path('C:/Program Files (x86)/Steam/steamapps/common/American Truck Simulator/bin/win_x64/amtrucks.exe')

def discover(path):
    data = path.read_bytes()
    pe = pefile.PE(data=data)
    base = pe.OPTIONAL_HEADER.ImageBase
    anchors = {}
    for section in pe.sections:
        if section.Characteristics & 0x20000000:
            continue
        for match in re.finditer(rb'[\x20-\x7e]{6,}\x00', section.get_data()):
            value = match.group()[:-1].decode('ascii')
            if re.search(r'anim|skeleton|steering|driver|skinning|bone', value, re.I):
                if len(value) < 700:
                    anchors[base + section.VirtualAddress + match.start()] = value
    functions = sorted((base + item.struct.BeginAddress, base + item.struct.EndAddress)
                       for item in getattr(pe, 'DIRECTORY_ENTRY_EXCEPTION', []))
    starts = [item[0] for item in functions]
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    references = []
    for section in pe.sections:
        if not section.Characteristics & 0x20000000:
            continue
        code = section.get_data()
        address = base + section.VirtualAddress
        # Candidate x64 RIP-relative LEA/MOV. Full instruction decode verifies each candidate.
        for match in re.finditer(rb'[\x48-\x4f][\x8d\x8b][\x05\x0d\x15\x1d\x25\x2d\x35\x3d]', code):
            offset = match.start()
            if offset + 7 > len(code):
                continue
            target = address + offset + 7 + struct.unpack_from('<i', code, offset + 3)[0]
            if target not in anchors:
                continue
            instruction = next(decoder.disasm(code[offset:offset+15], address+offset, count=1), None)
            if not instruction or instruction.size != 7:
                continue
            index = bisect.bisect_right(starts, instruction.address)-1
            function = functions[index] if index >= 0 and instruction.address < functions[index][1] else None
            references.append({'anchor': anchors[target], 'anchor_va': hex(target),
                               'instruction_va': hex(instruction.address),
                               'instruction': instruction.mnemonic+' '+instruction.op_str,
                               'function_start': hex(function[0]) if function else None,
                               'function_end': hex(function[1]) if function else None})
    return {'executable_sha256': hashlib.sha256(data).hexdigest(),
            'image_base': hex(base), 'anchor_count': len(anchors),
            'references': references,
            'limitations': 'Static candidate references only. Byte scanning can hit instruction interiors; validate function boundaries and control flow in a disassembler. No function ABI, actor layout or runtime hook is verified.'}

if __name__ == '__main__':
    report = discover(Path(sys.argv[1]) if len(sys.argv)>1 else DEFAULT)
    target = ROOT / 'work/animation_hook_candidates.json'
    target.parent.mkdir(exist_ok=True)
    target.write_text(json.dumps(report, indent=2))
    print(f"Found {report['anchor_count']} anchors, {len(report['references'])} candidate references")
    for item in report['references']:
        if re.search(r'skeleton|animated_hookup|\[anim|driver.*anim|bone.*anim', item['anchor'], re.I):
            print(item['function_start'], item['anchor'][:160])
