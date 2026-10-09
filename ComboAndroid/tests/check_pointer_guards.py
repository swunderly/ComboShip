"""Exercise the production texture signature guard with Android ARM64 pointer tags.

Run from an x64 Visual Studio developer shell (or pass --cxx clang++/g++).
The resource manager stub records the original pointer without dereferencing fake
addresses, so every Android tag can be checked safely on a desktop host.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def run(root, compiler):
    source = (root / 'libultraship/src/fast/interpreter.cpp').read_text(encoding='utf-8')
    helper = ''
    if 'static bool IsPlausibleStringAddress(' in source:
        helper = function(source, 'static bool IsPlausibleStringAddress(')
        assert 'if (!IsPlausibleStringAddress(w1))' in source, 'OTR filepath guard must use the same check'
    signature = function(source, 'int32_t gfx_check_image_signature(')
    harness = r'''
#include <cstdint>
#include <cstdio>
#include <cstdlib>
struct ResourceManagerStub {
    uintptr_t seen = 0;
    int calls = 0;
    bool OtrSignatureCheck(const char* p) { seen = (uintptr_t)p; ++calls; return true; }
} resources;
ResourceManagerStub* ActiveResMgr() { return &resources; }
'''+helper+'\n'+signature+r'''
void check(uintptr_t address, bool accept) {
    resources.seen = 0; resources.calls = 0;
    const auto result = gfx_check_image_signature(reinterpret_cast<const char*>(address));
    if (result != static_cast<int>(accept) || resources.calls != (accept ? 1 : 0) ||
        (accept && resources.seen != address)) {
        std::fprintf(stderr, "FAIL address=0x%llx expected=%d result=%d calls=%d preserved=%d\n",
            (unsigned long long)address, accept, result, resources.calls, resources.seen == address);
        std::exit(1);
    }
}
int main() {
    check(0, false); check(0xFFFE, false); check(0x10000, true);
    check(0x0000007AB1234560ull, true); check(0x0000007AB1234561ull, false);
    check(0xFFFFFFFFFFFFFFFFull, false); check(0xFFFF000012345678ull, false);
    check(0x0001000012345678ull, false);
    for (uintptr_t tag = 0; tag < 256; ++tag) {
        const auto high = tag << 56;
#if defined(__ANDROID__) && defined(__aarch64__)
        check(high | 0x0000007AB1234560ull, true);
#else
        check(high | 0x0000007AB1234560ull, tag == 0);
#endif
        check(high, false); check(high | 0xFFFE, false);
        check(high | 0x0000007AB1234561ull, false);
        check(high | 0x00FF000012345678ull, false);
    }
    std::puts("PASS: 1288 pointer cases; valid pointers reach the resource manager with their tags intact");
}
'''
    with tempfile.TemporaryDirectory(prefix='combo-pointer-tests-') as directory:
        work = Path(directory)
        test = work / 'pointer_test.cpp'
        test.write_text(harness, encoding='utf-8')
        for android in (True, False):
            output = work / ('android.exe' if android else 'desktop.exe')
            msvc = Path(compiler).name.lower() in ('cl', 'cl.exe')
            if msvc:
                command = [compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', str(test), '/Fe:'+str(output)]
                if android: command += ['/D__ANDROID__', '/D__aarch64__']
            else:
                command = [compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(test), '-o', str(output)]
                if android: command += ['-D__ANDROID__', '-D__aarch64__']
            subprocess.run(command, cwd=work, check=True)
            print('Android ARM64' if android else 'Desktop', flush=True)
            subprocess.run([str(output)], cwd=work, check=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('upstream', type=Path)
    parser.add_argument('--cxx', default='cl.exe')
    args = parser.parse_args()
    run(args.upstream.resolve(), args.cxx)
