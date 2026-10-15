"""Execute MM's real installer, Wallmaster transport and capture fallback, shipping and dev.

Engine boundary stubs observe preflight, arrival fixup, dungeon-respawn clearing and transition
writes in order. Real entrance headers validate every installed route; assumptions cannot transport.
The pre-M1 legacy body is frozen separately so its existing semantics remain byte-identical.
"""
from pathlib import Path
import argparse
import hashlib
import re
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
text = (source/'mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp').read_text(encoding='utf-8')


def extract(text, signature):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production function')


transport = extract(text, 'extern "C" int Combo_MMWallmasterTransport(')
legacy_start = transport.index('    if (!play || !actor || !IS_RANDO || !CE::ActiveMmWallmasters().active())')
assert hashlib.sha256(transport[legacy_start:].encode()).hexdigest() == \
    '644ed2f6bee01494752520986ed35f142ad5a12947acb1f22823e78210054c28', 'Legacy transport changed'
installer = extract(text, 'extern "C" COMBO_EXPORT int MM_ApplyEntranceExpansionForCombo(')
active = extract(text, 'extern "C" COMBO_EXPORT int Combo_MMWallmasterErActive(')
actor = (source/'mm/src/overlays/actors/ovl_En_Wallmas/z_en_wallmas.c').read_text(encoding='utf-8')
capture = extract(extract(actor, 'void EnWallmas_TakePlayer(EnWallmas* this, PlayState* play) {'),
                  'if (this->timer == 30)')
capture = capture.replace('this->', 'self->')  # C++ reserves the C actor's parameter name.
includes = '\n'.join(re.findall(r'^#include "rando/entrances/[^\"]+"$', text, re.M))
support = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
''' + includes + r'''
namespace CE = ComboRando::Entrances;
CE::MmMarkedSourceTransition sMmMarkedSource;
CE::MmMixedMarkedTransition sMmMixedSource;
#define COMBO_EXPORT
bool rando = true;
#define IS_RANDO rando
int logs = 0;
template<class... A> void Log(const char*, const A&...) { ++logs; }
#define SPDLOG_ERROR(...) Log(__VA_ARGS__)
enum { SCENE_DANPEI2TEST = 4, SCENE_REDEAD = 5, TRANS_TRIGGER_START = 6,
       TRANS_TYPE_FADE_BLACK = 7, WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE = 0,
       WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE = 1, WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE = 2,
       WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE = 3, NA_SE_OC_ABYSS = 8 };
