"""The opposite-age Temple probe stays unwired: no production source assigns the hook or reaches it.

Static scan. Bindings::validateAgeTemple stays unassigned, so active OoT/Both entrance logic keeps
failing closed at the Shuffler's missing-hook check; no combo, launcher or engine source looks up the
probe exports or the diagnostic getter, or includes the adapter headers; the reserved entrance-layout
seed format still rejects every layout, before decoding it, in a build with no active entrance
capability.
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
probe = {'soh/soh/Enhancements/randomizer/TempleProbe.cpp', 'soh/soh/Enhancements/randomizer/TempleProbe.h'}
for path, text in files('combo', 'ComboAndroid/app/src', 'soh/soh', 'mm/2s2h'):
    rel = path.relative_to(source).as_posix()
    # The hook is declared and called by Shuffle.h, never assigned or designated-initialized.
    assert not re.search(r'validateAgeTemple\s*=(?!=)', text), rel
    assert not re.search(r'\.validateAgeTemple\s*[={]', text), rel
    if rel not in probe:
        assert 'SOH_TempleProbe_' not in text, rel
    if rel != 'soh/soh/OTRGlobals.cpp':
        assert 'SOH_GetOracleTempleAges' not in text, rel
    if rel not in adapter:
        assert not re.search(r'#\s*include\s*[<"][^>"]*Temple(Adapter|Candidate)\.h', text), rel
        assert 'MakeTempleValidator' not in text and 'TempleProbeSession' not in text, rel
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
print('Temple probe unwired: hook unassigned, exports and getter unreferenced, adapter unused, layout guard intact')
