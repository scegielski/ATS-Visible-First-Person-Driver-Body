"""Read-only audit of the current Mustang animation integration surface."""
from pathlib import Path
import hashlib, json, struct

ROOT = Path(__file__).resolve().parents[2]
CHARS = '\x000123456789abcdefghijklmnopqrstuvwxyz_'

def token(value):
    result = ''
    while value:
        result += CHARS[value % 38]
        value //= 38
    return result

def skeleton(path):
    data = path.read_bytes()
    assert struct.unpack_from('<I', data)[0] == 0x506d6715
    count = struct.unpack_from('<I', data, 12)[0]
    offset = struct.unpack_from('<I', data, 72)[0]
    bones = []
    for index in range(count):
        start = offset + index * 200
        bones.append({'index': index, 'name': token(struct.unpack_from('<Q', data, start)[0]),
                      'parent': struct.unpack_from('<I', data, start + 196)[0]})
    return bones

def audit():
    ford = ROOT.parent / 'extracted-ford'
    driver = skeleton(ROOT / 'package/model/spf_stock_driver/driver.pmg')
    interior = skeleton(ford / 'vehicle/car/ford_mustang_67/interior/anim/anim.pmg')
    clip = (ROOT / 'stock-reference/ford_mustang_67_ani.pma').read_bytes()
    version, frames, count = struct.unpack_from('<3I', clip)
    assert version == 5
    times, ids, transforms, end, expanded = struct.unpack_from('<5I', clip, 64)
    assert ids == times + frames * 4 and transforms == ids + count
    assert end == len(clip) == transforms + frames * count * 56
    assert expanded == end + frames * 12
    moving = []
    for track, index in enumerate(clip[ids:transforms]):
        neutral = struct.unpack_from('<14f', clip, transforms + track * 56)
        delta = max(abs(value - neutral[component])
                    for frame in range(frames)
                    for component, value in enumerate(struct.unpack_from('<14f', clip, transforms + (frame * count + track) * 56)))
        if delta > 1e-5:
            moving.append({'index': index, 'name': driver[index]['name'], 'max_local_component_delta': delta})
    definitions = (ford / 'def/vehicle/car/ford.mustang_67/interior/animations.sui').read_text()
    release = ROOT / 'releases/ATS-VisibleDriverBody-Mustang-0.2.4.scs'
    installed = Path('C:/Users/scegi/OneDrive/Documents/American Truck Simulator/mod/ATS-VisibleDriverBody-Mustang-0.1.0.scs')
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    return {'driver_bones': driver, 'interior_bones': interior,
            'original_driver_frames': frames, 'moving_driver_tracks': moving,
            'mustang_has_wheel_anim': 'wheel_anim:' in definitions,
            'release_sha256': digest(release),
            'installed_sha256': digest(installed) if installed.exists() else None,
            'runtime_steering_verified': False,
            'note': 'Track variation is animation data evidence, not proof of steering-input synchronization.'}

if __name__ == '__main__':
    print(json.dumps(audit(), indent=2))
