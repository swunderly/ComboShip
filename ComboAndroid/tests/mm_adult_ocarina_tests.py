"""Adult Link playing the ocarina in MM (Cross-Games Age): the adult-only fixes and their release switches.

Decision: tooling/claude-cleanup/t6/decision.md, owner ruling 2026-10-12, revised the same day (t10). A1 films adult
human Link playing with ITEM1's Fierce Deity row (Camera_KeepOn4, CAM_MODE_JUMP = D_801B5410: MM data, which vanilla
MM never uses for the Fierce Deity playing); B3 moves the child Fairy item on the adult open hand by the placement
fitted from OoT's own hands (ComboFairyOcarinaFit.h); B1, kept for A/B runs, keeps OoT's native adult instrument hand
instead; the recoloured hand draws that native hand from a copy with the Fairy Ocarina's texture
(ComboFairyOcarinaRecolour.h). Each has a release switch, and a diagnostics build can turn each on with a default-off
debug CVar.

Compiles the production helpers (BenPort.cpp), the form switch of Camera_KeepOn4 (z_camera.c), the Fairy hand gate
(z_player_lib.c) and the Fairy composite (ComboFairyOcarinaHand.h) with stand-in engine types, for a release and a
diagnostics build, each with the switches as committed and flipped on (A1 with B1, A1 with B3, the recoloured hand
alone), and checks every input: only adult human Link (Combo_MM_IsLinkAdult) playing in CAM_SET_ITEM1 ever takes the
Fierce Deity row, and only adult human Link holding the Fairy Ocarina the native hand, the fitted item or the
recoloured hand; child Link, old saves (Combo_MM_IsLinkAdult is 0) and the other forms keep MM's own choice, and a
release build ignores the CVars. The fitted composite pushes the fit onto the hand limb's matrix around the item only
and pops it before the hand, and falls back to today's composite when there is no fit. The recoloured hand is the
limb's whole list for the LOD drawn, B1 and B3 win over it, and without it the composite is drawn. Also checks that
the camera row is MM's unchanged native data and that the probe stays in diagnostics builds. The archive side (OoT's
gLinkAdultSkel equals MM's gLinkFierceDeitySkel; OoT's gLinkAdultRightHandHoldingOotNear/FarDL exist) is
mm_cross_age_archive_tests.py (H8); the fit itself is mm_adult_fairy_fit_tests.py and, on the user's archives,
mm_adult_fairy_fit_archive_tests.py; the texture swap is mm_adult_fairy_recolour_tests.py and
mm_adult_fairy_recolour_archive_tests.py.
"""
from pathlib import Path
import argparse
import re
import subprocess
import sys

import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
root, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def squash(text):
    return re.sub(r'\s+', '', text)


def block(source, start):
    """Text from start through the brace matching the first opening brace at or after it."""
    i = source.index('{', start)
    depth = 0
    while True:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
        if depth == 0:
            return source[start:i]


DIAG_BLOCK = '#if defined(COMBO_BUILD) && defined(COMBO_ADULT_MM_RUNTIME_DIAGNOSTICS)'
benport = read('mm/2s2h/BenPort.cpp')
camera = read('mm/src/code/z_camera.c')
camera_data = read('mm/src/code/z_camera_data.inc')
lib = read('mm/src/code/z_player_lib.c')
fairy_hand = read('combo/menu/ComboFairyOcarinaHand.h')

# ---- The production text under test.
switches = {}
for name in ('kAdultOcarinaFdCamera', 'kAdultFairyNativeHand', 'kAdultFairyFittedPlacement',
             'kAdultFairyRecolouredOot'):
    m = re.search(r'static constexpr bool ' + name + r' = (true|false);', benport)
    check(m is not None, f'{name}: no release switch')
    switches[name] = m.group(1) if m else 'false'
helpers_at = benport.index('static constexpr bool kAdultOcarinaFdCamera')
recoloured_at = benport.index('extern "C" int Combo_MM_AdultFairyRecolouredOot(void) {')
helpers = benport[helpers_at:recoloured_at + len(block(benport, recoloured_at))]
check(helpers.count('extern "C" int ') == 4, 'the four helpers are not side by side')
check('static_assert(kAdultFairyNativeHand+kAdultFairyFittedPlacement+kAdultFairyRecolouredOot<=1,' in squash(helpers),
      'nothing stops B1, B3 and the recoloured hand from being released together')

