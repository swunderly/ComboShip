"""The developer Master Quest walker (combo/mq/MqWalk.h) can never be in a shipping build.

The walker, its teleport and its adb command file are compiled only with the CMake option COMBO_MQ_WALK_DEV. Checks:
- the root CMakeLists.txt declares the option OFF and defines the macro only when it is on, before the game
  subprojects; no other build file, script or Gradle file sets it, and no source defines the macro;
- every mention of the walker in the production sources (combo, soh, mm, libultraship, ComboAndroid/app), comments
  aside, sits inside a block that needs COMBO_MQ_WALK_DEV, and only the walker's own files and debugconsole.cpp's two
  hooks mention it;
- compiled as shipped (COMBO_BUILD without COMBO_MQ_WALK_DEV, with debug information) the glue ComboMqWalk.cpp is an
  empty translation unit and MqWalk.h declares nothing (code naming its Walker does not compile); neither object
  carries the marker, the log tag, the command file or the glue's entry point, and an object compiled with the option
  carries the first three;
- verify_apk.py refuses a library with any of those strings, accepts one without, and verify() refuses a walker APK
  by name; its marker is MqWalk.h's.
"""
from pathlib import Path
import argparse
import importlib.util
import os
import re
import subprocess
import sys
import zipfile
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
root, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


def read(relative):
    return (root / relative).read_text(encoding='utf-8', errors='replace')


OPTION = 'COMBO_MQ_WALK_DEV'
header = read('combo/mq/MqWalk.h')
marker = re.search(r'kMarker\[\] = "([^"]+)";', header)
tag = re.search(r'kTag\[\] = "([^"]+)";', header)
command_file = re.search(r'kCommandFile\[\] = "([^"]+)";', header)
check(marker and tag and command_file, 'MqWalk.h defines kMarker, kTag and kCommandFile')
# In the sources: the walker's strings and names. In a library: its strings and the glue's exported entry point (an
# unstripped library still names the empty ComboMqWalk.cpp in its debug information, so not the bare name).
NEEDLES = [marker.group(1), tag.group(1), command_file.group(1).split('.')[0], 'ComboMqWalk', 'MqWalk.h',
           'ComboMq::Walk']
BINARY_NEEDLES = [needle.encode('ascii') for needle in NEEDLES[:3]] + [b'ComboMqWalk_Init']

# --- The option is OFF, and it is the only way the macro is defined.
cmake = read('CMakeLists.txt')
block = re.findall(r'option\(COMBO_MQ_WALK_DEV "[^"]+" (\w+)\)\s*if\(COMBO_MQ_WALK_DEV\)\s*'
                   r'add_compile_definitions\(COMBO_MQ_WALK_DEV=1\)\s*endif\(\)', cmake)
check(block == ['OFF'], 'root CMakeLists.txt must declare COMBO_MQ_WALK_DEV OFF and define the macro only when on')
check(cmake.count(OPTION) == 3, f'{OPTION} is used only by its option block')
check(cmake.index(OPTION) < cmake.index('add_subdirectory(soh'), 'the option comes before the game subprojects')
SKIP_DIRS = {'.git', 'assets', 'x64', '_deps', 'node_modules'}
CPP = ('.c', '.cc', '.cpp', '.h', '.hpp', '.inc')
BUILD_FILES = ('CMakeLists.txt', '.cmake', '.gradle', '.properties', '.ps1', '.sh', '.bat', '.yml', '.yaml')


def walk(top, suffixes):
    for directory, dirs, files in os.walk(root / top):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS and not d.lower().startswith('build')]
        for name in files:
            if name.endswith(suffixes):
                yield Path(directory, name)


for path in walk('.', BUILD_FILES):
    relative = path.relative_to(root).as_posix()
    if relative not in ('CMakeLists.txt', 'ComboAndroid/tests/CMakeLists.txt'):
        check(OPTION not in path.read_text(encoding='utf-8', errors='replace'), f'{relative} must not set {OPTION}')


def code_only(text):
    """The text with comments blanked, string and character literals kept, line numbers unchanged."""
    out, at, size = [], 0, len(text)
    while at < size:
        if text.startswith('//', at):
            end = text.find('\n', at)
            at = size if end < 0 else end
        elif text.startswith('/*', at):
            end = text.find('*/', at + 2)
            end = size if end < 0 else end + 2
            out.append('\n' * text.count('\n', at, end))
            at = end
        elif text[at] in '"\'':
            quote, end = text[at], at + 1
            while end < size and text[end] != quote and text[end] != '\n':
                end += 2 if text[end] == '\\' else 1
            out.append(text[at:end + 1])
            at = end + 1
        else:
            out.append(text[at])
            at += 1
    return ''.join(out)