bool cleared[4]{};
#define CHECK_WEEKEVENTREG(flag) cleared[flag]
struct Actor { int room; };
struct PlayState { int sceneId, nextEntrance, transitionTrigger, transitionType, unrelated; };
struct SaveContext { int fileNum, nextCutsceneIndex, respawnFlag, unrelated; } gSaveContext;
PlayState initialPlay;
SaveContext initialSave;
PlayState* currentPlay;
int effectiveAge = 1, preflightResult = 1, fallbackCalls = 0, cycleCalls = 0;
uint32_t expectedDestination;
std::vector<std::string> order;
void Unchanged() {
    assert(!std::memcmp(&gSaveContext, &initialSave, sizeof(gSaveContext)));
    assert(!std::memcmp(currentPlay, &initialPlay, sizeof(initialPlay)));
}
int MM_GetEffectiveHumanAge(int file) {
    assert(file == initialSave.fileNum); Unchanged(); order.push_back("age"); return effectiveAge;
}
int MM_CanEntranceArrival(int file, uint32_t id, int age, int crossAge) {
    assert(file == initialSave.fileNum && id == expectedDestination && age == effectiveAge && crossAge == 0);
    assert(order == std::vector<std::string>{"age"}); Unchanged(); order.push_back("preflight");
    return preflightResult;
}
namespace ComboRando::Entrances {
uint32_t TrackedFixMmNativeArrival(uint32_t id, bool wf, bool sh, bool gb, bool st, int& cutscene) {
    assert((order == std::vector<std::string>{"age", "preflight"})); Unchanged();
    assert(id == expectedDestination); order.push_back("fix");
    return FixMmNativeArrival(id, wf, sh, gb, st, cutscene);
}
}
void Combo_MMFaroreEndCycle() {
    Unchanged(); ++cycleCalls;
    if (CE::ActiveMmUnified().active())
        assert((order == std::vector<std::string>{"age", "preflight", "fix"}));
    else assert(order.empty());
    order.push_back("end-cycle");
}
void Audio_PlaySfx(int sound) { assert(sound == NA_SE_OC_ABYSS); }
void func_80169FDC(PlayState*) { ++fallbackCalls; Unchanged(); }
struct EnWallmas { Actor actor; int timer; };
void Reset(PlayState& play, int scene = SCENE_DANPEI2TEST) {
    play = {scene, 1234, 20, 21, 22}; currentPlay = &play; initialPlay = play;
    gSaveContext = {2, 0x321, -3, 456}; initialSave = gSaveContext;
    logs = fallbackCalls = cycleCalls = 0; order.clear(); effectiveAge = preflightResult = 1;
}
'''
tests = r'''
using Json = nlohmann::json;
Json Seed(size_t rotation = 0) {
    CE::EntranceLayout layout;
    layout.required = {std::string(CE::kMmTrancheCapability)};
    layout.devBuild = true;
    layout.settings.erWallmasters = CE::Mode::OwnGame;
    CE::Overrides routes;
    for (size_t i = 0; i < CE::kMmWallmasterSources.size(); ++i)
        routes[CE::kMmWallmasterSources[i]] = CE::kMmBaseWallmasterDestinations[(i + rotation) % 7];
    layout.overrides = routes;
    return {{"fileType", ComboRando::kEntranceSeedFileType}, {"version", ComboRando::kEntranceSeedVersion},
            {"entranceLayout", CE::EncodeEntranceLayout(layout)}};
}
Json LegacySeed() {
    std::vector<CE::SavedOverride> rows;
    for (size_t i = 0; i < CE::kMmWallmasterSources.size(); ++i) {
        const auto* src = CE::Find(CE::kMmWallmasterSources[i]);
        const auto* dst = CE::Find(CE::kMmBaseWallmasterDestinations[i]);
        rows.push_back({std::string(src->name), std::string(dst->name), src->game, dst->game, src->id, dst->id});
    }
    return {{"entranceExpansion", CE::EncodeMmWallmasterExtension(rows)}};
}
int Bind(const Json& seed) { return MM_ApplyEntranceExpansionForCombo(seed.dump().c_str()); }
int main() {
    PlayState play{};
    EnWallmas actor{{99}, 30};
    assert(Bind(Json::object()));
    Reset(play); Capture(&actor, &play);
    assert(fallbackCalls == 1 && cycleCalls == 0 && !Combo_MMWallmasterErActive()); Unchanged();
    // Oracle assumptions/generation settings can never activate runtime capture or room-clear.
    CE::ActiveMmUnifiedAssumptions().emplace();
    CE::ActiveMmGenerationSettings().emplace();
    CE::ActiveMmGenerationSettings()->erWallmasters = CE::Mode::OwnGame;
    Reset(play); Capture(&actor, &play);
    assert(fallbackCalls == 1 && cycleCalls == 0 && !Combo_MMWallmasterErActive());
    CE::ActiveMmGenerationSettings().reset();
    unsigned cases = 0;
    for (size_t rotation = 0; rotation < 7; ++rotation) {
        const bool accepted = Bind(Seed(rotation));
#ifdef COMBO_ENTRANCE_DEV
        assert(accepted && Combo_MMWallmasterErActive());
        for (size_t src = 0; src < 4; ++src)
        for (int age : {0, 1})
        for (int flags = 0; flags < 16; ++flags) {
            Reset(play, src == 0 ? SCENE_DANPEI2TEST : SCENE_REDEAD);
            actor.actor.room = src == 0 ? 99 : src == 1 ? 1 : src == 2 ? 5 : 7;
            effectiveAge = age;
            for (int bit = 0; bit < 4; ++bit) cleared[bit] = (flags & (1 << bit)) != 0;
            expectedDestination = CE::Find(CE::kMmBaseWallmasterDestinations[(src + rotation) % 7])->id;
            int cutscene = initialSave.nextCutsceneIndex;
            const auto fixed = CE::FixMmNativeArrival(expectedDestination, cleared[0], cleared[1], cleared[2], cleared[3], cutscene);
            Capture(&actor, &play);
            assert(cycleCalls == 1 && fallbackCalls == 0 && logs == 0);
            assert((order == std::vector<std::string>{"age", "preflight", "fix", "end-cycle"}));
            assert(play.nextEntrance == static_cast<int>(fixed) && play.transitionTrigger == TRANS_TRIGGER_START &&
                   play.transitionType == TRANS_TYPE_FADE_BLACK && play.unrelated == initialPlay.unrelated);
            assert(gSaveContext.nextCutsceneIndex == cutscene && gSaveContext.respawnFlag == 0 &&
                   gSaveContext.fileNum == initialSave.fileNum && gSaveContext.unrelated == initialSave.unrelated);
            ++cases;
        }
#else
        assert(!accepted && !CE::ActiveMmUnified().active() && !Combo_MMWallmasterErActive());
        Reset(play); Capture(&actor, &play); assert(fallbackCalls == 1 && cycleCalls == 0); Unchanged();
#endif
    }
#ifdef COMBO_ENTRANCE_DEV
    assert(Bind(Seed()));
    for (int room = -128; room < 128; ++room) {
        if (room == 1 || room == 5 || room == 7) continue;
        Reset(play, SCENE_REDEAD); actor.actor.room = room; Capture(&actor, &play);
        assert(fallbackCalls == 1 && logs == 1 && cycleCalls == 0 && order.empty()); Unchanged();
    }
    Reset(play, 999); Capture(&actor, &play);
    assert(fallbackCalls == 1 && logs == 1 && cycleCalls == 0 && order.empty()); Unchanged();
    for (int rejection = 0; rejection < 4; ++rejection) {
        Reset(play); actor.actor.room = 3;
        expectedDestination = CE::Find(CE::kMmBaseWallmasterDestinations[0])->id;
        effectiveAge = rejection == 0 ? -1 : 1;
        preflightResult = rejection == 1 ? 0 : 1;
        rando = rejection != 2;
        if (rejection == 3) {
            assert(!Combo_MMWallmasterTransport(nullptr, &actor.actor));
            assert(!Combo_MMWallmasterTransport(&play, nullptr));
            assert(logs == 2 && order.empty());
        } else {
            Capture(&actor, &play); assert(fallbackCalls == 1 && logs == 1);
            assert(order.size() == static_cast<size_t>(rejection == 0 ? 1 : rejection == 1 ? 2 : 0));
        }
        assert(cycleCalls == 0); Unchanged(); rando = true;
    }
    // A refused rebind clears the prior successful table before another capture.
    auto bad = Seed(); bad["entranceLayout"]["schema"] = "future";
    assert(!Bind(bad)); Reset(play); Capture(&actor, &play);
    assert(fallbackCalls == 1 && !Combo_MMWallmasterErActive()); Unchanged();
#endif
    // Existing private profile remains available in shipping and dev, with no new preflight/fixup.
    assert(Bind(LegacySeed()) && Combo_MMWallmasterErActive());
    Reset(play); actor.actor.room = 91; effectiveAge = -1; preflightResult = 0;
    Capture(&actor, &play);
    assert(cycleCalls == 1 && fallbackCalls == 0 && logs == 0 && order == std::vector<std::string>{"end-cycle"});
    assert(play.nextEntrance == static_cast<int>(CE::Find(CE::kMmBaseWallmasterDestinations[0])->id));
    assert(gSaveContext.nextCutsceneIndex == initialSave.nextCutsceneIndex && gSaveContext.respawnFlag == 0);
    assert(Bind(Json::object())); Reset(play); Capture(&actor, &play);
    assert(fallbackCalls == 1 && !Combo_MMWallmasterErActive()); Unchanged();
    std::printf("MM capture: %u unified destination/age/clear cases, fallback, ordering, shipping rejection and legacy passed\n", cases);
}
'''
for dev in (False, True):
    cpp = work/('mm_wallmaster_transport_dev.cpp' if dev else 'mm_wallmaster_transport_shipping.cpp')
    cpp.write_text(support + installer + '\n' + active + '\n#define FixMmNativeArrival TrackedFixMmNativeArrival\n' +
                   transport + '\n#undef FixMmNativeArrival\nvoid Capture(EnWallmas* self, PlayState* play) {\n' +
                   capture + '\n}\n' + tests, encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo', args.json_include],
                       defines=['COMBO_BUILD'] + (['COMBO_ENTRANCE_DEV'] if dev else []), werror=True)
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
print('Shipping/dev MM transport and native fallback passed; legacy body is byte-identical.')