keepon4_at = camera.index('s32 Camera_KeepOn4(Camera* camera) {')
switch_at = hostcxx.find(camera, 'switch (((Player*)camera->focalActor)->transformation) {', keepon4_at)
form_switch = block(camera, switch_at)
check(camera.count('Combo_MM_AdultOcarinaFdCamera()') == 1 and 'Combo_MM_AdultOcarinaFdCamera()' in form_switch,
      'the Fierce Deity camera is asked anywhere but the form switch of Camera_KeepOn4')

gate = re.search(r'Pinned Player_CustomHandEq.*?\n\s*if \((.*?)\) \{\s*\n\s*extern Gfx\* Combo_MM_FairyOcarinaHand',
                 lib, re.S)
check(gate is not None, 'the Fairy hand gate in Player_OverrideLimbDrawGameplayDefault is not found')
gate = gate.group(1) if gate else 'false'
check(lib.count('Combo_MM_AdultFairyNativeHand()') == 1 and 'Combo_MM_AdultFairyNativeHand()' in gate,
      'the native adult hand is asked anywhere but the Fairy hand gate')
composite_at = hostcxx.find(fairy_hand,
                            'extern "C" Gfx* Combo_MM_FairyOcarinaHand(PlayState* play, Gfx* nativeHand, s32 lod) {')
composite = block(fairy_hand, composite_at)
check(fairy_hand.count('Combo_MM_AdultFairyFittedPlacement()') == 1 and 'Combo_MM_AdultFairyFittedPlacement()' in composite
      and 'Combo_MM_AdultFairyFittedPlacement' not in lib,
      'the fitted placement is asked anywhere but the Fairy composite')
check(fairy_hand.count('Combo_MM_AdultFairyRecolouredOot()') == 1 and
      'Combo_MM_AdultFairyRecolouredOot()' in composite and 'Combo_MM_AdultFairyRecolouredOot' not in lib,
      'the recoloured hand is asked anywhere but the Fairy composite')
check(hostcxx.contains(lib, 'Combo_MM_FairyOcarinaHand(play, gPlayerRightHandOpenDLs[D_801F59E0 + sPlayerLod], '
                                'sPlayerLod)'),
      'the Fairy hand gate does not pass the open hand and the LOD it draws')

# ---- Source checks.
# A1 takes MM's own row: ITEM1's JUMP entry is D_801B5410, the Fierce Deity's mode in Camera_KeepOn4, unchanged.
item1_modes = block(camera_data, camera_data.index('CameraMode sCamSetItem1Modes[] = {'))
check(re.search(r'CAM_SETTING_MODE_ENTRY\(CAM_FUNC_KEEPON4, D_801B53EC\), // CAM_MODE_NORMAL\s*'
                r'CAM_SETTING_MODE_ENTRY\(CAM_FUNC_KEEPON4, D_801B5410\), // CAM_MODE_JUMP', item1_modes) is not None,
      'ITEM1 NORMAL/JUMP are not D_801B53EC/D_801B5410')
check(squash('CameraModeValue D_801B5410[] = { CAM_FUNCDATA_KEEP4(-40, 70, 20, 140, 5, 70,') in squash(camera_data),
      "D_801B5410 is not MM's native Fierce Deity ITEM1 row (-40, 70, 20, 140, 5, 70)")
check(re.search(r'\{ 0x0000002F, 0x00000005, sCamSetItem1Modes \},\s*// CAM_SET_ITEM1', camera_data) is not None,
      'CAM_SET_ITEM1 does not use sCamSetItem1Modes')
check('casePLAYER_FORM_FIERCE_DEITY:camMode=CAM_MODE_JUMP;break;' in squash(form_switch),
      'the Fierce Deity no longer takes CAM_MODE_JUMP in Camera_KeepOn4')
# The diagnostics define never reaches C files (mm/CMakeLists.txt), so the CVars live in the C++ helpers only.
check('$<$<COMPILE_LANGUAGE:CXX>:COMBO_ADULT_MM_RUNTIME_DIAGNOSTICS=1>' in read('mm/CMakeLists.txt'),
      'the diagnostics define is no longer C++ only; recheck where the A/B CVars are read')
