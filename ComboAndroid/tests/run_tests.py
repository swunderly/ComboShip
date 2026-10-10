"""Configure, build and run the ComboAndroid host test suite with one command.

    python ComboAndroid/tests/run_tests.py [--build-dir DIR] [-R REGEX] [--fresh]

On Windows the newest Visual Studio x64 C++ environment is imported (vswhere,
then vcvars64.bat) unless cl is already on PATH; elsewhere the default c++ is
used. Third-party headers (nlohmann json, Dear ImGui, spdlog) come from a
FetchContent _deps directory of a sibling desktop build, or --deps. Tests whose
inputs or platform are unavailable are registered by CMake and reported as
skipped with the reason, never silently dropped. Exit status is nonzero when
configuration, building or any test fails.
"""
from pathlib import Path
import argparse
import ctypes
import os
import platform
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ElementTree

TESTS = Path(__file__).resolve().parent
REPO = TESTS.parents[1]
WINDOWS = os.name == 'nt'


def visual_studio_environment():
    """Return the environment of an x64 Visual Studio developer prompt."""
    installer = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'))/'Microsoft Visual Studio/Installer'
    roots = []
    if (installer/'vswhere.exe').exists():
        found = subprocess.run([str(installer/'vswhere.exe'), '-latest', '-products', '*', '-requires',
                                'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
                               capture_output=True, text=True, timeout=120).stdout
        roots += [Path(line.strip()) for line in found.splitlines() if line.strip()]
    for base in (os.environ.get('ProgramFiles', r'C:\Program Files'), os.environ.get('ProgramFiles(x86)', '')):
        roots += sorted((Path(base)/'Microsoft Visual Studio').glob('*/*'), reverse=True) if base else []
    for root in roots:
        vcvars = root/'VC/Auxiliary/Build/vcvars64.bat'
        if vcvars.exists():
            env = dict(os.environ, PATH=str(installer) + os.pathsep + os.environ.get('PATH', ''))
            dump = subprocess.run(f'"{vcvars}" >nul && set', shell=True, env=env,
                                  capture_output=True, text=True, check=True, timeout=300).stdout
            print(f'Using Visual Studio at {root}')
            return {key.upper(): value for key, value in
                    (line.split('=', 1) for line in dump.splitlines() if '=' in line and not line.startswith('='))}
    sys.exit('Visual Studio with the x64 C++ tools was not found. Run from an x64 developer prompt instead.')


def find_deps(explicit):
    """Locate a _deps directory providing nlohmann_json-src (plus imgui-src and spdlog-src)."""
    if explicit:
        return explicit.resolve()
    if os.environ.get('COMBO_TEST_DEPS'):
        return Path(os.environ['COMBO_TEST_DEPS']).resolve()
    candidates = sorted(REPO.glob('build*/_deps')) + sorted(REPO.parent.glob('build*/_deps'))
    complete = [d for d in candidates if (d/'nlohmann_json-src/include/nlohmann/json.hpp').exists()]
    # Prefer a desktop build that also fetched Dear ImGui and spdlog.
    complete.sort(key=lambda d: (not (d/'imgui-src/imgui.cpp').exists(), 'android' in d.parent.name.lower()))
    return complete[0] if complete else None


def find_block_clang(env):
    """A GNU-driver clang for the contracted-math collision regression (Windows only)."""
    if os.environ.get('COMBO_BLOCK_TEST_CLANG'):
        return os.environ['COMBO_BLOCK_TEST_CLANG']
    found = shutil.which('clang', path=env.get('PATH'))
    if found:
        return found
    roots = [Path(env.get('VCINSTALLDIR', 'missing'))/'Tools/Llvm/x64/bin',
             Path(os.environ.get('ProgramFiles', r'C:\Program Files'))/'LLVM/bin']
    sdks = [os.environ.get(name) for name in ('ANDROID_NDK_HOME', 'ANDROID_NDK_ROOT')]
    sdks = [Path(p) for p in sdks if p]
    for name in ('ANDROID_HOME', 'ANDROID_SDK_ROOT'):
        if os.environ.get(name):
            sdks += sorted(Path(os.environ[name]).glob('ndk/*'), reverse=True)
    properties = REPO/'ComboAndroid/local.properties'
    if properties.exists():
        for line in properties.read_text(encoding='utf-8').splitlines():
            if line.startswith('sdk.dir='):
                sdks += sorted(Path(line[len('sdk.dir='):].replace('\\:', ':')).glob('ndk/*'), reverse=True)
    roots += [ndk/'toolchains/llvm/prebuilt/windows-x86_64/bin' for ndk in sdks]
    for root in roots:
        if (root/'clang.exe').exists():
            return str(root/'clang.exe')
    return None


def find_jdk():
    """JAVA_HOME, else a JDK bundled with Android Studio or installed under Program Files."""
    homes = [Path(os.environ['JAVA_HOME'])] if os.environ.get('JAVA_HOME') else []
    if WINDOWS:
        programs = Path(os.environ.get('ProgramFiles', r'C:\Program Files'))
        for pattern in ('Android/openjdk/*', 'Android/Android Studio/jbr', 'Java/jdk*', 'Eclipse Adoptium/jdk*', 'Microsoft/jdk*'):
            homes += sorted(programs.glob(pattern), reverse=True)
    javac = 'javac.exe' if WINDOWS else 'javac'
    for home in homes:
        if (home/'bin'/javac).exists():
            return home
    return None


def run(command, env, **kwargs):
    print('+', ' '.join(str(part) for part in command), flush=True)
    return subprocess.run([str(part) for part in command], env=env, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--build-dir', type=Path, default=REPO.parent/'build-tests',
                        help='Test build directory (default: build-tests next to the repository)')
    parser.add_argument('--deps', type=Path, help='FetchContent _deps directory with nlohmann_json-src, imgui-src, spdlog-src')
    parser.add_argument('--sdl', type=Path, help='SDL2 prefix (include/SDL2, lib/) for the audio latency harness')
    parser.add_argument('--jobs', '-j', type=int, default=os.cpu_count() or 4)
    parser.add_argument('--tests-regex', '-R', help='Only run tests matching this ctest regex')
    parser.add_argument('--timeout', type=int, default=300, help='Per-test timeout in seconds (default 300)')
    parser.add_argument('--fresh', action='store_true', help='Delete the build directory first')
    parser.add_argument('--no-build', action='store_true', help='Run ctest against the existing build')
    args = parser.parse_args()
    build = args.build_dir.resolve()

    env = dict(os.environ)
    if WINDOWS:
        if not shutil.which('cl'):
            env = visual_studio_environment()
        # Inherited by ctest and the tests: a crash must not open an error dialog.
        ctypes.windll.kernel32.SetErrorMode(0x0001 | 0x0002 | 0x8000)  # FAILCRITICALERRORS|NOGPFAULTERRORBOX|NOOPENFILEERRORBOX
    path = env.get('PATH')
    cmake, ctest = shutil.which('cmake', path=path), shutil.which('ctest', path=path)
    if not cmake or not ctest:
        sys.exit('cmake and ctest must be on PATH.')
    cache = [f'-DCOMBO_SOURCE_DIR={(REPO/"combo").as_posix()}', f'-DCOMBO_TEST_TIMEOUT={args.timeout}']
    deps = find_deps(args.deps)
    if deps:
        print(f'Using third-party headers from {deps}')
        cache.append(f'-DCOMBO_JSON_INCLUDE_DIR={(deps/"nlohmann_json-src/include").as_posix()}')
    elif Path('/usr/include/nlohmann/json.hpp').exists():
        cache.append('-DCOMBO_JSON_INCLUDE_DIR=/usr/include')
    else:
        print('No nlohmann json headers found (pass --deps); JSON-dependent tests will be skipped.')
    if WINDOWS:
        clang = find_block_clang(env)
        if clang:
            cache.append(f'-DCOMBO_BLOCK_TEST_CLANG={Path(clang).as_posix()}')
    sdl = args.sdl or os.environ.get('COMBO_SDL_DIR')
    if sdl:
        cache.append(f'-DCOMBO_SDL_DIR={Path(sdl).resolve().as_posix()}')
    jdk = find_jdk()
    if jdk:
        env['JAVA_HOME'] = str(jdk)

    if args.fresh and build.exists():
        shutil.rmtree(build)
    if not args.no_build:
        configured = (build/'CMakeCache.txt').exists()
        generator = ['-G', 'Ninja'] if not configured and shutil.which('ninja', path=path) else []
        if run([cmake, '-S', TESTS, '-B', build, *generator, *cache], env).returncode:
            sys.exit('CMake configuration failed.')
        # Keep going past a broken target: its tests then fail as "not run".
        cached = (build/'CMakeCache.txt').read_text(encoding='utf-8', errors='replace')
        keep_going = (['--', '-k', '0'] if 'CMAKE_GENERATOR:INTERNAL=Ninja' in cached else
                      ['--', '-k'] if 'Makefiles' in cached else [])
        built = run([cmake, '--build', build, '-j', str(args.jobs), *keep_going], env)

    junit = build/'ctest-results.xml'
    junit.unlink(missing_ok=True)
    command = [ctest, '--test-dir', build, '--output-on-failure', '-j', str(args.jobs), '--timeout', str(args.timeout),
               '--output-junit', junit]
    if args.tests_regex:
        command += ['-R', args.tests_regex]
    run(command, env)
    if not junit.exists():
        sys.exit('ctest produced no results.')

    passed, failed, skipped = [], [], []
    for case in ElementTree.parse(junit).getroot().iter('testcase'):
        name, status = case.get('name'), case.get('status')
        lines = [line.strip() for line in (case.findtext('system-out') or '').splitlines() if line.strip()]
        reason = next((line.split('SKIPPED:', 1)[1].strip() for line in lines if line.startswith('SKIPPED:')), None)
        if status == 'run':
            passed.append((name, ''))
        elif reason or status == 'disabled':
            skipped.append((name, reason or 'disabled'))
        else:  # failed, crashed, timed out, or its executable did not build
            problem = case.find('failure')
            failed.append((name, lines[-1][:160] if lines else problem.get('message') if problem is not None else status))

    print('\n' + '=' * 72)
    for title, rows in (('PASSED', passed), ('SKIPPED', skipped), ('FAILED', failed)):
        print(f'{title} ({len(rows)})')
        for name, detail in sorted(rows):
            print(f'  {name}' + (f'  -- {detail}' if detail else ''))
    build_failed = not args.no_build and built.returncode
    if build_failed:
        print('Some targets failed to build; their tests are listed as failed.')
    print(f'{len(passed)} passed, {len(failed)} failed, {len(skipped)} skipped ({platform.system()})')
    return 1 if failed or build_failed else 0


if __name__ == '__main__':
    sys.exit(main())