def unguarded(text, needles):
    """Line numbers holding a needle outside every #if block that needs COMBO_MQ_WALK_DEV."""
    stack, hits = [], []
    for number, line in enumerate(code_only(text).splitlines(), 1):
        directive = re.sub(r'^#\s*', '#', line.strip())
        if directive.startswith('#if'):
            condition = directive[3:]
            needs = (re.match(r'(def)?\s', condition) is not None and OPTION in condition and '||' not in condition
                     and '!' not in condition and not condition.startswith('ndef'))
            stack.append(needs)
            continue
        if directive.startswith(('#else', '#elif')) and stack:
            stack[-1] = False
            continue
        if directive.startswith('#endif') and stack:
            stack.pop()
            continue
        if any(needle in line for needle in needles) and not any(stack):
            hits.append(number)
    return hits


mentions, scanned = set(), 0
for top in ('combo', 'soh', 'mm', 'libultraship', 'ComboAndroid/app'):
    for path in walk(top, CPP):
        text = path.read_text(encoding='utf-8', errors='replace')
        relative = path.relative_to(root).as_posix()
        scanned += 1
        check(not re.search(r'#\s*define\s+' + OPTION + r'\b', text), f'{relative} defines {OPTION}')
        if any(needle in code_only(text) for needle in NEEDLES):
            mentions.add(relative)
        hits = unguarded(text, NEEDLES)
        check(not hits, f'{relative}:{hits} names the MQ walker outside #if defined({OPTION})')
check(scanned > 4000, f'only {scanned} production sources scanned')
check(mentions == {'combo/mq/MqWalk.h', 'soh/soh/Enhancements/randomizer/ComboMqWalk.h',
                   'soh/soh/Enhancements/randomizer/ComboMqWalk.cpp', 'soh/soh/Enhancements/debugconsole.cpp'},
      f'files naming the walker: {sorted(mentions)}')
console = code_only(read('soh/soh/Enhancements/debugconsole.cpp'))
check(console.count('ComboMqWalk') == 2, 'debugconsole.cpp only includes the glue and calls ComboMqWalk_Init')
glue = read('soh/soh/Enhancements/randomizer/ComboMqWalk.cpp')
check(glue.startswith('#if defined(COMBO_BUILD) && defined(COMBO_MQ_WALK_DEV)\n') and glue.rstrip().endswith('#endif'),
      'the whole glue sits inside its guard')
body = header[header.index('#if defined(COMBO_MQ_WALK_DEV)'):]
check(not code_only(header[:header.index('#if defined(COMBO_MQ_WALK_DEV)')]).replace('#pragma once', '').strip()
      and body.rstrip().endswith('#endif'), 'all of MqWalk.h but #pragma once sits inside its guard')

# --- Compiled as shipped, and with the option.
probe = work / 'mq_walk_probe.cpp'
probe.write_text('#include "mq/MqWalk.h"\n'
                 '#if defined(COMBO_MQ_WALK_DEV)\n'
                 'extern const char* const gComboMqWalkProbe[] = { ComboMq::Walk::kMarker, ComboMq::Walk::kTag,\n'
                 '    ComboMq::Walk::kCommandFile };\n'
                 '#endif\n'
                 'int mq_walk_probe_anchor = 1;\n', encoding='utf-8')
uses = work / 'mq_walk_uses.cpp'
uses.write_text('#include "mq/MqWalk.h"\nint size_of_walker = sizeof(ComboMq::Walk::Walker);\n', encoding='utf-8')
msvc = hostcxx.msvc_style(args.compiler)


def compile_object(source, out, defines, includes):
    # With debug information, as the Android libraries are built (RelWithDebInfo).
    if msvc:
        command = [args.compiler, '/nologo', '/EHsc', '/std:c++17', '/Z7', '/c', str(source), '/Fo:' + str(out),
                   *('/D' + d for d in defines), *('/I' + str(i) for i in includes)]
    else:
        command = [args.compiler, '-std=c++17', '-g', '-c', str(source), '-o', str(out),
                   *('-D' + d for d in defines), *('-I' + str(i) for i in includes)]
    return subprocess.run(command, cwd=work, capture_output=True, text=True, timeout=hostcxx.COMPILE_TIMEOUT)


