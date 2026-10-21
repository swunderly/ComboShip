"""tools/mq_walk_driver.py on a fake device that answers with the developer walker's own lines.

The fake device takes the command file the way the DEV build does (the first look after a write finds the file and
takes it) and logs what the game would: the walker's lines from mq_walk_tests --emit (combo/mq/MqWalk.h's real
state machine and formats) and the room log's lines from mq_room_log_tests --emit-sample (combo/mq/MqRoomLog.h).
Checks:
- the driver's dungeon table: the 12 OoTMM dungeons in mq_bins order, each scene the one mq_walk_compare.py maps to
  that dungeon, and each entrance a row of SoH's entrance_table.h for that scene's first spawn;
- walk: the driver sends mq_status (its start marker), mq_log 1, then entrance, mq_wait and mq_walk for each of the
  12 dungeons in order, one command file at a time, each after the game took the last one; it keeps only the
  "[ComboShip MQ" lines after its marker (an earlier session's room line is left out), and tools/mq_walk_compare.py
  passes with --require-all; a subset of dungeons is compared without --require-all; an aborted walk, a refused
  command, Link not in play and a device that never takes the file each fail with the reason;
- wallmaster: the 16 En_Wallmas spots come from mq_rooms.bin (Forest 19, 20; Spirit 5 twice, 8 twice, 15 four
  times; Bottom of the Well 0, 1, 3; Gerudo Training Ground 2 twice; Ganon's Castle 18) at the positions the MQ design
  doc lists; the teleport sends that actor's position and warps only when Link is in another scene;
- the real adb commands: the command file goes to a .tmp file renamed in the same adb shell call, every word quoted
  for the device shell; the log is read with grep -F; the driver never pulls a file or names a save;
- --help names every command and the end lines, and DEVICE_TESTS.md documents the driver and the DEV build.
"""
from pathlib import Path
import argparse
import importlib.util
import io
import contextlib
import re
import shutil
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--walk-exe', type=Path, required=True, help='mq_walk_tests')
parser.add_argument('--room-log-exe', type=Path, required=True, help='mq_room_log_tests')
args = parser.parse_args()
source, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
for old in ('out', 'part', 'abort', 'refuse', 'menu', 'never'):
    shutil.rmtree(work / old, ignore_errors=True)
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


spec = importlib.util.spec_from_file_location('mq_walk_driver', source / 'tools/mq_walk_driver.py')
driver = importlib.util.module_from_spec(spec)
spec.loader.exec_module(driver)
sys.path.insert(0, str(source / 'tools'))
import mq_walk_compare  # noqa: E402
driver.time.sleep = lambda seconds: None

# --- The dungeon table against SoH's tables and the comparer.
scenes = re.findall(r'/\* (0x[0-9A-Fa-f]+) \*/ DEFINE_SCENE\(\w+, \w+, (\w+),',
                    (source / 'soh/include/tables/scene_table.h').read_text(encoding='utf-8'))
scene_ids = {name: int(number, 16) for number, name in scenes}
entrances = re.findall(r'DEFINE_ENTRANCE\((\w+), (\w+), (\d+),',
                       (source / 'soh/include/tables/entrance_table.h').read_text(encoding='utf-8'))
check(len(driver.DUNGEONS) == 12 == len(driver.mq_bins.DUNGEONS), 'twelve dungeons')
for index, (name, scene, entrance) in enumerate(driver.DUNGEONS):
    check(mq_walk_compare.dungeon_from_scene(scene) == index, f'{name}: scene 0x{scene:02x} is dungeon {index}')
    row = entrances[entrance] if entrance < len(entrances) else ('', '', '')
    check(scene_ids.get(row[1]) == scene and row[2] == '0' and row[0].endswith('_ENTRANCE'),
          f'{name}: entrance 0x{entrance:04x} is {row}, not the first spawn of scene 0x{scene:02x}')

# --- What the game logs, from the production formats.
sample = work / 'room-sample.log'
subprocess.run([str(args.room_log_exe), '--emit-sample', str(sample)], check=True)
room_lines = {}
for line in sample.read_text(encoding='utf-8').splitlines():
    fields = dict(token.partition('=')[::2] for token in line.split()[3:])
    if fields.get('quest') == 'mq' and fields.get('layer') == '0':
        room_lines[(int(fields['dungeon']), int(fields['room']))] = line
bins = driver.bins()
counts = driver.room_counts(bins)
check(counts == [12, 17, 16, 23, 27, 23, 29, 23, 7, 12, 11, 20], f'bins room counts {counts}')
check(len(room_lines) == 220, f'{len(room_lines)} MQ child room lines in the sample')