for rel, text in (('mm/src/code/z_camera.c', camera), ('mm/src/code/z_player_lib.c', lib)):
    check('COMBO_ADULT_MM_RUNTIME_DIAGNOSTICS' not in text and 'gCombo.Debug.Adult' not in text,
          f'{rel} reads a diagnostics switch the C build never defines')
# The G0 probe is compiled into diagnostics builds only and runs behind its own default-off CVar, not
# AdultMmRuntimeValidation, which also opens the dormant Adult Mask readiness on every Player_Update.
probe_blocks = [m.group(1) for m in re.finditer(re.escape(DIAG_BLOCK) + r'\n(.*?)\n#endif', benport, re.S)]
check(benport.count('static void Combo_LogAdultOcarinaFrame(') == 1 and
      any('static void Combo_LogAdultOcarinaFrame(Player* player, Camera* camera) {' in text for text in probe_blocks),
      'the ocarina probe is not inside a diagnostics-only block')
calls = [m.start() for m in re.finditer(r'Combo_LogAdultOcarinaFrame\(player, camera\);', benport)]
check(len(calls) == 1, 'the ocarina probe is not called exactly once')
if len(calls) == 1:
    hook = benport.rindex(DIAG_BLOCK, 0, calls[0])
    hook_text = squash(benport[hook:calls[0]])
    check(benport.index('#endif', hook) > calls[0] and
          'constboolocarinaProbe=CVarGetInteger("gCombo.Debug.AdultOcarinaProbe",0);' in hook_text and
          hook_text.endswith('if(ocarinaProbe)'),
          'the ocarina probe is not behind its own default-off CVar in the diagnostics player hook')

