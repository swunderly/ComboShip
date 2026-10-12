"""Every OoT draw that shows a check's reward model must report it to the tracker first.

Scans the OoT sources for GetItemEntry_Draw calls (the shared reward-model draw). Each one must be
preceded, in the same function and under COMBO_BUILD, by CheckTracker::RecordObservedItem or
Combo_OOTRecordObservedItem with the same entry, so the check can become Seen. Recording comes
before the draw because the visibility test reads the model matrix the draw then changes. An MM
item's sighting is kept only when the foreign draw reports that its real model, not the sentinel
Blue Rupee, was drawn.
"""
import argparse
from pathlib import Path
import re
import sys

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
source = p.parse_args().source.resolve()

# Draws that never show an unobtained check in play: DemoEffect draws a dungeon reward or the Light
# Arrows only inside the cutscene that awards them, and Spiritual Stones the player already owns.
EXEMPT = {'soh/src/overlays/actors/ovl_Demo_Effect/z_demo_effect.c'}
DEFINITION = 'soh/src/code/z_draw.c'
RECORD = re.compile(r'(?:CheckTracker::RecordObservedItem|Combo_OOTRecordObservedItem)\(\s*[^,]+,\s*&?\s*(\w+)\s*\)')
DRAW = re.compile(r'GetItemEntry_Draw\(\s*\w+\s*,\s*(\w+)\s*\)')

failures, hosts = [], []
files = sorted([*source.glob('soh/soh/**/*.cpp'), *source.glob('soh/src/**/*.c')])
for path in files:
    rel = path.relative_to(source).as_posix()
    text = path.read_text(encoding='utf-8', errors='replace')
    if rel == DEFINITION or 'GetItemEntry_Draw(' not in text:
        continue
    for draw in DRAW.finditer(text):
        line = text.count('\n', 0, draw.start()) + 1
        if rel in EXEMPT:
            continue
        # The enclosing function starts after the previous top-level closing brace.
        start = text.rfind('\n}\n', 0, draw.start())
        function = text[start + 1 if start >= 0 else 0:draw.start()]
        records = list(RECORD.finditer(function))
        where = f'{rel}:{line}'
        if not records:
            failures.append(f'{where}: draws a reward model without recording the sighting first')
            continue
        record = records[-1]
        if record.group(1) != draw.group(1):
            failures.append(f'{where}: records {record.group(1)!r} but draws {draw.group(1)!r}')
        # The record call must be compiled only in the combo build, like the rest of the tracker.
        directives = re.findall(r'^#\s*(ifdef|ifndef|if|else|elif|endif)\b(.*)$', function[:record.start()], re.M)
        if not directives or directives[-1][0] != 'ifdef' or directives[-1][1].strip() != 'COMBO_BUILD':
            failures.append(f'{where}: the tracker call is not inside #ifdef COMBO_BUILD')
        hosts.append(where)

# The hosts the coverage map found (freestanding/drops, heart containers, Etcetera items, bowling and
# chest-game items, Skulltula tokens, shop shelves, silver rupees, fairies, overworld fish, the
# Ocarina of Time) must all still be scanned, so a refactor cannot silently drop one from the check.
expected = {
    'soh/soh/Enhancements/randomizer/hook_handlers.cpp': 5,
    'soh/soh/Enhancements/randomizer/ShuffleFairies.cpp': 1,
    'soh/soh/Enhancements/randomizer/fishsanity.cpp': 1,
    'soh/soh/Enhancements/randomizer/silver_rupees/SilverRupees.cpp': 1,
    'soh/src/overlays/actors/ovl_En_GirlA/z_en_girla.c': 1,
    'soh/src/overlays/actors/ovl_Item_Ocarina/z_item_ocarina.c': 1,
}
for rel, count in expected.items():
    found = sum(host.startswith(rel + ':') for host in hosts)
    if found != count:
        failures.append(f'{rel}: expected {count} recorded reward draws, found {found}')

# An MM item's sighting is stored only after its real model drew: every sentinel fallback in the
# foreign draw reports false, and the draw hook hands that result to the tracker.
foreign = (source/'combo/menu/ComboForeignDrawOOT.h').read_text(encoding='utf-8')
body = foreign[foreign.index('inline bool OOT_DrawComboForeign('):]
body = body[:body.index('\n}\n')]
fallbacks = [body[m.end():body.index(';', m.end())].strip()
             for m in re.finditer(r'GetItem_Draw\(play, getItemEntry->gid\);', body)]
if len(fallbacks) != 2 or any(after != 'return false' for after in fallbacks):
    failures.append('OOT_DrawComboForeign: a sentinel fallback does not return false')
hook = (source/'soh/soh/Enhancements/randomizer/draw.cpp').read_text(encoding='utf-8')
hook = hook[hook.index('extern "C" void Randomizer_DrawComboForeign('):]
hook = hook[:hook.index('\n}\n')]
if not re.search(r'const bool (\w+) = OOT_DrawComboForeign\(play, getItemEntry\);.*'
                 r'CheckTracker::RecordForeignDraw\([^;]*comboForeignCheck[^;]*,\s*\1\);', hook, re.S):
    failures.append('Randomizer_DrawComboForeign: the foreign draw result does not reach the tracker')
if failures:
    print('\n'.join(failures))
    sys.exit(1)
print(f'PASS: all {len(hosts)} OoT reward-model draws record the sighting first ({len(EXEMPT)} cutscene-only exemption)')
