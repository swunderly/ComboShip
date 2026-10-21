#!/usr/bin/env python3
"""Check APK architecture, native alignment, manifest hashes, and absence of game ROMs."""
import argparse,json,zipfile,hashlib,struct
from pathlib import Path
from stage_runtime import REQUIRED,arm64
from verify_native import verify_runtime
# Compiled into every library that can admit dev-only entrance capabilities (combo/rando/entrances/Capabilities.h,
# CMake option COMBO_ENTRANCE_DEV). No release may carry it.
ENTRANCE_DEV_MARKER=b'ComboShip entrance dev build - not for release'
def entrance_dev_libraries(libraries):
    """Names of the (name, bytes) libraries built with COMBO_ENTRANCE_DEV; reads them one at a time."""
    return sorted(name for name,data in libraries if ENTRANCE_DEV_MARKER in data)
# The developer Master Quest walker (combo/mq/MqWalk.h, CMake option COMBO_MQ_WALK_DEV): its marker, its log tag, its
# adb command file and its glue's exported entry point. No release may carry any of them. (Not the bare glue name: an
# unstripped shipping library still names the empty ComboMqWalk.cpp in its debug information.)
MQ_WALK_DEV_NEEDLES=(b'ComboShip MQ walker dev build - not for release',b'[ComboShip MQ walk]',
    b'mq-walk-commands',b'ComboMqWalk_Init')
def mq_walk_dev_libraries(libraries):
    """Names of the (name, bytes) libraries that carry the MQ walker; reads them one at a time."""
    return sorted(name for name,data in libraries if any(needle in data for needle in MQ_WALK_DEV_NEEDLES))
def unreferenced_bytes(apk,z):
    """Bytes between local entries that no central-directory entry points at (left by incremental packaging)."""
    data=apk.read_bytes();end=0;gaps=0
    for info in sorted(z.infolist(),key=lambda i:i.header_offset):
        gaps+=max(0,info.header_offset-end)
        name_length,extra_length=struct.unpack_from('<HH',data,info.header_offset+26)
        end=info.header_offset+30+name_length+extra_length+info.compress_size+(16 if info.flag_bits&8 else 0)
    return gaps
def verify(apk):
    with zipfile.ZipFile(apk) as z:
        stale=unreferenced_bytes(apk,z)
        if stale>1024*1024:raise RuntimeError(f'{stale} bytes of unreferenced space in the APK; package from clean outputs')
        names=z.namelist()
        if len(names)!=len(set(names)):raise RuntimeError('Duplicate APK entries')
        libs={Path(name).name:name for name in names if name.startswith('lib/arm64-v8a/') and name.endswith('.so')}
        if REQUIRED-libs.keys():raise RuntimeError('APK is missing game runtime libraries')
        dev=entrance_dev_libraries((name,z.read(name)) for name in names if name.endswith('.so'))
        if dev:raise RuntimeError(f'COMBO_ENTRANCE_DEV build, not for release: {", ".join(dev)}')
        walker=mq_walk_dev_libraries((name,z.read(name)) for name in names if name.endswith('.so'))
        if walker:raise RuntimeError(f'COMBO_MQ_WALK_DEV build, not for release: {", ".join(walker)}')
        for name,path in libs.items():arm64(z.read(path),name)
        verify_runtime({name:z.read(path) for name,path in libs.items()},json.loads(z.read('assets/native-exports.json')))
        for name in names:
            lower=name.lower()
            if lower.endswith(('.z64','.n64','.v64')):raise RuntimeError('ROM found in APK')
            if lower.endswith(('.o2r','.otr')) and Path(lower).name not in {'soh.o2r','2ship.o2r'}:raise RuntimeError('Game-derived archive found in APK')
        for entry in json.loads(z.read('assets/support-manifest.json'))['files']:
            if hashlib.sha256(z.read('assets/runtime/'+entry['path'])).hexdigest()!=entry['sha256']:raise RuntimeError('Support manifest mismatch')
        if 'classes.dex' not in names:raise RuntimeError('No Android application code')
    print(f'PASS: {apk.name}: ARM64 game libraries, packaged dependencies, native entry points, 16 KiB ELF alignment, support hashes, no ROM filenames, no entrance dev build, no MQ walker dev build. This verifier does not test device operation.')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('apk',type=Path);verify(p.parse_args().apk)
