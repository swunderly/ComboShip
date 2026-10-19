"""The vendored OoTMM v32.3 Master Quest bins: pins, provenance, notices and the C++ decoder.

- The three files in combo/mq/data hash to the pins in combo/mq/ComboMq.cmake, in this test and in
  combo/mq/README.md, and .gitattributes keeps git from converting them.
- An independent reader (mq_bins.py, written from OoTMM's make_mq.ts and mq.c) decodes exactly what the C++ decoder
  compiled into the library decodes (combo_mq_data_tests --dump-json), field by field.
- CMake/ComboEmbedBinary.cmake accepts the pinned files and stops on a changed or truncated copy.
- LICENSE.OoTMM sits beside the data, NOTICE.txt and README.md name it, and the root CMakeLists.txt compiles the data
  and decoder into soh for the Android build.
- Local only: when an OoTMM v32.3 checkout is beside the repository, the files are byte-identical to its
  packages/generator/data/static copies.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import mq_bins  # noqa: E402

PINS = {
    'mq_rooms.bin': '3b3e1f836124f756560382e7ae033bfb7bd84570b2b1c5ea228655728cb6f636',
    'mq_scenes.bin': 'e1b77a41950ef656b7fcf411b119ea5b9617dc3dac3aa1560faf3567881c9a49',
    'mq_maps.bin': '43f8f13fa1b8290f661dbc66d2e89cfcc6f59169e70ae5fa1ad629ac657bffaa',
}
SIZES = {'mq_rooms.bin': 58896, 'mq_scenes.bin': 36352, 'mq_maps.bin': 0x4158}

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--exe', type=Path, required=True, help='combo_mq_data_tests built by CMake')
parser.add_argument('--cmake', required=True)
args = parser.parse_args()
repo = args.source.resolve()
mq = repo / 'combo/mq'
data_dir = mq / 'data'
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


def first_difference(a, b, path='$'):
    """Where two decoded structures first differ, or None."""
    if isinstance(a, dict) and isinstance(b, dict):
        if a.keys() != b.keys():
            return f'{path}: keys {sorted(a)} != {sorted(b)}'
        for key in a:
            found = first_difference(a[key], b[key], f'{path}.{key}')
            if found:
                return found
        return None
    if isinstance(a, list) and isinstance(b, list):
        if len(a) != len(b):
            return f'{path}: length {len(a)} != {len(b)}'
        for i, (x, y) in enumerate(zip(a, b)):
            found = first_difference(x, y, f'{path}[{i}]')
            if found:
                return found
        return None
    return None if a == b and type(a) is type(b) else f'{path}: {a!r} != {b!r}'


# Pins: the files, this test, the CMake list and the provenance README agree.
cmake_pins = dict(re.findall(r'"\$\{COMBO_MQ_DIR\}/data/(mq_\w+\.bin)"\s+([0-9a-f]{64})',
                             (mq / 'ComboMq.cmake').read_text(encoding='utf-8')))
check(cmake_pins == PINS, f'combo/mq/ComboMq.cmake pins {cmake_pins} differ from this test')
readme = (mq / 'README.md').read_text(encoding='utf-8')
for name, digest in PINS.items():
    blob = (data_dir / name).read_bytes()
    check(len(blob) == SIZES[name], f'{name} is {len(blob)} bytes')
    check(hashlib.sha256(blob).hexdigest() == digest, f'{name} does not match its pin')
    check(digest in readme, f'combo/mq/README.md does not list the sha256 of {name}')
attributes = (repo / '.gitattributes').read_text(encoding='utf-8')
check(re.search(r'^combo/mq/data/\*\.bin\s+binary\s*$', attributes, re.M),
      '.gitattributes must mark combo/mq/data/*.bin binary')

# The C++ decoder against the independent reader.
rooms, scenes, maps = mq_bins.load(data_dir)
reference = mq_bins.decode(rooms, scenes, maps)
dump = work / 'mq_dump.json'
result = subprocess.run([str(args.exe.resolve()), '--dump-json', str(dump)], capture_output=True, text=True, timeout=120)
if check(result.returncode == 0, f'--dump-json failed: {result.stderr.strip()}'):
    decoded = json.loads(dump.read_text(encoding='utf-8'))
    difference = first_difference(decoded, reference)
    check(difference is None, f'C++ decoder and mq_bins.py differ at {difference}')
check(len(reference['rooms']) == 220 and len(reference['scenes']) == 12 and len(reference['pause']) == 34,
      'reference counts changed')
for header in mq_bins.scene_headers(scenes):
    if header['dungeon'] in (5, 9):  # Water and Ice: no terminator inside the entry
        check(header['pathOffset'] + 8 * mq_bins.PATH_COUNTS[header['dungeon']] == header['size'],
              f'{mq_bins.DUNGEONS[header["dungeon"]]} path headers no longer end at the entry size')
check(reference['pause'][17][0][5][5] == [0, mq_bins.float_bits(46.5), mq_bins.float_bits(-17.5)],
      'Fire pause floor 17 lost its sixth chest')

# The CMake helper: the pinned files embed; a changed pin or a truncated copy stops the configuration.
helper = repo / 'CMake/ComboEmbedBinary.cmake'
script = work / 'embed.cmake'


def embed(entries, out):
    lines = ' '.join(f'{symbol} "{path.as_posix()}" {digest}' for symbol, path, digest in entries)
    script.write_text(f'include("{helper.as_posix()}")\n'
                      f'combo_embed_binaries("{out.as_posix()}" ComboMqBlobs ComboMq::Blobs {lines})\n',
                      encoding='utf-8')
    return subprocess.run([args.cmake, '-P', str(script)], capture_output=True, text=True, timeout=120)


symbols = {'mq_rooms.bin': 'kMqRooms', 'mq_scenes.bin': 'kMqScenes', 'mq_maps.bin': 'kMqMaps'}
pinned = [(symbols[name], data_dir / name, digest) for name, digest in PINS.items()]
out = work / 'embed-ok'
shutil.rmtree(out, ignore_errors=True)
result = embed(pinned, out)
if check(result.returncode == 0, f'the helper refused the pinned files: {result.stderr.strip()}'):
    header = (out / 'ComboMqBlobs.h').read_text(encoding='utf-8')
    for name, symbol in symbols.items():
        check(f'extern const std::uint8_t {symbol}[{SIZES[name]}];' in header, f'generated header lacks {symbol}')
    source = (out / 'ComboMqBlobs.cpp').read_text(encoding='utf-8')
    rows = re.findall(r'const std::uint8_t (\w+)\[\d+\] = \{([^}]*)\};', source)
    embedded = {symbol: bytes(int(b, 16) for b in re.findall(r'0x([0-9a-f]{2})', body)) for symbol, body in rows}
    for name, symbol in symbols.items():
        check(embedded.get(symbol) == (data_dir / name).read_bytes(), f'generated {symbol} differs from {name}')
    stamp = (out / 'ComboMqBlobs.cpp').stat().st_mtime_ns
    embed(pinned, out)
    check((out / 'ComboMqBlobs.cpp').stat().st_mtime_ns == stamp, 'an unchanged embed rewrote the generated source')
wrong = [(pinned[0][0], pinned[0][1], 'f' * 64)] + pinned[1:]
result = embed(wrong, work / 'embed-wrong-pin')
check(result.returncode != 0 and 'SHA-256 mismatch' in result.stderr, 'a wrong pin did not stop the configuration')
with tempfile.TemporaryDirectory() as temp:
    cut = Path(temp) / 'mq_scenes.bin'
    cut.write_bytes(scenes[:-16])
    result = embed([pinned[0], (pinned[1][0], cut, pinned[1][2]), pinned[2]], work / 'embed-truncated')
    check(result.returncode != 0 and 'SHA-256 mismatch' in result.stderr, 'a truncated copy did not stop the configuration')

# Licence and notices.
license_text = (mq / 'LICENSE.OoTMM').read_bytes()
check(license_text == (repo / 'ComboAndroid/licenses/OoTMM-MIT-LICENSE.txt').read_bytes(),
      'combo/mq/LICENSE.OoTMM must be the OoTMM MIT licence shipped in the APK')
notice = (repo / 'ComboAndroid/licenses/NOTICE.txt').read_text(encoding='utf-8')
check(all(name in notice for name in PINS) and 'OoTMM' in notice and 'combo/mq' in notice,
      'ComboAndroid/licenses/NOTICE.txt must name the bundled MQ data')
top_readme = (repo / 'README.md').read_text(encoding='utf-8')
check('combo/mq' in top_readme and 'Master Quest' in top_readme, 'README.md must credit the bundled MQ data')
check('OoTMM v32.3' in readme and 'make_mq.ts' in readme and 'Master Quest ROM' in readme,
      'combo/mq/README.md must give the source tag, the script and the origin of the data')

# The Android (and desktop) build compiles the data and decoder into soh.
root_cmake = (repo / 'CMakeLists.txt').read_text(encoding='utf-8')
check(re.search(r'include\("\$\{CMAKE_SOURCE_DIR\}/combo/mq/ComboMq\.cmake"\)\s*\ncombo_mq_add_sources\(soh\)',
                root_cmake), 'the root CMakeLists.txt must add the MQ data and decoder to soh')

# Local only: byte-identical to an OoTMM v32.3 checkout beside the repository.
candidates = [base / 'references/ootmm-stable-32.3/packages/generator/data/static'
              for base in (repo.parent, repo.parent.parent, repo.parent.parent.parent)]
static = next((c for c in candidates if (c / 'mq_rooms.bin').is_file()), None)
if static:
    for name in PINS:
        check((static / name).read_bytes() == (data_dir / name).read_bytes(), f'{name} differs from {static / name}')
    print(f'Compared with the OoTMM checkout at {static}')
else:
    print('No OoTMM v32.3 checkout beside the repository; the byte comparison with it was not run (pins still hold)')

if failures:
    print('FAIL:\n  ' + '\n  '.join(failures))
    sys.exit(1)
print('PASS: MQ bins match their pins, the C++ decoder matches the reference reader, the embed helper refuses '
      'changed copies, and the notices name the data')
