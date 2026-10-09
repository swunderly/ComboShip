"""Check preset scalar choices against the native engines' actual declarations."""
from pathlib import Path
import argparse,re
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);a=p.parse_args()
r=a.source.resolve()
header=(r/'combo/rando/ComboPresets.h').read_text(encoding='utf-8')
oot=(r/'soh/soh/Enhancements/randomizer/settings.cpp').read_text(encoding='utf-8')
mm=(r/'mm/2s2h/Rando/StaticData/Options.cpp').read_text(encoding='utf-8')
native={}
for kind,body in re.findall(r'OPT_(BOOL|U8)\((.*?)\);',oot,re.S):
    key=re.search(r'CVAR_RANDOMIZER_SETTING\("([^"]+)"\)',body)
    if not key:continue
    if kind=='BOOL':maximum=1
    else:
        choices=re.search(r'\{(.*?)\}',body,re.S)
        if not choices: continue # dynamically assembled options outside the preset scope
        numbers=re.search(r'NumOpts\((\d+),\s*(\d+)',choices.group(1))
        maximum=int(numbers.group(2)) if numbers else len(re.findall(r'"(?:[^"\\]|\\.)*"',choices.group(1)))-1
    native[key.group(1)]=maximum
count=0
for key,value in re.findall(r'\{"([^"]+)",\s*(\d+)\}',header):
    value=int(value)
    if key.startswith('gRandoSettings.'):
        suffix=key.removeprefix('gRandoSettings.')
        assert suffix in native,(key,'not a native setting')
        assert 0<=value<=native[suffix],(key,value,native[suffix])
        count+=1
    elif key.startswith('gRando.Options.'):
        suffix=key.removeprefix('gRando.Options.')
        assert re.search(r'RO\('+suffix+r'\s*,',mm),(key,'not a native setting')
        # Boolean shuffles/start/hints are shared RO_GENERIC_OFF/ON enums.
        if suffix.startswith(('RO_SHUFFLE_','RO_STARTING_','RO_HINTS_')) and suffix!='RO_HINTS_GOSSIP_STONE_STRENGTH':
            assert value in (0,1),(key,value)
        count+=1
assert count>=50,count
print(f'Validated {count} preset key/value pairs against native option declarations.')
