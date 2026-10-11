"""Adult Link's Epona against an extracted OoT archive (feature plan F5b, local part of host test H8).

Local only: reads the user's own extracted oot.o2r (--archive, COMBO_OOT_ARCHIVE, or the first oot.o2r under a
local-roms folder beside the checkout) and is reported as skipped where there is none. Nothing is copied from it. Checks
that every OoT object_horse name ComboAdultEponaResources.h binds is there and has the shape the loader and MM's Skin
code rely on: gEponaSkel a Normal skeleton of 47 Skin limbs whose display lists exist and use no raw segment but 0x08
(the skin vertex buffer and the eye texture), limbs 13 and 30 display-list limbs, skin data within its bounds, the nine
animations with the loader's frame counts and 48 joints, and the three eyes 32x16 CI8 textures.
"""
from pathlib import Path
import argparse
import os
import re
import struct
import sys
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--archive', default='')
a = p.parse_args()
root = a.source.resolve()

archive = a.archive or os.environ.get('COMBO_OOT_ARCHIVE', '')
if not archive:
    for folder in (root.parent / 'local-roms', root.parent.parent / 'local-roms'):
        found = sorted(folder.rglob('oot.o2r')) if folder.is_dir() else []
        if found:
            archive = str(found[0])
            break
if not archive or not Path(archive).is_file():
    print('SKIPPED: no extracted OoT archive (pass --archive or set COMBO_OOT_ARCHIVE); this check is local only')
    sys.exit(0)

failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


OSKL, OSLB, OANM, ODLT, OTEX = 0x4F534B4C, 0x4F534C42, 0x4F414E4D, 0x4F444C54, 0x4F544558


class Reader:
    """A little-endian LUS resource body (after the 0x40-byte header)."""

    def __init__(self, data):
        self.data, self.at = data, 0x40

    def take(self, fmt):
        value = struct.unpack_from('<' + fmt, self.data, self.at)
        self.at += struct.calcsize('<' + fmt)
        return value[0] if len(value) == 1 else value

    def string(self):
        n = self.take('I')
        self.at += n
        return self.data[self.at - n:self.at].decode('utf-8')


resources = (root / 'mm/2s2h/ComboAdultEponaResources.h').read_text(encoding='utf-8')
skel_name = re.search(r'sAdultEponaSkelName\[\] = "([^"]+)"', resources).group(1)
anim_names = re.findall(r'"(objects/object_horse/\w+Anim)"', resources)
frames = [int(n) for n in re.search(r'sAdultEponaAnimFrames\[\] = \{([^}]*)\}', resources).group(1).split(',')]
eyes = re.findall(r'"__OTR__@oot:(objects/object_horse/\w+)"', resources)
limbs_expected = int(re.search(r'kAdultEponaLimbs = (\d+);', resources).group(1))
check(len(anim_names) == 9 and len(frames) == 9 and len(eyes) == 3, 'could not read the loader\'s name table')

with zipfile.ZipFile(archive) as z:
    names = set(z.namelist())

    def load(name, kind):
        if name not in names:
            failures.append(f'{name} is not in the archive')
            return None
        data = z.read(name)
        if struct.unpack_from('<I', data, 4)[0] != kind:
            failures.append(f'{name} is not a {kind:08X} resource')
            return None
        return Reader(data)

    r = load(skel_name, OSKL)
    limb_paths = []
    if r:
        skel_type, limb_type, limb_count, _, _, table_count = r.take('B'), r.take('B'), r.take('I'), r.take('I'), \
            r.take('B'), r.take('I')
        check(skel_type == 0 and limb_type == 3, 'gEponaSkel is not a Normal skeleton of Skin limbs')
        check(limb_count == limbs_expected == table_count, f'gEponaSkel has {limb_count} limbs, not {limbs_expected}')
        limb_paths = [r.string() for _ in range(table_count)]
    display_lists = []
    for i, path in enumerate(limb_paths):
        r = load(path, OSLB)
        if not r:
            continue
        limb_type, skin_type, skin_dl, total = r.take('B'), r.take('B'), r.string(), r.take('H')
        modifs = []
        for _ in range(r.take('I')):
            index = r.take('H')
            vertices = [r.take('HhhbbbB')[0] for _ in range(r.take('I'))]
            transforms = [r.take('BhhhB')[0] for _ in range(r.take('I'))]
            modifs.append((index, vertices, transforms))
        skin_dl2 = r.string()
        r.take('fffHHH')
        r.string(), r.string(), r.string(), r.string()
        r.take('hhh')
        child, sibling = r.take('B'), r.take('B')
        check(limb_type == 3 and skin_type in (0, 4, 5, 11), f'limb {i} is not a Skin limb the loader accepts')
        check(child in (0xFF,) or child < limbs_expected, f'limb {i} has child {child}')
        check(sibling in (0xFF,) or sibling < limbs_expected, f'limb {i} has sibling {sibling}')
        if i in (13, 30):
            check(skin_type == 11, f'limb {i} is not a display-list limb')
        if skin_type == 11:
            check(bool(skin_dl), f'limb {i} has no display list')
            display_lists.append(skin_dl)
        if skin_type == 4:
            check(bool(skin_dl2), f'limb {i} has no skin display list')
            display_lists.append(skin_dl2)
            for index, vertices, transforms in modifs:
                check(transforms and index < len(transforms), f'limb {i} has a modification without its transformation')
                check(all(v < total for v in vertices), f'limb {i} indexes past its {total} vertices')
                check(all(t < limbs_expected for t in transforms), f'limb {i} moves with a limb past {limbs_expected}')
    check(len(display_lists) == 11, f'{len(display_lists)} Epona display lists, not 11')
    raw_segments = set()
    for path in display_lists:
        r = load(path, ODLT)
        if not r:
            continue
        at = 0x40 + 1
        at += -at % 8
        data = r.data
        while at + 8 <= len(data):
            w0, w1 = struct.unpack_from('<II', data, at)
            at += 8
            op = w0 >> 24
            if op in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42):
                at += 8  # 128-bit OTR command (hash, marker, matrix, movemem)
                continue
            if op in (0x01, 0xDE, 0xFD, 0xDA, 0xDC):
                raw_segments.add(w1 >> 24)
            if op == 0xDF:
                break
    check(raw_segments <= {0x08}, f'Epona display lists use raw segments {sorted(raw_segments)}')
    for name, count in zip(anim_names, frames):
        r = load(name, OANM)
        if r:
            kind, frame_count, values = r.take('I'), r.take('h'), r.take('I')
            r.at += 2 * values
            joints = r.take('I')
            check(kind == 0 and frame_count == count, f'{name} is not a Normal animation of {count} frames')
            check(joints == limbs_expected + 1, f'{name} animates {joints} joints, not {limbs_expected + 1}')
    for name in eyes:
        r = load(name, OTEX)
        if r:
            check(r.take('IIII')[:3] == (4, 32, 16), f'{name} is not a 32x16 CI8 texture')

if failures:
    print('\n'.join(failures))
    sys.exit(1)
print(f'Adult Epona: OoT object_horse in {archive} has every resource the loader binds, in the shape it checks')