def emit(*words):
    run = subprocess.run([str(args.walk_exe), '--emit', *map(str, words)], capture_output=True, text=True, check=True)
    return run.stdout.splitlines()


PREFIX = '[2026-10-21 10:00:00.000] [ComboMqWalk.cpp:99] [info] '


class FakeDevice:
    """The DEV build behind adb: takes the command file on the first look after a write and logs its answers."""

    def __init__(self, scene=0x51, in_play=True, abort=None, refuse=None, never_take=False):
        self.scene, self.room, self.in_play = scene, 0, in_play
        self.abort, self.refuse, self.never_take = abort, refuse, never_take
        self.log = [PREFIX + '[ComboShip MQ room] scene=0x03 room=4 dungeon=3 quest=vanilla layer=2 earlier session',
                    PREFIX + 'Room Init - curRoom.num: 0x0']
        self.pending = None
        self.commands, self.overlaps, self.roots = [], 0, set()

    def check(self, root):
        self.roots.add(root)

    def exists(self, path):
        check(path == '/sdcard/ComboShip/mq-walk-commands.txt', f'command file {path}')
        if self.pending is None:
            return False
        if not self.never_take:
            self.take()
        return True

    def write_atomic(self, path, line):
        check(path == '/sdcard/ComboShip/mq-walk-commands.txt', f'command file {path}')
        if self.pending is not None:
            self.overlaps += 1
        self.pending = line

    def grep(self, path, needle):
        check(path == '/sdcard/ComboShip/native.log', f'log {path}')
        if self.pending is not None and not self.never_take:
            self.take()
        return [line for line in self.log if needle in line]

    def say(self, ident, lines):
        self.log += [PREFIX + '[ComboShip MQ walk] ' + line.replace('id=@ ', f'id={ident} ', 1) for line in lines]

    def take(self):
        ident, command = self.pending.split(' ', 1)
        self.pending = None
        self.commands.append(command)
        words = command.split()
        name = words[0]
        if name == self.refuse:
            self.say(ident, ['id=@ refused room log off (mq_log 1)'] + emit('reply', 1, name, 'refused', 'room', 'log'))
            return
        dungeon = mq_walk_compare.dungeon_from_scene(self.scene)
        if name == 'mq_status':
            self.say(ident, emit('status', self.scene, self.room, counts[dungeon] if dungeon >= 0 else 1)
                     if self.in_play else ['id=@ status play=0 busy=none'])
            self.say(ident, emit('reply', 0, name))
        elif name == 'mq_log':
            self.say(ident, ['id=@ log room=1'] + emit('reply', 0, name, 'log', 'room=1'))
        elif name == 'entrance':
            index = next(i for i, d in enumerate(driver.DUNGEONS) if d[2] == int(words[1], 16))
            self.scene, self.room = driver.DUNGEONS[index][1], 0
            self.say(ident, emit('reply', 0, name))
            self.log.append(PREFIX + room_lines[(index, 0)])
        elif name == 'mq_wait':
            self.say(ident, ['id=@ wait queued'] + emit('reply', 0, name, 'wait', 'queued'))
            self.say(ident, emit('wait', int(words[1], 16)))
        elif name == 'mq_walk':
            self.say(ident, ['id=@ walk queued'] + emit('reply', 0, name, 'walk', 'queued'))
            lines = emit('walk', self.scene, counts[dungeon], self.room)
            if dungeon == self.abort:
                lines = lines[:4] + [f'id=@ walk aborted scene=0x{self.scene:02x} rooms=3 reason=scene-changed']
            for line in lines:
                self.say(ident, [line])
                room = re.search(r' walk room scene=0x[0-9a-f]+ room=(\d+) ', line)
                if room:
                    self.log.append(PREFIX + room_lines[(dungeon, int(room.group(1)))])
        elif name == 'mq_tp':
            room, x, y, z = int(words[1]), *map(float, words[2:5])
            self.say(ident, ['id=@ tp queued'] + emit('reply', 0, name, 'tp', 'queued'))
            self.say(ident, emit('tp', self.scene, counts[dungeon], self.room, room, int(x), int(y), int(z)))
            self.log.append(PREFIX + room_lines[(dungeon, room)])
            self.room = room
        else:
            self.say(ident, emit('reply', 1, name, 'unknown'))


def run(device, *argv):
    out = io.StringIO()
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(out):
        code = driver.main(['--serial', 'emulator-5554', *argv], adb_factory=lambda adb, serial: device)
    return code, out.getvalue()


