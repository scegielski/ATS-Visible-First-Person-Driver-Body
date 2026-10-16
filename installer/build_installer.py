"""Stage an explicit, hashed payload and compile the Windows setup executable.

Compiler dependency: official Inno Setup 6.7.3 (ISCC.exe). No developer tools are
needed on the target computer. SteamVR controller model assets are NOT bundled.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--iscc', type=Path, default=ROOT / 'work/inno/ISCC.exe')
    parser.add_argument('--version', help='New three-part installer version; existing versions cannot be overwritten')
    args = parser.parse_args()
    output = ROOT / 'releases/installer'
    versions = [tuple(map(int, match.groups())) for path in output.glob('*.exe')
                if (match := re.fullmatch(r'SPF-Visible-First-Person-Driver-Body-Setup-(\d+)\.(\d+)\.(\d+)\.exe', path.name))]
    latest = max(versions, default=(0, 39, 2))
    version = args.version or '.'.join(map(str, (*latest[:2], latest[2] + 1)))
    if not re.fullmatch(r'\d+\.\d+\.\d+', version):
        parser.error('Version must contain three numeric components')
    if tuple(map(int, version.split('.'))) <= latest:
        parser.error('Installer version must be newer than every existing distributed build')
    exe = output / f'SPF-Visible-First-Person-Driver-Body-Setup-{version}.exe'
    stage = ROOT / f'work/installer-payload-{version}'
    # Explicit staging root only; no recursive delete is required for rebuilding.
    stage.mkdir(parents=True, exist_ok=True)
    files = []

    def add(source, destination, target, mutable=False, missing=False):
        source = Path(source)
        if not source.is_file():
            raise FileNotFoundError(source)
        relative = Path(destination) / target
        staged = stage / relative
        staged.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, staged)
        files.append(dict(source=relative.as_posix(), destination=destination,
                          target=Path(target).as_posix(), sha256=sha(staged),
                          mutable=mutable, onlyIfMissing=missing))

    for name in ['SPF_VisibleFirstPersonDriverBody.dll', 'spf_controller_xr_layer.dll',
                 'spf_controller_openvr.dll', 'OPENVR_LICENSE.txt', 'OPENXR_LICENSE.txt']:
        add(ROOT / 'releases' / name, 'plugin', name)
    add(ROOT / 'releases/mustang_boxes.ini', 'plugin', 'config/mustang_boxes.ini', mutable=True)
    add(ROOT / 'releases/mustang_poses.ini', 'plugin', 'config/mustang_poses.ini', mutable=True)
    for path in sorted((ROOT / 'releases').glob('*.wav')):
        add(path, 'plugin', 'sounds/' + path.name)
    add(ROOT / 'releases/controller-layer/openxr_loader.dll', 'bin', 'openxr_loader.dll', missing=True)
    add(ROOT / 'releases/controller-layer/LOADER_LICENSE.txt', 'plugin', 'LOADER_LICENSE.txt')
    add(ROOT / 'releases/ATS-VisibleDriverBody-Mustang.scs',
        'mod', 'ATS-VisibleDriverBody-Mustang-0.1.0.scs')
    for path in sorted((ROOT / 'releases/adjustment-sounds').glob('*.wav')):
        add(path, 'plugin', 'sounds/' + path.name)
    for path in sorted((ROOT / 'native-diagnostics/adjustments/licenses').glob('*')):
        if path.is_file():
            add(path, 'plugin', 'licenses/' + path.name)
    add(ROOT / 'native-diagnostics/vendor/minhook/LICENSE.txt', 'plugin', 'licenses/MinHook.txt')
    # Only manifest-listed files are copied by the helper, even if stale staging
    # files exist. Reject unlisted archive contents in tests/build inspection.
    template = json.loads((ROOT / 'releases/controller-layer/spf_controller_xr_layer.json').read_text())
    (stage / 'layer-template.json').write_text(json.dumps(template, indent=2) + '\n')
    source = (ROOT / 'native-diagnostics/DriverAnimationDiagnostics.cpp').read_text()
    supported = re.search(r'kHash\[\]="([a-f0-9]{64})"', source).group(1)
    package = dict(version=version, supportedExe=supported, files=files)
    (stage / 'package.json').write_text(json.dumps(package, indent=2) + '\n')
    output = ROOT / 'releases/installer'
    output.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(args.iscc), '/Qp', '/DPayload=' + str(stage),
                    '/DOutputRoot=' + str(output), '/DSetupVersion=' + version, str(ROOT / 'installer/DriverBody.iss')], check=True)
    receipt = dict(installer=exe.name, sha256=sha(exe), payload=package,
                   tested='Automated fixture lifecycle; live install pending')
    (output / 'payload-manifest.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(f'Created {exe.name}: {exe.stat().st_size:,} bytes, {len(files)} payload files')


if __name__ == '__main__':
    main()
