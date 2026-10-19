"""tools/mq_walk_compare.py on Master Quest room logs (MQ port unit U8).

mq_room_log_tests --emit-sample writes, through the production line format (combo/mq/MqRoomLog.h), the lines a
correct runtime logs for every MQ room in the bins as child and as adult, and a vanilla line for each dungeon. The
comparer reads them back from behind the prefixes a phone's native.log and logcat add, among other log lines. Checks:
- every room matches the bins, every dungeon's rooms are seen at both ages, and --require-all passes;
- each kind of wrong line fails with its reason: a changed actor, a kept broken actor, a dropped object, a vanilla
  list in an MQ room, a wrong transition or path count, missing or unexpected paths, collision left vanilla, failed,
  patched elsewhere or with other counts, an MQ list in a vanilla dungeon, the wrong dungeon, an MQ room the bins
  lack, and a line it cannot read;
- a room never seen fails only with --require-all, and a log with no room lines fails;
- Jabu-Jabu's and Ice Cavern's cutscene layer keeps vanilla lists with a note, and no other dungeon's does.
"""
from pathlib import Path
import argparse
import re
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--exe', type=Path, required=True, help='mq_room_log_tests')
args = parser.parse_args()
source, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
tool = source / 'tools/mq_walk_compare.py'
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


sample = work / 'sample.log'
subprocess.run([str(args.exe), '--emit-sample', str(sample)], check=True)
lines = sample.read_text(encoding='utf-8').splitlines()
check(len(lines) == 220 * 2 + 12, f'the sample has {len(lines)} lines, expected 452')

# The prefixes the log sinks add: spdlog's in native.log and the logs folder, and logcat's, some with colour codes.
PREFIXES = ('[2026-10-19 10:00:00.000] [ComboShip] [info] [ComboMqRuntime.cpp:300] ',
            '10-19 10:00:00.000  4321  4400 I ComboShip: ', '\x1b[32m[info]\x1b[m ')


def write(name, room_lines):
    out = []
    for i, line in enumerate(room_lines):
        out.append(f'[2026-10-19 10:00:00.000] [ComboShip] [info] Master Quest room {i} of scene 0x04: 1 actors')
        out.append(PREFIXES[i % len(PREFIXES)] + line + ('\x1b[m' if i % len(PREFIXES) == 2 else ''))
    path = work / name
    path.write_text('\n'.join(out) + '\n', encoding='utf-8')
    return path


def compare(path, *extra):
    run = subprocess.run([sys.executable, str(tool), str(path), *extra], capture_output=True, text=True)
    return run.returncode, run.stdout + run.stderr


code, out = compare(write('all.log', lines), '--require-all')
check(code == 0 and out.rstrip().endswith('OK'), f'the correct sample does not pass:\n{out}')
check('452 room lines: 220 distinct MQ rooms matched the bins, 12 vanilla lines' in out, f'the summary:\n{out}')
for name, count in (('DT', 12), ('Fire', 27), ('Spirit', 29), ('Ice', 12), ('Ganon', 20)):
    check(f'  {name}: {count}/{count} rooms (child {count}, adult {count})' in out, f'{name} coverage:\n{out}')


def find(pattern):
    for i, line in enumerate(lines):
        if re.search(pattern, line):
            return i
    raise AssertionError(f'no sample line matches {pattern}')


def mutated(pattern, change):
    copy = list(lines)
    i = find(pattern)
    copy[i] = change(copy[i])
    check(copy[i] != lines[i], f'the mutation for {pattern} changed nothing')
    return copy


def sub(old, new, count=1):
    return lambda line: re.sub(old, new, line, count=count)


def drop_last_object(line):
    objects = re.search(r'objectList=(\S+)', line).group(1).split(',')
    line = re.sub(r'objects=mq:\d+', f'objects=mq:{len(objects) - 1}', line)
    return line.replace('objectList=' + ','.join(objects), 'objectList=' + ','.join(objects[:-1]))


def flip_first_params(line):
    first = re.search(r'actorList=([0-9a-f]{4})/([0-9a-f]{4})', line)
    return line.replace(first.group(0), f'actorList={first.group(1)}/{int(first.group(2), 16) ^ 1:04x}', 1)


