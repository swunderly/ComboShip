"""Shared helpers for the Python harnesses that compile extracted production code.

Each harness writes one C++ file built from production functions and engine
stubs. build() compiles it with an MSVC-style driver (cl, clang-cl) or a
GNU-style one (c++, g++, clang++), always with assertions enabled and with
support/no_crash_dialogs.cpp linked in, and exits with the compiler output on
failure. Run the result with timeout=RUN_TIMEOUT. find()/contains() locate production text
regardless of formatting: the port's sources are clang-formatted, so a
signature or expression may be wrapped or spaced differently from the
harness's literal.
"""
from pathlib import Path
import os
import re
import subprocess
import sys

DEFAULT_COMPILER = 'cl' if os.name == 'nt' else 'c++'
COMPILE_TIMEOUT = 600  # seconds
RUN_TIMEOUT = 300
# On Windows, failed asserts and crashes print to stderr instead of opening dialogs.
NO_CRASH_DIALOGS = Path(__file__).resolve().parent/'support/no_crash_dialogs.cpp'


def msvc_style(compiler):
    return Path(compiler).stem.lower() in ('cl', 'clang-cl')


def build(compiler, source, *, std='c++20', includes=(), defines=(), warnings=True, werror=False,
          msvc=(), gnu=(), link=(), sources=(), exe=None):
    """Compile source (plus any extra sources) into an executable and return its path.

    The executable defaults to the source's name in the same directory. warnings
    maps to /W4 or -Wall -Wextra and werror to /WX or -Werror. msvc and gnu are
    extra flags for that driver style only; link lists linker inputs: libraries
    and /LIBPATH options for the MSVC style, -L/-l flags for the GNU style.
    """
    # The compiler runs in the source's directory, so pass absolute paths.
    source = Path(source).resolve()
    exe = Path(exe).resolve() if exe else source.with_suffix('.exe' if os.name == 'nt' else '')
    includes = [Path(path).resolve() for path in includes]
    inputs = [str(source), *(str(Path(path).resolve()) for path in sources), str(NO_CRASH_DIALOGS)]
    if msvc_style(compiler):
        command = [compiler, '/nologo', '/EHsc', '/std:' + std, '/UNDEBUG',
                   *(['/W4'] if warnings else []), *(['/WX'] if werror else []),
                   *('/I' + str(path) for path in includes), *('/D' + name for name in defines), *msvc,
                   *inputs, '/Fe:' + str(exe), '/Fo:' + str(exe.parent) + os.sep]
        if link:
            command += ['/link', *link]
    else:
        command = [compiler, '-std=' + std, '-UNDEBUG',
                   # Harness stubs declare engine constants the extracted code may not use.
                   *(['-Wall', '-Wextra', '-Wno-unused-const-variable'] if warnings else []), *(['-Werror'] if werror else []),
                   *('-I' + str(path) for path in includes), *('-D' + name for name in defines), *gnu,
                   *inputs, '-o', str(exe), *link]
    result = subprocess.run([str(part) for part in command], cwd=source.parent, capture_output=True, text=True,
                            timeout=COMPILE_TIMEOUT)
    exe.with_suffix('.log').write_text(' '.join(map(str, command)) + '\n' + result.stdout + result.stderr, encoding='utf-8')
    if result.returncode:
        print(' '.join(map(str, command)))
        print(result.stdout + result.stderr)
        sys.exit(f'Compilation failed with exit code {result.returncode}')
    return exe


def _pattern(needle):
    return re.compile(r'\s*'.join(re.escape(char) for char in needle if not char.isspace()))


def find(text, needle, start=0):
    """Like text.index(needle, start), ignoring all whitespace differences."""
    match = _pattern(needle).search(text, start)
    if not match:
        raise ValueError(f'production text not found: {needle!r}')
    return match.start()


def contains(text, needle):
    return _pattern(needle).search(text) is not None
