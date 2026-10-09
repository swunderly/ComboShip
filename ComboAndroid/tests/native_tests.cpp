#include "InputState.h"
#include "RuntimePaths.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
using namespace comboandroid;
int main() {
    InputState input;
    input.set(0xffffffffU, {2.f, -2.f, std::numeric_limits<float>::infinity(), 0.5f});
    auto state = input.snapshot();
    assert(state.buttons == 0x001fffffU);
    assert(state.axes[0] == 1 && state.axes[1] == -1 && state.axes[2] == 0);
    assert(axisToSdl(-1) == -32768 && axisToSdl(1) == 32767);
    assert(axisToSdl(std::numeric_limits<float>::quiet_NaN()) == 0);
    std::thread writer([&] { for (int i=0; i<10000; ++i) input.set(3, {1,1,1,1}); });
    for(int i=0;i<10000;++i) { state = input.snapshot(); assert(state.axes[0] <= 1.f); }
    writer.join(); input.releaseAll(); assert(input.snapshot().buttons == 0);
    char root[] = "/tmp/comboship-native-XXXXXX"; assert(mkdtemp(root));
    assert(libraryPath(root, "libsoh.so") == std::string(root) + "/libsoh.so");
    bool rejected=false; try { libraryPath(root, "../libsoh.so"); } catch(...) { rejected=true; }
    assert(rejected);
    rejected=false; try { requireDirectory("relative"); } catch(...) { rejected=true; }
    assert(rejected);
    {
        SessionLock held(root);
        const pid_t child = fork(); assert(child >= 0);
        if (child == 0) { try { SessionLock other(root); _exit(1); } catch(...) { _exit(0); } }
        int status=0; assert(waitpid(child,&status,0) == child); assert(WIFEXITED(status) && WEXITSTATUS(status)==0);
    }
    { SessionLock released(root); }
    std::filesystem::remove_all(root);
    return 0;
}