MQ = r'quest=mq layer=0 '
WRONG = [
    ('a changed actor', mutated(r'dungeon=3 ' + MQ + r'.*actors=mq:[1-9]', flip_first_params),
     'actorList: ', 'first difference at index 0'),
    ('a kept broken actor', mutated(r'scene=0x01 room=9 .*' + MQ, sub(r'actorList=ffff/', 'actorList=0012/')),
     'DC room 9: actorList: ', 'first difference at index 0'),
    ('a dropped object', mutated(r'dungeon=7 ' + MQ + r'.*objects=mq:[2-9]', drop_last_object),
     'objectList: ', 'objects differ from the MQ room objects'),
    ('vanilla actors in an MQ room', mutated(r'dungeon=7 ' + MQ, sub(r'actors=mq:', 'actors=vanilla:')),
     'actors are vanilla, expected the MQ room actors'),
    ('a transition count', mutated(r'dungeon=5 ' + MQ,
                                   lambda line: re.sub(r'transitions=mq:(\d+)',
                                                       lambda m: f'transitions=mq:{int(m.group(1)) + 1}', line)),
     'transitions differ from the MQ transition actors'),
    ('a path count', mutated(r'dungeon=6 ' + MQ, sub(r'paths=mq:5', 'paths=mq:4')), 'paths differ from the MQ paths (5)'),
    ('vanilla paths in Fire', mutated(r'dungeon=4 ' + MQ, sub(r'paths=mq:4', 'paths=vanilla')),
     'paths are vanilla, expected the MQ paths (4)'),
    ('paths in the Deku Tree', mutated(r'dungeon=0 ' + MQ, sub(r'paths=vanilla', 'paths=mq:3')),
     'paths are MQ, but the dungeon has no MQ paths'),
    ('Fire collision vanilla', mutated(r'dungeon=4 ' + MQ, sub(r'collision=patched:553:33', 'collision=vanilla')),
     'collision is vanilla, expected the copy patched at 553 polygons, 33 surface types'),
    ('Fire collision failed', mutated(r'dungeon=4 ' + MQ, sub(r'collision=patched:553:33', 'collision=failed')),
     'collision is failed'),
    ('Ganon collision counts', mutated(r'dungeon=11 ' + MQ, sub(r'collision=patched:16:0', 'collision=patched:15:0')),
     'collision changes 15 polygons and 0 surface types, the patches name 16 and 0'),
    ('Water collision patched', mutated(r'dungeon=5 ' + MQ, sub(r'collision=vanilla', 'collision=patched:1:0')),
     'collision is patched, but the dungeon has no MQ collision patches'),
    ('an MQ list in a vanilla dungeon', mutated(r'dungeon=9 quest=vanilla', sub(r'transitions=vanilla:', 'transitions=mq:')),
     'transitions are MQ in a vanilla dungeon'),
    ('Fire collision in a vanilla dungeon', mutated(r'dungeon=4 quest=vanilla', sub(r'collision=vanilla', 'collision=patched:553:33')),
     'collision is patched in a vanilla dungeon'),
    ('the wrong dungeon', mutated(r'scene=0x04 .*' + MQ, sub(r'dungeon=4 ', 'dungeon=5 ')), 'scene 0x04 is not dungeon 5'),
    ('a room the bins lack', mutated(r'scene=0x08 room=0 .*' + MQ, sub(r'room=0 ', 'room=40 ')),
     'actors are MQ, but the bins have no room 40'),
    ('a count off its list', mutated(r'dungeon=2 ' + MQ + r'actors=mq:[1-9]', sub(r'actors=mq:(\d+)', r'actors=mq:\g<1>0')),
     'cannot read the line (a count differs from its list)'),
    ('a missing field', mutated(r'dungeon=10 ' + MQ, sub(r' layer=0', '')), 'cannot read the line (missing layer)'),
    ('Forest on a cutscene layer', mutated(r'dungeon=3 ' + MQ, sub(r'layer=0 actors=mq:', 'layer=4 actors=vanilla:')),
     'actors are vanilla, expected the MQ room actors'),
]
for name, room_lines, *messages in WRONG:
    code, out = compare(write(re.sub(r'\W+', '-', name) + '.log', room_lines))
    check(code == 1 and all(message in out for message in messages), f'{name}: exit {code}, expected {messages}:\n{out}')

# Jabu-Jabu on its cutscene layer runs its vanilla alternate headers (mq.c rewrites only the top-level header).
jabu = mutated(r'dungeon=2 ' + MQ, lambda line: re.sub(r'(actors|objects|transitions)=mq:', r'\1=vanilla:',
                                                     line.replace('layer=0', 'layer=4')))
code, out = compare(write('jabu-cutscene.log', jabu))
check(code == 0 and 'NOTE:' in out and 'actors vanilla on cutscene layer 4' in out,
      f'Jabu-Jabu\'s cutscene layer:\n{out}')

# A room never seen: reported, and a failure only with --require-all.
unseen = [line for line in lines if not line.startswith('[ComboShip MQ room] scene=0x09 room=5 ')]
code, out = compare(write('unseen.log', unseen))
check(code == 0 and 'Ice: 11/12 rooms (child 11, adult 11); not seen: 5' in out, f'an unseen room:\n{out}')
code, out = compare(write('unseen.log', unseen), '--require-all')
check(code == 1 and 'Ice: rooms never seen as MQ: 5' in out, f'--require-all with an unseen room:\n{out}')
code, out = compare(write('none.log', []))
check(code == 1 and 'no "[ComboShip MQ room]" lines' in out, f'a log without room lines:\n{out}')

for failure in failures:
    print('FAIL:', failure)
if failures:
    sys.exit(1)
print(f'PASS: tools/mq_walk_compare.py matches all 220 MQ rooms of the sample at both ages and refuses '
      f'{len(WRONG)} kinds of wrong line, an unseen room under --require-all and an empty log')
