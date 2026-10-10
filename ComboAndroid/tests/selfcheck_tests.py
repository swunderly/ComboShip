"""Prove that a failing assert() fails a test quickly and visibly.

Runs support/failing_assert.cpp built two ways: as a CMake test target (same
flags and support library as the C++ tests) and through hostcxx.build() (as
the Python harnesses build). Each must exit nonzero within seconds and print
the failed expression with its file and line on stderr, without a dialog.
"""
from pathlib import Path
import argparse
import re
import subprocess
import time
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
parser.add_argument('--exe', type=Path, required=True, help='failing_assert built by CMake')
args = parser.parse_args()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
source = Path(__file__).resolve().parent/'support/failing_assert.cpp'
harness_copy = work/'failing_assert.cpp'
harness_copy.write_text(source.read_text(encoding='utf-8'), encoding='utf-8')
harness_exe = hostcxx.build(args.compiler, harness_copy, werror=True)
for exe in (args.exe.resolve(), harness_exe):
    started = time.monotonic()
    result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=60)
    elapsed = time.monotonic() - started
    print(f'{exe.name}: exit {result.returncode:#x} after {elapsed:.1f} s; stderr: {result.stderr.strip()}')
    assert result.returncode != 0, 'a failed assertion must fail the process'
    assert 'COMBO_SELFCHECK_EXPECTED_FAILURE' in result.stderr, 'the failed expression must reach stderr'
    # MSVC: 'file ...failing_assert.cpp, line 8'; glibc: '...failing_assert.cpp:8:'.
    assert re.search(r'failing_assert.cpp(, line |:)8', result.stderr), 'file and line must reach stderr'
    assert elapsed < 20, 'the failure must not wait on a dialog or error reporting'
print('PASS: failed assertions report expression, file and line on stderr and exit nonzero at once')
