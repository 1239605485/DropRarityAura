"""Combine validated per-target packages into one KernelLoader installation ZIP."""
from pathlib import Path
import sys, zipfile

TARGET_LIBRARIES = {
    'android-arm64': 'libdrop_rarity_aura.android.arm64.so',
    'windows-x86': 'drop_rarity_aura.windows.x86.dll',
    'windows-x64': 'drop_rarity_aura.windows.x64.dll',
    'linux-x86': 'libdrop_rarity_aura.linux.x86.so',
    'linux-x64': 'libdrop_rarity_aura.linux.x64.so',
}

def combine(source, output):
    common = None
    libraries = {}
    for target, name in TARGET_LIBRARIES.items():
        with zipfile.ZipFile(source / f'DropRarityAura-{target}.zip') as archive:
            assert archive.testzip() is None, target
            path = 'Resources/lib/' + name
            files = {n: archive.read(n) for n in archive.namelist()}
            assert path in files, target
            libraries[path] = files.pop(path)
            assert not any(n.startswith('Resources/lib/') for n in files), target
            if common is None:
                common = files
            else:
                assert common == files, f'Metadata or effect assets differ: {target}'
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted({**common, **libraries}.items()):
            archive.writestr(name, data)
    print(f'Combined five platform libraries: {output}')

if __name__ == '__main__':
    combine(Path(sys.argv[1]), Path(sys.argv[2]))
