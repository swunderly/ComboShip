"""Both-game generator (vanilla and Master Quest contexts) and whole host-fill differential; native engines
are explicit fakes. With --songs, the same generator with foreign-song logic on (limits design, Stage 1): the songs
id in every quest context and the shortcut differential with the songs on."""
from pathlib import Path
import argparse
import subprocess
import hostcxx
p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
p.add_argument('--songs', action='store_true')
a = p.parse_args()
a.build_dir.mkdir(parents=True, exist_ok=True)
exe = hostcxx.build(a.compiler, a.source/'ComboAndroid/tests/entrance_vanilla_generator_tests.cpp',
                   includes=[a.source/'combo', a.json_include], defines=['_CRT_SECURE_NO_WARNINGS'],
                   msvc=['/bigobj', '/O2'], gnu=['-O2'], exe=a.build_dir/'vanilla_generator.exe')
subprocess.run([str(exe), str(a.build_dir/('songs.json' if a.songs else 'differential.json')),
                str(a.source/'tools/ootmm/m1-contract.json')] + (['songs'] if a.songs else []),
               check=True, timeout=hostcxx.RUN_TIMEOUT)
