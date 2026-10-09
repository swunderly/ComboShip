#include "gui/ComboGenProgress.h"
#include <cassert>
#include <future>
#include <thread>
using namespace ComboRando;
int main() {
    ComboGenProgress progress;
    progress.Reset(); progress.startedMs=1000;
    assert(progress.ElapsedSeconds(300999)==299);
    assert(!progress.WarningDue(299) && progress.WarningDue(300));
    progress.KeepTrying(310);
    assert(!progress.WarningDue(609) && progress.WarningDue(610));
    std::promise<void> started;
    bool cancelled=false;
    std::thread worker([&]{
        GenerationScope scope(&progress);
        started.set_value();
        try { for(;;) { CheckGenerationCancellation(); std::this_thread::yield(); } }
        catch(const GenerationCancelled&) { cancelled=true; }
    });
    started.get_future().wait();
    assert(progress.RequestCancel());
    worker.join(); assert(cancelled && progress.CancelRequested());
    bool blocked=false;
    try { progress.BeginCommit(); } catch(const GenerationCancelled&) { blocked=true; }
    assert(blocked); // a cancelled worker must never publish a seed
    progress.Reset(); assert(!progress.CancelRequested() && progress.CanCancel());
    progress.BeginCommit(); assert(!progress.RequestCancel() && !progress.CanCancel());
    progress.Reset(); assert(progress.nextWarningSeconds==300);
    CheckGenerationCancellation(); // worker-local cancellation never poisons another operation
}
