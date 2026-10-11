"""Exercise the real headless entry wrapper across successful and failed returns."""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
text = (source/'combo/ComboRandoHeadless.cpp').read_text(encoding='utf-8')
assert '#define SDL_MAIN_HANDLED' in text.split('#include', 1)[0]
lifetime = text.split('struct HeadlessContextLifetime {', 1)[1].split('\n};', 1)[0]
lifetime = 'struct HeadlessContextLifetime {' + lifetime + '\n};\n'
entry = text[text.index('int main(int argc, char** argv) {'):]
entry = entry.replace('int main(', 'int HeadlessEntry(', 1)
cmake = (source/'combo/CMakeLists.txt').read_text(encoding='utf-8')
assert re.search(r'target_link_libraries\(comborando\s+PRIVATE\s+libultraship\b', cmake)
assert '#include "ship/Context.h"' in text
cpp = work/'headless_lifetime.cpp'
cpp.write_text(r'''
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
bool registryAlive = false, contextAlive = false;
int destroyed = 0;
namespace Ship {
struct Context {
    static void DestroyInstance() {
        if (!contextAlive) return;
        // Reproduces the real registry/context lifetime obligation without invoking
        // undefined behavior by locking an already destroyed host mutex.
        assert(registryAlive);
        contextAlive = false;
        ++destroyed;
    }
};
}
// Like Context::mContext, this owner is registered before lazy logger creation.
struct ContextOwner {~ContextOwner() {Ship::Context::DestroyInstance();}} contextOwner;
struct Registry {
    Registry() {registryAlive = true;}
    ~Registry() {assert(!contextAlive); registryAlive = false;}
};
void InitializeHeadless() {static Registry registry; contextAlive = true;}
int mode = 0;
int RunHeadless(int, char**) {
    if (mode == 4) return 2; // setup failure before any shared context exists
    InitializeHeadless();
    if (mode == 5) throw std::runtime_error("fixture setup exception");
    if (mode == 6) throw 42;
    return mode; // production exits 0,1,2,3 must keep their exact meaning
}
''' + lifetime + entry + r'''
int main(int argc, char** argv) {
    if (argc > 1) {
        // Negative control: without the production entry guard, static teardown
        // reaches the registry first while Context still intends to use it.
        assert(argv[1][0] == 'n');
        InitializeHeadless();
        return 0;
    }
    for (mode = 0; mode <= 6; ++mode) {
        const int previous = destroyed;
        const int result = HeadlessEntry(0, nullptr);
        assert(result == (mode >= 4 ? 2 : mode));
        assert(!contextAlive && registryAlive);
        assert(destroyed == previous + (mode != 4));
    }
    std::cout << "Headless cleanup precedes registry destruction on every exit path\n";
}
''', encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, werror=True)
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
negative = subprocess.run([str(exe), 'negative'], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
assert negative.returncode != 0, 'Missing cleanup unexpectedly survived the static-order negative control'
assert 'contextAlive' in negative.stderr, negative.stderr
print('Real entry/guard: exits 0/1/2/3, pre-init failure, handled exceptions and late-teardown negative control verified.')
