"""Package the audited runtime asset set, excluding historical prototypes.

Uses the validated marker/animation archive as input; does not rebuild geometry.
"""
from pathlib import Path
import hashlib
import json
import re
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'releases/ATS-VisibleDriverBody-Mustang-0.18.0-radio-interaction-box-test.scs'
TARGET = ROOT / 'releases/ATS-VisibleDriverBody-Mustang.scs'
KEEP = {
    'manifest.sii', 'mod_description.txt',
    'def/vehicle/car/ford.mustang_67/accessory/carpet/spf_stock_driver.sii',
    'unit/hookup/vehicle/spf_stock_driver.sii',
    'unit/hookup/vehicle/spf_controller_markers.sii',
    *('model/spf_stock_driver/' + f for f in ('driver.pmd', 'driver.pmg', 'mount.pmd', 'mount.pmg', 'seat_hold.pma', 'steering_all_pedals.pma')),
    *('model/spf_controller_markers/' + f for f in ('driver.pmd', 'driver.pmg', 'white.tobj', 'white.dds')),
    *(f'automat/cf/{i:016x}.mat' for i in (1, 2, 3)),
}
MANIFEST = b'''SiiNunit
{
mod_package : .package_name {
 package_version: "0.39.2"
 display_name: "SPF Visible First Person Driver Body - Mustang"
 author: "SPF"
 category[]: "interior"
 description_file: "mod_description.txt"
}
}
'''
DESCRIPTION = b'''Animated driver body and VR interaction markers for the Mustang.
Requires the matching SPF Visible First Person Driver Body plugin, SPF Framework,
and the Mustang vehicle. Select SPF Stock Driver Body in carpet accessories.
Controller models are loaded from SteamVR at runtime. No diagnostic accessories
or static prototype mannequin are included.
'''

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    with ZipFile(SOURCE) as source:
        assert KEEP <= set(source.namelist())
        content = {name: source.read(name) for name in KEEP}
        removed = sorted(set(source.namelist()) - KEEP)
        content['manifest.sii'] = MANIFEST
        content['mod_description.txt'] = DESCRIPTION
        definition = 'def/vehicle/car/ford.mustang_67/accessory/carpet/spf_stock_driver.sii'
        content[definition] = content[definition].replace(b'SPF Stock Driver Body (animated prototype)', b'SPF Stock Driver Body')
        with ZipFile(TARGET, 'w', ZIP_DEFLATED, compresslevel=6) as result:
            for name in sorted(content):
                result.writestr(name, content[name])
        with ZipFile(TARGET) as result:
            assert result.testzip() is None
            assert set(result.namelist()) == KEEP
            assert not any('diagnostic' in n or 'tifa' in n or n.startswith('model/spf_driver_body/') for n in result.namelist())
            for name in KEEP - {'manifest.sii', 'mod_description.txt', definition}:
                assert result.read(name) == source.read(name), name
            # All private model, animation and texture references must resolve in-package.
            refs = set()
            for name in KEEP:
                if name.endswith(('.sii', '.mat', '.pmd', '.tobj')):
                    refs.update(x.decode() for x in re.findall(rb'/(?:model|automat)/[A-Za-z0-9_./-]+\.(?:pmd|pmg|pma|mat|tobj|dds)', result.read(name)))
            external = []
            for ref in sorted(refs):
                if ref.lstrip('/') in KEEP:
                    continue
                if ref.startswith('/automat/'):
                    external.append(ref)  # Stock driver materials supplied by the game.
                else:
                    raise AssertionError('Missing private dependency: ' + ref)
        receipt = dict(archive=TARGET.name, files=len(KEEP), sha256=sha(TARGET.read_bytes()), removed=removed,
                       retained_runtime_assets='Byte-identical except manifest, description and accessory display name',
                       external_stock_materials=external, in_game_check='pending')
        (ROOT / 'development/evidence/release_asset_audit.json').write_text(json.dumps(receipt, indent=2) + '\n')
        print(f'Clean archive: {len(KEEP)} files; removed {len(removed)} historical assets; CRC and private references verified.')

if __name__ == '__main__':
    main()
