"""Drive the developer Master Quest walker over adb: walk all 12 MQ dungeons, or stand Link at a Wallmaster.

    python tools/mq_walk_driver.py --serial emulator-5554 walk [--dungeons all] [--settle 20] [--out DIR]
    python tools/mq_walk_driver.py --serial emulator-5554 wallmaster Spirit 15 [--index 0]
    python tools/mq_walk_driver.py wallmasters
    python tools/mq_walk_driver.py --serial emulator-5554 send mq_status
    python tools/mq_walk_driver.py --serial emulator-5554 excerpt [--out DIR]

Needs a DEV APK, one built with the CMake option COMBO_MQ_WALK_DEV=ON (verify_apk.py refuses it, so it is never a
release). It installs over a release with `adb install -r`: both are signed with the same debug key. Before running:
start the app, load a file whose seed plays all 12 dungeons as Master Quest (ComboAndroid/DEVICE_TESTS.md, Master Quest
dungeons, check 3: MQ Dungeon Count 12) and stand Link anywhere in OoT, on solid ground, with no menu open. OoT must be
the game in front: the walker is part of the OoT engine.

How it talks to the game. The DEV build looks for the command file <root>/mq-walk-commands.txt every 15 frames, takes
it (renames it to mq-walk-commands.taken, then deletes it) and runs each line "<id> <command>" on the console. Only
these commands may come from the file:
    entrance <hex>                SoH's own developer warp (debugconsole.cpp): sets nextEntranceIndex and starts the
                                  transition, as the console window's command does
    mq_wait <scene hex> [frames]  waits until that scene has loaded and stays stable for 10 frames
    mq_walk [settle 1-90]         loads every room of the scene in turn through SoH's room-load path (each stays
                                  loaded `settle` frames, default 20), the start room last; ends "walk done scene=X
                                  rooms=N" or "walk aborted ... reason=R"
    mq_tp <room> <x> <y> <z> [yaw]  loads that room the same way and stands Link on the first floor below
                                  (x, y + 50, z); ends "tp done ... floor=found|none" or "tp failed"
    mq_status                     scene, room, Link's position, grounded, age, scene layer, room log, walker state
    mq_log 0|1                    sets the room log CVar gCombo.Debug.MqRoomLog without saving it (a
                                  later settings change in the menu saves it with the rest)
    mq_stop                       stops a walk (after going back to the start room), teleport or wait
The driver writes each line to a temporary file and renames it into place with adb shell, then waits for the replies
the game logs in <root>/native.log: "[ComboShip MQ walk] id=<id> ...", one "rc=<code> cmd=<name>" line per command,
and the end line of mq_wait, mq_walk and mq_tp. The same commands can be typed in the console window.

walk: sends mq_status (the start marker; Link must be in play) and mq_log 1, then for each dungeon: entrance <its
entrance>, mq_wait <its scene>, mq_walk <settle>. At the end it reads, with adb shell grep, only the "[ComboShip MQ"
lines of native.log written since its start marker (never a save, the configuration or any other file), writes them to
<out>/mq-walk-<run>.log and runs tools/mq_walk_compare.py on them: with --require-all when all 12 dungeons were walked.
Its exit code is the comparer's. The room log stays on afterwards; mq_log 0 turns it off.

wallmaster DUNGEON ROOM: warps to the dungeon if Link is not in it, then mq_tp onto the En_Wallmas (actor 0x0011) of
that room in combo/mq/data/mq_rooms.bin (--index picks among several), and prints mq_status a second later. A timer
Wallmaster (type 0) drops while Link stands anywhere in its room, a proximity one (type 1) while he stands within 320
units of it; a switch one (type 2, flag in the high byte) waits for that switch flag, so light the sun or press the
switch the room has (DEVICE_TESTS.md, Wallmaster Shuffle with Master Quest dungeons). `wallmasters` lists all 16 and
the commands, without a device.

What the walk changes: nothing is saved. A room load marks the room visited on the pause map (the walk puts the scene's
visited rooms back when it ends), clears the temporary clear and switch flags, adds an entry to the play statistics,
and can record a check's item that the camera sees; the warp sets the entrance a save would start from. Use a spare
file, keep autosave off, and do not save after a walk unless that is part of a check.

Dungeon entrances (SoH entrance_table.h) and scenes: Deku Tree 0x0000/0x00, Dodongo's Cavern 0x0004/0x01, Jabu-Jabu's
Belly 0x0028/0x02, Forest 0x0169/0x03, Fire 0x0165/0x04, Water 0x0010/0x05, Spirit 0x0082/0x06, Shadow 0x0037/0x07,
Bottom of the Well 0x0098/0x08, Ice Cavern 0x0088/0x09, Gerudo Training Ground 0x0008/0x0b, Ganon's Castle 0x0467/0x0d.
"""
from pathlib import Path
import argparse
import os
import re
import secrets
import shlex
import shutil
import subprocess
import sys
import time

REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / 'ComboAndroid/tests'))
import mq_bins  # noqa: E402  (the reader the host tests check against combo/mq/ComboMqData.cpp)

DEFAULT_ROOT = '/sdcard/ComboShip'
COMMAND_FILE = 'mq-walk-commands.txt'
WALK_TAG = '[ComboShip MQ walk]'
EXCERPT_TAG = '[ComboShip MQ '  # the walker's lines and the room log's "[ComboShip MQ room]" lines
WALLMASTER = 0x0011
# By OoTMM dungeon index (mq_bins.DUNGEONS): name, scene, SoH entrance index (soh/include/tables/entrance_table.h).
DUNGEONS = [
    ('Deku Tree', 0x00, 0x0000),
    ("Dodongo's Cavern", 0x01, 0x0004),
    ("Jabu-Jabu's Belly", 0x02, 0x0028),
    ('Forest Temple', 0x03, 0x0169),
    ('Fire Temple', 0x04, 0x0165),
    ('Water Temple', 0x05, 0x0010),
    ('Spirit Temple', 0x06, 0x0082),
    ('Shadow Temple', 0x07, 0x0037),
    ('Bottom of the Well', 0x08, 0x0098),
    ('Ice Cavern', 0x09, 0x0088),
    ('Gerudo Training Ground', 0x0B, 0x0008),
    ("Ganon's Castle", 0x0D, 0x0467),
]
REPLY = re.compile(r'rc=(-?\d+) cmd=(\S+)(?: (.*))?$')
WAIT_END = re.compile(r'wait (done|timeout|stopped)\b')
WALK_END = re.compile(r'walk (done|aborted) scene=0x([0-9a-f]+) rooms=(\d+)(?: reason=(\S+))?')
TP_END = re.compile(r'tp (done|failed)\b')
ENDS = {'mq_wait': WAIT_END, 'mq_walk': WALK_END, 'mq_tp': TP_END}
SAFE = re.compile(r'^[A-Za-z0-9_.:\- ]+$')


class DriverError(Exception):
    pass


def find_adb(explicit):
    if explicit:
        return explicit
    found = shutil.which('adb')
    if found:
        return found
    for name in ('ANDROID_HOME', 'ANDROID_SDK_ROOT'):
        if os.environ.get(name):
            for exe in ('adb.exe', 'adb'):
                path = Path(os.environ[name]) / 'platform-tools' / exe
                if path.exists():
                    return str(path)
    return 'adb'


