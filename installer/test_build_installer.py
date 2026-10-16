import unittest
import tempfile
import subprocess
from pathlib import Path
from unittest import mock
import build_installer
from build_installer import readme_for_installer


class InstallerReadmeTests(unittest.TestCase):
    def test_legacy_link_upgrades_to_direct_download(self):
        before = 'Intro\nUse the [Windows installer](releases/installer/SPF-Visible-First-Person-Driver-Body-Setup-0.39.2.exe) with ATS closed.\nOther text\n'
        after = readme_for_installer(before, '0.39.48')
        self.assertIn('[Windows installer (0.39.48)](https://raw.githubusercontent.com/', after)
        self.assertIn('Driver-Body-Setup-0.39.48.exe)', after)
        self.assertTrue(after.startswith('Intro\nUse the '))
        self.assertTrue(after.endswith(' with ATS closed.\nOther text\n'))

    def test_version_label_and_url_advance_together(self):
        old = '[Windows installer (0.39.47)](https://example.test/Setup-0.39.47.exe)'
        after = readme_for_installer(old, '1.0.0')
        self.assertIn('installer (1.0.0)', after)
        self.assertIn('Setup-1.0.0.exe)', after)
        self.assertNotIn('0.39.47', after)
        self.assertEqual(after, readme_for_installer(after, '1.0.0'))

    def test_missing_or_duplicate_links_fail_instead_of_silent_stale_version(self):
        link = '[Windows installer](old.exe)'
        for text in ('No download link', link + '\n' + link):
            with self.assertRaises(ValueError):
                readme_for_installer(text, '1.2.3')

    def test_failed_compiler_leaves_readme_unchanged(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            readme = root / 'README.md'
            original = '[Windows installer (0.39.2)](old.exe)\n'
            readme.write_text(original, encoding='utf-8')
            payloads = ['SPF_VisibleFirstPersonDriverBody.dll', 'spf_controller_xr_layer.dll',
                        'spf_controller_openvr.dll', 'OPENVR_LICENSE.txt', 'OPENXR_LICENSE.txt',
                        'mustang_boxes.ini', 'mustang_poses.ini', 'controller-layer/openxr_loader.dll',
                        'controller-layer/LOADER_LICENSE.txt', 'ATS-VisibleDriverBody-Mustang.scs',
                        'controller-layer/spf_controller_xr_layer.json']
            for relative in payloads:
                path = root / 'releases' / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('{}', encoding='utf-8')
            source = root / 'native-diagnostics/DriverAnimationDiagnostics.cpp'
            source.parent.mkdir(parents=True)
            source.write_text('constexpr char kHash[]="' + '0' * 64 + '";', encoding='utf-8')
            license_path = root / 'native-diagnostics/vendor/minhook/LICENSE.txt'
            license_path.parent.mkdir(parents=True)
            license_path.write_text('fixture license', encoding='utf-8')
            with mock.patch.object(build_installer, 'ROOT', root), \
                 mock.patch('sys.argv', ['build_installer.py', '--iscc', 'fixture-iscc']), \
                 mock.patch.object(build_installer.subprocess, 'run',
                                   side_effect=subprocess.CalledProcessError(1, 'fixture-iscc')):
                with self.assertRaises(subprocess.CalledProcessError):
                    build_installer.main()
            self.assertEqual(original, readme.read_text(encoding='utf-8'))

    def test_invalid_version_rejected(self):
        with self.assertRaises(ValueError):
            readme_for_installer('[Windows installer](old.exe)', 'latest')


if __name__ == '__main__':
    unittest.main()
