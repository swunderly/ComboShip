"""The opposite-age Temple probe is reachable only through its production wiring.

Static scan. Only the unified generator (combo/rando/entrances/UnifiedBridge.h, M1-07) may assign
Bindings::validateAgeTemple, and only from a TempleProbeRun (TempleNative.h: the gate, a prepared
session and the code mapping); without it the hook stays unassigned, so active OoT/Both entrance
logic keeps failing closed at the Shuffler's missing-hook check. Only TempleNative.h names the probe
exports; nothing but that bridge includes TempleNative.h, and nothing but TempleNative.h uses the
adapter headers or calls the gate. No source looks up the diagnostic getter. The reserved
entrance-layout seed format still rejects every layout, before decoding it, in a build with no active
entrance capability.
"""
from pathlib import Path
import argparse
import re

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
args = parser.parse_args()
source = args.source.resolve()
SUFFIXES = {'.c', '.cpp', '.h', '.hpp', '.inc', '.java', '.kt'}


def files(*roots):
    for root in roots:
        for path in sorted((source/root).rglob('*')):
            if path.suffix in SUFFIXES and path.is_file():
                yield path, path.read_text(encoding='utf-8', errors='replace')


adapter = {'combo/rando/entrances/TempleAdapter.h', 'combo/rando/entrances/TempleCandidate.h'}
native = 'combo/rando/entrances/TempleNative.h'
bridge = 'combo/rando/entrances/UnifiedBridge.h'
probe = {'soh/soh/Enhancements/randomizer/TempleProbe.cpp', 'soh/soh/Enhancements/randomizer/TempleProbe.h'}
assigned = []
for path, text in files('combo', 'ComboAndroid/app/src', 'soh/soh', 'mm/2s2h'):
    rel = path.relative_to(source).as_posix()
    # The hook is declared and called by Shuffle.h; only the bridge assigns it, from a TempleProbeRun.
    assignments = re.findall(r'validateAgeTemple\s*=(?!=)[^;]*;', text)
    if rel == bridge:
        for assignment in assignments:
            assert re.fullmatch(r'validateAgeTemple\s*=\s*\w+(\.|->)Validator\(\);', assignment), assignment
        assigned += assignments
    else:
        assert not assignments, rel
        assert not re.search(r'\.validateAgeTemple\s*[={]', text), rel
    assert not re.search(r'validateAgeTemple\s*\{', text), rel  # never brace-initialized
    if rel not in probe | {native}:
        assert 'SOH_TempleProbe_' not in text, rel
    if rel != 'soh/soh/OTRGlobals.cpp':
        assert 'SOH_GetOracleTempleAges' not in text, rel
    if rel not in adapter | {native}:
        assert not re.search(r'#\s*include\s*[<"][^>"]*Temple(Adapter|Candidate)\.h', text), rel
        assert 'MakeTempleValidator' not in text and 'TempleProbeSession' not in text, rel
        # The gate runs only inside a TempleProbeRun, which skips it when nothing can be queried.
        assert 'CheckTempleCapability' not in text, rel
    if rel not in {native, bridge}:
        assert not re.search(r'#\s*include\s*[<"][^>"]*TempleNative\.h', text), rel
        assert 'TempleProbeRun' not in text and 'ResolveTempleProbe' not in text, rel
    if rel not in probe | {'soh/soh/OTRGlobals.cpp'}:
        assert not re.search(r'#\s*include\s*[<"][^>"]*TempleProbe(Core)?\.h', text), rel

shuffle = (source/'combo/rando/entrances/Shuffle.h').read_text(encoding='utf-8')
assert 'if (bindings.logicEnabled && settings.games != Games::Mm && !bindings.validateAgeTemple)' in shuffle
assert 'throw std::invalid_argument("OoT entrance validation requires the starting-item opposite-age Temple probe");' in shuffle
seed = (source/'combo/rando/ComboSeedFormat.h').read_text(encoding='utf-8')
guard = seed[seed.index('inline void RequireSupportedLayout'):]
guard = guard[:guard.index('\n}\n')]
presence = guard.index('if (seed.contains("entranceLayout") || (type != seed.end() && *type == kEntranceSeedFileType)) {')
empty = guard.index('if (caps.Empty())\n            throw UnsupportedComboSeedFormat();')
assert presence < empty < guard.index('try {')
for wrapper in ('RequireSupportedSeedCapabilities', 'RequireSupportedSettingsCapabilities'):
    body = seed[seed.index('inline void ' + wrapper):]
    assert 'RequireSupportedLayout(' in body[:body.index('\n}\n')], wrapper
state = 'assigned only in UnifiedBridge.h from a TempleProbeRun' if assigned else 'unassigned'
print(f'Temple probe wiring: hook {state}, exports named only in TempleNative.h, getter unreferenced, '
      'adapter used only by TempleNative.h, layout guard intact')
