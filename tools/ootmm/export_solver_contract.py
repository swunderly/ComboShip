"""Extract the bounded M0 solver contract from the exact v32.3 Git blob."""
import hashlib
import re
from export_registry import PIN, arguments, json_bytes, publish, verified_blob

SOURCE = 'packages/logic/src/solver/entrances.ts'


def extract(reference):
    raw = verified_blob(reference, SOURCE)
    text = raw.decode('utf-8')
    def method(name, next_name):
        return text.split('  private '+name+'(', 1)[1].split('  private '+next_name+'(', 1)[0]
    polarity = {}
    for name in ('IN', 'OUT', 'ANY_OVERWORLD'):
        body = re.search(r'const POLARITY_'+name+r' = new Set<string>\(\[(.*?)\]\)', text, re.S)[1]
        polarity[name] = re.findall(r"'([^']+)'", body)
    pools = {}
    for name, following in [('poolSpawns', 'poolWallmasters'), ('poolWallmasters', 'poolBoss')]:
        body = method(name, following)
        pools[name] = {op: re.findall(r"types\."+op+r"\('([^']+)'\)", body) for op in ('add', 'delete')}
    age = method('validateAgeTemple', 'changeWorldAssumePools')
    assert 'new Pathfinder(worlds, this.settings, this.startingItems)' in age
    assert 'assumedItems' not in age
    assert "a.exits['OOT SPAWN ADULT'] = exprAge(AGE_ADULT)" in age
    assert "a.exits['OOT SPAWN CHILD'] = exprFalse()" in age
    assert "a.exits['OOT SPAWN ADULT'] = exprFalse()" in age
    assert "a.exits['OOT SPAWN CHILD'] = exprAge(AGE_CHILD)" in age
    target = re.search(r"const target = '([^']+)'", age)[1]
    return {'commit': PIN, 'path': SOURCE, 'sha256': hashlib.sha256(raw).hexdigest(),
            'polarity': polarity, 'pools': pools,
            'backtracks': int(re.search(r'this.backtrackCount >= (\d+)', text)[1]),
            'attempts': int(re.search(r'attempts >= (\d+)', text)[1]),
            'ageTemple': {'target': target, 'inventory': 'startingItems', 'oppositeSpawnOnly': True}}


def main():
    args = arguments(__doc__)
    publish(args.output_root, {
        'ComboAndroid/tests/fixtures/ootmm-32.3-solver-contract.json': json_bytes(extract(args.reference))
    }, args.check)
    print('Pinned v32.3 solver contract verified')


if __name__ == '__main__':
    main()