suffix = '.obj' if msvc else '.o'
objects = {}
glue_source = root / 'soh/soh/Enhancements/randomizer/ComboMqWalk.cpp'
for name, source, defines in (('glue_shipping', glue_source, ['COMBO_BUILD']),
                              ('probe_shipping', probe, ['COMBO_BUILD']),
                              ('probe_dev', probe, ['COMBO_BUILD', 'COMBO_MQ_WALK_DEV=1'])):
    out = work / (name + suffix)
    out.unlink(missing_ok=True)
    result = compile_object(source, out, defines, [root / 'combo'])
    if check(result.returncode == 0 and out.exists(), f'{name} compiles: {result.stdout}{result.stderr}'):
        objects[name] = out.read_bytes()
needles = BINARY_NEEDLES
for name in ('glue_shipping', 'probe_shipping'):
    if name in objects:
        found = [needle for needle in needles if needle in objects[name]]
        check(not found, f'the shipping {name} object carries {found}')
if 'probe_dev' in objects:
    check(all(needle in objects['probe_dev'] for needle in needles[:3]),
          'an object compiled with the option carries the marker, the tag and the command file')
shipped = compile_object(uses, work / ('uses_shipping' + suffix), ['COMBO_BUILD'], [root / 'combo'])
check(shipped.returncode != 0, 'as shipped, MqWalk.h declares no Walker')
dev = compile_object(uses, work / ('uses_dev' + suffix), ['COMBO_BUILD', 'COMBO_MQ_WALK_DEV=1'], [root / 'combo'])
check(dev.returncode == 0, f'with the option, MqWalk.h declares the Walker: {dev.stdout}{dev.stderr}')

# --- verify_apk.py refuses the walker.
sys.path.insert(0, str(root / 'ComboAndroid/tools'))
spec = importlib.util.spec_from_file_location('verify_apk', root / 'ComboAndroid/tools/verify_apk.py')
verify_apk = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verify_apk)
check(verify_apk.MQ_WALK_DEV_NEEDLES[:2] == (marker.group(1).encode('ascii'), tag.group(1).encode('ascii')),
      'verify_apk.py and MqWalk.h share the marker and the tag')
check(set(verify_apk.MQ_WALK_DEV_NEEDLES) == set(needles), f'verify_apk.py looks for {verify_apk.MQ_WALK_DEV_NEEDLES}')
for needle in needles:
    libraries = [('lib/arm64-v8a/libsoh.so', b'\x7fELF' + needle + b'\0'), ('lib/arm64-v8a/lib2ship.so', b'\x7fELF')]
    check(verify_apk.mq_walk_dev_libraries(libraries) == ['lib/arm64-v8a/libsoh.so'],
          f'verify_apk.py refuses a library with {needle}')
room_log_only = [('lib/arm64-v8a/libsoh.so', b'\x7fELF [ComboShip MQ room] mq_walk_compare')]
check(verify_apk.mq_walk_dev_libraries(room_log_only) == [], 'verify_apk.py accepts a library with the room log')
if 'probe_dev' in objects and 'glue_shipping' in objects:
    check(verify_apk.mq_walk_dev_libraries([('dev.o', objects['probe_dev']), ('ship.o', objects['glue_shipping'])])
          == ['dev.o'], 'verify_apk.py tells the compiled objects apart')
apk = work / 'walker.apk'
with zipfile.ZipFile(apk, 'w') as z:
    for name in sorted(verify_apk.REQUIRED):
        walker = marker.group(1).encode('ascii') if name == 'libsoh.so' else b''
        z.writestr('lib/arm64-v8a/' + name, b'\x7fELF ' + walker)
try:
    verify_apk.verify(apk)
    check(False, 'verify() accepted a walker APK')
except RuntimeError as error:
    check(str(error) == 'COMBO_MQ_WALK_DEV build, not for release: lib/arm64-v8a/libsoh.so', f'verify(): {error}')
check('no MQ walker dev build' in read('ComboAndroid/tools/verify_apk.py'), 'its PASS line says so')

for failure in failures:
    print('FAIL:', failure)
if failures:
    sys.exit(1)
print(f'MQ walker gating passed: option OFF, {scanned} sources scanned, shipping objects clean, verify_apk refuses it.')