class Adb:
    """The device, through adb shell: reads only the native.log lines grep selects, writes only the command file."""

    def __init__(self, adb, serial):
        self.base = [adb] + (['-s', serial] if serial else [])

    def shell_argv(self, script):
        return self.base + ['shell', script]

    def shell(self, script, check=True):
        run = subprocess.run(self.shell_argv(script), capture_output=True, text=True, encoding='utf-8',
                             errors='replace', timeout=120)
        if check and run.returncode != 0:
            raise DriverError(f'adb shell failed ({run.returncode}): {script}\n{run.stdout}{run.stderr}')
        return run

    def check(self, root):
        state = subprocess.run(self.base + ['get-state'], capture_output=True, text=True, timeout=60)
        if state.returncode != 0 or state.stdout.strip() != 'device':
            raise DriverError(f'no adb device: {state.stdout.strip()} {state.stderr.strip()}')
        if self.shell(f'test -f {shlex.quote(root + "/native.log")} && echo yes', check=False).stdout.strip() != 'yes':
            raise DriverError(f'{root}/native.log not found; pass --root with the folder the app shows under Files')

    def exists(self, path):
        return self.shell(f'test -e {shlex.quote(path)} && echo yes', check=False).stdout.strip() == 'yes'

    def write_atomic(self, path, line):
        """Writes one line to path.tmp and renames it to path, so the game never reads half a file."""
        temp = path + '.tmp'
        self.shell(f"printf '%s\\n' {shlex.quote(line)} > {shlex.quote(temp)} && "
                   f'mv -f {shlex.quote(temp)} {shlex.quote(path)}')

    def grep(self, path, needle):
        run = self.shell(f'grep -a -F -e {shlex.quote(needle)} {shlex.quote(path)}', check=False)
        if run.returncode not in (0, 1):
            raise DriverError(f'adb grep failed ({run.returncode}): {run.stderr.strip()}')
        return run.stdout.splitlines()


class Session:
    def __init__(self, adb, root, run=None, poll=0.5, scale=1.0, echo=print):
        self.adb, self.root, self.poll, self.scale, self.echo = adb, root.rstrip('/'), poll, scale, echo
        self.run = run or 'w' + secrets.token_hex(3)
        self.count = 0
        self.log = self.root + '/native.log'
        self.file = self.root + '/' + COMMAND_FILE

    def texts(self, ident):
        prefix = f'{WALK_TAG} id={ident} '
        return [line[line.index(prefix) + len(prefix):].strip() for line in self.adb.grep(self.log, prefix)]

    def send(self, command, timeout=60.0):
        """Runs one command; returns (rc, texts). Waits for the end line of mq_wait, mq_walk and mq_tp."""
        if not SAFE.match(command):
            raise DriverError(f'unsafe characters in "{command}"')
        name = command.split()[0]
        ident = f'{self.run}-{self.count}'
        self.count += 1
        # The last command was answered, so the game has read its file; it is normally gone by now. If the app cannot
        # delete it, the game skips the ids it already ran, and the next file simply replaces it.
        settle = time.monotonic() + 5 * self.scale
        while self.adb.exists(self.file) and time.monotonic() < settle:
            time.sleep(self.poll)
        deadline = time.monotonic() + timeout * self.scale
        self.adb.write_atomic(self.file, f'{ident} {command}')
        end = ENDS.get(name)
        while True:
            texts = self.texts(ident)
            reply = next((REPLY.match(text) for text in texts if REPLY.match(text)), None)
            if reply and (int(reply.group(1)) != 0 or end is None or any(end.match(t) for t in texts)):
                return int(reply.group(1)), texts
            if time.monotonic() > deadline:
                raise DriverError(f'no {"end line" if reply else "reply"} for "{command}" (id {ident}) after '
                                  f'{timeout * self.scale:.0f} s: is the DEV build running with OoT in front?')
            time.sleep(self.poll)

    def require(self, command, timeout=60.0):
        rc, texts = self.send(command, timeout)
        if rc != 0:
            raise DriverError(f'"{command}" failed: {" | ".join(texts)}')
        return texts

    def status(self):
        texts = self.require('mq_status')
        line = next((t for t in texts if t.startswith('status ')), '')
        return dict(token.partition('=')[::2] for token in line.split()[1:])

    def go(self, dungeon):
        name, scene, entrance = DUNGEONS[dungeon]
        self.require(f'entrance {entrance:x}')
        texts = self.require(f'mq_wait {scene:02x}', timeout=180)
        if not any(text.startswith('wait done') for text in texts):
            raise DriverError(f'{name} did not load: {" | ".join(texts)}')

    def excerpt(self):
        """The "[ComboShip MQ" lines of native.log since this run's first command."""
        lines = self.adb.grep(self.log, EXCERPT_TAG)
        marker = f'id={self.run}-0 '
        start = next((i for i, line in enumerate(lines) if marker in line), None)
        if start is None:
            raise DriverError(f'this run\'s first reply ({marker.strip()}) is not in {self.log}; did the app restart?')
        return lines[start:]


