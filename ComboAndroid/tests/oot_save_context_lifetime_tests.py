"""Run the production oracle reset against tracked C++ and borrowed save storage."""
from pathlib import Path
import argparse
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
text = (args.source/'soh/soh/Enhancements/randomizer/logic.cpp').read_text(encoding='utf-8')
start = hostcxx.find(text, 'void Logic::NewSaveContext() {')
end = hostcxx.find(text, 'uint8_t Logic::InventorySlot(', start)
reset = text[start:end]

support = r'''
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <unordered_set>
std::unordered_set<void*> owned;
int allocations = 0, deletions = 0, destructors = 0, initializations = 0;
struct SaveContext {
    int value = 0;
    static void* operator new(std::size_t size) {
        void* result = std::malloc(size);
        if (!result) throw std::bad_alloc();
        assert(owned.insert(result).second);
        ++allocations;
        return result;
    }
    static void operator delete(void* value) noexcept {
        // A borrowed global must never be treated as private oracle storage.
        assert(owned.erase(value) == 1);
        ++deletions;
        std::free(value);
    }
    ~SaveContext() { ++destructors; }
};
SaveContext gSaveContext;
[[maybe_unused]] void CheckedCFree(void* value) {
    // Explicit allocator-family check: portable and deterministic even on hosts
    // whose malloc/new implementations happen to share an allocator internally.
    if (owned.count(value)) {
        std::fputs("C free received C++ new SaveContext storage\n", stderr);
        std::exit(86);
    }
    std::fputs("C free received unowned or borrowed save storage\n", stderr);
    std::exit(87);
}
struct Logic {
    SaveContext* mSaveContext = nullptr;
    void InitSaveContext() {
        assert(mSaveContext && owned.count(mSaveContext));
        assert(mSaveContext->value == 0);
        mSaveContext->value = ++initializations;
    }
    void NewSaveContext();
};
// Intercept only the extracted production function's C deallocation calls.
#define free CheckedCFree
'''
checks = r'''
#undef free
int main() {
    Logic logic;
    for (int i = 1; i <= 256; ++i) {
        logic.NewSaveContext();
        assert(logic.mSaveContext != &gSaveContext);
        assert(logic.mSaveContext->value == i);
        assert(allocations == i && deletions == i - 1 && destructors == i - 1);
        assert(owned.size() == 1);
        logic.mSaveContext->value = -1;
    }
    delete logic.mSaveContext;
    logic.mSaveContext = &gSaveContext;
    gSaveContext.value = 12345;
    logic.NewSaveContext();
    assert(gSaveContext.value == 12345);
    assert(allocations == 257 && deletions == 256 && destructors == 256);
    assert(logic.mSaveContext != &gSaveContext && logic.mSaveContext->value == 257);
    delete logic.mSaveContext;
    assert(owned.empty() && deletions == allocations && destructors == allocations);
    std::puts("Actual NewSaveContext: null, 256 replacements, borrowed global and balanced C++ lifetime verified");
}
'''
cpp = work/'oot_save_context_lifetime.cpp'
cpp.write_text(support + reset + checks, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, werror=True)
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)

# Keep the original defect as an executable negative control, not a text-only
# assertion about which allocator spelling the implementation currently uses.
assert hostcxx.contains(reset, 'delete mSaveContext;')
legacy = reset.replace('delete mSaveContext;', 'free(mSaveContext);')
assert legacy != reset
cpp = work/'oot_save_context_legacy_negative.cpp'
cpp.write_text(support + legacy + checks, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, werror=True)
result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
assert result.returncode == 86, result
assert 'C free received C++ new SaveContext storage' in result.stderr, result.stderr
print('Original C/C++ allocator mismatch rejected by the executable negative control.')