# ---- Behaviour of the production text, every input.
HARNESS = r'''
#define COMBO_BUILD 1
#include <cassert>
#include <cstdio>
#include <algorithm>
#include <iterator>
#include <map>
#include <string>
typedef short s16;
typedef int s32;
enum { PLAYER_FORM_FIERCE_DEITY, PLAYER_FORM_GORON, PLAYER_FORM_ZORA, PLAYER_FORM_DEKU, PLAYER_FORM_HUMAN };
enum { CAM_MODE_NORMAL, CAM_MODE_JUMP, CAM_MODE_GORONDASH, CAM_MODE_DEKUSHOOT, CAM_MODE_BOWARROWZ, CAM_MODE_DEKUFLY };
enum { CAM_SET_NORMAL0 = 0x01, CAM_SET_ITEM0 = 0x12, CAM_SET_ITEM1 = 0x13, CAM_SET_ITEM2 = 0x14, CAM_SET_ITEM3 = 0x15 };
enum { PLAYER_MODELTYPE_RH_OPEN = 10, PLAYER_MODELTYPE_RH_INSTRUMENT = 13 };
enum { ITEM_OCARINA_OF_TIME = 0, ITEM_OCARINA_FAIRY = 5 };
struct Player { int transformation; int rightHandType; };
struct Camera { void* focalActor; s16 setting; };
// The Fairy composite's engine: each display-list macro records one command; the instrument path and the fitted matrix
// are switched on and off by the test.
struct Mtx { int unused; };
struct Gfx { char op; const void* arg; int flags; };
struct GraphicsContext {};
struct PlayState { struct { GraphicsContext* gfxCtx; } state; };
enum { G_MTX_NOPUSH = 0, G_MTX_PUSH = 1, G_MTX_MUL = 0, G_MTX_LOAD = 2, G_MTX_MODELVIEW = 0, G_MTX_PROJECTION = 4 };
enum { G_DL_OTR_FILEPATH = 0x27, G_DL_PUSH = 0 };
static Gfx sPool[8];
static size_t sAllocated = 0;
#define GRAPH_ALLOC(ctx, size) (sAllocated = (size) / sizeof(Gfx), (void*)sPool)
#define gSPMatrix(pkt, m, f) (*(pkt) = Gfx{ 'M', (m), (f) })
#define gDma1p(pkt, c, s, l, p) (*(pkt) = Gfx{ 'I', (s), (c) })
#define gSPPopMatrix(pkt, n) (*(pkt) = Gfx{ 'P', nullptr, (n) })
#define gSPDisplayList(pkt, dl) (*(pkt) = Gfx{ 'H', (dl), 0 })
#define gSPEndDisplayList(pkt) (*(pkt) = Gfx{ 'E', nullptr, 0 })
static const char kInstrument[] = "__OTR__@oot:combo/runtime/fairy_ocarina_instrument";
static const char* const kRecoloured[2] = { "__OTR__@oot:combo/runtime/adult_fairy_ocarina_hand_near",
                                            "__OTR__@oot:combo/runtime/adult_fairy_ocarina_hand_far" };
static bool sPathReady = true, sFitReady = true, sRecolourReady = true;
static int sPlacementAsked = 0, sRecolourAsked = 0, sRecolourLod = -1;
static Mtx sPlacement;
static const char* Combo_FairyInstrumentPath() { return sPathReady ? kInstrument : nullptr; }
static Mtx* Combo_FairyInstrumentPlacement() { ++sPlacementAsked; return sFitReady ? &sPlacement : nullptr; }
static const char* Combo_FairyRecolouredHandPath(s32 lod) {
    ++sRecolourAsked;
    sRecolourLod = lod;
    return sRecolourReady ? kRecoloured[lod] : nullptr;
}
static std::map<std::string, int> cvars;
static std::map<std::string, int> asked;
static int linkAdult = 0, ocarinaSlot = ITEM_OCARINA_OF_TIME;
int CVarGetInteger(const char* key, int fallback) {
    ++asked[key];
    auto it = cvars.find(key);
    return it == cvars.end() ? fallback : it->second;
}
extern "C" int Combo_MM_IsLinkAdult(void) { return linkAdult; }
#define INV_CONTENT(item) ((item) == ITEM_OCARINA_OF_TIME ? ocarinaSlot : -1)

@HELPERS@

static s16 PickMode(Camera* camera) {
    s16 camMode;
    @FORM_SWITCH@
    return camMode;
}

static bool FairyComposite(Player* player) {
    return @GATE@;
}

@COMPOSITE@

// What the composite draws: "" (nothing, the gate's native list stays), "HIE" (today: hand, item), "MIPHE" (B3: the
// fit pushed, the item, popped, then the hand) or "R" (the recoloured hand for the LOD drawn, as the whole list).
static std::string Drawn(Gfx* nativeHand, s32 lod) {
    PlayState play = {};
    std::fill(std::begin(sPool), std::end(sPool), Gfx{ 0, nullptr, 0 });
    sAllocated = 0;
    Gfx* list = Combo_MM_FairyOcarinaHand(&play, nativeHand, lod);
    if (!list)
        return "";
    if ((const void*)list == kRecoloured[0] || (const void*)list == kRecoloured[1]) {
        assert((const void*)list == kRecoloured[lod] && sAllocated == 0);
        return "R";
    }
    std::string ops;
    for (size_t i = 0; i < sAllocated; ++i) {
        ops += list[i].op;
        if (list[i].op == 'M')
            assert(list[i].arg == &sPlacement && list[i].flags == (G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW));
        if (list[i].op == 'P')
            assert(list[i].flags == G_MTX_MODELVIEW);
        if (list[i].op == 'I')
            assert(list[i].arg == kInstrument && list[i].flags == G_DL_OTR_FILEPATH);
        if (list[i].op == 'H')
            assert(list[i].arg == nativeHand);
    }
    return ops;
}

static s16 MmMode(int form) {
    return form == PLAYER_FORM_DEKU ? CAM_MODE_DEKUSHOOT : form == PLAYER_FORM_GORON ? CAM_MODE_GORONDASH
         : form == PLAYER_FORM_ZORA ? CAM_MODE_DEKUFLY : form == PLAYER_FORM_FIERCE_DEITY ? CAM_MODE_JUMP
         : CAM_MODE_NORMAL;
}

int main() {
    const bool diag = @DIAG@, fdSwitch = @FD_SWITCH@, handSwitch = @HAND_SWITCH@, fittedSwitch = @FITTED_SWITCH@,
               recolourSwitch = @RECOLOUR_SWITCH@;
    const int forms[] = { PLAYER_FORM_FIERCE_DEITY, PLAYER_FORM_GORON, PLAYER_FORM_ZORA, PLAYER_FORM_DEKU,
                          PLAYER_FORM_HUMAN };
    const s16 settings[] = { CAM_SET_NORMAL0, CAM_SET_ITEM0, CAM_SET_ITEM1, CAM_SET_ITEM2, CAM_SET_ITEM3 };
    int fdTaken = 0, handTaken = 0, fittedTaken = 0, recolourTaken = 0;
    Gfx hand = { 'h', nullptr, 0 };
    for (int form : forms)
        for (s16 setting : settings)
            for (int bits = 0; bits < 2048; ++bits) {
                linkAdult = bits & 1;
                const int fdCvar = (bits >> 1) & 1, handCvar = (bits >> 2) & 1, fittedCvar = (bits >> 5) & 1,
                          recolourCvar = (bits >> 8) & 1;
                const int rightHand = (bits & 8) ? PLAYER_MODELTYPE_RH_INSTRUMENT : PLAYER_MODELTYPE_RH_OPEN;
                ocarinaSlot = (bits & 16) ? ITEM_OCARINA_FAIRY : ITEM_OCARINA_OF_TIME;
                sPathReady = !(bits & 64);
                sFitReady = !(bits & 128);
                sRecolourReady = !(bits & 512);
                const s32 lod = (bits >> 10) & 1;
                cvars = { { "gCombo.Debug.AdultOcarinaFdCamera", fdCvar },
                          { "gCombo.Debug.AdultFairyNativeHand", handCvar },
                          { "gCombo.Debug.AdultFairyFittedPlacement", fittedCvar },
                          { "gCombo.Debug.AdultFairyRecolouredOot", recolourCvar } };
                Player player = { form, rightHand };
                Camera camera = { &player, setting };

                const bool fdOn = (fdSwitch || (diag && fdCvar)) && linkAdult;
                const bool handOn = (handSwitch || (diag && handCvar)) && linkAdult;
                const bool fittedOn = (fittedSwitch || (diag && fittedCvar)) && linkAdult;
                const bool recolourOn = (recolourSwitch || (diag && recolourCvar)) && linkAdult;
                assert(Combo_MM_AdultOcarinaFdCamera() == (fdOn ? 1 : 0));
                assert(Combo_MM_AdultFairyNativeHand() == (handOn ? 1 : 0));
                assert(Combo_MM_AdultFairyFittedPlacement() == (fittedOn ? 1 : 0));
                assert(Combo_MM_AdultFairyRecolouredOot() == (recolourOn ? 1 : 0));

                // A1: only adult human Link playing (ITEM1) takes the Fierce Deity row; everyone else keeps MM's.
                const bool fd = form == PLAYER_FORM_HUMAN && setting == CAM_SET_ITEM1 && fdOn;
                assert(PickMode(&camera) == (fd ? CAM_MODE_JUMP : MmMode(form)));
                fdTaken += fd;

                // B1: only adult human Link with the Fairy Ocarina in hand keeps the native hand; child Link keeps the
                // Fairy composite exactly as before.
                const bool mmComposite = form == PLAYER_FORM_HUMAN && rightHand == PLAYER_MODELTYPE_RH_INSTRUMENT &&
                                         ocarinaSlot == ITEM_OCARINA_FAIRY;
                assert(FairyComposite(&player) == (mmComposite && !handOn));
                handTaken += mmComposite && handOn;

                // The recoloured hand: only adult Link with its switch on, and neither B1 nor B3, gets OoT's adult
                // hand coloured like the Fairy Ocarina, for the LOD drawn; where it cannot be built, the composite.
                // B3: the composite moves the item only for adult Link with the switch on and a fit; without a fit it
                // is today's composite. Neither is even looked up for anyone else.
                sPlacementAsked = sRecolourAsked = 0;
                sRecolourLod = -1;
                const std::string drawn = FairyComposite(&player) ? Drawn(&hand, lod) : "";
                const bool recoloured = FairyComposite(&player) && !fittedOn && recolourOn && sRecolourReady;
                const bool fitted = FairyComposite(&player) && fittedOn && sPathReady && sFitReady;
                assert(drawn == (!FairyComposite(&player) ? ""
                                 : recoloured             ? "R"
                                 : !sPathReady            ? ""
                                 : fitted                 ? "MIPHE"
                                                          : "HIE"));
                assert(sRecolourAsked == (FairyComposite(&player) && !fittedOn && recolourOn ? 1 : 0));
                assert(sRecolourAsked == 0 || sRecolourLod == lod);
                assert(sPlacementAsked == (FairyComposite(&player) && fittedOn && sPathReady ? 1 : 0));
                fittedTaken += fitted;
                recolourTaken += recoloured;

                // Child Link and saves without Cross-Games Age (Combo_MM_IsLinkAdult 0) never change.
                if (!linkAdult) {
                    assert(PickMode(&camera) == MmMode(form));
                    assert(FairyComposite(&player) == mmComposite);
                    assert(drawn == (mmComposite && sPathReady ? "HIE" : ""));
                }
            }
    // A release build never reads the debug CVars.
    assert(diag || (asked.count("gCombo.Debug.AdultOcarinaFdCamera") == 0 &&
                    asked.count("gCombo.Debug.AdultFairyNativeHand") == 0 &&
                    asked.count("gCombo.Debug.AdultFairyFittedPlacement") == 0 &&
                    asked.count("gCombo.Debug.AdultFairyRecolouredOot") == 0));
    // B1 switched on wins over B3 and the recoloured hand, and B3 wins over the recoloured hand.
    assert((fdTaken > 0) == (fdSwitch || diag) && (handTaken > 0) == (handSwitch || diag) &&
           (fittedTaken > 0) == ((fittedSwitch || diag) && !handSwitch) &&
           (recolourTaken > 0) == ((recolourSwitch || diag) && !handSwitch && !fittedSwitch));
    std::printf("PASS %s build, camera switch %s, native hand switch %s, fitted item switch %s, recoloured hand switch "
                "%s: Fierce Deity row in %d cases, native hand in %d, fitted item in %d, recoloured hand in %d, all "
                "adult human\n", diag ? "diagnostics" : "release", fdSwitch ? "on" : "off", handSwitch ? "on" : "off",
                fittedSwitch ? "on" : "off", recolourSwitch ? "on" : "off", fdTaken, handTaken, fittedTaken,
                recolourTaken);
}
'''