def bins():
    return mq_bins.decode(*mq_bins.load(REPO / 'combo/mq/data'))


def room_counts(data):
    counts = [0] * len(DUNGEONS)
    for room in data['rooms']:
        counts[room['dungeon']] += 1
    return counts


def wallmasters(data):
    """[(dungeon, room, index in the room's list, x, y, z, params)] for every En_Wallmas in mq_rooms.bin."""
    return [(room['dungeon'], room['room'], i, actor[1], actor[2], actor[3], actor[7] & 0xffff)
            for room in data['rooms'] for i, actor in enumerate(room['actors']) if actor[0] == WALLMASTER]


def wallmaster_kind(params):
    kind = params & 0xff
    if kind == 0:
        return 'timer: drops while Link stands in the room'
    if kind == 1:
        return 'proximity: drops while Link stands within 320 units'
    return f'switch: drops once switch flag 0x{params >> 8:02x} is set'


def dungeon_index(text):
    """A dungeon by mq_bins' short name (DT, DC, JJ, Forest, ..., GTG, Ganon) or full name, in any case."""
    for i, (name, _, _) in enumerate(DUNGEONS):
        if text.lower() in (mq_bins.DUNGEONS[i].lower(), name.lower()):
            return i
    raise DriverError(f'unknown dungeon "{text}"; use one of {", ".join(mq_bins.DUNGEONS)}')


def write_out(out, name, lines):
    out.mkdir(parents=True, exist_ok=True)
    path = out / name
    path.write_text(''.join(line + '\n' for line in lines), encoding='utf-8')
    return path


def walk(session, dungeons, settle, out, compare=True):
    data = bins()
    counts = room_counts(data)
    first = session.status()
    if first.get('play') != '1':
        raise DriverError('Link is not in play: load the MQ file and stand Link in OoT first')
    session.require('mq_log 1')
    results = []
    for dungeon in dungeons:
        name, scene, _ = DUNGEONS[dungeon]
        session.echo(f'{name}: warp, wait, walk')
        session.go(dungeon)
        rc, texts = session.send(f'mq_walk {settle}', timeout=60 + counts[dungeon] * (settle + 60) / 20)
        end = next((WALK_END.match(text) for text in texts if WALK_END.match(text)), None)
        if rc != 0 or end is None or end.group(1) != 'done':
            raise DriverError(f'{name}: the walk did not finish: {" | ".join(texts)}')
        rooms = int(end.group(3))
        session.echo(f'  walk done scene=0x{int(end.group(2), 16):02x} rooms={rooms} (bins: {counts[dungeon]})')
        results.append((name, rooms, counts[dungeon]))
    lines = session.excerpt()
    path = write_out(out, f'mq-walk-{session.run}.log', lines)
    session.echo(f'{len(lines)} lines in {path}')
    for name, rooms, want in results:
        if rooms != want:
            session.echo(f'NOTE: {name} walked {rooms} rooms, the bins have {want}')
    if not compare:
        return 0
    command = [sys.executable, str(REPO / 'tools/mq_walk_compare.py'), str(path)]
    if sorted(dungeons) == list(range(len(DUNGEONS))):
        command.append('--require-all')
    run = subprocess.run(command, capture_output=True, text=True, encoding='utf-8', errors='replace')
    write_out(out, f'mq-walk-{session.run}-compare.txt', (run.stdout + run.stderr).splitlines())
    session.echo(run.stdout + run.stderr)
    return run.returncode