# --- The whole walk.
device = FakeDevice()
code, output = run(device, 'walk', '--out', str(work / 'out'))
check(code == 0, f'the walk of all 12 passes: {output[-2000:]}')
expected = ['mq_status', 'mq_log 1']
for name, scene, entrance in driver.DUNGEONS:
    expected += [f'entrance {entrance:x}', f'mq_wait {scene:02x}', 'mq_walk 20']
check(device.commands == expected, f'commands {device.commands}')
check(device.overlaps == 0, 'one command file at a time')
excerpt = sorted((work / 'out').glob('mq-walk-*.log'))
check(len(excerpt) == 1, f'one excerpt: {excerpt}')
if excerpt:
    text = excerpt[0].read_text(encoding='utf-8')
    check('earlier session' not in text and 'Room Init' not in text, 'only this run\'s MQ lines')
    check(text.count('[ComboShip MQ room]') == 220 + 12, f'{text.count("[ComboShip MQ room]")} room lines')
    check(text.count(' walk done scene=') == 12, 'twelve walk done lines')
    compare = subprocess.run([sys.executable, str(source / 'tools/mq_walk_compare.py'), str(excerpt[0]),
                              '--require-all'], capture_output=True, text=True)
    check(compare.returncode == 0 and compare.stdout.strip().endswith('OK'), 'the excerpt passes --require-all')
check('12/12 rooms' in output and output.strip().endswith('OK'), f'the comparer ran and passed: {output[-600:]}')
compared = sorted((work / 'out').glob('mq-walk-*-compare.txt'))
check(len(compared) == 1 and 'OK' in compared[0].read_text(encoding='utf-8'), 'the comparer\'s output is kept')

# A subset: compared, but not with --require-all, which it could not pass.
device = FakeDevice(scene=0x03)
code, output = run(device, 'walk', '--dungeons', 'Forest,spirit temple', '--settle', '5', '--out', str(work / 'part'))
check(code == 0, f'a two-dungeon walk passes without --require-all: {output[-800:]}')
check(device.commands[2:] == ['entrance 169', 'mq_wait 03', 'mq_walk 5', 'entrance 82', 'mq_wait 06', 'mq_walk 5'],
      f'subset commands {device.commands}')
check('not seen' in output, 'the comparer lists the dungeons not walked')

# Failures.
code, output = run(FakeDevice(abort=4), 'walk', '--out', str(work / 'abort'))
check(code == 2 and 'Fire Temple: the walk did not finish' in output and 'reason=scene-changed' in output,
      f'an aborted walk fails: {output[-400:]}')
code, output = run(FakeDevice(refuse='mq_walk'), 'walk', '--out', str(work / 'refuse'))
check(code == 2 and 'refused room log off' in output, f'a refused walk fails: {output[-400:]}')
code, output = run(FakeDevice(in_play=False), 'walk', '--out', str(work / 'menu'))
check(code == 2 and 'Link is not in play' in output, f'not in play: {output[-400:]}')
code, output = run(FakeDevice(never_take=True), '--timeout-scale', '0.002', 'walk', '--out', str(work / 'never'))
check(code == 2 and ('never taken' in output or 'no reply' in output), f'a device that never takes the file: {output}')

# --- The Wallmasters.
spots = [(d, r, x, y, z, p) for d, r, _, x, y, z, p in driver.wallmasters(bins)]
check(spots == [(3, 19, -1763, 1467, -3318, 0x0001), (3, 20, 1998, 1472, -3326, 0x0001),
                (6, 5, 66, 353, -1188, 0x0001), (6, 5, 426, 1041, -1237, 0x3302),
                (6, 8, -1497, 843, -103, 0x3a02), (6, 8, -1522, 843, 184, 0x3b02),
                (6, 15, 1152, 480, -874, 0x0000), (6, 15, 976, 480, -910, 0x3a02),
                (6, 15, 1130, 480, -812, 0x3b02), (6, 15, 1102, 480, -937, 0x3d02),
                (8, 0, -2, 0, -742, 0x0001), (8, 1, -26, -622, -324, 0x0000), (8, 3, 1004, 177, -1066, 0x0000),
                (10, 2, -1589, -80, -1329, 0x0001), (10, 2, -1579, 193, -1547, 0x1102),
                (11, 18, -1201, 150, 1237, 0x3902)], f'wallmasters {spots}')
device = FakeDevice(scene=0x51)
code, output = run(device, 'wallmaster', 'Spirit', '15', '--index', '3')
check(code == 0 and
      device.commands == ['mq_status', 'entrance 82', 'mq_wait 06', 'mq_tp 15 1102 480 -937', 'mq_status'],
      f'a Wallmaster teleport warps first: {device.commands} {output}')