if failures:
    print('\n'.join('FAIL: ' + f for f in failures))
    sys.exit(1)

committed = (switches['kAdultOcarinaFdCamera'], switches['kAdultFairyNativeHand'],
             switches['kAdultFairyFittedPlacement'], switches['kAdultFairyRecolouredOot'])
print(f'Release switches as committed: camera {committed[0]}, native hand {committed[1]}, fitted item {committed[2]}, '
      f'recoloured hand {committed[3]}', flush=True)
for diag in (False, True):
    # B1, B3 and the recoloured hand are alternatives (a static_assert keeps any two from being switched on together).
    for label, (fd, hand, fitted, recolour) in (('committed', committed),
                                                ('flipped_b1', ('true', 'true', 'false', 'false')),
                                                ('flipped_b3', ('true', 'false', 'true', 'false')),
                                                ('flipped_recolour', ('false', 'false', 'false', 'true'))):
        text = re.sub(r'(static constexpr bool kAdultOcarinaFdCamera = )(true|false);', r'\g<1>' + fd + ';', helpers)
        text = re.sub(r'(static constexpr bool kAdultFairyNativeHand = )(true|false);', r'\g<1>' + hand + ';', text)
        text = re.sub(r'(static constexpr bool kAdultFairyFittedPlacement = )(true|false);', r'\g<1>' + fitted + ';',
                      text)
        text = re.sub(r'(static constexpr bool kAdultFairyRecolouredOot = )(true|false);', r'\g<1>' + recolour + ';',
                      text)
        code = (HARNESS.replace('@DIAG@', 'true' if diag else 'false').replace('@FD_SWITCH@', fd)
                .replace('@HAND_SWITCH@', hand).replace('@FITTED_SWITCH@', fitted)
                .replace('@RECOLOUR_SWITCH@', recolour).replace('@FORM_SWITCH@', form_switch)
                .replace('@GATE@', gate).replace('@COMPOSITE@', composite).replace('@HELPERS@', text))
        name = f'adult_ocarina_{"diagnostics" if diag else "release"}_{label}'
        source = work / f'{name}.cpp'
        source.write_text(code, encoding='utf-8')
        exe = hostcxx.build(a.compiler, source, std='c++17', warnings=False,
                            defines=['COMBO_ADULT_MM_RUNTIME_DIAGNOSTICS=1'] if diag else [])
        subprocess.run([str(exe)], cwd=work, check=True, timeout=hostcxx.RUN_TIMEOUT)
print('PASS')