def teleport(session, dungeon, room, index):
    spots = [w for w in wallmasters(bins()) if w[0] == dungeon and w[1] == room]
    if not spots:
        raise DriverError(f'{DUNGEONS[dungeon][0]} room {room} has no Wallmaster in mq_rooms.bin')
    if not 0 <= index < len(spots):
        raise DriverError(f'{DUNGEONS[dungeon][0]} room {room} has {len(spots)} Wallmasters; '
                          f'--index 0-{len(spots) - 1}')
    _, _, actor, x, y, z, params = spots[index]
    if session.status().get('scene') != f'0x{DUNGEONS[dungeon][1]:02x}':
        session.go(dungeon)
    texts = session.require(f'mq_tp {room} {x} {y} {z}', timeout=120)
    session.echo(f'{DUNGEONS[dungeon][0]} room {room}, actor {actor} params 0x{params:04x} '
                 f'({wallmaster_kind(params)}): {" | ".join(t for t in texts if t.startswith("tp "))}')
    time.sleep(1.0)
    session.echo(' '.join(f'{k}={v}' for k, v in session.status().items()))
    return 0 if any(text.startswith('tp done') for text in texts) else 1


def list_wallmasters(echo=print):
    for dungeon, room, actor, x, y, z, params in wallmasters(bins()):
        name, scene, entrance = DUNGEONS[dungeon]
        echo(f'{name} room {room} actor {actor} params 0x{params:04x} ({wallmaster_kind(params)}): '
             f'entrance {entrance:x}; mq_wait {scene:02x}; mq_tp {room} {x} {y} {z}')
    return 0


def main(argv=None, adb_factory=Adb):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--serial', help='adb serial (adb devices); required unless one device is attached')
    parser.add_argument('--adb', help='adb executable (default: PATH, then ANDROID_HOME/platform-tools)')
    parser.add_argument('--root', default=DEFAULT_ROOT,
                        help=f'the app\'s data folder on the device (default {DEFAULT_ROOT})')
    parser.add_argument('--timeout-scale', type=float, default=1.0, help='multiply every timeout (slow emulators)')
    commands = parser.add_subparsers(dest='command', required=True)
    walk_parser = commands.add_parser('walk', help='walk every room of the chosen dungeons and compare with the bins')
    walk_parser.add_argument('--dungeons', default='all', help='comma-separated (DT,DC,JJ,Forest,...) or all')
    walk_parser.add_argument('--settle', type=int, default=20, help='frames each room stays loaded (1-90)')
    walk_parser.add_argument('--out', type=Path, default=Path('mq-walk-out'), help='where the excerpt goes')
    walk_parser.add_argument('--no-compare', action='store_true', help='only write the excerpt')
    wm_parser = commands.add_parser('wallmaster', help='stand Link where a Master Quest Wallmaster drops')
    wm_parser.add_argument('dungeon')
    wm_parser.add_argument('room', type=int)
    wm_parser.add_argument('--index', type=int, default=0, help='which of the room\'s Wallmasters (default 0)')
    commands.add_parser('wallmasters', help='list the 16 MQ Wallmasters and their commands (no device)')
    send_parser = commands.add_parser('send', help='run one command and print its replies')
    send_parser.add_argument('words', nargs='+')
    excerpt_parser = commands.add_parser('excerpt', help='copy the MQ lines of native.log (all of this session)')
    excerpt_parser.add_argument('--out', type=Path, default=Path('mq-walk-out'))
    args = parser.parse_args(argv)
    if args.command == 'wallmasters':
        return list_wallmasters()
    if args.command == 'walk' and not 1 <= args.settle <= 90:
        parser.error('--settle must be 1-90')
    try:
        adb = adb_factory(find_adb(args.adb), args.serial)
        adb.check(args.root)
        session = Session(adb, args.root, scale=args.timeout_scale)
        if args.command == 'walk':
            dungeons = list(range(len(DUNGEONS))) if args.dungeons == 'all' else \
                [dungeon_index(d.strip()) for d in args.dungeons.split(',')]
            return walk(session, dungeons, args.settle, args.out, not args.no_compare)
        if args.command == 'wallmaster':
            return teleport(session, dungeon_index(args.dungeon), args.room, args.index)
        if args.command == 'send':
            rc, texts = session.send(' '.join(args.words), timeout=600)
            print('\n'.join(texts))
            return 0 if rc == 0 else 1
        lines = [line for line in adb.grep(session.log, EXCERPT_TAG)]
        print(write_out(args.out, f'mq-excerpt-{session.run}.log', lines))
        return 0
    except DriverError as error:
        print(f'FAIL: {error}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