check('switch: drops once switch flag 0x3d is set' in output and 'tp done scene=0x06 room=15' in output,
      f'the teleport is described: {output}')
device = FakeDevice(scene=0x08)
code, output = run(device, 'wallmaster', 'BotW', '1')
check(code == 0 and device.commands == ['mq_status', 'mq_tp 1 -26 -622 -324', 'mq_status'],
      f'no warp inside the dungeon: {device.commands}')
code, output = run(FakeDevice(scene=0x07), 'wallmaster', 'Shadow', '0')
check(code == 2 and 'has no Wallmaster' in output, 'MQ Shadow has none')
code, output = run(FakeDevice(), 'wallmaster', 'GTG', '2', '--index', '2')
check(code == 2 and 'has 2 Wallmasters' in output, 'an index out of range')
listing = io.StringIO()
with contextlib.redirect_stdout(listing):
    driver.main(['wallmasters'])
check(len(listing.getvalue().splitlines()) == 16 and 'mq_tp 18 -1201 150 1237' in listing.getvalue(),
      'wallmasters lists all 16 commands')

# --- The real adb commands.
calls = []


class Completed:
    def __init__(self, stdout='', returncode=0):
        self.stdout, self.stderr, self.returncode = stdout, '', returncode


def fake_run(argv, **kwargs):
    calls.append(list(argv))
    return Completed('device\n' if 'get-state' in argv else 'yes\n' if argv[-1].startswith('test ') else '')


real_run = driver.subprocess.run
driver.subprocess.run = fake_run
try:
    adb = driver.Adb('adb', 'emulator-5554')
    adb.check('/sdcard/ComboShip')
    adb.write_atomic('/sdcard/ComboShip/mq-walk-commands.txt', 'w1-0 mq_tp 15 1152 480 -874')
    adb.grep('/sdcard/ComboShip/native.log', '[ComboShip MQ walk] id=w1-0 ')
finally:
    driver.subprocess.run = real_run
check(calls[0] == ['adb', '-s', 'emulator-5554', 'get-state'], f'get-state {calls[0]}')
check(calls[2] == ['adb', '-s', 'emulator-5554', 'shell',
                   "printf '%s\\n' 'w1-0 mq_tp 15 1152 480 -874' > /sdcard/ComboShip/mq-walk-commands.txt.tmp && "
                   'mv -f /sdcard/ComboShip/mq-walk-commands.txt.tmp /sdcard/ComboShip/mq-walk-commands.txt'],
      f'the command file write {calls[2]}')
check(calls[3] == ['adb', '-s', 'emulator-5554', 'shell',
                   "grep -a -F -e '[ComboShip MQ walk] id=w1-0 ' /sdcard/ComboShip/native.log"], f'the grep {calls[3]}')
tool = (source / 'tools/mq_walk_driver.py').read_text(encoding='utf-8')
code_only = re.sub(r'"""(?:.|\n)*?"""', '', tool)
for word in ("'pull'", 'run-as', '.combosav', 'Save/', "'push'", 'rm ', 'comboship.json'):
    check(word not in code_only, f'the driver never uses {word}')
try:
    driver.Session(FakeDevice(), '/sdcard/ComboShip').send('mq_status; rm -rf /')
    check(False, 'a command with shell characters is refused')
except driver.DriverError:
    pass

# --- Documentation.
help_text = subprocess.run([sys.executable, str(source / 'tools/mq_walk_driver.py'), '--help'], capture_output=True,
                           text=True).stdout
for word in ('entrance <hex>', 'mq_wait', 'mq_walk', 'mq_tp', 'mq_status', 'mq_log', 'mq_stop', 'walk done scene=X',
             'COMBO_MQ_WALK_DEV', 'mq-walk-commands.txt', '--require-all', 'wallmaster', 'never a save'):
    check(word in help_text, f'--help mentions {word}')
tests_doc = (source / 'ComboAndroid/DEVICE_TESTS.md').read_text(encoding='utf-8')
for word in ('tools/mq_walk_driver.py', 'COMBO_MQ_WALK_DEV', 'mq_tp', 'mq_walk', 'walk done'):
    check(word in tests_doc, f'DEVICE_TESTS.md mentions {word}')

for failure in failures:
    print('FAIL:', failure)
if failures:
    sys.exit(1)
print('MQ walk driver: 12-dungeon walk, subsets, failures, Wallmaster teleports, adb commands and docs passed.')
