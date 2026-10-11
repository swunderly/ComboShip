#include "2s2h/Rando/ActorBehavior/BeanSources.h"
#include "Rando/ActorBehavior/ExpandedSoulsReadiness.h"
#ifdef COMBO_BUILD
extern "C" int gMMComboSharedMask;
#endif
#include "Rando/ActorBehavior/MissingMmSouls.h"
#include "Rando/Ocarinas.h"
#include "Rando/ActorBehavior/ExpandedSoulsNative.h"
#include "BenPort.h"
#include "ComboAdultMaskInventory.h"
#include "ComboAdultGearInventory.h"
#ifdef COMBO_BUILD
#include "ComboExport.h"
#include "ComboResolve.h"
#include "2s2h/Enhancements/Songs/CrossWarpAudio.h"
#endif
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <set>
#include <sstream>
#include <unordered_map> // ComboShip: oracle name->id lookup maps

#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManagerScope.h>
#include <ship/resource/ResourceManager.h>
#include <fast/Fast3dWindow.h>
// ComboShip: our newer libultraship moved these headers; the mm baseline assumed older paths.
#include <fast/debug/GfxDebugger.h>
#include <stb_image.h>
#include <ship/resource/File.h>
#include <ship/window/Window.h>

#include "z64animation.h"
#include "z64bgcheck.h"
#include <libultraship/libultra/gbi.h>
#include <ship/window/gui/Fonts.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif
#include <ship/audio/AudioPlayer.h>
#include "variables.h"
#include "z64.h"
#include "macros.h"
#include <ship/utils/StringHelper.h>
#include <nlohmann/json.hpp>
#include "build.h"
#include <stb_image.h>

#include <fast/interpreter.h>
#include <fast/backends/gfx_rendering_api.h>
#include <fast/Fast3dWindow.h>

#ifdef __APPLE__
#include <SDL_scancode.h>
#else
#include <SDL2/SDL_scancode.h>
#endif
#include "Extractor/Extract.h"
// OTRTODO
// #include <functions.h>
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"

#ifdef ENABLE_CROWD_CONTROL
#include "Enhancements/crowd-control/CrowdControl.h"
CrowdControl* CrowdControl::Instance;
#endif

#include <libultraship/libultraship.h>
#include <libultraship/controller/controldeck/ControlDeck.h>
#include <fast/resource/ResourceType.h>
#include <BenGui/BenGui.hpp>
#include <BenGui/BenMenu.h>
#ifdef COMBO_BUILD
#include "ComboMenuSharedContext.h"               // ComboShip: shared per-DLL ImGui context helper (combo-owned)
#include "rando/SharedItems.h"                    // ComboShip: Shared Items family table
#include "rando/FaroreRules.h"
#include "2s2h/Rando/FaroresWind.h"
#include "2s2h/Rando/Spells.h"
#include "rando/SpellRules.h"
#include "2s2h/Rando/MiscBehavior/MiscBehavior.h" // ComboShip: MM_LoadComboRando cache invalidation + ComboRando types
#endif

#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Enhancements/Enhancements.h"
#include "2s2h/Enhancements/GfxPatcher/AuthenticGfxPatches.h"
#include "2s2h/Enhancements/GfxPatcher/PlayerCustomFlipbooks.h"
#include "2s2h/Enhancements/ModMenu/ModMenu.h"
#include "2s2h/DeveloperTools/DebugConsole.h"
#include "2s2h/Rando/Rando.h"
#include "2s2h/Rando/Spoiler/Spoiler.h"
#include "2s2h/Rando/Logic/Logic.h"
#include "2s2h/Rando/MiscBehavior/ClockShuffle.h"
#include "2s2h/SaveManager/SaveManager.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/BenGui/Notification.h"
#include "2s2h/ShipUtils.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/PresetManager/PresetManager.h"
#include "2s2h/config/ConfigUpdaters.h"

// Resource Types/Factories
#include <ship/resource/type/Blob.h>
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Matrix.h>
#include <fast/resource/type/Texture.h>
#include <fast/resource/type/Vertex.h>
#include "2s2h/resource/type/2shResourceType.h"
#include "2s2h/resource/type/Animation.h"
#include "2s2h/resource/type/Array.h"
#include "2s2h/resource/type/AudioSample.h"
#include "2s2h/resource/type/AudioSequence.h"
#include "2s2h/resource/type/AudioSoundFont.h"
#include "2s2h/resource/type/CollisionHeader.h"
#include "2s2h/resource/type/Cutscene.h"
#include "2s2h/resource/type/Path.h"
#include "2s2h/resource/type/PlayerAnimation.h"
#include "2s2h/resource/type/Scene.h"
#include "2s2h/resource/type/Skeleton.h"
#include "2s2h/resource/type/SkeletonLimb.h"
#include <ship/resource/factory/BlobFactory.h>
#include <fast/resource/factory/DisplayListFactory.h>
#include <fast/resource/factory/MatrixFactory.h>
#include <fast/resource/factory/TextureFactory.h>
#include <fast/resource/factory/VertexFactory.h>
#include "2s2h/resource/importer/AnimationFactory.h"
#include "2s2h/resource/importer/ArrayFactory.h"
#include "2s2h/resource/importer/AudioSampleFactory.h"
#include "2s2h/resource/importer/AudioSequenceFactory.h"
#include "2s2h/resource/importer/AudioSoundFontFactory.h"
#include "2s2h/resource/importer/CollisionHeaderFactory.h"
#include "2s2h/resource/importer/CutsceneFactory.h"
#include "2s2h/resource/importer/PathFactory.h"
#include "2s2h/resource/importer/PlayerAnimationFactory.h"
#include "2s2h/resource/importer/SceneFactory.h"
#include "2s2h/resource/importer/SkeletonFactory.h"
#include "2s2h/resource/importer/SkeletonLimbFactory.h"
#include "2s2h/resource/importer/TextMMFactory.h"
#include "2s2h/resource/importer/BackgroundFactory.h"
#include "2s2h/resource/importer/TextureAnimationFactory.h"
#include "2s2h/resource/importer/KeyFrameFactory.h"
#include <ship/window/gui/resource/Font.h>
#include <ship/window/FileDropMgr.h>
#include <ship/window/gui/resource/FontFactory.h>
#include "2s2h/Enhancements/Audio/AudioCollection.h"
#include "BenGui/BenInputEditorWindow.h"

OTRGlobals* OTRGlobals::Instance;
GameInteractor* GameInteractor::Instance;
AudioCollection* AudioCollection::Instance;

#ifdef COMBO_BUILD
// ComboShip: set before MM_RunGame to reuse the OOT context. One shared libultraship.dll means
// Context::mContext is the same instance in every DLL, so GetInstance() already returns it.
static bool sComboTransitionActive = false;
extern "C" int MM_FaroreDeparturePending(void);
extern "C" int MM_DispatchNativeDeparture(PlayState*);
extern "C" void MM_CancelNativeDeparture(void);
static int sSongDeparture = -1;
static void ProcessForeignSongReply();
static int sSongArrival = -1;

extern "C" COMBO_EXPORT void MM_NotifyComboTransition(void) {
    sComboTransitionActive = true;
}

// kind: 0 = portal (walked out the Clock Tower), 1 = Ctrl+R reset, 2 = owl-save quit. Only a portal
// return continues the session in OOT; the other two end it and boot OOT to its title.
extern "C" void (*gComboReturnCallback)(int kind) = nullptr;
// Shared Items per-frame drain seam (defined with the rest of the Shared Items ABI further down).
extern "C" void (*gMMComboSharedTick)(void);
extern "C" COMBO_EXPORT void MM_SetOnComboReturnCallback(void (*cb)(int kind)) {
    gComboReturnCallback = cb;
}
static bool sComboReturnPending = false;
// ComboShip: Ctrl+R reset while MM is foreground. Like the portal return, but only persists MM if
// autosave is enabled (an authentic reset otherwise discards unsaved progress). Set via the export.
static bool sComboResetReturnPending = false;
extern "C" COMBO_EXPORT void MM_RequestComboReturn(void) {
    sComboResetReturnPending = true;
}
// ComboShip (#89): owl save. MM would SET_NEXT_GAMESTATE(TitleSetup_Init) here (z_play.c) and quit to
// its own file select, which combo can't enter — it re-runs MM's boot, wipes the save and lets MM's
// file select write the wipe into the container. Quit to OOT's title instead; the owl save's own
// flashrom write has already persisted the MM section.
static bool sComboOwlSaveQuitPending = false;
extern "C" void Combo_RequestOwlSaveQuit(void) {
    sComboOwlSaveQuitPending = true;
}
// Drop any unconsumed return request. Called as MM is entered: one left over from the previous session
// would immediately quit the new one.
static void Combo_ClearReturnRequests(void) {
    sComboReturnPending = false;
    sComboResetReturnPending = false;
    sComboOwlSaveQuitPending = false;
}
// MM's own ResourceManager, created at first boot and kept alive for the whole process. A combo
// transition swaps the Context's active RM between MM's and OOT's, so each game keeps its archives +
// resource cache resident and nothing is ever unloaded (no dangling cached pointers). See MM_ResumeGame.
// (Upstream merge: the old RegisterMMResourceFactories factoring was dropped — Initialize() now owns
// the inline factory registration and it runs against whichever RM is active. See docs/UPSTREAM_MERGES.md.)
static std::shared_ptr<Ship::ResourceManager> sMMResourceManager;
#endif

extern "C" char** cameraStrings;
bool prevAltAssets = false;
std::vector<std::shared_ptr<std::string>> cameraStdStrings;

Color_RGB8 kokiriColor = { 0x1E, 0x69, 0x1B };
Color_RGB8 goronColor = { 0x64, 0x14, 0x00 };
Color_RGB8 zoraColor = { 0x00, 0xEC, 0x64 };

int32_t previousImGuiScaleIndex;
float previousImGuiScale;

typedef struct {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
} ArchiveVersion;

std::shared_ptr<Fast::Fast3dWindow> benFast3dWindow;
static ArchiveVersion DetectArchiveVersion(std::string path, bool isO2rType);
static bool VerifyArchiveVersion(ArchiveVersion version);
std::string portArchivePath = "";
static bool shipArchiveVersionMatch = false;

OTRGlobals::OTRGlobals() {
#ifdef COMBO_BUILD
    // Combo OOT->MM forward transition: reuse OOT's already-initialized shared context + window
    // instead of creating new ones (one shared libultraship.dll => GetInstance() is the OOT context).
    // SOH_PrepareForTransition() stopped OOT's audio first.
    bool usingExistingCtx = false;
    if (sComboTransitionActive) {
        auto existingCtx = Ship::Context::GetRawInstance();
        if (existingCtx != nullptr) {
            context = existingCtx;
            portArchivePath = Ship::Context::LocateFileAcrossAppDirs("2ship.o2r");
            shipArchiveVersionMatch = true; // 2ship.o2r already validated at OOT boot; enable font load below
            // MM's OWN ResourceManager: own archives + factories + resource cache. Make it active before
            // any GetResourceManager() lookup; OOT's RM stays alive (sOOTResourceManager) so nothing is
            // unloaded and no OOT cached pointer dangles. Initialize() adds mm.o2r + the factories onto it.
            auto mmResourceManager = std::make_shared<Ship::ResourceManager>();
            context->SetResourceManager(mmResourceManager);
            mmResourceManager->Init({ portArchivePath }, {}, 3);
            sMMResourceManager = mmResourceManager;
            Ship::CrossRMRegistry::Register("mm", sMMResourceManager); // ComboShip: cross-game rendering
            // MM's fresh RM lacks the Gui-owned infra factories (Font, GuiTexture) the shared Gui
            // registered on OOT's RM at boot; register them so font/gui-texture loads work.
            context->GetWindow()->GetGui()->RegisterResourceFactories();
            // OOT closed the shared window backend on exit (mIsRunning=false); re-arm it so MM's
            // `while (WindowIsRunning())` loop runs instead of returning immediately.
            if (auto fast3d = std::dynamic_pointer_cast<Fast::Fast3dWindow>(context->GetWindow())) {
                fast3d->SetIsRunning(true);
            }
            usingExistingCtx = true;
        }
        sComboTransitionActive = false;
    }
    if (!usingExistingCtx) {
#endif
        context =
            Ship::Context::CreateUninitializedInstance("2 Ship 2 Harkinian", appShortName, "2ship2harkinian.json");

        portArchivePath = Ship::Context::LocateFileAcrossAppDirs("2ship.o2r");
        ArchiveVersion portArchiveVersion = DetectArchiveVersion("2ship.o2r", true);
        shipArchiveVersionMatch = portArchiveVersion.major == gBuildVersionMajor &&
                                  portArchiveVersion.minor == gBuildVersionMinor &&
                                  portArchiveVersion.patch == gBuildVersionPatch;

        context->InitConfiguration();
        context->InitConsoleVariables();

        auto controlDeck = std::make_shared<LUS::ControlDeck>(std::vector<CONTROLLERBUTTONS_T>({
            BTN_CUSTOM_MODIFIER1,
            BTN_CUSTOM_MODIFIER2,
            BTN_CUSTOM_OCARINA_NOTE_D4,
            BTN_CUSTOM_OCARINA_NOTE_F4,
            BTN_CUSTOM_OCARINA_NOTE_A4,
            BTN_CUSTOM_OCARINA_NOTE_B4,
            BTN_CUSTOM_OCARINA_NOTE_D5,
            BTN_CUSTOM_OCARINA_DISABLE_SONGS,
            BTN_CUSTOM_OCARINA_PITCH_UP,
            BTN_CUSTOM_OCARINA_PITCH_DOWN,
        }));
        context->InitControlDeck(controlDeck);
        context->InitResourceManager({ portArchivePath }, {}, 3, true);
        context->InitConsole();

        auto benInputEditorWindow =
            std::make_shared<BenInputEditorWindow>("gWindows.BenInputEditor", "2S2H Input Editor");
        benFast3dWindow = std::make_shared<Fast::Fast3dWindow>(
            std::vector<std::shared_ptr<Ship::GuiWindow>>({ benInputEditorWindow }));
        context->InitWindow(benFast3dWindow);

        BenGui::SetupMenu();
#ifdef COMBO_BUILD
    } // end if (!usingExistingCtx)
    // ImGui's current-context global (GImGui) is a per-module static; this 2ship.dll has its own,
    // separate from libultraship.dll where the context lives. Point it at the shared context (works
    // for both the reuse path and standalone window creation) before any ImGui use here.
    ImGui::SetCurrentContext(context->GetWindow()->GetGui()->GetImGuiContext());
    // ComboShip: the reuse path skipped BenGui::SetupMenu(), so MM's BenMenu was never built and
    // the shared Gui's single menu slot still holds OOT's SohMenu. Build MM's menu now that the
    // ImGui context is current (widgets populate lazily via BenMenu::InitElement).
    if (usingExistingCtx) {
        BenGui::ActivateMenu(); // ComboShip: no-op under COMBO_BUILD (comboui owns the menu)
    }
#endif

    if (shipArchiveVersionMatch) {

        auto overlay = context->GetWindow()->GetGui()->GetGameOverlay();
        overlay->LoadFont("Press Start 2P", 12.0f, "fonts/PressStart2P-Regular.ttf");
        overlay->LoadFont("Fipps", 32.0f, "fonts/Fipps-Regular.otf");
        overlay->SetCurrentFont(CVarGetString(CVAR_GAME_OVERLAY_FONT, "Press Start 2P"));

        fontMono = CreateFontWithSize(16.0f, "fonts/Inconsolata-Regular.ttf");
        fontMonoLarger = CreateFontWithSize(20.0f, "fonts/Inconsolata-Regular.ttf");
        fontMonoLargest = CreateFontWithSize(24.0f, "fonts/Inconsolata-Regular.ttf");
        fontStandard = CreateFontWithSize(16.0f, "fonts/Montserrat-Regular.ttf");
        fontStandardLarger = CreateFontWithSize(20.0f, "fonts/Montserrat-Regular.ttf");
        fontStandardLargest = CreateFontWithSize(24.0f, "fonts/Montserrat-Regular.ttf");
        ImGui::GetIO().FontDefault = fontStandardLarger;
    }

    previousImGuiScaleIndex = -1;
    previousImGuiScale = defaultImGuiScale;
    ScaleImGui();
#ifdef COMBO_BUILD
    if (usingExistingCtx) {
        // MM fonts were just added to the shared ImGui atlas (TexReady=false); the renderer backend
        // already built its font texture for OOT and won't rebuild on its own -> MM's first
        // ImGui::NewFrame() would assert "Font Atlas not built!". Invalidate so the next frame rebuilds.
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->RebuildFontTexture();
    }
#endif
}

typedef enum ExtractSteps {
    ES_PORT_ARCHIVE,
    ES_WINDOWS,
    ES_EXTRACT_ARGS,
    ES_EXTRACT,
    ES_VERIFY,
} ExtractSteps;

typedef enum PromptSteps {
    PS_FILE_CHECK,
    PS_LOCAL,
    PS_FIRST,
    PS_DUPE,
    PS_WAIT,
    PS_NONE,
} PromptSteps;

typedef enum WindowsSteps {
    WS_TEMP,
    WS_PERMS,
    WS_ONEDRIVE,
    WS_DONE,
} WindowsSteps;

bool IsSubpath(const std::filesystem::path& path, const std::filesystem::path& base) {
    auto rel = std::filesystem::relative(path, base);
    return !rel.empty() && rel.native()[0] != '.';
}

bool PathTestCleanup(FILE* tfile) {
    try {
        if (std::filesystem::exists("./text.txt"))
            std::filesystem::remove("./text.txt");
        if (std::filesystem::exists("./test/"))
            std::filesystem::remove("./test/");
    } catch (std::filesystem::filesystem_error const& ex) { return false; }
    return true;
}

void CheckAndCreateModFolder() {
    try {
        std::string modsPath = Ship::Context::LocateFileAcrossAppDirs("mods/" + appShortName, appShortName);
        if (!std::filesystem::exists(modsPath)) {
            // Create mods folder relative to app dir
            modsPath = Ship::Context::GetPathRelativeToAppDirectory("mods/" + appShortName, appShortName);
            std::string filePath = modsPath + "/custom_mod_files_go_here.txt";
            if (std::filesystem::create_directories(modsPath)) {
                std::ofstream(filePath).close();
            }
        }
    } catch (std::filesystem::filesystem_error const& ex) {
        // Couldn't make the folder, continue silently
        return;
    }
}

namespace BenGui {
extern std::shared_ptr<BenGui::BenMenu> mBenMenu;
}

void OTRGlobals::RunExtract(int argc, char* argv[]) {
    bool extractDone = false;
    ExtractSteps extractStep = ES_PORT_ARCHIVE;
    WindowsSteps windowsStep = WS_TEMP;
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(OTRGlobals::Instance->context->GetWindow());
    auto gui = wnd->GetGui();

    bool shouldRegen = VerifyArchiveVersion(DetectArchiveVersion("mm.o2r", true));

    std::filesystem::path ownPath;
    std::vector<std::string> args;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            args.push_back(argv[i]);
        }
    }
    Extractor extract;
    PromptSteps promptStep = PS_FILE_CHECK;
    std::atomic<size_t> extractCount = 0, totalExtract = 0;

    std::string installPath = Ship::Context::GetAppBundlePath();
    std::string dataPath = Ship::Context::GetAppDirectoryPath(appShortName);
    std::string file;

#if defined(__SWITCH__)
    BenGui::RegisterPopup("Outdated ROM Archives",
                          "\x1b[2;2HYou've launched 2Ship with an old ROM O2R file."
                          "\x1b[4;2HPlease regenerate a new ROM O2R and relaunch."
                          "\x1b[6;2HPress the Home button to exit...",
                          "OK", "", [&]() { exit(1); });
#elif defined(__WIIU__)
    BenGui::RegisterPopup("Outdated ROM Archives",
                          "You've launched 2Ship with an old a ROM O2R file.\n\n"
                          "Please generate a ROM O2R and relaunch.\n\n"
                          "Press and hold the Power button to shutdown...",
                          "OK", "", [&]() { exit(1); });
    OSFatal();
#endif

    if (!std::filesystem::exists(installPath + "/assets")) {
        BenGui::RegisterPopup("Extractor assets not found",
                              "No O2R files found. Missing 'assets/' folder needed to generate OTR file.\nPlease "
                              "re-extract them from the download or.\n\nExiting...",
                              "OK", "", [&]() { exit(1); });
    } else if (shouldRegen) {
        BenGui::RegisterPopup("Outdated ROM Archives",
                              "Your mm.o2r was created with incompatible versions of 2Ship.\nYou will "
                              "now be redirected to re-extract them.");
        std::filesystem::remove("mm.o2r");
    }

    std::shared_ptr<BS::thread_pool> threadPool = std::make_shared<BS::thread_pool>(1);
    std::optional<std::future<void>> extractionTask;

#if not defined(__SWITCH__) && not defined(__WIIU__)
    CheckAndCreateModFolder();
#endif

    while (!extractDone) {
        if (BenGui::PopupsQueued() > 0 || extractionTask.has_value()) {
            goto render;
        }
        switch (extractStep) {
            case ES_PORT_ARCHIVE: {
                if (shipArchiveVersionMatch) {
#ifdef _WIN32
                    extractStep = ES_WINDOWS;
#elif (defined(__WIIU__) || defined(__SWITCH__))
                    extractStep = ES_VERIFY;
#else
                    extractStep = args.empty() ? ES_EXTRACT : ES_EXTRACT_ARGS;
#endif
                } else {
                    std::string msg;

#if defined(__SWITCH__)
                    msg = "\x1b[4;2HPlease re-extract it from the download.\n"
                          "\x1b[6;2HPress the Home button to exit...";
#elif defined(__WIIU__)
                    msg = "Please extract the 2ship.o2r from the 2 Ship 2 Harkinian download\nto your folder.\n\nPress "
                          "and hold the power\n"
                          "button to shutdown...";
#else
                    msg = "Please extract the 2ship.o2r from the 2 Ship 2 Harkinian download to your "
                          "folder.\n\nExiting...";
#endif
                    std::string title =
                        !std::filesystem::exists(portArchivePath) ? "Missing 2ship.o2r" : "2ship.o2r is outdated";
                    BenGui::RegisterPopup(title, msg, "OK", "", [&]() { exit(1); });
                }
                continue;
            }
            case ES_WINDOWS: {
                switch (windowsStep) {
                    case WS_TEMP: {
#ifdef _WIN32
                        char* tempVar = getenv("TEMP");
                        std::filesystem::path tempPath;
                        try {
                            tempPath = std::filesystem::canonical(tempVar);
                        } catch (std::filesystem::filesystem_error const& ex) {
                            std::string userPath = getenv("USERPROFILE");
                            userPath.append("\\AppData\\Local\\Temp");
                            tempPath = std::filesystem::canonical(userPath);
                        }
                        wchar_t buffer[MAX_PATH];
                        GetModuleFileName(NULL, buffer, _countof(buffer));
                        ownPath = std::filesystem::canonical(buffer).parent_path();
                        if (IsSubpath(ownPath, tempPath)) {
                            BenGui::RegisterPopup("2S2H Path Error",
                                                  "2S2H is running in a temp folder.\nExtract the .zip and run again.",
                                                  "OK", "", [&]() { exit(0); });
                        } else {
                            windowsStep = WS_PERMS;
                        }
#endif
                        continue;
                    }
                    case WS_PERMS: {
                        FILE* tfile = fopen("./text.txt", "w");
                        std::filesystem::path tfolder = std::filesystem::path("./test/");
                        bool error = false;
                        try {
                            create_directories(tfolder);
                        } catch (std::filesystem::filesystem_error const& ex) { error = true; }
                        if (tfile == NULL || error) {
                            BenGui::RegisterPopup("2S2H Permissions Error",
                                                  "2S2H does not have proper file permissions.\nPlease move it to a "
                                                  "folder that does and run again.",
                                                  "OK", "", [&]() {
                                                      fclose(tfile);
                                                      PathTestCleanup(tfile);
                                                      exit(0);
                                                  });
                        } else {
                            fclose(tfile);
                            if (!PathTestCleanup(tfile)) {
                                BenGui::RegisterPopup(
                                    "2S2H Permissions Error",
                                    "2S2H does not have proper file permissions.\nPlease move it to a "
                                    "folder that does and run again.",
                                    "OK", "", [&]() { exit(0); });
                            }
                            windowsStep = WS_ONEDRIVE;
                        }
                        continue;
                    }
                    case WS_ONEDRIVE: {
                        if (ownPath.string().find("OneDrive") != std::string::npos) {
                            BenGui::RegisterPopup("2S2H Path Error",
                                                  "2S2H appears to be in a OneDrive folder, which will cause issues.\n"
                                                  "Please move it to a folder outside of OneDrive, like the root of a\n"
                                                  "drive (e.g. \"C:\\Games\\2S2H\").",
                                                  "OK", "", [&]() { exit(0); });
                        } else {
                            windowsStep = WS_DONE;
                            extractStep = args.empty() ? ES_EXTRACT : ES_EXTRACT_ARGS;
                        }
                        continue;
                    }
                    default:
                        continue;
                }
                break;
            }
            case ES_EXTRACT_ARGS: {
#if !defined(__SWITCH__) && !defined(__WIIU__)
                if (args.empty()) {
                    BenGui::RegisterPopup(
                        "Run 2 Ship 2 Harkinian", "All files have been processed. Run 2S2H?", "Yes", "No",
                        [&]() {
                            if (!std::filesystem::exists(Ship::Context::GetAppDirectoryPath(appShortName) +
                                                         "/mm.o2r")) {
                                extractStep = ES_EXTRACT;
                                promptStep = PS_FILE_CHECK;
                            } else {
                                extractStep = ES_VERIFY;
                            }
                        },
                        [&]() { exit(0); });
                    break;
                }
                file = args.at(0);
                args.erase(args.begin());
                extract = Extractor();
                if (extract.RunFileStandalone(file)) {
                    bool doExtract = true;
                    if (std::filesystem::exists(Ship::Context::GetAppDirectoryPath(appShortName) + "/mm.o2r")) {
                        std::string msg = "Archive for current ROM, mm.o2r, already exists.\nExtract again?";
                        BenGui::RegisterPopup("Confirm Re-extract", msg.c_str(), "Yes", "No", [&]() {
                            extractionTask = threadPool->submit_task([&]() -> void {
                                extract.CallZapd(installPath, Ship::Context::GetAppDirectoryPath(appShortName),
                                                 &extractCount, &totalExtract);
                                extractCount = totalExtract = 0;
                            });
                        });
                    } else {
                        extractionTask = threadPool->submit_task([&]() -> void {
                            extract.CallZapd(installPath, Ship::Context::GetAppDirectoryPath(appShortName),
                                             &extractCount, &totalExtract);
                            extractCount = totalExtract = 0;
                        });
                    }
                } else {
                    bool open = true;
                    std::string msg = "File\n" + std::string(file) + "\nis not a ROM or does not match supported ROMs.";
                    BenGui::RegisterPopup("2S2H ROM Error", msg.c_str());
                }
#else
                extractStep = ES_VERIFY;
#endif
                break;
            }
            case ES_EXTRACT: {
                switch (promptStep) {
                    case PS_FILE_CHECK: {
                        if (!std::filesystem::exists(Ship::Context::LocateFileAcrossAppDirs("mm.o2r", appShortName))) {
                            BenGui::RegisterPopup(
                                "No O2R Files", "No O2R files found. Generate one now?", "Yes", "No",
                                [&]() { promptStep = PS_LOCAL; }, [&]() { exit(0); });
                        } else {
                            extractStep = ES_VERIFY;
                        }
                        continue;
                    }
                    case PS_LOCAL: {
                        extract = Extractor();
                        extract.SetSearchPath(installPath);
                        extract.GetRoms(args);
                        extract.SetSearchPath(dataPath);
                        extract.GetRoms(args);
                        if (!args.empty()) {
                            promptStep = PS_WAIT;
                            BenGui::RegisterPopup(
                                "ROMs found", "ROMs found in application directory. Would you like to process them?",
                                "Yes", "No", [&]() { extractStep = ES_EXTRACT_ARGS; },
                                [&]() { promptStep = PS_FIRST; });
                        } else {
                            promptStep = PS_FIRST;
                        }
                        continue;
                    }
                    case PS_FIRST: {
                        if (!extract.ManuallySearchForRomMatchingType(RomSearchMode::Both)) {
                            promptStep = PS_FILE_CHECK;
                            continue;
                        }
                        extractionTask = threadPool->submit_task([&]() -> void {
                            extract.CallZapd(installPath, Ship::Context::GetAppDirectoryPath(appShortName),
                                             &extractCount, &totalExtract);
                            extractStep = ES_VERIFY;
                            extractCount = 0;
                            totalExtract = 0;
                        });
                        continue;
                    }
                    default:
                        break;
                }
                break;
            }
            case ES_VERIFY: {
                if (!std::filesystem::exists(Ship::Context::LocateFileAcrossAppDirs("mm.o2r", appShortName))) {
                    BenGui::RegisterPopup("No ROM Archives",
                                          "No ROM O2R files detected. Please generate a ROM O2R and relaunch.", "OK",
                                          "", [&]() { exit(0); });
                }
                extractDone = true;
                continue;
            }
            default:
                break;
        }

    render:
        if (!WindowIsRunning()) {
            exit(0);
        }
        // Process window events for resize, mouse, keyboard events
        wnd->HandleEvents();
        UIWidgets::Colors themeColor =
            static_cast<UIWidgets::Colors>(CVarGetInteger("gSettings.Menu.Theme", UIWidgets::Colors::LightBlue));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, UIWidgets::ColorValues.at(themeColor));
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, UIWidgets::ColorValues.at(UIWidgets::Colors::DarkGray));

        // Skip dropped frames
        if (!wnd->IsFrameReady()) {
            continue;
        }
        gui->StartDraw();
        benFast3dWindow->StartFrame();
        benFast3dWindow->RunGuiOnly();
        if (extractionTask.has_value()) {
            auto status = extractionTask->wait_for(std::chrono::milliseconds(0));
            if (status == std::future_status::ready) {
                try {
                    extractionTask->get();
                } catch (const std::exception& e) {
                    BenGui::RegisterPopup("Extraction Crashed", e.what(), "Close", "", []() { exit(1); });
                }
                extractionTask.reset();
            } else {
                if (!ImGui::IsPopupOpen("ROM Extraction")) {
                    ImGui::OpenPopup("ROM Extraction");
                }
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 8.0f));
                auto color = UIWidgets::ColorValues.at(THEME_COLOR);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(color.x, color.y, color.z, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(color.x, color.y, color.z, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
                if (ImGui::BeginPopupModal("ROM Extraction", NULL,
                                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
                                               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                               ImGuiWindowFlags_NoSavedSettings)) {
                    float progress = (totalExtract > 0.0f ? (float)extractCount / (float)totalExtract : 0) * 100.0f;
                    auto filename = std::filesystem::path(file).filename().string();
                    ImGui::Text("Extracting %s...%s", filename.c_str(),
                                roundf(progress) == 100.0f ? " Done. Finishing up." : "");
                    std::string overlay = extractCount > 0 ? fmt::format("{:.0f}%", progress) : "Starting Up";
                    ImGui::ProgressBar(progress / 100.0f, ImVec2(600.0f, 50.0f), overlay.c_str());
                    ImGui::EndPopup();
                }
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar(2);
            }
        }
        gui->EndDraw();
        benFast3dWindow->EndFrame();
        ImGui::PopStyleColor(2);
    }

#ifdef __SWITCH__
    Ship::Switch::Init(Ship::PreInitPhase);
#elif defined(__WIIU__)
    Ship::WiiU::Init(appShortName);
#endif
}

// ComboShip: our newer libultraship dropped Context::InitGfxDebugger; mirror soh's free helper
// (the mm baseline still called it as a Context method).
static void InitGfxDebugger() {
    auto dbg =
        std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow())->GetGfxDebugger();
    if (dbg != nullptr) {
        return;
    }
    dbg = std::make_shared<Fast::GfxDebugger>();
    if (dbg != nullptr) {
        SPDLOG_ERROR("Failed to initialize gfx debugger");
    }
}

void OTRGlobals::Initialize() {
    std::string mmPath = Ship::Context::LocateFileAcrossAppDirs("mm.o2r", appShortName);
    if (std::filesystem::exists(mmPath)) {
        context->GetResourceManager()->GetArchiveManager()->AddArchive(mmPath);
    }
#ifdef COMBO_BUILD
    // Locally prepared resources from the user's imported ROMs. Never APK/bundle data.
    const auto adultLocal = Ship::Context::LocateFileAcrossAppDirs("combo_adult_mm.local.o2r", appShortName);
    if (std::filesystem::exists(adultLocal))
        context->GetResourceManager()->GetArchiveManager()->AddArchive(adultLocal);
#endif

    std::unordered_set<uint32_t> validHashes = { MM_NTSC_US_10, MM_NTSC_US_GC };

#if (_DEBUG)
    auto defaultLogLevel = spdlog::level::debug;
#else
    auto defaultLogLevel = spdlog::level::info;
#endif
    context->InitConfiguration();
    context->InitConsoleVariables();
    auto logLevel = static_cast<spdlog::level::level_enum>(CVarGetInteger("gDeveloperTools.LogLevel", defaultLogLevel));
    context->InitLogging(logLevel, logLevel);
    Ship::Context::GetRawInstance()->GetLogger()->set_pattern("[%H:%M:%S.%e] [%s:%#] [%^%l%$] %v");

    InitGfxDebugger();
    context->InitFileDropMgr();

    // tell LUS to reserve 3 2S2H specific threads (Game, Audio, Save)
    prevAltAssets = CVarGetInteger("gEnhancements.Mods.AlternateAssets", 0);
    context->GetResourceManager()->SetAltAssetsEnabled(prevAltAssets);

    context->InitCrashHandler();

    context->GetWindow()->SetAutoCaptureMouse(CVarGetInteger("gSettings.EnableMouse", 0) &&
                                              CVarGetInteger("gSettings.AutoCaptureMouse", 1));
    context->GetWindow()->SetForceCursorVisibility(CVarGetInteger("gSettings.CursorVisibility", 0));

    context->InitAudio({ .SampleRate = 32000, .SampleLength = 1024, .DesiredBuffered = 1680 });

    SPDLOG_INFO("Starting 2 Ship 2 Harkinian version {} (Branch: {} | Commit: {})", (char*)gBuildVersion,
                (char*)gGitBranch, (char*)gGitCommitHash);

    auto loader = context->GetResourceManager()->GetResourceLoader();
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryTextureV0>(), RESOURCE_FORMAT_BINARY,
                                    "Texture", static_cast<uint32_t>(Fast::ResourceType::Texture), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryTextureV1>(), RESOURCE_FORMAT_BINARY,
                                    "Texture", static_cast<uint32_t>(Fast::ResourceType::Texture), 1);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryVertexV0>(), RESOURCE_FORMAT_BINARY,
                                    "Vertex", static_cast<uint32_t>(Fast::ResourceType::Vertex), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryXMLVertexV0>(), RESOURCE_FORMAT_XML, "Vertex",
                                    static_cast<uint32_t>(Fast::ResourceType::Vertex), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryDisplayListV0>(),
                                    RESOURCE_FORMAT_BINARY, "DisplayList",
                                    static_cast<uint32_t>(Fast::ResourceType::DisplayList), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryXMLDisplayListV0>(), RESOURCE_FORMAT_XML,
                                    "DisplayList", static_cast<uint32_t>(Fast::ResourceType::DisplayList), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryMatrixV0>(), RESOURCE_FORMAT_BINARY,
                                    "Matrix", static_cast<uint32_t>(Fast::ResourceType::Matrix), 0);
    loader->RegisterResourceFactory(std::make_shared<Ship::ResourceFactoryBinaryBlobV0>(), RESOURCE_FORMAT_BINARY,
                                    "Blob", static_cast<uint32_t>(Ship::ResourceType::Blob), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryArrayV0>(), RESOURCE_FORMAT_BINARY,
                                    "Array", static_cast<uint32_t>(SOH::ResourceType::SOH_Array), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAnimationV0>(), RESOURCE_FORMAT_BINARY,
                                    "Animation", static_cast<uint32_t>(SOH::ResourceType::SOH_Animation), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryPlayerAnimationV0>(),
                                    RESOURCE_FORMAT_BINARY, "PlayerAnimation",
                                    static_cast<uint32_t>(SOH::ResourceType::SOH_PlayerAnimation), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinarySceneV0>(), RESOURCE_FORMAT_BINARY,
                                    "Room", static_cast<uint32_t>(SOH::ResourceType::SOH_Room), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryCollisionHeaderV0>(),
                                    RESOURCE_FORMAT_BINARY, "CollisionHeader",
                                    static_cast<uint32_t>(SOH::ResourceType::SOH_CollisionHeader), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinarySkeletonV0>(), RESOURCE_FORMAT_BINARY,
                                    "Skeleton", static_cast<uint32_t>(SOH::ResourceType::SOH_Skeleton), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryXMLSkeletonV0>(), RESOURCE_FORMAT_XML,
                                    "Skeleton", static_cast<uint32_t>(SOH::ResourceType::SOH_Skeleton), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinarySkeletonLimbV0>(),
                                    RESOURCE_FORMAT_BINARY, "SkeletonLimb",
                                    static_cast<uint32_t>(SOH::ResourceType::SOH_SkeletonLimb), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryXMLSkeletonLimbV0>(), RESOURCE_FORMAT_XML,
                                    "SkeletonLimb", static_cast<uint32_t>(SOH::ResourceType::SOH_SkeletonLimb), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryPathMMV0>(), RESOURCE_FORMAT_BINARY,
                                    "Path", static_cast<uint32_t>(SOH::ResourceType::SOH_Path), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryCutsceneV0>(), RESOURCE_FORMAT_BINARY,
                                    "Cutscene", static_cast<uint32_t>(SOH::ResourceType::SOH_Cutscene), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryTextMMV0>(), RESOURCE_FORMAT_BINARY,
                                    "TextMM", static_cast<uint32_t>(SOH::ResourceType::TSH_TextMM), 0);

    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSampleV2>(), RESOURCE_FORMAT_BINARY,
                                    "AudioSample", static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSample), 2);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryXMLAudioSampleV0>(), RESOURCE_FORMAT_XML,
                                    "Sample", static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSample), 0);

    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSoundFontV2>(),
                                    RESOURCE_FORMAT_BINARY, "AudioSoundFont",
                                    static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSoundFont), 2);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryXMLSoundFontV0>(), RESOURCE_FORMAT_XML,
                                    "SoundFont", static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSoundFont), 0);

    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSequenceV2>(),
                                    RESOURCE_FORMAT_BINARY, "AudioSequence",
                                    static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSequence), 2);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryXMLAudioSequenceV0>(), RESOURCE_FORMAT_XML,
                                    "Sequence", static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSequence), 0);

    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryBackgroundV0>(), RESOURCE_FORMAT_BINARY,
                                    "Background", static_cast<uint32_t>(SOH::ResourceType::SOH_Background), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryTextureAnimationV0>(),
                                    RESOURCE_FORMAT_BINARY, "TextureAnimation",
                                    static_cast<uint32_t>(SOH::ResourceType::TSH_TexAnim), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryKeyFrameAnim>(), RESOURCE_FORMAT_BINARY,
                                    "KeyFrameAnim", static_cast<uint32_t>(SOH::ResourceType::TSH_CKeyFrameAnim), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryKeyFrameSkel>(), RESOURCE_FORMAT_BINARY,
                                    "KeyFrameSkel", static_cast<uint32_t>(SOH::ResourceType::TSH_CKeyFrameSkel), 0);

    // gSaveStateMgr = std::make_shared<SaveStateMgr>();
    // gRandomizer = std::make_shared<Randomizer>();

    auto versions = context->GetResourceManager()->GetArchiveManager()->GetGameVersions();
    for (uint32_t version : versions) {
        if (!validHashes.contains(version)) {
#if defined(__SWITCH__)
            SPDLOG_ERROR("Invalid O2R File!");
#elif defined(__WIIU__)
            Ship::WiiU::ThrowInvalidOTR();
#else
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Invalid O2R File",
                                     "Attempted to load an invalid O2R file. Try regenerating.", nullptr);
            SPDLOG_ERROR("Invalid O2R File!");
#endif
            exit(1);
        }
    }
}

OTRGlobals::~OTRGlobals() {
}

extern "C" uint32_t Ship_GetInterpolationFPS() {
    return OTRGlobals::Instance->GetInterpolationFPS();
}

struct ExtensionEntry {
    std::string path;
    std::string ext;
};

void OTRGlobals::ScaleImGui() {
    int32_t imGuiScaleIndex = CVarGetInteger("gSettings.ImGuiScale", defaultImGuiScale);
    if (imGuiScaleIndex == previousImGuiScaleIndex) {
        return;
    }

    float scale = imguiScaleOptionToValue[imGuiScaleIndex];
    float newScale = scale / previousImGuiScale;
    ImGui::GetStyle().ScaleAllSizes(newScale);
    ImGui::GetIO().FontGlobalScale = scale;
    previousImGuiScale = scale;
    previousImGuiScaleIndex = imGuiScaleIndex;
}

ImFont* OTRGlobals::CreateDefaultFontWithSize(float size) {
    auto mImGuiIo = &ImGui::GetIO();
    ImFontConfig fontCfg = ImFontConfig();
    fontCfg.OversampleH = fontCfg.OversampleV = 1;
    fontCfg.PixelSnapH = true;
    fontCfg.SizePixels = size;
    ImFont* font = mImGuiIo->Fonts->AddFontDefault(&fontCfg);
    // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
    float iconFontSize = size * 2.0f / 3.0f;
    static const ImWchar sIconsRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig iconsConfig;
    iconsConfig.MergeMode = true;
    iconsConfig.PixelSnapH = true;
    iconsConfig.GlyphMinAdvanceX = iconFontSize;
    mImGuiIo->Fonts->AddFontFromMemoryCompressedBase85TTF(fontawesome_compressed_data_base85, iconFontSize,
                                                          &iconsConfig, sIconsRanges);
    return font;
}

uint32_t OTRGlobals::GetInterpolationFPS() {
#ifdef COMBO_BUILD
    // ComboShip: the shared Graphics tab is OOT-rendered and writes the gSettings.* names.
    const char* matchRefreshCvar = "gSettings.MatchRefreshRate";
    const char* interpFpsCvar = "gSettings.InterpolationFPS";
#else
    const char* matchRefreshCvar = "gMatchRefreshRate";
    const char* interpFpsCvar = "gInterpolationFPS";
#endif
    if (CVarGetInteger(matchRefreshCvar, 0)) {
        return Ship::Context::GetRawInstance()->GetWindow()->GetCurrentRefreshRate();
    } else if (CVarGetInteger(CVAR_VSYNC_ENABLED, 1) ||
               !Ship::Context::GetRawInstance()->GetWindow()->CanDisableVerticalSync()) {
        return std::min<uint32_t>(Ship::Context::GetRawInstance()->GetWindow()->GetCurrentRefreshRate(),
                                  CVarGetInteger(interpFpsCvar, 20));
    }
    return CVarGetInteger(interpFpsCvar, 20);
}

extern "C" void OTRMessage_Init();
extern "C" void AudioMgr_CreateNextAudioBuffer(s16* samples, u32 num_samples);
extern "C" void AudioPlayer_Play(const uint8_t* buf, uint32_t len);
extern "C" int AudioPlayer_Buffered(void);
extern "C" int AudioPlayer_GetDesiredBuffered(void);
extern "C" void ResourceMgr_LoadDirectory(const char* resName);
std::unordered_map<std::string, ExtensionEntry> ExtensionCache;

static struct {
    std::thread thread;
    std::condition_variable cv_to_thread, cv_from_thread;
    std::mutex mutex;
    bool running;
    bool processing;
} audio;

void OTRAudio_Thread() {
    while (audio.running) {
        {
            std::unique_lock<std::mutex> Lock(audio.mutex);
            while (!audio.processing && audio.running) {
                audio.cv_to_thread.wait(Lock);
            }

            if (!audio.running) {
                break;
            }
        }
        std::unique_lock<std::mutex> Lock(audio.mutex);
// AudioMgr_ThreadEntry(&gAudioMgr);
//  528 and 544 relate to 60 fps at 32 kHz 32000/60 = 533.333..
//  in an ideal world, one third of the calls should use num_samples=544 and two thirds num_samples=528
#define SAMPLES_HIGH 560
#define SAMPLES_LOW 528

#define AUDIO_FRAMES_PER_UPDATE (R_UPDATE_RATE > 0 ? R_UPDATE_RATE : 1)
#define NUM_AUDIO_CHANNELS 2

        int samples_left = AudioPlayer_Buffered();
        u32 num_audio_samples = samples_left < AudioPlayer_GetDesiredBuffered() ? SAMPLES_HIGH : SAMPLES_LOW;

        // 3 is the maximum authentic frame divisor.
        s16 audio_buffer[SAMPLES_HIGH * NUM_AUDIO_CHANNELS * 3];
        for (int i = 0; i < AUDIO_FRAMES_PER_UPDATE; i++) {
            AudioMgr_CreateNextAudioBuffer(audio_buffer + i * (num_audio_samples * NUM_AUDIO_CHANNELS),
                                           num_audio_samples);
        }

        AudioPlayer_Play((u8*)audio_buffer,
                         num_audio_samples * (sizeof(int16_t) * NUM_AUDIO_CHANNELS * AUDIO_FRAMES_PER_UPDATE));

        audio.processing = false;
        audio.cv_from_thread.notify_one();
    }
}

// C->C++ Bridge
extern "C" void OTRAudio_Init() {
    // Precache all our samples, sequences, etc...
    ResourceMgr_LoadDirectory("audio");

    if (!audio.running) {
        audio.running = true;
        audio.thread = std::thread(OTRAudio_Thread);
    }
}

extern "C" char** gSequenceMap;
extern "C" size_t gSequenceMapSize;

extern "C" char** gFontMap;
extern "C" size_t gFontMapSize;

extern "C" void OTRAudio_Exit() {
    // Tell the audio thread to stop
    {
        std::unique_lock<std::mutex> Lock(audio.mutex);
        audio.running = false;
    }
    audio.cv_to_thread.notify_all();

    // Wait until the audio thread quit
    // ComboShip: guard the join — at combo shutdown MM_Deinit calls this again after
    // MM_PrepareForTransition already joined the thread, and join() on a non-joinable thread
    // throws std::system_error -> terminate.
    if (audio.thread.joinable()) {
        audio.thread.join();
    }
#ifndef COMBO_BUILD
    // In a combo build OTRAudio_Exit runs on every OOT<->MM transition, not just at shutdown. These
    // maps (gFontMap/gSequenceMap) + load-status arrays are populated once by AudioLoad_Init at boot
    // and must stay resident so a later MM resume can use them (its OTRAudio_Init only restarts the
    // thread, it doesn't repopulate them). Freeing them left gFontMap[fontId] dangling ->
    // AudioHeap_Init/LoadPermanentSamples strlen(null) crash. Keep them (tiny leak only at real exit).
    for (size_t i = 0; i < gSequenceMapSize; i++) {
        free(gSequenceMap[i]);
    }
    free(gSequenceMap);

    for (size_t i = 0; i < gFontMapSize; i++) {
        free(gFontMap[i]);
    }
    free(gFontMap);
    free(gAudioCtx.seqLoadStatus);
    free(gAudioCtx.fontLoadStatus);
#endif
}

extern "C" void OTRExtScanner() {
    auto lst = *Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->ListFiles("*").get();

    for (auto& rPath : lst) {
        std::vector<std::string> raw = StringHelper::Split(rPath, ".");
        std::string ext = raw[raw.size() - 1];
        std::string nPath = rPath.substr(0, rPath.size() - (ext.size() + 1));
        replace(nPath.begin(), nPath.end(), '\\', '/');

        ExtensionCache[nPath] = { rPath, ext };
    }
}

// Read the port version from an archive file
ArchiveVersion ReadPortVersionFromArchive(std::string archivePath, bool isO2rType) {
    ArchiveVersion version = {};

    // Use a temporary archive instance to load the archive appropriately and read the version file
    std::shared_ptr<Ship::Archive> archive;
    if (isO2rType) {
        archive = make_shared<Ship::O2rArchive>(archivePath);
    } else {
#ifdef INCLUDE_MPQ_SUPPORT
        archive = make_shared<Ship::OtrArchive>(archivePath);
#else
        SPDLOG_ERROR("An OTR File, {}, was found but support for them is not included. File will be ignored.",
                     archivePath.c_str());
#endif
    }
    if (archive->Open()) {
        auto t = archive->LoadFile("portVersion");
        if (t != nullptr && t->IsLoaded) {
            auto stream = std::make_shared<Ship::MemoryStream>(t->Buffer->data(), t->Buffer->size());
            auto reader = std::make_shared<Ship::BinaryReader>(stream);
            Ship::Endianness endianness = (Ship::Endianness)reader->ReadUByte();
            reader->SetEndianness(endianness);
            version.major = reader->ReadUInt16();
            version.minor = reader->ReadUInt16();
            version.patch = reader->ReadUInt16();
        }
    }

    return version;
}

// Check that a 2ship.o2r exists and matches the version of 2ship running
// Otherwise show a message and exit
// For Windows/Mac/Linux if the version doesn't match, offer to regenerate it
ArchiveVersion DetectArchiveVersion(std::string fileName, bool isO2rType) {
    bool isArchiveOld = false;
    std::string archivePath = Ship::Context::LocateFileAcrossAppDirs(fileName, appShortName);

    // Doesn't exist so nothing to do here
    if (!std::filesystem::exists(archivePath)) {
        return { INT16_MAX, INT16_MAX, INT16_MAX };
    }

    return ReadPortVersionFromArchive(archivePath, isO2rType);
}

extern "C" void Messagebox_ShowErrorBox(char* title, char* body) {
    Extractor::ShowErrorBox(title, body);
}

bool VerifyArchiveVersion(ArchiveVersion version) {
    return version.major != INT16_MAX && version.major != gBuildVersionMajor;
}

extern "C" void InitOTR(int argc, char* argv[]) {
    OTRGlobals::Instance = new OTRGlobals();
    OTRGlobals::Instance->RunExtract(argc, argv);

    OTRGlobals::Instance->Initialize();

    std::shared_ptr<Ship::Config> conf = OTRGlobals::Instance->context->GetConfig();
    conf->RegisterVersionUpdater(std::make_shared<Ben::ConfigVersion1Updater>());
    conf->RunVersionUpdates();
    Ship::Context::GetRawInstance()->GetConsoleVariables()->Save();

    GameInteractor::Instance = new GameInteractor();
    AudioCollection::Instance = new AudioCollection();
    LoadGuiTextures();
    ModMenu_LoadArchives();
    BenGui::SetupGuiElements();
    ShipInit::InitAll();
#ifdef COMBO_BUILD
    // Reverse MM->OOT trigger: the Clock Tower interior's South-Clock-Town door (spawn 1 only —
    // cycle resets respawn in this scene at spawns 0/2/3/6 and must stay in MM).
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s8 sceneId, s8 spawnNum) {
        // GAMEMODE_NORMAL only: MM's attract demo (GAMEMODE_TITLE_SCREEN, after Sram_InitNewSave wiped
        // the save) scene-hops through here, and would both teleport the player to OOT and persist the
        // wiped save over the slot.
        if (sceneId == SCENE_INSIDETOWER && spawnNum == 1 && gSaveContext.gameMode == GAMEMODE_NORMAL) {
            sComboReturnPending = true;
        }
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateMainStart>([]() {
        ProcessForeignSongReply();
        if (sSongDeparture >= 0 || MM_FaroreDeparturePending())
            sComboReturnPending = true;
        if (!sComboReturnPending && !sComboResetReturnPending && !sComboOwlSaveQuitPending)
            return;
        const bool isReset = sComboResetReturnPending;
        const bool isOwlSaveQuit = sComboOwlSaveQuitPending;
        sComboReturnPending = false;
        sComboResetReturnPending = false;
        sComboOwlSaveQuitPending = false;
        // An owl save quit lands on OOT's title, like Ctrl+R, rather than resuming OOT gameplay.
        if (isOwlSaveQuit) {
            static void (*sFn)(void) = nullptr;
            static bool sTried = false;
            if (!sTried) {
                sTried = true;
                sFn = (void (*)(void))Combo_ResolveSym("soh", "SOH_SetComboBootToTitle");
            }
            if (sFn)
                sFn();
        }
        // Portal return always persists MM; a reset persists only when autosave is enabled. Never
        // persist outside gameplay: the title/attract path wipes save first (Sram_InitNewSave). An owl
        // save has already written itself through the flashrom seam.
        if (!isOwlSaveQuit && (!isReset || CVarGetInteger("gEnhancements.Saving.Autosave", 0)) &&
            gSaveContext.gameMode == GAMEMODE_NORMAL)
            SaveManager_SaveCurrentForCombo();
        if (MM_FaroreDeparturePending() && !isReset && !isOwlSaveQuit) {
            if (!MM_DispatchNativeDeparture(gPlayState))
                return;
        } else {
            if (isReset || isOwlSaveQuit)
                MM_CancelNativeDeparture();
            if (gComboReturnCallback)
                gComboReturnCallback(isOwlSaveQuit ? 2 : (isReset ? 1 : 0));
        }
        if (auto fast3d = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow())) {
            fast3d->SetIsRunning(false);
        }
    });
    // Shared Items: per-frame drain seam. Always-on (not Anchor-gated) so it also runs solo.
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateUpdate>([]() {
        if (gMMComboSharedTick) {
            gMMComboSharedTick();
        }
    });
#endif
    Rando::Init();
    GfxPatcher_ApplyNecessaryAuthenticPatches();
    DebugConsole_Init();
#if defined(COMBO_BUILD) && defined(COMBO_ADULT_MM_RUNTIME_DIAGNOSTICS)
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorUpdate>(ACTOR_PLAYER, [](Actor* actor) {
        static unsigned snapshotFrames = 0;
        if (!CVarGetInteger("gCombo.Debug.AdultMmRuntimeValidation", 0) || !gPlayState || gSaveContext.fileNum > 2 ||
            !actor)
            return;
        Player* player = reinterpret_cast<Player*>(actor);
        Camera* camera = GET_ACTIVE_CAM(gPlayState);
        if (!camera || !player->ageProperties || !player->skelAnime.jointTable)
            return;
        if (snapshotFrames++ >= 5 && snapshotFrames % 300 != 0)
            return;
        const Vec3s root = player->skelAnime.jointTable[0], head = player->skelAnime.jointTable[PLAYER_LIMB_HEAD];
        SPDLOG_INFO("[ComboShip AdultReadonly] form={} scale={}/{}/{} height={} propertyHeight={} effectiveAge={} "
                    "rawAge={} enabled={} eye={}/{}/{} at={}/{}/{} cameraDist={} skeleton={} humanTable={} "
                    "jointsRoot={}/{}/{} jointsHead={}/{}/{}",
                    int(player->transformation), player->actor.scale.x, player->actor.scale.y, player->actor.scale.z,
                    Player_GetHeight(player), player->ageProperties->unk_34,
                    int(gSaveContext.save.shipSaveInfo.adultMm.enabled ? (gSaveContext.save.linkAge == 0 ? 0 : 1) : 1),
                    int(gSaveContext.save.linkAge), int(gSaveContext.save.shipSaveInfo.adultMm.enabled), camera->eye.x,
                    camera->eye.y, camera->eye.z, camera->at.x, camera->at.y, camera->at.z, camera->dist,
                    static_cast<const void*>(player->skelAnime.skeleton),
                    static_cast<const void*>(gPlayerSkeletons[PLAYER_FORM_HUMAN]), root.x, root.y, root.z, head.x,
                    head.y, head.z);
    });
#endif
    GameInteractor::Instance->RegisterOwnHooks();
    CustomItem::RegisterHooks();
    CustomMessage::RegisterHooks();
    Rando::StaticData::PopulateCheckNames();

    OTRMessage_Init();
    OTRAudio_Init();
    OTRExtScanner();
    PlayerCustomFlipbooks_Patch();

    // Just came up with arbitrary numbers that seemed to work, this is
    // usually set once(?) in currently stubbed out areas of code.
    gIrqMgrRetraceTime = Ship_Random(700000, 850000);

    time_t now = time(NULL);
    tm* tm_now = localtime(&now);
    if (tm_now->tm_mon == 11 && tm_now->tm_mday >= 24 && tm_now->tm_mday <= 25) {
        CVarRegisterInteger("gLetItSnow", 1);
    } else {
        CVarClear("gLetItSnow");
    }

    srand(now);
#ifdef ENABLE_CROWD_CONTROL
    CrowdControl::Instance = new CrowdControl();
    CrowdControl::Instance->Init();
    if (CVarGetInteger("gCrowdControl", 0)) {
        CrowdControl::Instance->Enable();
    } else {
        CrowdControl::Instance->Disable();
    }
#endif

    Ship::Context::GetRawInstance()->GetFileDropMgr()->RegisterDropHandler(BinarySaveConverter_HandleFileDropped);
    Ship::Context::GetRawInstance()->GetFileDropMgr()->RegisterDropHandler(SaveManager_HandleFileDropped);
}

extern "C" void SaveManager_ThreadPoolWait() {
    // SaveManager::Instance->ThreadPoolWait();
}

extern "C" void DeinitOTR() {
    SaveManager_ThreadPoolWait();
    OTRAudio_Exit();
#ifdef ENABLE_CROWD_CONTROL
    CrowdControl::Instance->Disable();
    CrowdControl::Instance->Shutdown();
#endif

    // Destroying gui here because we have shared ptrs to LUS objects which output to SPDLOG which is destroyed before
    // these shared ptrs.
    BenGui::Destroy();
    benFast3dWindow = nullptr;

#ifdef COMBO_BUILD
    // ComboShip: drop the resident-RM refs now so MM's ResourceManager dies on the main thread
    // during shutdown. Left in these statics/registry, it would instead die in DLL-unload static
    // destructors, where its thread pool joins workers under the loader lock and deadlocks.
    Ship::CrossRMRegistry::Unregister("mm");
    sMMResourceManager = nullptr;
#endif

    OTRGlobals::Instance->context = nullptr;
    delete AudioCollection::Instance;
#if defined(COMBO_BUILD) && !defined(__ANDROID__)
    // ComboShip: this DLL's module-local GImGui still points at the shared ImGui context, which is
    // freed when soh's DeinitOTR releases the last Context ref right after this returns. A later
    // 2ship static destructor calling ImGui::MemFree would then touch freed memory. Null it now.
    ImGui::SetCurrentContext(nullptr);
#endif
    // Android resolves ImGui through shared libultraship. Clearing its context here would make
    // the subsequent SOH_Deinit crash in ImGui_ImplSDL2_Shutdown; SOH destroys it after both games.
}

#ifdef COMBO_BUILD
// ComboShip: full MM teardown at process shutdown. MM holds a shared_ptr to the shared Context
// (the ctor reused OOT's), so without this its refcount never hits zero, ~Context never runs, and
// window geometry/config are never saved. Call this before SOH_Deinit (the Context must stay alive
// for BenGui::Destroy) so SOH's DeinitOTR releases the last ref and ~Context saves on the main thread.
extern "C" COMBO_EXPORT void MM_Deinit(void) {
    DeinitOTR();
}
#endif

#ifdef _WIN32
extern "C" uint64_t GetFrequency() {
    LARGE_INTEGER nFreq;

    QueryPerformanceFrequency(&nFreq);

    return nFreq.QuadPart;
}

extern "C" uint64_t GetPerfCounter() {
    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);

    return ticks.QuadPart;
}
#else
extern "C" uint64_t GetFrequency() {
    return 1000; // sec -> ms
}

extern "C" uint64_t GetPerfCounter() {
    struct timespec monotime;
    clock_gettime(CLOCK_MONOTONIC, &monotime);

    uint64_t remainingMs = (monotime.tv_nsec / 1000000);

    // in milliseconds
    return monotime.tv_sec * 1000 + remainingMs;
}
#endif

extern "C" uint64_t GetUnixTimestamp() {
    auto time = std::chrono::system_clock::now();
    auto since_epoch = time.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
    long now = millis.count();
    return now;
}

extern "C" void Graph_StartFrame() {
#ifndef __WIIU__
    using Ship::KbScancode;
    int32_t dwScancode = OTRGlobals::Instance->context->GetWindow()->GetLastScancode();
    OTRGlobals::Instance->context->GetWindow()->SetLastScancode(-1);

    switch (dwScancode) {
#if 0
        case KbScancode::LUS_KB_F5: {
            if (CVarGetInteger("gSaveStatesEnabled", 0) == 0) {
                Ship::Context::GetRawInstance()->GetWindow()->GetGui()->GetGameOverlay()->TextDrawNotification(
                    6.0f, true, "Save states not enabled. Check Cheats Menu.");
                return;
            }
            const unsigned int slot = OTRGlobals::Instance->gSaveStateMgr->GetCurrentSlot();
            const SaveStateReturn stateReturn =
                OTRGlobals::Instance->gSaveStateMgr->AddRequest({ slot, RequestType::SAVE });

            switch (stateReturn) {
                case SaveStateReturn::SUCCESS:
                    SPDLOG_INFO("[SOH] Saved state to slot {}", slot);
                    break;
                case SaveStateReturn::FAIL_WRONG_GAMESTATE:
                    SPDLOG_ERROR("[SOH] Can not save a state outside of \"GamePlay\"");
                    break;
                    [[unlikely]] default : break;
            }
            break;
        }
        case KbScancode::LUS_KB_F6: {
            if (CVarGetInteger("gSaveStatesEnabled", 0) == 0) {
                Ship::Context::GetRawInstance()->GetWindow()->GetGui()->GetGameOverlay()->TextDrawNotification(
                    6.0f, true, "Save states not enabled. Check Cheats Menu.");
                return;
            }
            unsigned int slot = OTRGlobals::Instance->gSaveStateMgr->GetCurrentSlot();
            slot++;
            if (slot > 5) {
                slot = 0;
            }
            OTRGlobals::Instance->gSaveStateMgr->SetCurrentSlot(slot);
            SPDLOG_INFO("Set SaveState slot to {}.", slot);
            break;
        }
        case KbScancode::LUS_KB_F7: {
            if (CVarGetInteger("gSaveStatesEnabled", 0) == 0) {
                Ship::Context::GetRawInstance()->GetWindow()->GetGui()->GetGameOverlay()->TextDrawNotification(
                    6.0f, true, "Save states not enabled. Check Cheats Menu.");
                return;
            }
            const unsigned int slot = OTRGlobals::Instance->gSaveStateMgr->GetCurrentSlot();
            const SaveStateReturn stateReturn =
                OTRGlobals::Instance->gSaveStateMgr->AddRequest({ slot, RequestType::LOAD });

            switch (stateReturn) {
                case SaveStateReturn::SUCCESS:
                    SPDLOG_INFO("[SOH] Loaded state from slot {}", slot);
                    break;
                case SaveStateReturn::FAIL_INVALID_SLOT:
                    SPDLOG_ERROR("[SOH] Invalid State Slot Number {}", slot);
                    break;
                case SaveStateReturn::FAIL_STATE_EMPTY:
                    SPDLOG_ERROR("[SOH] State Slot {} is empty", slot);
                    break;
                case SaveStateReturn::FAIL_WRONG_GAMESTATE:
                    SPDLOG_ERROR("[SOH] Can not load a state outside of \"GamePlay\"");
                    break;
                    [[unlikely]] default : break;
            }

            break;
        }
#endif
#if defined(_WIN32) || defined(__APPLE__)
        case KbScancode::LUS_KB_F9: {
            // Toggle TTS
#ifdef COMBO_BUILD
            // ComboShip: shared gSettings.* name (OOT-rendered settings widget writes it).
            CVarSetInteger("gSettings.A11yTTS", !CVarGetInteger("gSettings.A11yTTS", 0));
#else
            CVarSetInteger("gA11yTTS", !CVarGetInteger("gA11yTTS", 0));
#endif
            break;
        }
#endif
        case KbScancode::LUS_KB_TAB: {
            // Toggle HD Assets
            if (CVarGetInteger("gEnhancements.Mods.AlternateAssetsHotkey", 1)) {
                CVarSetInteger("gEnhancements.Mods.AlternateAssets",
                               !CVarGetInteger("gEnhancements.Mods.AlternateAssets", 0));
            }
            break;
        }
    }
#endif
}

// Interpolated frames of a tick are evenly spaced numerators time+step, time+2*step, ... over denom.
void RunCommands(Gfx* Commands, int time, int step, int denom, int count) {
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(OTRGlobals::Instance->context->GetWindow());

    if (wnd == nullptr) {
        return;
    }

    // Process window events for resize, mouse, keyboard events
    wnd->HandleEvents();

    auto intp = wnd->GetInterpreterWeak().lock().get();
    intp->mInterpolationIndex = 0;

    UIWidgets::Colors themeColor =
        static_cast<UIWidgets::Colors>(CVarGetInteger("gSettings.Menu.Theme", UIWidgets::Colors::LightBlue));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, UIWidgets::ColorValues.at(themeColor));
    for (int i = 0; i < count; i++) {
        time += step;
        std::unordered_map<Mtx*, MtxF> mtx_replacements =
            (time == denom) ? std::unordered_map<Mtx*, MtxF>() : FrameInterpolation_Interpolate((float)time / denom);
        intp->mInterpolationT = (float)time / denom;
        wnd->DrawAndRunGraphicsCommands(Commands, mtx_replacements);
        intp->mInterpolationIndex++;
    }
    ImGui::PopStyleColor();
}

// C->C++ Bridge
extern "C" void Graph_ProcessGfxCommands(Gfx* commands) {
    {
        std::unique_lock<std::mutex> Lock(audio.mutex);
        audio.processing = true;
    }

    audio.cv_to_thread.notify_one();
    int target_fps = OTRGlobals::Instance->GetInterpolationFPS();
    static int last_fps;
    static int last_update_rate;
    static int time;
    int fps = target_fps;
    int original_fps = 60 / R_UPDATE_RATE;
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow());

    if (target_fps == 20 || original_fps > target_fps) {
        fps = original_fps;
    }

    if (last_fps != fps || last_update_rate != R_UPDATE_RATE) {
        time = 0;
    }

    // time_base = fps * original_fps (one second)
    int next_original_frame = fps;

    int start_time = time;
    int count = 0;
    while (time + original_fps <= next_original_frame) {
        time += original_fps;
        count++;
    }

    time -= fps;

    if (wnd != nullptr) {
        wnd->SetTargetFps(fps);
    }

    int step = original_fps;
    // When the gfx debugger is active, only run with the final mtx
    if (GfxDebuggerIsDebugging()) {
        start_time = next_original_frame;
        step = 0;
        count = 1;
    }

    RunCommands(commands, start_time, step, next_original_frame, count);

    last_fps = fps;
    last_update_rate = R_UPDATE_RATE;

    {
        std::unique_lock<std::mutex> Lock(audio.mutex);
        while (audio.processing) {
            audio.cv_from_thread.wait(Lock);
        }
    }

    bool curAltAssets = CVarGetInteger("gEnhancements.Mods.AlternateAssets", 0);
    if (prevAltAssets != curAltAssets) {
        prevAltAssets = curAltAssets;
        Ship::Context::GetRawInstance()->GetResourceManager()->SetAltAssetsEnabled(curAltAssets);
        gfx_texture_cache_clear();
        PlayerCustomFlipbooks_Patch();
        SOH::SkeletonPatcher::UpdateSkeletons();
        // GameInteractor::Instance->ExecuteHooks<GameInteractor::OnAssetAltChange>();
    }

    // OTRTODO: FIGURE OUT END FRAME POINT
    /* if (OTRGlobals::Instance->context->GetWindow()->lastScancode != -1)
         OTRGlobals::Instance->context->GetWindow()->lastScancode = -1;*/
}

float divisor_num = 0.0f;

// Batch a coordinate to have its depth read later by OTRGetPixelDepth
extern "C" void OTRGetPixelDepthPrepare(float x, float y) {
    // Invert the Y value to match the origin values used in the renderer
    float adjustedY = SCREEN_HEIGHT - y;

    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow());
    if (wnd == nullptr) {
        return;
    }

    wnd->GetPixelDepthPrepare(x, adjustedY);
}

extern "C" uint16_t OTRGetPixelDepth(float x, float y) {
    // Invert the Y value to match the origin values used in the renderer
    float adjustedY = SCREEN_HEIGHT - y;

    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow());
    if (wnd == nullptr) {
        return 0;
    }

    return wnd->GetPixelDepth(x, adjustedY);
}

extern "C" bool ResourceMgr_IsAltAssetsEnabled() {
    return Ship::Context::GetRawInstance()->GetResourceManager()->IsAltAssetsEnabled();
}

extern "C" uint32_t ResourceMgr_GetNumGameVersions() {
    return Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions().size();
}

extern "C" uint32_t ResourceMgr_GetGameVersion(int index) {
    return Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions()[index];
}

extern "C" uint32_t ResourceMgr_GetGamePlatform(int index) {
    uint32_t version =
        Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions()[index];

    switch (version) {
        case MM_NTSC_US_10:
            return GAME_PLATFORM_N64;
        case MM_NTSC_US_GC:
            return GAME_PLATFORM_GC;
    }
}

extern "C" uint32_t ResourceMgr_GetGameRegion(int index) {
    uint32_t version =
        Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions()[index];

    switch (version) {
        case MM_NTSC_US_10:
        case MM_NTSC_US_GC:
            return GAME_REGION_NTSC;
    }
}

extern "C" void ResourceMgr_LoadDirectory(const char* resName) {
    Ship::Context::GetRawInstance()->GetResourceManager()->LoadResources(resName);
}
extern "C" void ResourceMgr_DirtyDirectory(const char* resName) {
    Ship::Context::GetRawInstance()->GetResourceManager()->DirtyResources(resName);
}

extern "C" void ResourceMgr_UnloadResource(const char* resName) {
    std::string path = resName;
    if (path.starts_with("__OTR__")) {
        path = path.substr(7);
    }
    Ship::Context::GetRawInstance()->GetResourceManager()->UnloadResource(path);
}

static void ResourceMgr_PreloadAltWhenItExists(const char* resName) {
    std::string path = resName;
    if (path.starts_with("__OTR__")) {
        path = path.substr(7);
    }

    if (ResourceMgr_IsAltAssetsEnabled() && ExtensionCache.contains(Ship::IResource::gAltAssetPrefix + path)) {
        Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(Ship::IResource::gAltAssetPrefix + path,
                                                                            true);
    }
}

// OTRTODO: There is probably a more elegant way to go about this...
// Kenix: This is definitely leaking memory when it's called.
extern "C" char** ResourceMgr_ListFiles(const char* searchMask, int* resultSize) {
    auto lst = Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->ListFiles(searchMask);
    char** result = (char**)malloc(lst->size() * sizeof(char*));

    for (size_t i = 0; i < lst->size(); i++) {
        char* str = (char*)malloc(lst.get()[0][i].size() + 1);
        memcpy(str, lst.get()[0][i].data(), lst.get()[0][i].size());
        str[lst.get()[0][i].size()] = '\0';
        result[i] = str;
    }
    *resultSize = lst->size();

    return result;
}

extern "C" uint8_t ResourceMgr_FileExists(const char* filePath) {
    std::string path = filePath;
    if (path.substr(0, 7) == "__OTR__") {
        path = path.substr(7);
    }

    return ExtensionCache.contains(path);
}

extern "C" void ResourceMgr_LoadFile(const char* resName) {
    Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(resName);
}

std::shared_ptr<Ship::IResource> GetResourceByName(const char* path) {
    return Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(path);
}

extern "C" char* ResourceMgr_LoadFileFromDisk(const char* filePath) {
    FILE* file = fopen(filePath, "r");
    fseek(file, 0, SEEK_END);
    int fSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* data = (char*)malloc(fSize);
    fread(data, 1, fSize, file);

    fclose(file);

    return data;
}

extern "C" uint8_t ResourceMgr_ResourceIsBackground(char* texPath) {
    auto res = GetResourceByName(texPath);
    return res->GetInitData()->Type == static_cast<uint32_t>(SOH::ResourceType::SOH_Background);
}

extern "C" char* ResourceMgr_LoadJPEG(char* data, size_t dataSize) {
    static char* finalBuffer = 0;

    if (finalBuffer == 0)
        finalBuffer = (char*)malloc(dataSize);

    int w;
    int h;
    int comp;

    unsigned char* pixels =
        stbi_load_from_memory((const unsigned char*)data, 320 * 240 * 2, &w, &h, &comp, STBI_rgb_alpha);
    // unsigned char* pixels = stbi_load_from_memory((const unsigned char*)data, 480 * 240 * 2, &w, &h, &comp,
    // STBI_rgb_alpha);
    int idx = 0;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint16_t* bufferTest = (uint16_t*)finalBuffer;
            int pixelIdx = ((y * w) + x) * 4;

            uint8_t r = pixels[pixelIdx + 0] / 8;
            uint8_t g = pixels[pixelIdx + 1] / 8;
            uint8_t b = pixels[pixelIdx + 2] / 8;

            uint8_t alphaBit = pixels[pixelIdx + 3] != 0;

            uint16_t data = (r << 11) + (g << 6) + (b << 1) + alphaBit;

            finalBuffer[idx++] = (data & 0xFF00) >> 8;
            finalBuffer[idx++] = (data & 0x00FF);
        }
    }

    return (char*)finalBuffer;
}

extern "C" uint16_t ResourceMgr_LoadTexWidthByName(char* texPath);

extern "C" uint16_t ResourceMgr_LoadTexHeightByName(char* texPath);

extern "C" char* ResourceMgr_LoadTexOrDListByName(const char* filePath) {
    auto res = GetResourceByName(filePath);

#ifdef COMBO_BUILD
    // ComboShip: a miss returns null here (same as ResourceMgr_LoadIfDListByName). The GBI wrappers
    // that reach this (stubs.c gSPInvalidateTexCache / gSPSegmentLoadRes) tolerate a null address.
    if (res == nullptr) {
        return nullptr;
    }
#endif
    if (res->GetInitData()->Type == static_cast<uint32_t>(Fast::ResourceType::DisplayList))
        return (char*)&((std::static_pointer_cast<Fast::DisplayList>(res))->Instructions[0]);
    else if (res->GetInitData()->Type == static_cast<uint32_t>(SOH::ResourceType::SOH_Array))
        return (char*)(std::static_pointer_cast<SOH::Array>(res))->Vertices.data();
    else {
        return (char*)ResourceGetDataByName(filePath);
    }
}

extern "C" char* ResourceMgr_LoadIfDListByName(const char* filePath) {
    auto res = GetResourceByName(filePath);

#ifdef COMBO_BUILD
    // ComboShip: a miss returns null here; callers already treat a null return as "not a DList".
    if (res == nullptr) {
        return nullptr;
    }
#endif
    if (res->GetInitData()->Type == static_cast<uint32_t>(Fast::ResourceType::DisplayList))
        return (char*)&((std::static_pointer_cast<Fast::DisplayList>(res))->Instructions[0]);

    return nullptr;
}

// extern "C" Sprite* GetSeedTexture(uint8_t index) {
//     return OTRGlobals::Instance->gRandomizer->GetSeedTexture(index);
// }

extern "C" char* ResourceMgr_LoadPlayerAnimByName(const char* animPath) {
    auto anim = std::static_pointer_cast<SOH::PlayerAnimation>(GetResourceByName(animPath));

    return (char*)&anim->limbRotData[0];
}

extern "C" void ResourceMgr_PushCurrentDirectory(char* path) {
    Fast::gfx_push_current_dir(path);
}

extern "C" Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    ResourceMgr_PreloadAltWhenItExists(path);

    auto res = std::static_pointer_cast<Fast::DisplayList>(GetResourceByName(path));
    return (Gfx*)&res->Instructions[0];
}

typedef struct {
    int index;
    Gfx instruction;
} GfxPatch;

std::unordered_map<std::string, std::unordered_map<std::string, GfxPatch>> originalGfx;

// Attention! This is primarily for cosmetics & bug fixes. For things like mods and model replacement you should be
// using OTRs instead (When that is available). Index can be found using the commented out section below.
extern "C" void ResourceMgr_PatchGfxByName(const char* path, const char* patchName, int index, Gfx instruction) {
    auto res = std::static_pointer_cast<Fast::DisplayList>(
        Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(path));

    // Leaving this here for people attempting to find the correct Dlist index to patch
    /*if (strcmp("__OTR__objects/object_gi_longsword/gGiBiggoronSwordDL", path) == 0) {
        for (int i = 0; i < res->instructions.size(); i++) {
            Gfx* gfx = (Gfx*)&res->instructions[i];
            // Log all commands
            // SPDLOG_INFO("index:{} command:{}", i, gfx->words.w0 >> 24);
            // Log only SetPrimColors
            if (gfx->words.w0 >> 24 == 250) {
                SPDLOG_INFO("index:{} r:{} g:{} b:{} a:{}", i, _SHIFTR(gfx->words.w1, 24, 8), _SHIFTR(gfx->words.w1, 16,
    8), _SHIFTR(gfx->words.w1, 8, 8), _SHIFTR(gfx->words.w1, 0, 8));
            }
        }
    }*/

    // Index refers to individual gfx words, which are half the size on 32-bit
    // if (sizeof(uintptr_t) < 8) {
    // index /= 2;
    // }

    // Do not patch custom assets as they most likely do not have the same instructions as authentic assets
    if (res->GetInitData()->IsCustom) {
        return;
    }

    Gfx* gfx = (Gfx*)&res->Instructions[index];

    if (!originalGfx.contains(path) || !originalGfx[path].contains(patchName)) {
        originalGfx[path][patchName] = { index, *gfx };
    }

    *gfx = instruction;
}

extern "C" void ResourceMgr_PatchGfxCopyCommandByName(const char* path, const char* patchName, int destinationIndex,
                                                      int sourceIndex) {
    auto res = std::static_pointer_cast<Fast::DisplayList>(
        Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(path));

    // Do not patch custom assets as they most likely do not have the same instructions as authentic assets
    if (res->GetInitData()->IsCustom) {
        return;
    }

    Gfx* destinationGfx = (Gfx*)&res->Instructions[destinationIndex];
    Gfx sourceGfx = *(Gfx*)&res->Instructions[sourceIndex];

    if (!originalGfx.contains(path) || !originalGfx[path].contains(patchName)) {
        originalGfx[path][patchName] = { destinationIndex, *destinationGfx };
    }

    *destinationGfx = sourceGfx;
}

extern "C" void ResourceMgr_UnpatchGfxByName(const char* path, const char* patchName) {
    if (originalGfx.contains(path) && originalGfx[path].contains(patchName)) {
        auto res = std::static_pointer_cast<Fast::DisplayList>(
            Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(path));

        if (res->GetInitData()->IsCustom) {
            return;
        }

        Gfx* gfx = (Gfx*)&res->Instructions[originalGfx[path][patchName].index];
        *gfx = originalGfx[path][patchName].instruction;

        originalGfx[path].erase(patchName);
    }
}

extern "C" size_t ResourceMgr_GetPatchCountForDL(const char* path) {
    if (originalGfx.contains(path)) {
        return originalGfx[path].size();
    }
    return 0;
}

extern "C" void ResourceMgr_ResetAllPatchesForDL(const char* path) {
    if (!originalGfx.contains(path)) {
        return;
    }

    auto res = std::static_pointer_cast<Fast::DisplayList>(
        Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(path));

    // Iterate through all patches and restore original instructions
    auto& patches = originalGfx[path];
    for (auto it = patches.begin(); it != patches.end();) {
        Gfx* gfx = (Gfx*)&res->Instructions[it->second.index];
        *gfx = it->second.instruction;
        // erase() returns the next iterator, allowing safe iteration during removal
        it = patches.erase(it);
    }

    // Clean up empty map entry
    if (patches.empty()) {
        originalGfx.erase(path);
    }
}

extern "C" char* ResourceMgr_LoadVtxArrayByName(const char* path) {
    auto res = std::static_pointer_cast<SOH::Array>(GetResourceByName(path));

    return (char*)res->Vertices.data();
}

extern "C" size_t ResourceMgr_GetVtxArraySizeByName(const char* path) {
    auto res = std::static_pointer_cast<SOH::Array>(GetResourceByName(path));

    return res->Vertices.size();
}

extern "C" char* ResourceMgr_LoadArrayByName(const char* path) {
    auto res = std::static_pointer_cast<SOH::Array>(GetResourceByName(path));

    return (char*)res->Scalars.data();
}

extern "C" size_t ResourceMgr_GetArraySizeByName(const char* path) {
    auto res = std::static_pointer_cast<SOH::Array>(GetResourceByName(path));

    return res->Scalars.size();
}

// Loads U8 data from an Array resource into an externally managed buffer, or mallocs a new buffer
// if the passed in a nullptr. This malloced buffer must be freed by the caller.
extern "C" u8* ResourceMgr_LoadArrayByNameAsU8(const char* path, u8* buffer) {
    auto res = std::static_pointer_cast<SOH::Array>(GetResourceByName(path));

    if (buffer == nullptr) {
        buffer = (u8*)malloc(sizeof(u8) * res->Scalars.size());
    }

    for (size_t i = 0; i < res->Scalars.size(); i++) {
        buffer[i] = res->Scalars[i].u8;
    }

    return buffer;
}

// Loads Vec3s data from an Array resource.
// mallocs a new buffer that must be freed by the caller.
extern "C" char* ResourceMgr_LoadArrayByNameAsVec3s(const char* path) {
    auto res = std::static_pointer_cast<SOH::Array>(GetResourceByName(path));

    // if (res->CachedGameAsset != nullptr)
    //     return (char*)res->CachedGameAsset;
    // else
    // {
    Vec3s* data = (Vec3s*)malloc(sizeof(Vec3s) * res->Scalars.size());

    for (size_t i = 0; i < res->Scalars.size(); i += 3) {
        data[(i / 3)].x = res->Scalars[i + 0].s16;
        data[(i / 3)].y = res->Scalars[i + 1].s16;
        data[(i / 3)].z = res->Scalars[i + 2].s16;
    }

    // res->CachedGameAsset = data;

    return (char*)data;
    // }
}

extern "C" AnimatedMaterial* ResourceMgr_LoadAnimatedMatByName(const char* path) {
    return (AnimatedMaterial*)ResourceGetDataByName(path);
}

extern "C" CollisionHeader* ResourceMgr_LoadColByName(const char* path) {
    return (CollisionHeader*)ResourceGetDataByName(path);
}

extern "C" Vtx* ResourceMgr_LoadVtxByName(char* path) {
    return (Vtx*)ResourceGetDataByName(path);
}

extern "C" Mtx* ResourceMgr_LoadMtxByName(char* path) {
    return (Mtx*)ResourceGetDataByName(path);
}

// ComboShip: audio loads pinned to MM's own RM — the audio thread races active-RM swaps
// (ResourceManagerScope) made on other threads.
#ifdef COMBO_BUILD
extern "C" SequenceData* Combo_MM_AdultSequenceForPath(const char* path);
extern "C" SoundFont* Combo_MM_AdultFontForPath(const char* path);
#endif
extern "C" SequenceData ResourceMgr_LoadSeqByName(const char* path) {
#ifdef COMBO_BUILD
    if (auto* adult = Combo_MM_AdultSequenceForPath(path))
        return *adult;
#endif
#ifdef COMBO_BUILD
    if (auto* foreign = Combo_MMWarpAudioResource(path))
        return *static_cast<SequenceData*>(foreign);
#endif
    SequenceData* sequence = (SequenceData*)Ship::CrossRMRegistry::GetOrActive("mm")->GetResourceRawPointer(path);
    return *sequence;
}
extern "C" SequenceData* ResourceMgr_LoadSeqPtrByName(const char* path) {
#ifdef COMBO_BUILD
    if (auto* adult = Combo_MM_AdultSequenceForPath(path))
        return adult;
#endif
#ifdef COMBO_BUILD
    if (auto* foreign = Combo_MMWarpAudioResource(path))
        return static_cast<SequenceData*>(foreign);
#endif
    SequenceData* sequence = (SequenceData*)Ship::CrossRMRegistry::GetOrActive("mm")->GetResourceRawPointer(path);
    return sequence;
}
extern "C" KeyFrameSkeleton* ResourceMgr_LoadKeyFrameSkelByName(const char* path) {
    return (KeyFrameSkeleton*)ResourceGetDataByName(path);
}

extern "C" KeyFrameAnimation* ResourceMgr_LoadKeyFrameAnimByName(const char* path) {
    return (KeyFrameAnimation*)ResourceGetDataByName(path);
}
// std::map<std::string, SoundFontSample*> cachedCustomSFs;
#if 0
extern "C" SoundFontSample* ReadCustomSample(const char* path) {
    return nullptr;
    /*
        if (!ExtensionCache.contains(path))
            return nullptr;

        ExtensionEntry entry = ExtensionCache[path];

        auto sampleRaw = Ship::Context::GetRawInstance()->GetResourceManager()->LoadFile(entry.path);
        uint32_t* strem = (uint32_t*)sampleRaw->Buffer.get();
        uint8_t* strem2 = (uint8_t*)strem;

        SoundFontSample* sampleC = new SoundFontSample;

        if (entry.ext == "wav") {
            drwav_uint32 channels;
            drwav_uint32 sampleRate;
            drwav_uint64 totalPcm;
            drmp3_int16* pcmData =
                drwav_open_memory_and_read_pcm_frames_s16(strem2, sampleRaw->BufferSize, &channels, &sampleRate,
       &totalPcm, NULL); sampleC->size = totalPcm; sampleC->sampleAddr = (uint8_t*)pcmData; sampleC->codec = CODEC_S16;

            sampleC->loop = new AdpcmLoop;
            sampleC->loop->start = 0;
            sampleC->loop->end = sampleC->size - 1;
            sampleC->loop->count = 0;
            sampleC->sampleRateMagicValue = 'RIFF';
            sampleC->sampleRate = sampleRate;

            cachedCustomSFs[path] = sampleC;
            return sampleC;
        } else if (entry.ext == "mp3") {
            drmp3_config mp3Info;
            drmp3_uint64 totalPcm;
            drmp3_int16* pcmData =
                drmp3_open_memory_and_read_pcm_frames_s16(strem2, sampleRaw->BufferSize, &mp3Info, &totalPcm, NULL);

            sampleC->size = totalPcm * mp3Info.channels * sizeof(short);
            sampleC->sampleAddr = (uint8_t*)pcmData;
            sampleC->codec = CODEC_S16;

            sampleC->loop = new AdpcmLoop;
            sampleC->loop->start = 0;
            sampleC->loop->end = sampleC->size;
            sampleC->loop->count = 0;
            sampleC->sampleRateMagicValue = 'RIFF';
            sampleC->sampleRate = mp3Info.sampleRate;

            cachedCustomSFs[path] = sampleC;
            return sampleC;
        }

        return nullptr;
    */
}

extern "C" SoundFontSample* ResourceMgr_LoadAudioSample(const char* path) {
    return (SoundFontSample*)ResourceGetDataByName(path);
}
#endif

extern "C" SoundFont* ResourceMgr_LoadAudioSoundFontByName(const char* path) {
#ifdef COMBO_BUILD
    if (auto* adult = Combo_MM_AdultFontForPath(path))
        return adult;
#endif
#ifdef COMBO_BUILD
    if (auto* foreign = Combo_MMWarpAudioResource(path))
        return static_cast<SoundFont*>(foreign);
#endif
    return (SoundFont*)Ship::CrossRMRegistry::GetOrActive("mm")->GetResourceRawPointer(path);
}

extern "C" SoundFont* ResourceMgr_LoadAudioSoundFontByCRC(uint64_t crc) {
    return (SoundFont*)Ship::CrossRMRegistry::GetOrActive("mm")->GetResourceRawPointer(crc);
}

extern "C" int ResourceMgr_OTRSigCheck(char* imgData) {
    uintptr_t i = (uintptr_t)(imgData);

    // if (i == 0xD9000000 || i == 0xE7000000 || (i & 1) == 1)
    if ((i & 1) == 1)
        return 0;

    // if ((i & 0xFF000000) != 0xAB000000 && (i & 0xFF000000) != 0xCD000000 && i != 0) {
    if (i != 0) {
        if (imgData[0] == '_' && imgData[1] == '_' && imgData[2] == 'O' && imgData[3] == 'T' && imgData[4] == 'R' &&
            imgData[5] == '_' && imgData[6] == '_')
            return 1;
    }

    return 0;
}

// Load animation with explicit alt asset path checking.
// When Alt Assets is OFF: use original path directly (O2R or vanilla)
// When Alt Assets is ON: try alt/ prefix first, fall back to regular path if not found or invalid
extern "C" AnimationHeaderCommon* ResourceMgr_LoadAnimByName(const char* path) {
    bool isAlt = ResourceMgr_IsAltAssetsEnabled();

    if (isAlt) {
        std::string pathStr = std::string(path);
        static const std::string sOtr = "__OTR__";

        if (pathStr.starts_with(sOtr)) {
            pathStr = pathStr.substr(sOtr.length());
        }

        // Try alt/ first
        pathStr = Ship::IResource::gAltAssetPrefix + pathStr;
        AnimationHeaderCommon* animHeader = (AnimationHeaderCommon*)ResourceGetDataByName(pathStr.c_str());

        // If alt loaded successfully, verify it has valid data
        if (animHeader != NULL) {
            // Check for valid frame count (> 0)
            if (animHeader->frameCount > 0) {
                // For Normal animations: check frameData (comes after frameCount in AnimationHeader)
                // For Link animations: check segment (comes after frameCount in LinkAnimationHeader)
                // We check both to be safe - if either is valid, the animation is usable
                AnimationHeader* normalAnim = (AnimationHeader*)animHeader;
                PlayerAnimationHeader* playerAnim = (PlayerAnimationHeader*)animHeader;

                // Valid if Normal animation has frameData OR Link animation has segment
                if (normalAnim->frameData != NULL || playerAnim->segmentVoid != NULL) {
                    return animHeader;
                }
            }
            // Alt loaded but is invalid (broken), fall through to original path
        }

        // Fall back to original path
        return (AnimationHeaderCommon*)ResourceGetDataByName(path);
    }

    // Alt OFF: use original path directly
    return (AnimationHeaderCommon*)ResourceGetDataByName(path);
}

extern "C" SkeletonHeader* ResourceMgr_LoadSkeletonByName(const char* path, SkelAnime* skelAnime) {
    std::string pathStr = std::string(path);
    static const std::string sOtr = "__OTR__";

    if (pathStr.starts_with(sOtr)) {
        pathStr = pathStr.substr(sOtr.length());
    }

    bool isAlt = ResourceMgr_IsAltAssetsEnabled();

    if (isAlt) {
        pathStr = Ship::IResource::gAltAssetPrefix + pathStr;
    }

    SkeletonHeader* skelHeader = (SkeletonHeader*)ResourceGetDataByName(pathStr.c_str());

    // If there isn't an alternate model, load the regular one
    if (isAlt && skelHeader == NULL) {
        skelHeader = (SkeletonHeader*)ResourceGetDataByName(path);
    }

    // This function is only called when a skeleton is initialized.
    // Therefore we can take this opportunity to take note of the Skeleton that is created...
    if (skelAnime != nullptr) {
        auto stringPath = std::string(path);
        SOH::SkeletonPatcher::RegisterSkeleton(stringPath, skelAnime);
    }

    return skelHeader;
}

extern "C" void ResourceMgr_UnregisterSkeleton(SkelAnime* skelAnime) {
    if (skelAnime != nullptr)
        SOH::SkeletonPatcher::UnregisterSkeleton(skelAnime);
}

extern "C" void ResourceMgr_ClearSkeletons() {
    SOH::SkeletonPatcher::ClearSkeletons();
}

extern "C" s32* ResourceMgr_LoadCSByName(const char* path) {
    return (s32*)ResourceGetDataByName(path);
}

ImFont* OTRGlobals::CreateFontWithSize(float size, std::string fontPath) {
    auto mImGuiIo = &ImGui::GetIO();
    ImFont* font;
    if (fontPath == "") {
        ImFontConfig fontCfg = ImFontConfig();
        fontCfg.OversampleH = fontCfg.OversampleV = 1;
        fontCfg.PixelSnapH = true;
        fontCfg.SizePixels = size;
        font = mImGuiIo->Fonts->AddFontDefault(&fontCfg);
    } else {
        auto initData = std::make_shared<Ship::ResourceInitData>();
        initData->Format = RESOURCE_FORMAT_BINARY;
        initData->Type = static_cast<uint32_t>(RESOURCE_TYPE_FONT);
        initData->ResourceVersion = 0;
        initData->Path = fontPath;
        std::shared_ptr<Ship::Font> fontData = std::static_pointer_cast<Ship::Font>(
            Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(fontPath, false, initData));
        ImFontConfig fontConf;
        fontConf.FontDataOwnedByAtlas = false;
        font = mImGuiIo->Fonts->AddFontFromMemoryTTF(fontData->Data, fontData->DataSize, size, &fontConf, nullptr);
    }
    // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
    float iconFontSize = size * 2.0f / 3.0f;
    static const ImWchar sIconsRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig iconsConfig;
    iconsConfig.MergeMode = true;
    iconsConfig.PixelSnapH = true;
    iconsConfig.GlyphMinAdvanceX = iconFontSize;
    mImGuiIo->Fonts->AddFontFromMemoryCompressedBase85TTF(fontawesome_compressed_data_base85, iconFontSize,
                                                          &iconsConfig, sIconsRanges);
    return font;
}

std::filesystem::path GetSaveFile(std::shared_ptr<Ship::Config> Conf) {
    const std::string fileName =
        Conf->GetString("Game.SaveName", Ship::Context::GetPathRelativeToAppDirectory("oot_save.sav"));
    std::filesystem::path saveFile = std::filesystem::absolute(fileName);

    if (!exists(saveFile.parent_path())) {
        create_directories(saveFile.parent_path());
    }

    return saveFile;
}

std::filesystem::path GetSaveFile() {
    const std::shared_ptr<Ship::Config> pConf = OTRGlobals::Instance->context->GetConfig();

    return GetSaveFile(pConf);
}

void OTRGlobals::CheckSaveFile(size_t sramSize) const {
    const std::shared_ptr<Ship::Config> pConf = Instance->context->GetConfig();

    std::filesystem::path savePath = GetSaveFile(pConf);
    std::fstream saveFile(savePath, std::fstream::in | std::fstream::out | std::fstream::binary);
    if (saveFile.fail()) {
        saveFile.open(savePath, std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::app);
        for (int i = 0; i < sramSize; ++i) {
            saveFile.write("\0", 1);
        }
    }
    saveFile.close();
}

// extern "C" void Ctx_ReadSaveFile(uintptr_t addr, void* dramAddr, size_t size) {
//     SaveManager::ReadSaveFile(GetSaveFile(), addr, dramAddr, size);
// }

// extern "C" void Ctx_WriteSaveFile(uintptr_t addr, void* dramAddr, size_t size) {
//     SaveManager::WriteSaveFile(GetSaveFile(), addr, dramAddr, size);
// }

std::wstring StringToU16(const std::string& s) {
    std::vector<unsigned long> result;
    size_t i = 0;
    while (i < s.size()) {
        unsigned long uni;
        size_t nbytes;
        bool error = false;
        unsigned char c = s[i++];
        if (c < 0x80) { // ascii
            uni = c;
            nbytes = 0;
        } else if (c <= 0xBF) { // assuming kata/hiragana delimiter
            nbytes = 0;
            uni = '\1';
        } else if (c <= 0xDF) {
            uni = c & 0x1F;
            nbytes = 1;
        } else if (c <= 0xEF) {
            uni = c & 0x0F;
            nbytes = 2;
        } else if (c <= 0xF7) {
            uni = c & 0x07;
            nbytes = 3;
        }
        for (size_t j = 0; j < nbytes; ++j) {
            unsigned char c = s[i++];
            uni <<= 6;
            uni += c & 0x3F;
        }
        if (uni != '\1')
            result.push_back(uni);
    }
    std::wstring utf16;
    for (size_t i = 0; i < result.size(); ++i) {
        unsigned long uni = result[i];
        if (uni <= 0xFFFF) {
            utf16 += (wchar_t)uni;
        } else {
            uni -= 0x10000;
            utf16 += (wchar_t)((uni >> 10) + 0xD800);
            utf16 += (wchar_t)((uni & 0x3FF) + 0xDC00);
        }
    }
    return utf16;
}

int CopyStringToCharBuffer(const std::string& inputStr, char* buffer, const int maxBufferSize) {
    if (!inputStr.empty()) {
        // Prevent potential horrible overflow due to implicit conversion of maxBufferSize to an unsigned. Prevents
        // negatives.
        memset(buffer, 0, std::max<int>(0, maxBufferSize));
        // Gaurentee that this value will be greater than 0, regardless of passed variables.
        const int copiedCharLen = std::min<int>(std::max<int>(0, maxBufferSize - 1), inputStr.length());
        memcpy(buffer, inputStr.c_str(), copiedCharLen);
        return copiedCharLen;
    }

    return 0;
}

extern "C" void OTRGfxPrint(const char* str, void* printer, void (*printImpl)(void*, char)) {
    const std::vector<uint32_t> hira1 = {
        u'を', u'ぁ', u'ぃ', u'ぅ', u'ぇ', u'ぉ', u'ゃ', u'ゅ', u'ょ', u'っ', u'-',  u'あ', u'い',
        u'う', u'え', u'お', u'か', u'き', u'く', u'け', u'こ', u'さ', u'し', u'す', u'せ', u'そ',
    };

    const std::vector<uint32_t> hira2 = {
        u'た', u'ち', u'つ', u'て', u'と', u'な', u'に', u'ぬ', u'ね', u'の', u'は', u'ひ', u'ふ', u'へ', u'ほ', u'ま',
        u'み', u'む', u'め', u'も', u'や', u'ゆ', u'よ', u'ら', u'り', u'る', u'れ', u'ろ', u'わ', u'ん', u'゛', u'゜',
    };

    std::wstring wstr = StringToU16(str);

    for (const auto& c : wstr) {
        if (c < 0x80) {
            printImpl(printer, c);
        } else if (c >= u'｡' && c <= u'ﾟ') { // katakana
            printImpl(printer, c - 0xFEC0);
        } else {
            auto it = std::find(hira1.begin(), hira1.end(), c);
            if (it != hira1.end()) { // hiragana block 1
                printImpl(printer, 0x88 + std::distance(hira1.begin(), it));
            }

            auto it2 = std::find(hira2.begin(), hira2.end(), c);
            if (it2 != hira2.end()) { // hiragana block 2
                printImpl(printer, 0xe0 + std::distance(hira2.begin(), it2));
            }
        }
    }
}

// Gets the width of the main ImGui window
extern "C" uint32_t OTRGetCurrentWidth() {
    return OTRGlobals::Instance->context->GetWindow()->GetWidth();
}

// Gets the height of the main ImGui window
extern "C" uint32_t OTRGetCurrentHeight() {
    return OTRGlobals::Instance->context->GetWindow()->GetHeight();
}

Color_RGB8 GetColorForControllerLED() {
#if 0
    auto brightness = CVarGetFloat("gLedBrightness", 1.0f) / 1.0f;
    Color_RGB8 color = { 0, 0, 0 };
    if (brightness > 0.0f) {
        LEDColorSource source =
            static_cast<LEDColorSource>(CVarGetInteger("gLedColorSource", LED_SOURCE_TUNIC_ORIGINAL));
        bool criticalOverride = CVarGetInteger("gLedCriticalOverride", 1);
        if (gPlayState && (source == LED_SOURCE_TUNIC_ORIGINAL || source == LED_SOURCE_TUNIC_COSMETICS)) {
            switch (CUR_EQUIP_VALUE(EQUIP_TUNIC) - 1) {
                case PLAYER_TUNIC_KOKIRI:
                    color = source == LED_SOURCE_TUNIC_COSMETICS
                                ? CVarGetColor24("gCosmetics.Link_KokiriTunic.Value", kokiriColor)
                                : kokiriColor;
                    break;
                case PLAYER_TUNIC_GORON:
                    color = source == LED_SOURCE_TUNIC_COSMETICS
                                ? CVarGetColor24("gCosmetics.Link_GoronTunic.Value", goronColor)
                                : goronColor;
                    break;
                case PLAYER_TUNIC_ZORA:
                    color = source == LED_SOURCE_TUNIC_COSMETICS
                                ? CVarGetColor24("gCosmetics.Link_ZoraTunic.Value", zoraColor)
                                : zoraColor;
                    break;
            }
        }
        if (source == LED_SOURCE_CUSTOM) {
            color = CVarGetColor24("gLedPort1Color", { 255, 255, 255 });
        }
        if (criticalOverride || source == LED_SOURCE_HEALTH) {
            if (HealthMeter_IsCritical()) {
                color = { 0xFF, 0, 0 };
            } else if (source == LED_SOURCE_HEALTH) {
                if (gSaveContext.health / gSaveContext.healthCapacity <= 0.4f) {
                    color = { 0xFF, 0xFF, 0 };
                } else {
                    color = { 0, 0xFF, 0 };
                }
            }
        }
        color.r = color.r * brightness;
        color.g = color.g * brightness;
        color.b = color.b * brightness;
    }
#endif
    return { 0, 0, 0 };
}

extern "C" void OTRControllerCallback(uint8_t rumble) {
    // We call this every tick, SDL accounts for this use and prevents driver spam
    // https://github.com/libsdl-org/SDL/blob/f17058b562c8a1090c0c996b42982721ace90903/src/joystick/SDL_joystick.c#L1114-L1144
    Ship::Context::GetRawInstance()->GetControlDeck()->GetControllerByPort(0)->GetLED()->SetLEDColor(
        GetColorForControllerLED());

    static std::shared_ptr<BenInputEditorWindow> controllerConfigWindow = nullptr;
    if (controllerConfigWindow == nullptr) {
        controllerConfigWindow = std::dynamic_pointer_cast<BenInputEditorWindow>(
            Ship::Context::GetRawInstance()->GetWindow()->GetGui()->GetGuiWindow("2S2H Input Editor"));
        // note: the current implementation may not be desired in LUS, as "true" rumble support
        //    using osMotor calls is planned: https://github.com/Kenix3/libultraship/issues/9
    }
    if (controllerConfigWindow->TestingRumble()) {
        return;
    }

    // TODO: other ports?
    if (rumble) {
        Ship::Context::GetRawInstance()->GetControlDeck()->GetControllerByPort(0)->GetRumble()->StartRumble();
    } else {
        Ship::Context::GetRawInstance()->GetControlDeck()->GetControllerByPort(0)->GetRumble()->StopRumble();
    }
}

extern "C" float OTRGetAspectRatio() {
    return Ship::Context::GetRawInstance()->GetWindow()->GetAspectRatio();
}

extern "C" float OTRGetDimensionFromLeftEdge(float v) {
    auto fastWnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow());
    auto intP = fastWnd->GetInterpreterWeak().lock();

    if (!intP) {
        assert(false && "Lost reference to Fast::Interpreter");
        return v;
    }

    auto gfx_native_dimensions = intP->mNativeDimensions;

    return (gfx_native_dimensions.width / 2 - gfx_native_dimensions.height / 2 * OTRGetAspectRatio() + (v));
}

extern "C" float OTRGetDimensionFromRightEdge(float v) {
    auto fastWnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow());
    auto intP = fastWnd->GetInterpreterWeak().lock();

    if (!intP) {
        assert(false && "Lost reference to Fast::Interpreter");
        return v;
    }

    auto gfx_native_dimensions = intP->mNativeDimensions;

    return (gfx_native_dimensions.width / 2 + gfx_native_dimensions.height / 2 * OTRGetAspectRatio() -
            (gfx_native_dimensions.width - v));
}

// Gets the width of the current render target area
extern "C" uint32_t OTRGetGameRenderWidth() {
    auto fastWnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow());
    auto intP = fastWnd->GetInterpreterWeak().lock();

    if (!intP) {
        assert(false && "Lost reference to Fast::Interpreter");
        return 320;
    }

    uint32_t height, width;
    intP->GetCurDimensions(&width, &height);

    return width;
}

// Gets the height of the current render target area
extern "C" uint32_t OTRGetGameRenderHeight() {
    auto fastWnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow());
    auto intP = fastWnd->GetInterpreterWeak().lock();

    if (!intP) {
        assert(false && "Lost reference to Fast::Interpreter");
        return 240;
    }

    uint32_t height, width;
    intP->GetCurDimensions(&width, &height);

    return height;
}

f32 floorf(f32 x);
f32 ceilf(f32 x);

extern "C" int16_t OTRGetRectDimensionFromLeftEdge(float v) {
    return ((int)floorf(OTRGetDimensionFromLeftEdge(v)));
}

extern "C" int16_t OTRGetRectDimensionFromRightEdge(float v) {
    return ((int)ceilf(OTRGetDimensionFromRightEdge(v)));
}

// Takes a HUD coordinate(320x240) and converts it to the game window pixel coordinates (any size, any aspect ratio)
// Though the HUD uses a 320x240 coordinates system, the size of the HUD box is scaled up to match the window height
// If the game window is 4:3, this will return the same value.

/*
Example, if the game window is 16:9 at twice the resolution of the HUD:
Calling with X (0,0) will return 8
Calling with Y (1,1) will return 10

. . . x _ _ _ _ _ _ _ . . .
. . . _ y _ _ _ _ _ _ . . .
. . . _ _ _ HUD _ _ _ . . .
. . . _ _ _ _ _ _ _ _ . . .
. . . _ _ _ _ _ _ _ _ . . .
*/
extern "C" int32_t OTRConvertHUDXToScreenX(int32_t v) {
    auto fastWnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow());
    auto intP = fastWnd->GetInterpreterWeak().lock();

    if (!intP) {
        assert(false && "Lost reference to Fast::Interpreter");
        return v;
    }

    uint32_t gameHeight, gameWidth;
    float gameAspectRatio = fastWnd->GetAspectRatio();
    intP->GetCurDimensions(&gameWidth, &gameHeight);
    float hudAspectRatio = 4.0f / 3.0f;
    int32_t hudHeight = gameHeight;
    int32_t hudWidth = hudHeight * hudAspectRatio;

    float hudScreenRatio = (hudWidth / 320.0f);
    float hudCoord = v * hudScreenRatio;
    float gameOffset = (int32_t(gameWidth) - hudWidth) / 2;
    float gameCoord = hudCoord + gameOffset;
    float gameScreenRatio = (320.0f / gameWidth);
    float screenScaledCoord = gameCoord * gameScreenRatio;
    int32_t screenScaledCoordInt = screenScaledCoord;

    return screenScaledCoordInt;
}

extern "C" void Gfx_RegisterBlendedTexture(const char* name, u8* mask, u8* replacement) {
    if (auto intP = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow())
                        ->GetInterpreterWeak()
                        .lock()) {
        intP->RegisterBlendedTexture(name, mask, replacement);
    } else {
        assert(false && "Lost reference to Fast::Interpreter");
    }
}

extern "C" void Gfx_UnregisterBlendedTexture(const char* name) {
    if (auto intP = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow())
                        ->GetInterpreterWeak()
                        .lock()) {
        intP->UnregisterBlendedTexture(name);
    } else {
        assert(false && "Lost reference to Fast::Interpreter");
    }
}

extern "C" void Gfx_TextureCacheDelete(const uint8_t* texAddr) {
    char* imgName = (char*)texAddr;

    if (texAddr == nullptr) {
        return;
    }

    if (ResourceMgr_OTRSigCheck(imgName)) {
        texAddr = (const uint8_t*)ResourceGetDataByName(imgName);
    }

    if (auto intP = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetRawInstance()->GetWindow())
                        ->GetInterpreterWeak()
                        .lock()) {
        intP->TextureCacheDelete(texAddr);
    } else {
        assert(false && "Lost reference to Fast::Interpreter");
    }
}

extern "C" int AudioPlayer_Buffered(void) {
    return AudioPlayerBuffered();
}

extern "C" int AudioPlayer_GetDesiredBuffered(void) {
    return AudioPlayerGetDesiredBuffered();
}

extern "C" void AudioPlayer_Play(const uint8_t* buf, uint32_t len) {
    AudioPlayerPlayFrame(buf, len);
}

extern "C" int Controller_ShouldRumble(size_t slot) {
    // don't rumble if we don't have rumble mappings
    if (Ship::Context::GetRawInstance()
            ->GetControlDeck()
            ->GetControllerByPort(static_cast<uint8_t>(slot))
            ->GetRumble()
            ->GetAllRumbleMappings()
            .empty()) {
        return 0;
    }

    // don't rumble if we don't have connected gamepads
    if (Ship::Context::GetRawInstance()
            ->GetControlDeck()
            ->GetConnectedPhysicalDeviceManager()
            ->GetConnectedSDLGamepadsForPort(slot)
            .empty()) {
        return 0;
    }

    // rumble
    return 1;
}

// ============================================================
// ComboShip exports — 2ship.dll side
// ============================================================

static std::unique_ptr<Ship::ArchiveManager> gMMArchiveManager;

// Opens mm.o2r + 2ship.o2r into a MM-private ArchiveManager (no context, no window).
// This is the "dormant" MM state: archives open, no game loop running.
extern "C" COMBO_EXPORT void MM_InitArchives() {
    std::vector<std::string> archivePaths;

    std::string mmPathO2R = Ship::Context::LocateFileAcrossAppDirs("mm.o2r", appShortName);
    std::string mmPathZIP = Ship::Context::LocateFileAcrossAppDirs("mm.zip", appShortName);
    if (std::filesystem::exists(mmPathO2R)) {
        archivePaths.push_back(mmPathO2R);
    } else if (std::filesystem::exists(mmPathZIP)) {
        archivePaths.push_back(mmPathZIP);
    } else {
        std::string mmPathOtr = Ship::Context::LocateFileAcrossAppDirs("mm.otr", appShortName);
        if (std::filesystem::exists(mmPathOtr)) {
            archivePaths.push_back(mmPathOtr);
        }
    }

    std::string shipO2R = Ship::Context::GetPathRelativeToAppBundle("2ship.o2r");
    if (std::filesystem::exists(shipO2R)) {
        archivePaths.push_back(shipO2R);
    }

    const auto adultLocal = Ship::Context::LocateFileAcrossAppDirs("combo_adult_mm.local.o2r", appShortName);
    if (std::filesystem::exists(adultLocal))
        archivePaths.push_back(adultLocal);

    if (!archivePaths.empty()) {
        printf("[2ship] MM_InitArchives: opening %zu archive(s):\n", archivePaths.size());
        for (const auto& p : archivePaths) {
            printf("[2ship]   %s\n", p.c_str());
        }
        gMMArchiveManager = std::make_unique<Ship::ArchiveManager>();
        gMMArchiveManager->Init(archivePaths);
        printf("[2ship] MM archives loaded (dormant).\n");
    } else {
        printf("[2ship] MM_InitArchives: no archives found — MM will remain unloaded.\n");
    }
}

// Read-only foreign presentation assets. This RM is isolated from both running
// engines and does not replace their archives, factories, save state or audio.
extern "C" COMBO_EXPORT int MM_EnsureWarpResources() {
    if (Ship::CrossRMRegistry::Get("mm-warp"))
        return 1;
    auto path = Ship::Context::LocateFileAcrossAppDirs("mm.o2r", appShortName);
    if (!std::filesystem::exists(path))
        path = Ship::Context::LocateFileAcrossAppDirs("mm.otr", appShortName);
    if (!std::filesystem::exists(path))
        path = Ship::Context::LocateFileAcrossAppDirs("mm.zip", appShortName);
    if (!std::filesystem::exists(path))
        return 0;
    auto rm = std::make_shared<Ship::ResourceManager>();
    rm->Init({ path }, {}, 1);
    auto loader = rm->GetResourceLoader();
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryTextureV0>(), RESOURCE_FORMAT_BINARY,
                                    "Texture", static_cast<uint32_t>(Fast::ResourceType::Texture), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryTextureV1>(), RESOURCE_FORMAT_BINARY,
                                    "Texture", static_cast<uint32_t>(Fast::ResourceType::Texture), 1);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryVertexV0>(), RESOURCE_FORMAT_BINARY,
                                    "Vertex", static_cast<uint32_t>(Fast::ResourceType::Vertex), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryDisplayListV0>(),
                                    RESOURCE_FORMAT_BINARY, "DisplayList",
                                    static_cast<uint32_t>(Fast::ResourceType::DisplayList), 0);
    loader->RegisterResourceFactory(std::make_shared<Ship::ResourceFactoryBinaryBlobV0>(), RESOURCE_FORMAT_BINARY,
                                    "Blob", static_cast<uint32_t>(Ship::ResourceType::Blob), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryArrayV0>(), RESOURCE_FORMAT_BINARY,
                                    "Array", static_cast<uint32_t>(SOH::ResourceType::SOH_Array), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryTextureAnimationV0>(),
                                    RESOURCE_FORMAT_BINARY, "TextureAnimation",
                                    static_cast<uint32_t>(SOH::ResourceType::TSH_TexAnim), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryKeyFrameAnim>(), RESOURCE_FORMAT_BINARY,
                                    "KeyFrameAnim", static_cast<uint32_t>(SOH::ResourceType::TSH_CKeyFrameAnim), 0);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryKeyFrameSkel>(), RESOURCE_FORMAT_BINARY,
                                    "KeyFrameSkel", static_cast<uint32_t>(SOH::ResourceType::TSH_CKeyFrameSkel), 0);
    Ship::CrossRMRegistry::Register("mm-warp", rm);
    return 1;
}
extern "C" COMBO_EXPORT void* MM_LoadWarpResource(const char* path) {
    if (!path || !MM_EnsureWarpResources())
        return nullptr;
    Ship::OwnRMScope scope("mm-warp"); // Keyframe factories load their limb DLs recursively.
    return Ship::CrossRMRegistry::Get("mm-warp")->GetResourceRawPointer(path);
}
extern "C" COMBO_EXPORT int MM_WarpVertexCount(const char* path) {
    if (!path || !MM_EnsureWarpResources())
        return 0;
    Ship::OwnRMScope scope("mm-warp");
    auto resource = Ship::CrossRMRegistry::Get("mm-warp")->LoadResource(path);
    return resource ? static_cast<int>(resource->GetPointerSize() / sizeof(Vtx)) : 0;
}
extern "C" COMBO_EXPORT uint32_t MM_GetWarpMapClouds() {
    return gSaveContext.save.saveInfo.worldMapCloudVisibility;
}
extern "C" COMBO_EXPORT void MM_DropWarpResources() {
    Ship::CrossRMRegistry::Unregister("mm-warp");
}

// ComboShip: -1 = normal MM boot; >= 0 = game-switch: skip title/file-select and load this slot.
// extern "C" so title_setup.c (a C file) can link to it without name mangling.
extern "C" int gComboStartFileNum = -1;

// ComboShip (#89): how MM is being entered. 0 = through the Happy Mask Shop portal, which always
// arrives in South Clock Town (the fixed portal exit). 1 = resuming a slot that was last saved in MM,
// which is a real save load and so honors Remember Save Location. Set by the launcher before each
// MM_RunGame/MM_ResumeGame; read by title_setup.c.
extern "C" int gComboEntryIsResume = 0;
extern "C" COMBO_EXPORT void MM_SetComboEntryIsResume(int isResume) {
    gComboEntryIsResume = isResume ? 1 : 0;
    Combo_ClearReturnRequests(); // a stale request would quit the session we're about to start
}

// ComboShip (#83): adopt OOT's targeting/audio. MM normally reads these from global.json, which combo
// never writes (its file select is never reached), leaving SaveContext_Init's defaults — so
// Z-targeting silently reverted to Switch. Queries soh.dll; no-op if the export is missing.
extern "C" void Combo_AdoptOOTGlobalOptions(void) {
    static void (*sFn)(int*, int*) = nullptr;
    static bool sTried = false;
    if (!sTried) {
        sTried = true;
        sFn = (void (*)(int*, int*))Combo_ResolveSym("soh", "SOH_GetGlobalOptions");
    }
    if (!sFn)
        return;
    int zTarget = 0, audio = 0;
    sFn(&zTarget, &audio);
    gSaveContext.options.zTargetSetting = (u8)zTarget;
    gSaveContext.options.audioSetting = (u8)audio;
}

// C-callable wrapper used by title_setup.c (which is a C file) to load a MM save from disk.
// 0 = loaded a usable rando save; negative = nothing usable (SaveManager codes, plus -6 = loaded but not
// a rando save). The caller REBUILDS on a negative code — it never refuses entry.
extern "C" int Combo_LoadMMSaveFile(int mmFileNum) {
    int result = SaveManager_LoadSaveFile(mmFileNum);
    if (result != 0) {
        return result;
    }
    // No vanilla mode in ComboShip: a non-rando save means the slot was created wrong, and every
    // IS_RANDO hook stays unregistered (COND_HOOK tests the condition once, at OnSaveLoad).
    if (gSaveContext.save.shipSaveInfo.saveType != SAVETYPE_RANDO) {
        SPDLOG_ERROR("[ComboShip] MM save file{} is not SAVETYPE_RANDO — rebuilding a baseline", mmFileNum);
        // The load above already set fileNum to this slot; re-mark "no save" or it reads as resident.
        SaveManager_MarkNoSaveLoaded();
        return -6;
    }
    return 0;
}

extern "C" void MM_RunMain(void);

// Full MM initialization + game loop, entered after OOT has exited.
// fileNum is the OOT 0-indexed slot; we map it to the same MM slot.
extern "C" COMBO_EXPORT void MM_RunGame(int fileNum) {
    gComboStartFileNum = fileNum;
    MM_RunMain();
}

// ComboShip: read by main.c's MM_RunMain to skip the blocking game loop. Set only during MM_BootForCombo.
extern "C" int gComboBootOnly = 0;

// ComboShip: eagerly boot MM once at OOT startup (after SOH_Init) so the cross-world rando oracle
// queries a real, fully-initialized MM (region graph, GameInteractor, AudioCollection, RM) instead
// of a fragile headless fake. Reuses OOT's shared Context (sComboTransitionActive) and runs
// MM_RunMain's full init while skipping its game loop (gComboBootOnly). The caller brackets this
// with SOH_PrepareForTransition / MM_PrepareForTransition + SOH_ResumeForeground to hand the
// foreground back to OOT.
extern "C" COMBO_EXPORT void MM_BootForCombo(void) {
    gComboStartFileNum = -1;       // boot only — no save load / Play jump
    sComboTransitionActive = true; // OTRGlobals ctor reuses OOT's Context + creates MM's own RM
    gComboBootOnly = 1;
    MM_RunMain(); // full init; main.c skips Graph_ThreadEntry due to gComboBootOnly
    gComboBootOnly = 0;
    // Setup_InitImpl never runs on this boot-only path, so gSaveContext's BSS zero-state would
    // otherwise read as a loaded slot 1. Stamp it "no save" until a real load fills a slot.
    SaveManager_MarkNoSaveLoaded();
}

// ComboShip: headless rando-only MM init — builds ONLY the rando region graph via the "RANDO_LOGIC"
// ShipInit path (no window/RM/audio/GUI), unlike MM_BootForCombo's full boot. StaticData maps are
// populated at DLL load; CVars come from the shared libultraship Context that SOH_InitRandoHeadless
// stands up, so call that first. Enough for the MM reachability oracle. See docs/UPSTREAM_MERGES.md.
extern "C" COMBO_EXPORT void MM_InitRandoHeadless(void) {
    static bool inited = false;
    if (inited)
        return;
    inited = true;
    ShipInit::Init("RANDO_LOGIC"); // OwnRMScope("mm") no-ops headlessly (no "mm" RM registered)
    Rando::StaticData::PopulateCheckNames();
}

#ifdef COMBO_BUILD
// Defined in mm/src/code/main.c: re-enters ONLY MM's game loop (no heap/thread re-init).
extern "C" void MM_RunGameLoop(void);
// Defined in mm/src/code/graph.c: resets the frame state machine so MM_RunGameLoop restarts the
// gamestate sequence from Setup instead of resuming into the destroyed post-handoff gamestate.
extern "C" void MM_ResetFrameLoopForResume(void);
// Defined in mm/src/code/main.c: resets MM's system arena so RunFrame's SysCfb_Init + SystemArena_Malloc
// have a fresh arena on resume.
extern "C" void MM_ResetSystemHeapForResume(void);
// Defined in mm/2s2h/z_message_OTR.cpp: rebuilds the message tables whose backing resources were
// freed by the forward (MM->OOT) transition's UnloadResources.
extern "C" void OTRMessage_ResetForResume(void);

// ComboShip: OOT->MM forward transition. Stop MM audio without destroying the shared
// context/window/resource-manager (OOT reuses them). Mirrors SOH_PrepareForTransition.
extern "C" COMBO_EXPORT void MM_PrepareForTransition(void) {
    SaveManager_ThreadPoolWait();
    OTRAudio_Exit();
    // NOTE: do NOT BenGui::Destroy() here. The Gui is a single shared libultraship instance; tearing
    // down its windows forces the resuming game's SetupGuiElements to RE-CREATE them, which re-registers
    // SaveManager load functions and asserts (AddLoadFunction: duplicate). The shared Gui persists across
    // transitions; each game's windows are set up once at its first boot.
    // Context, window, and resource manager are intentionally kept alive for OOT to reuse.
}

// ComboShip: OOT->MM return. Re-enter MM's game loop on the same shared context/window and jump
// straight to Play in South Clock Town for the given slot. Counterpart to OOT's SOH_ResumeGame.
extern "C" COMBO_EXPORT void MM_ResumeGame(int fileNum) {
    auto ctx = Ship::Context::GetRawInstance();
    ctx->GetLogger()->flush_on(spdlog::level::trace);
    SPDLOG_INFO("[ComboShip] MM_ResumeGame: begin (fileNum={})", fileNum);

    // 1. Re-activate MM's own ResourceManager (created at MM's first boot, kept resident the whole
    //    time OOT was running). Its archives + factories + resource cache never went away, so there's
    //    nothing to swap, unload, re-register, or reset (message tables/audio samples stay valid).
    ctx->SetResourceManager(sMMResourceManager);

    // 2. Restart MM's audio thread (MM_PrepareForTransition stopped it). Soundfonts/samples are still
    //    resident in MM's RM, so it resumes against valid data with no reload/heap reset.
    OTRAudio_Init(); // counterpart to OTRAudio_Exit() in MM_PrepareForTransition

    // 4. Re-sync this DLL's ImGui current-context (GImGui is a per-module static). Do NOT re-run
    //    BenGui::SetupGuiElements() — the shared Gui's windows persist from MM's first boot;
    //    re-creating them would re-register SaveManager load functions and assert.
    ImGui::SetCurrentContext(ctx->GetWindow()->GetGui()->GetImGuiContext());

    // ComboShip: re-activate MM's menu in the shared Gui's single menu slot (OOT swapped in its
    // SohMenu while it was active). BenMenu persists — BenGui::Destroy isn't called in combo.
    BenGui::ActivateMenu(); // ComboShip: no-op under COMBO_BUILD (comboui owns the menu)

    // 5. Re-arm the shared window so MM's `while (WindowIsRunning())` loop runs instead of returning
    //    immediately (OOT cleared mIsRunning when its loop exited).
    if (auto fast3d = std::dynamic_pointer_cast<Fast::Fast3dWindow>(ctx->GetWindow())) {
        fast3d->SetIsRunning(true);
    }

    // 6. Hand off to MM's boot path: title_setup.c's Setup_InitImpl loads the save, sets the South
    //    Clock Town entrance, and jumps straight to Play when gComboStartFileNum >= 0.
    gComboStartFileNum = fileNum;
    // Reset MM's system arena: RunFrame's state-0 path re-runs SysCfb_Init + SystemArena_Malloc, which
    // need a fresh arena (MM_RunGameLoop skips MM_RunMain's SystemHeap_Init). Without this,
    // SystemArena_Malloc returns a bad pointer and RunFrame crashes in memset.
    MM_ResetSystemHeapForResume();
    MM_ResetFrameLoopForResume();
    SPDLOG_INFO("[ComboShip] MM_ResumeGame: entering MM loop (gComboStartFileNum={})", gComboStartFileNum);

    // 7. Re-run MM's game loop (returns when the shared window's running flag is cleared again).
    MM_RunGameLoop();
    SPDLOG_INFO("[ComboShip] MM_ResumeGame: MM loop RETURNED");
}
#endif

#ifdef COMBO_BUILD
// ComboShip: everything to the matching #endif is combo-only (MM_*/Combo_MM_* exports + their
// statics). Guarded so an upstream merge can see the whole added region at a glance.

// ComboShip: bring the MM save for the given OOT slot (0-indexed) into MM's dormant gSaveContext, so
// the tracker peek shows real items before MM is visited this session. Same headless load path
// title_setup.c runs on resume (no gPlayState needed). Nonzero = nothing usable was loaded.
extern "C" COMBO_EXPORT int MM_LoadSaveForCombo(int fileNum) {
    return Combo_LoadMMSaveFile(fileNum + 1); // shares the saveType tripwire
}

static void Combo_MM_ApplyCheckPrices();

// ComboShip (#136): defined further down (MM_SetComboGoal). The save-building paths below force MM's
// Triforce options from these — combo owns the goal and MM's own CVars are hidden in combo builds.
extern "C" int gMMComboGoalHunt;
extern "C" int gMMComboGoalRequired;
extern "C" int gMMComboGoalPieces;

// The adult/gear options have no menu, but profile or seed JSON and raw CVars can still set them, and
// SaveManager refuses every save that carries them while the adult runtime is unavailable. Keep them at
// their defaults (0) wherever options are gathered, so the pool, the oracle and the new save agree.
extern "C" int Combo_MM_AdultResourcesReady(void);
static constexpr RandoOptionId kComboHiddenAdultMmOptions[] = {
    RO_MM_ADULT_MASK,  RO_MM_STARTING_AGE, RO_MM_CROSS_AGE,  RO_MM_BOOTS_IRON,
    RO_MM_BOOTS_HOVER, RO_MM_TUNIC_GORON,  RO_MM_TUNIC_ZORA,
};
static bool Combo_MM_IsHiddenAdultOption(RandoOptionId id) {
    return std::find(std::begin(kComboHiddenAdultMmOptions), std::end(kComboHiddenAdultMmOptions), id) !=
           std::end(kComboHiddenAdultMmOptions);
}
extern "C" void Combo_MM_ClearHiddenAdultOptions(u32* options) {
    if (Combo_MM_AdultResourcesReady())
        return;
    for (const auto id : kComboHiddenAdultMmOptions)
        options[id] = 0;
}

// Combo master seed for MM's RNG, mirroring OOT's SOH_SetComboRandoSeed so confined placement
// (PreplaceConfinedItems, via Ship_Random) is reproducible per seed.
static ComboRando::SpellRules sMMSeedSpellRules;
static ComboRando::FaroreRules sMMSeedFaroreRules;
static bool sMMSeedFaroreRulesBound = false;
static uint64_t sMMComboRandoSeed = 0;
static bool sMMComboRandoSeedSet = false;
extern "C" COMBO_EXPORT void MM_SetComboRandoSeed(uint64_t seed) {
    sMMComboRandoSeed = seed;
    sMMComboRandoSeedSet = true;
}

// ComboShip: create a RANDO MM save for the given OOT slot from a combo placement slice.
// placementJson is the "mm" object of the combined spoiler: { "<RC_name>": "<itemSpoilerName>", ... }.
// The combo layer owns placement, so we do NOT run MM's own generator. We build the playable baseline
// (South Clock Town, post-first-cycle Human Link), mark the save SAVETYPE_RANDO, and feed the placement
// through Rando::Spoiler::ApplyToSaveContext. Headless-safe: never calls GrantStartingItems / Item_Give
// (those need gPlayState). Returns 0 on success, nonzero if no placements applied — the save stays
// SAVETYPE_RANDO either way, since a vanilla one disables every IS_RANDO hook.
extern "C" COMBO_EXPORT int MM_InitRandoSaveFile(int fileNum, const char* placementJson,
                                                 const unsigned char* ootName8) {
    // Playable combo baseline first (Human Link, South Clock Town, ocarina/songs, etc.).
    SaveManager_InitNewSaveForSlot(fileNum + 1, ootName8);
    // Sram_InitNewSave (inside the call above) resets fileNum; restore it so SaveManager_SaveCurrentForCombo
    // re-writes the correct slot (it targets gSaveContext.fileNum + 1).
    gSaveContext.fileNum = (s16)fileNum;

    if (!placementJson || placementJson[0] == '\0') {
        SPDLOG_ERROR("[ComboShip] MM_InitRandoSaveFile: no placement for slot {}", fileNum);
        gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
        SaveManager_SaveCurrentForCombo();
        return -1;
    }

    // Mark the save as rando and zero the rando struct (mirrors Rando::MiscBehavior::OnFileCreate).
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    memset(&gSaveContext.save.shipSaveInfo.rando, 0, sizeof(gSaveContext.save.shipSaveInfo.rando));
    memcpy(&gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys, &gSaveContext.save.saveInfo.inventory.dungeonKeys,
           sizeof(gSaveContext.save.saveInfo.inventory.dungeonKeys));

    // ComboShip: the baseline (SaveManager_InitNewSaveForSlot) force-grants a playable mid-playthrough
    // kit (Ocarina, Deku Mask, Song of Time/Healing) — correct for a vanilla combo MM save, but for a
    // RANDO save these must be shuffled. Strip them so only the configured starting items (below) and
    // the always-eligible Deku Mask / Song of Healing checks provide items. The Human / South-Clock-Town
    // state stays so the save remains playable.
    INV_CONTENT(ITEM_OCARINA_OF_TIME) = ITEM_NONE;
    INV_CONTENT(ITEM_MASK_DEKU) = ITEM_NONE;
    gSaveContext.save.saveInfo.inventory.questItems &= ~((1 << QUEST_SONG_TIME) | (1 << QUEST_SONG_HEALING));
    gSaveContext.save.saveInfo.playerData.isMagicAcquired = false;
    // Also strip the vanilla sword & shield (mirrors native OnFileCreate).
    SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_NONE);
    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) = ITEM_NONE;
    SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_NONE);

    try {
        // ApplyToSaveContext consumes a full MM spoiler: it requires finalSeed, options, startingItems
        // and checks keys (startingItems and finalSeed throw if absent). For the no-logic native phase
        // we supply empty options/startingItems (defaults) and feed the combo placement as "checks".
        nlohmann::json spoiler;
        // finalSeed feeds runtime per-check junk/trap variety (ConvertItem.cpp) and the clock-shuffle
        // starting-time roll; use the combo master seed like native OnFileCreate.
        spoiler["finalSeed"] = (uint32_t)sMMComboRandoSeed;
        // ComboShip: persist the player's chosen MM options into the save (mirrors OnFileCreate) so MM
        // honors its toggles at runtime; an empty options object would make ApplyToSaveContext default
        // everything (the analog of OOT's SetAllToContext fix).
        nlohmann::json options = nlohmann::json::object();
        for (auto& [id, opt] : Rando::StaticData::Options) {
            options[opt.name] = (uint32_t)CVarGetInteger(opt.cvar, opt.defaultValue);
        }
        options[Rando::StaticData::Options[RO_SHARED_OCARINA].name] =
            uint32_t((gMMComboSharedMask & (1u << ComboRando::SF_OCARINA)) != 0);
        options[Rando::StaticData::Options[RO_SHARED_SPELL_WIND].name] =
            uint32_t((gMMComboSharedMask & (1u << ComboRando::SF_SPELL_WIND)) != 0);
        options[Rando::StaticData::Options[RO_SHARED_SPELL_FIRE].name] =
            uint32_t((gMMComboSharedMask & (1u << ComboRando::SF_SPELL_FIRE)) != 0);
        options[Rando::StaticData::Options[RO_SHARED_SPELL_LOVE].name] =
            uint32_t((gMMComboSharedMask & (1u << ComboRando::SF_SPELL_LOVE)) != 0);
        // ComboShip (#136): combo owns the goal and MM's own toggle is hidden, so the CVar loop above
        // would save hunt=off — leaving Majora killable pre-completion and the tracker row missing.
        options[Rando::StaticData::Options[RO_SHUFFLE_TRIFORCE_PIECES].name] =
            (uint32_t)(gMMComboGoalHunt ? RO_GENERIC_YES : RO_GENERIC_NO);
        if (gMMComboGoalHunt) {
            options[Rando::StaticData::Options[RO_TRIFORCE_PIECES_REQUIRED].name] = (uint32_t)gMMComboGoalRequired;
            // #136: MM's half of the combined total; -1 = old seed, keep the CVar.
            if (gMMComboGoalPieces >= 0) {
                options[Rando::StaticData::Options[RO_TRIFORCE_PIECES_MAX].name] = (uint32_t)gMMComboGoalPieces;
            }
        }
        // The bound seed overrides generation preferences only when creating its native MM save.
        // RO_MM_SPELL_FIRE, RO_MM_SPELL_LOVE and RO_MM_CROSS_AGE are unreleased (no CVar, see
        // StaticData/Options.cpp): ApplyToSaveContext ignores their entries and saves the defaults.
        if (sMMSeedFaroreRulesBound) {
            options[Rando::StaticData::Options[RO_MM_SPELL_WIND].name] = uint32_t(sMMSeedFaroreRules.spellWindMm);
            options[Rando::StaticData::Options[RO_MM_SPELL_FIRE].name] = uint32_t(sMMSeedSpellRules.spellFireMm);
            options[Rando::StaticData::Options[RO_MM_SPELL_LOVE].name] = uint32_t(sMMSeedSpellRules.spellLoveMm);
            options[Rando::StaticData::Options[RO_CROSS_GAME_FARORE].name] = uint32_t(sMMSeedFaroreRules.crossGameFw);
            options[Rando::StaticData::Options[RO_MM_CROSS_AGE].name] = uint32_t(sMMSeedFaroreRules.crossAge);
        }
        // After the seed override above, which can otherwise bring Cross Age back.
        if (!Combo_MM_AdultResourcesReady())
            for (const auto id : kComboHiddenAdultMmOptions)
                options[Rando::StaticData::Options[id].name] = uint32_t(0);
        spoiler["options"] = options;
        spoiler["startingItems"] = nlohmann::json::array();
        // ComboShip: mirror native OnFileCreate's use of the player's configured priority list.
        auto sariaPriorityItems = Rando::GetSariaPriorityItemsFromConfig();
        Rando::SetSariaPriorityItemsInSpoiler(spoiler, sariaPriorityItems);
        // ComboShip: the combo spoiler is friendly-named (check + item), cross-game collisions suffixed
        // "(MM)". Translate to the RC_/RI_ shape ApplyToSaveContext consumes so that shared code stays
        // untouched. Strip our own "(MM)" suffix; the foreign sentinel + any raw RI_ pass through.
        auto stripMM = [](std::string s) {
            static const std::string suf = " (MM)";
            if (s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0)
                s.resize(s.size() - suf.size());
            return s;
        };
        nlohmann::json checks = nlohmann::json::object();
        nlohmann::json rawPlacements = nlohmann::json::parse(placementJson); // bind before .items() (no dangling temp)
        int unknownChecks = 0;
        int unknownItems = 0;
        for (auto& [friendlyCheck, val] : rawPlacements.items()) {
            if (!val.is_string())
                continue;
            RandoCheckId cid = Rando::StaticData::GetCheckIdFromDisplayName(stripMM(friendlyCheck).c_str());
            if (cid == RC_UNKNOWN) {
                unknownChecks++;
                SPDLOG_WARN("[ComboShip] MM_InitRandoSaveFile: unknown check '{}'", friendlyCheck);
                continue;
            }
            std::string v = stripMM(val.get<std::string>());
            RandoItemId iid = Rando::StaticData::GetItemIdFromDisplayName(v.c_str());
            if (iid == RI_UNKNOWN)
                iid = Rando::StaticData::GetItemIdFromName(v.c_str()); // foreign sentinel / raw RI_
            if (iid == RI_UNKNOWN) {
                unknownItems++;
                SPDLOG_WARN("[ComboShip] MM_InitRandoSaveFile: unknown item '{}' at '{}'", v, friendlyCheck);
                // Substitute junk rather than dropping the check: an omitted check reverts to its vanilla
                // item, and a vanilla small key would take the vanilla give path and desync the key mirror.
                iid = RI_JUNK;
            }
            checks[Rando::StaticData::Checks[cid].name] = Rando::StaticData::Items[iid].spoilerName;
        }
        if (unknownChecks != 0 || unknownItems != 0) {
            SPDLOG_ERROR("[ComboShip] MM_InitRandoSaveFile: placement payload had {} unknown checks (dropped) and "
                         "{} unknown items (junked) for slot {}",
                         unknownChecks, unknownItems, fileNum);
        }
        spoiler["checks"] = std::move(checks);

        Rando::Spoiler::ApplyToSaveContext(spoiler);

        // ComboShip: the apply stamps shuffled=true on every payload check incl. non-shuffled Remains;
        // restore native state (delivery reads randoItemId, not shuffled) so stones/tracker skip them.
        if (RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_REMAINS] == RO_GENERIC_NO) {
            for (auto& [id, chk] : Rando::StaticData::Checks) {
                if (chk.randoCheckType == RCTYPE_REMAINS)
                    RANDO_SAVE_CHECKS[id].shuffled = false;
            }
        }

        // ComboShip: store the chosen starting items and bake them into inventory, like native
        // OnFileCreate. Force gPlayState=NULL so GrantStartingItems takes Item_Give's null-guarded
        // headless path (eager-MM-boot may leave a stale gPlayState); SaveManager flush below persists it.
        {
            auto startingItems = Rando::GetStartingItemsFromConfig();
            Rando::SetStartingItemsInSave(gSaveContext.save.shipSaveInfo.rando, startingItems);

            PlayState* savedPlay = gPlayState;
            gPlayState = NULL;
            Rando::GrantStartingItems();
            gPlayState = savedPlay;
        }

        // The two always-eligible starting checks (mirrors OnFileCreate tail).
        RANDO_SAVE_CHECKS[RC_STARTING_ITEM_DEKU_MASK].eligible = true;
        RANDO_SAVE_CHECKS[RC_STARTING_ITEM_SONG_OF_HEALING].eligible = true;

        // Persist the rolled/spoiler prices into the real save (native OnFileCreate rolls them via
        // GeneratePools; the string-only placement apply above leaves every price 0).
        Combo_MM_ApplyCheckPrices();

        SPDLOG_INFO("[ComboShip] MM_InitRandoSaveFile: applied {} placements for slot {}", spoiler["checks"].size(),
                    fileNum);
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[ComboShip] MM_InitRandoSaveFile: {} — slot {} has no placements", e.what(), fileNum);
        // Rebuild the playable baseline: the rando strips above already ran, and persisting a stripped
        // save (no sword/ocarina/magic) would soft-lock the slot. Stays SAVETYPE_RANDO on purpose.
        SaveManager_InitNewSaveForSlot(fileNum + 1, ootName8);
        gSaveContext.fileNum = (s16)fileNum;
        gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
        SaveManager_SaveCurrentForCombo();
        return -1;
    }

    // ComboShip: combo never runs native OnFileCreate, whose tail is this hook's only fire site
    // (OnFileCreate.cpp:220) — without it MM's cosmetic/audio "randomize on rando gen" never triggers.
    // Fired post-apply and outside the try above so a subscriber throw can't void the placements.
    try {
        GameInteractor::Instance->ExecuteHooks<GameInteractor::OnRandoSeedGeneration>();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[ComboShip] MM_InitRandoSaveFile: gen hook threw: {}", e.what());
    } catch (...) { SPDLOG_ERROR("[ComboShip] MM_InitRandoSaveFile: gen hook threw a non-std exception"); }

    // Persist the rando save to the slot file.
    SaveManager_SaveCurrentForCombo();
    return 0;
}

// ComboShip (issue #1): cross-game erase seam. A save slot is one combined OOT+MM playthrough, so
// erasing it from either game's file-select must wipe both saves. Inbound: the launcher calls
// MM_DeleteSaveFile when OOT erases a slot. fileNum is the 0-based file-select slot; MM's JSON naming
// is 1-based (file1.json...), so the +1 lives here and the launcher does no index math. Deletes both
// the main and backup files (mirrors Enhancements/DifficultyOptions/DeleteFileOnDeath.cpp). This goes
// straight to the file system; it never re-enters MM's erase menu, so it cannot loop back into OOT.
// See docs/UPSTREAM_MERGES.md.
void SaveManager_DeleteSaveFile(const std::filesystem::path& fileName);
std::string SaveManager_GetFileName(int fileNum, bool isBackup);
extern "C" COMBO_EXPORT void MM_DeleteSaveFile(int fileNum) {
    SaveManager_DeleteSaveFile(SaveManager_GetFileName(fileNum + 1, false));
    SaveManager_DeleteSaveFile(SaveManager_GetFileName(fileNum + 1, true));
    SPDLOG_INFO("[ComboShip] MM_DeleteSaveFile: erased MM save slot {}", fileNum);
}

// Outbound seam: the launcher registers routing here so MM's own erase can wipe OOT's matching save.
// Fired from z_file_copy_erase.c on erase confirm with the 0-based slot.
extern "C" void (*gMMComboDeleteForeignSave)(int fileNum) = nullptr;
extern "C" COMBO_EXPORT void MM_SetDeleteForeignSave(void (*cb)(int)) {
    gMMComboDeleteForeignSave = cb;
}

// ComboShip: push the launcher's .combosav IO callbacks into SaveManager (routes file{N}.json into
// the merged container). Both primitives funnel through them; null-callbacks fall back to disk IO.
extern "C" COMBO_EXPORT void MM_SetComboSaveIO(ComboRando::FnComboReadSave r, ComboRando::FnComboWriteSave w) {
    SaveManager_SetComboSaveIO(r, w);
}

// ComboShip: receive the consolidated combo spoiler (foreign map + cross-hints), pushed once per
// save-load; store the blob and invalidate MM's lookup caches so they rebuild from it. Idempotent.
extern "C" COMBO_EXPORT void MM_LoadComboRando(const char* json) {
    sMMSeedSpellRules = {};
    sMMSeedFaroreRules = {};
    sMMSeedFaroreRulesBound = true;
    try {
        const auto seed = nlohmann::json::parse(json ? json : "{}");
        ComboRando::ReadFaroreRules(seed.value("faroreRules", nlohmann::json()), sMMSeedFaroreRules);
        ComboRando::ReadSpellRules(seed.value("spellRules", nlohmann::json()), sMMSeedSpellRules);
    } catch (...) {}
    ComboRando::Combo_SetForeignJson(json);
    Rando::MiscBehavior::InvalidateComboForeignCache();
}

// Returns the number of archives open in the MM-private ArchiveManager.
// 0 means MM_InitArchives was not called or found no files.
extern "C" COMBO_EXPORT int MM_ArchiveCount() {
    if (!gMMArchiveManager)
        return 0;
    auto archives = gMMArchiveManager->GetArchives();
    return archives ? static_cast<int>(archives->size()) : 0;
}

#if not defined(__SWITCH__) && not defined(__WIIU__)
extern "C" COMBO_EXPORT bool MM_Extract(const char* searchPath) {
    std::string path = searchPath ? searchPath : std::filesystem::current_path().string();
    std::string installPath = Ship::Context::GetAppBundlePath();

    // Guard: check assets folder exists before attempting extraction
    if (!std::filesystem::exists(installPath + "/assets")) {
        Extractor::ShowErrorBox(
            "Extractor assets not found",
            "No game O2R file found. Missing assets folder needed to generate O2R file.\n\nExiting...");
        return false;
    }

    Extractor extract;
    if (!extract.Run(path)) {
        return false;
    }
    // Upstream merge: CallZapd gained two atomic progress counters (extracted / total).
    std::atomic<size_t> extractCount = 0, totalExtract = 0;
    if (!extract.CallZapd(installPath, path, &extractCount, &totalExtract)) {
        Extractor::ShowErrorBox("Extraction failed",
                                "ROM extraction failed. Check the console window for details.\n\nExiting...");
        return false;
    }
    return true;
}

// ComboShip: UI-less extraction primitives mirroring the soh side (OTRGlobals.cpp). The launcher's
// combo-owned extraction screen owns the picker + progress bar; these do the ZAPD work and expose
// progress. CallZapd is context-independent, so these run fine while only OOT's shared window exists.
// See combo/ComboExtract.h + docs/UPSTREAM_MERGES.md.
static std::atomic<size_t> gComboMMExtractCount{ 0 };
static std::atomic<size_t> gComboMMExtractTotal{ 0 };
static std::atomic<bool> gComboMMExtractDone{ false };
static std::atomic<bool> gComboMMExtractSuccess{ false };
static std::future<void> gComboMMExtractFuture;
static std::string gComboMMExtractRomPath;

extern "C" COMBO_EXPORT int MM_ValidateRom(const char* romPath) {
    if (!romPath) {
        return 0;
    }
    Extractor extract;
    return extract.RunFileStandalone(romPath) ? 1 : 0;
}

// ComboShip: header-only version check for the folder auto-scan (no full-ROM read/CRC).
extern "C" COMBO_EXPORT int MM_ClassifyRom(const char* romPath) {
    if (!romPath) {
        return 0;
    }
    Extractor extract;
    return extract.ClassifyRom(romPath) ? 1 : 0;
}

extern "C" COMBO_EXPORT int MM_StartExtraction(const char* romPath) {
    if (!romPath) {
        return 0;
    }
    if (gComboMMExtractFuture.valid() &&
        gComboMMExtractFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        return 0; // a job is still running
    }
    gComboMMExtractRomPath = romPath;
    gComboMMExtractCount = 0;
    gComboMMExtractTotal = 0;
    gComboMMExtractDone = false;
    gComboMMExtractSuccess = false;
    gComboMMExtractFuture = std::async(std::launch::async, []() {
        bool ok = false;
        try {
            Extractor extract;
            if (extract.RunFileStandalone(gComboMMExtractRomPath)) {
                std::string installPath = Ship::Context::GetAppBundlePath();
                std::string exportPath = Ship::Context::GetAppDirectoryPath(appShortName);
                ok = extract.CallZapd(installPath, exportPath, &gComboMMExtractCount, &gComboMMExtractTotal);
            }
        } catch (const std::exception& error) {
            fprintf(stderr, "MM extraction failed: %s\n", error.what());
            ok = false;
        } catch (...) {
            fprintf(stderr, "MM extraction failed with an unknown error.\n");
            ok = false;
        }
        gComboMMExtractSuccess = ok;
        gComboMMExtractDone = true;
    });
    return 1;
}

extern "C" COMBO_EXPORT void MM_GetExtractionProgress(unsigned long long* count, unsigned long long* total, int* done,
                                                      int* success) {
    if (count) {
        *count = (unsigned long long)gComboMMExtractCount.load();
    }
    if (total) {
        *total = (unsigned long long)gComboMMExtractTotal.load();
    }
    if (done) {
        *done = gComboMMExtractDone.load() ? 1 : 0;
    }
    if (success) {
        *success = gComboMMExtractSuccess.load() ? 1 : 0;
    }
}
#endif

// ComboShip: headless dump of MM rando tables (checks + items), scoped to the current settings via
// Rando::Logic::GeneratePools (mirrors RefreshMetrics() in Rando/Menu.cpp). Only checks the current
// CVars actually shuffle are emitted, so the cross-world fill sees the same pool MM's own generator
// would. Recomputes every call since the result depends on live CVar state. Caller MUST invoke this
// after SOH_Init() returns (so the shared Context, logger, and CVars exist).
#include "../../combo/rando/MmTricks.h"
// ComboShip: snapshot every MM rando option as {cvar: value} for the consolidated spoiler, so a
// dropped/reloaded seed reproduces MM's settings on any machine (MM options are CVar-backed;
// MM_InitRandoSaveFile reads these CVars, so MM_RestoreRandoSettings writes them back before save
// creation). Mirrors the option walk MM_DumpRandoStaticData uses.
extern "C" COMBO_EXPORT const char* MM_DumpRandoSettings(void) {
    static std::string cached;
    nlohmann::json j = nlohmann::json::object();
    for (auto& [id, opt] : Rando::StaticData::Options) {
        if (opt.cvar && opt.cvar[0])
            j[opt.cvar] = (int)CVarGetInteger(opt.cvar, opt.defaultValue);
    }
    // The snapshot travels with seeds; keep it matching their pools (kComboHiddenAdultMmOptions).
    if (!Combo_MM_AdultResourcesReady())
        for (const auto id : kComboHiddenAdultMmOptions)
            j[Rando::StaticData::Options[id].cvar] = 0;
    // Excluded checks are a JSON-array config block of RC_ names (upstream #1817) outside the options
    // walk; GeneratePools reads them, so a replayed spoiler must carry them or local exclusions leak
    // in (GAP-7 mirror).
    nlohmann::json ec = nlohmann::json::array();
    for (RandoCheckId rcid : Rando::GetExcludedChecksFromConfig()) {
        auto cit = Rando::StaticData::Checks.find(rcid); // find, not [] — Checks is a std::map
        if (cit != Rando::StaticData::Checks.end() && cit->second.name && cit->second.name[0])
            ec.push_back(cit->second.name);
    }
    j["gRando.ExcludedChecks"] = ec;
    // Starting items are a JSON-array config block (not a flat CVar); carry them so a joiner's save
    // uses the author's kit instead of local defaults.
    nlohmann::json si = nlohmann::json::array();
    for (RandoItemId rid : Rando::GetStartingItemsFromConfig()) {
        const char* name = Rando::StaticData::Items[rid].spoilerName;
        if (name && name[0])
            si.push_back(name);
    }
    j["gRando.StartingItems"] = si;
    auto priority = nlohmann::json::array();
    for (auto id : Rando::GetDefaultSariaPriorityItems())
        priority.push_back(Rando::StaticData::Items.at(id).spoilerName);
    const auto config = Ship::Context::GetRawInstance()->GetConfig()->GetNestedJson();
    if (config.contains("CVars") && config["CVars"].contains("gRando") &&
        config["CVars"]["gRando"].contains("SariaPriorityItems")) {
        const auto& saved = config["CVars"]["gRando"]["SariaPriorityItems"];
        if (saved.is_array())
            priority = saved;
        else if (saved.is_null())
            priority = nlohmann::json::array();
    }
    j["gRando.SariaPriorityItems"] = std::move(priority);
    for (const auto& trick : ComboRando::kMmTricks)
        j[trick.key] = int(trick.publicOption && CVarGetInteger(trick.key, 0) != 0); // see IsHiddenMmTrick
    cached = j.dump();
    return cached.c_str();
}

// Read only source-owned tables/defaults. Never calls GeneratePools, reads
// active save inventory, or reads enabled-state CVars for profile inspection.
void MM_AppendProfileMenuMetadata(nlohmann::json&);
// Optional catalog assets may have no native icon; JSON cannot construct a string from nullptr.
static const char* MM_ProfileCatalogText(const char* value) {
    return value ? value : "";
}
extern "C" COMBO_EXPORT const char* MM_DumpProfileCatalog(void) {
    static std::string result;
    nlohmann::json data = { { "defaults", nlohmann::json::object() },
                            { "tricks", nlohmann::json::array() },
                            { "checks", nlohmann::json::array() },
                            { "startingItems", nlohmann::json::array() } };
    for (const auto& [id, option] : Rando::StaticData::Options)
        if (option.cvar && option.cvar[0])
            data["defaults"][option.cvar] = option.defaultValue;
    data["defaults"]["gRando.StartingItems"] = nlohmann::json::array();
    data["defaults"]["gRando.ExcludedChecks"] = nlohmann::json::array();
    data["defaults"]["gRando.SariaPriorityItems"] = nlohmann::json::array();
    for (auto id : Rando::GetDefaultSariaPriorityItems())
        data["defaults"]["gRando.SariaPriorityItems"].push_back(
            MM_ProfileCatalogText(Rando::StaticData::Items.at(id).spoilerName));
    for (auto id : Rando::GetSariaPriorityItemCandidates()) {
        const auto& item = Rando::StaticData::Items.at(id);
        data["sariaCandidates"].push_back(
            { { "value", MM_ProfileCatalogText(item.spoilerName) },
              { "label", Rando::StaticData::GetItemDisplayName(id) },
              { "texture", MM_ProfileCatalogText(Rando::StaticData::GetIconTexturePath(id)) } });
    }

    for (const auto& trick : ComboRando::kMmTricks) {
        data["defaults"][MM_ProfileCatalogText(trick.key)] = 0;
        if (trick.publicOption)
            data["tricks"].push_back({ { "value", MM_ProfileCatalogText(trick.key) },
                                       { "label", MM_ProfileCatalogText(trick.name) },
                                       { "description", MM_ProfileCatalogText(trick.description) },
                                       { "area", MM_ProfileCatalogText(trick.area) },
                                       { "tags", MM_ProfileCatalogText(trick.tags) },
                                       { "difficulty", trick.difficulty } });
    }
    for (const auto& [id, check] : Rando::StaticData::Checks)
        if (id > RC_UNKNOWN && id < RC_MAX && check.name && check.name[0])
            data["checks"].push_back(
                { { "value", check.name }, { "label", Rando::StaticData::GetCheckDisplayName(id) } });
    auto tierItem = [](RandoItemId item, int count) {
        const int tier = std::max(1, count) - 1;
        switch (item) {
            case RI_PROGRESSIVE_SWORD: {
                constexpr RandoItemId tiers[] = { RI_SWORD_KOKIRI, RI_SWORD_RAZOR, RI_SWORD_GILDED };
                return tiers[std::min(tier, 2)];
            }
            case RI_PROGRESSIVE_BOMB_BAG: {
                constexpr RandoItemId tiers[] = { RI_BOMB_BAG_20, RI_BOMB_BAG_30, RI_BOMB_BAG_40 };
                return tiers[std::min(tier, 2)];
            }
            case RI_PROGRESSIVE_BOW: {
                constexpr RandoItemId tiers[] = { RI_BOW, RI_QUIVER_40, RI_QUIVER_50 };
                return tiers[std::min(tier, 2)];
            }
            case RI_PROGRESSIVE_WALLET: {
                constexpr RandoItemId tiers[] = { RI_WALLET_ADULT, RI_WALLET_GIANT, RI_WALLET_TYCOON };
                return tiers[std::min(tier, 2)];
            }
            case RI_PROGRESSIVE_MAGIC:
                return tier ? RI_DOUBLE_MAGIC : RI_SINGLE_MAGIC;
            case RI_PROGRESSIVE_LULLABY:
                return tier ? RI_SONG_LULLABY : RI_SONG_LULLABY_INTRO;
            default:
                return item;
        }
    };
    // Native quest page slot order (QUEST_SONG_SONATA through QUEST_SONG_SUN).
    // Double/Inverted Time are extra shuffled songs without native quest slots.
    constexpr RandoItemId startingSongs[] = { RI_SONG_SONATA,      RI_PROGRESSIVE_LULLABY, RI_SONG_NOVA,
                                              RI_SONG_ELEGY,       RI_SONG_OATH,           RI_SONG_SARIA,
                                              RI_SONG_TIME,        RI_SONG_HEALING,        RI_SONG_EPONA,
                                              RI_SONG_SOARING,     RI_SONG_STORMS,         RI_SONG_SUN,
                                              RI_SONG_DOUBLE_TIME, RI_SONG_INVERTED_TIME };
    // Exact prim colors from z_kaleido_collect.c's sQuestSongsPrim* arrays.
    constexpr int startingSongColors[][3] = { { 150, 255, 100 }, { 255, 80, 40 },   { 100, 150, 255 },
                                              { 255, 160, 0 },   { 255, 100, 255 }, { 255, 240, 100 } };
    std::set<RandoItemId> items;
    for (const auto& [category, list] : Rando::StaticData::StartingItemsMap)
        items.insert(list.begin(), list.end());
    // Existing combo instrument tier picker owns these two supported identities.
    items.insert(RI_OCARINA_FAIRY);
    items.insert(RI_PROGRESSIVE_OCARINA);
    for (unsigned i = 0; i < 59; ++i)
        if (Rando::ExpandedSoulAvailable(i))
            items.insert(Rando::expandedSoulItems[i]);
    for (auto id : items) {
        const auto& item = Rando::StaticData::Items.at(id);
        const auto cap = Rando::StaticData::MaxStartingItemsMap.find(id);
        auto tiers = nlohmann::json::array();
        const int maximum = cap == Rando::StaticData::MaxStartingItemsMap.end() ? 1 : int(cap->second);
        for (int tier = 1; tier <= std::max(maximum, 3); ++tier) {
            const auto display = tierItem(id, tier);
            tiers.push_back({ { "texture", MM_ProfileCatalogText(Rando::StaticData::GetIconTexturePath(display)) },
                              { "label", Rando::StaticData::GetItemDisplayName(display) } });
        }
        const bool song = (id >= RI_SONG_DOUBLE_TIME && id <= RI_SONG_TIME) || id == RI_PROGRESSIVE_LULLABY;
        std::string category = "Other";
        for (const auto& [group, list] : Rando::StaticData::StartingItemsMap)
            if (std::find(list.begin(), list.end(), id) != list.end()) {
                category = group == STARTING_ITEMS_MASK                                         ? "Masks"
                           : group == STARTING_ITEMS_INVENTORY || group == STARTING_ITEMS_TRADE ? "Items"
                           : song                                                               ? "Songs"
                                                                                                : "Other";
                break;
            }
        for (auto equipment : { RI_PROGRESSIVE_SWORD, RI_SHIELD_HERO, RI_SHIELD_MIRROR, RI_PROGRESSIVE_MAGIC,
                                RI_DOUBLE_DEFENSE, RI_PROGRESSIVE_WALLET })
            if (id == equipment)
                category = "Equipment";
        nlohmann::json row = { { "value", MM_ProfileCatalogText(item.spoilerName) },
                               { "label", Rando::StaticData::GetItemDisplayName(id) },
                               { "maximum", maximum },
                               { "category", category },
                               { "song", song },
                               { "tiers", std::move(tiers) } };
        if (song) {
            const auto slot = std::find(std::begin(startingSongs), std::end(startingSongs), id);
            if (slot != std::end(startingSongs)) {
                const int index = int(slot - std::begin(startingSongs));
                row["songSlot"] = index;
                row["tint"] = index < 6 ? nlohmann::json::array({ startingSongColors[index][0] / 255.f,
                                                                  startingSongColors[index][1] / 255.f,
                                                                  startingSongColors[index][2] / 255.f, 1.f })
                                        : nlohmann::json::array({ 1.f, 1.f, 1.f, 1.f });
            }
        }
        auto gate = [&](RandoOptionId option) {
            row["poolKey"] = MM_ProfileCatalogText(Rando::StaticData::Options.at(option).cvar);
        };
        if (id == RI_PROGRESSIVE_SWORD || id == RI_SHIELD_HERO)
            row["alwaysPool"] = true;
        if (id >= RI_SOUL_BOSS_GOHT && id <= RI_SOUL_BOSS_TWINMOLD)
            gate(RO_SHUFFLE_BOSS_SOULS);
        if (id >= RI_OCARINA_BUTTON_A && id <= RI_OCARINA_BUTTON_C_UP)
            gate(RO_SHUFFLE_OCARINA_BUTTONS);
        if (id == RI_SONG_SUN)
            gate(RO_SHUFFLE_SONG_SUN);
        if (id == RI_SONG_DOUBLE_TIME)
            gate(RO_SHUFFLE_SONG_DOUBLE_TIME);
        if (id == RI_SONG_INVERTED_TIME)
            gate(RO_SHUFFLE_SONG_INVERTED_TIME);
        if (id == RI_SONG_SARIA)
            gate(RO_SHUFFLE_SONG_SARIA);
        if (id == RI_WOODFALL_KEY_RING)
            gate(RO_KEY_RING_WOODFALL);
        if (id == RI_SNOWHEAD_KEY_RING)
            gate(RO_KEY_RING_SNOWHEAD);
        if (id == RI_GREAT_BAY_KEY_RING)
            gate(RO_KEY_RING_GREAT_BAY);
        if (id == RI_STONE_TOWER_KEY_RING)
            gate(RO_KEY_RING_STONE_TOWER);
        if (id >= RI_TIME_DAY_1 && id <= RI_TIME_NIGHT_3 || id == RI_TIME_PROGRESSIVE) {
            gate(RO_CLOCK_SHUFFLE);
            row["category"] = "Clocks (MM)";
            row["progressiveClock"] = id == RI_TIME_PROGRESSIVE;
            if (id != RI_TIME_PROGRESSIVE)
                row["nightClock"] = id == RI_TIME_NIGHT_1 || id == RI_TIME_NIGHT_2 || id == RI_TIME_NIGHT_3;
        }
        if (id == RI_OCARINA_FAIRY || id == RI_PROGRESSIVE_OCARINA) {
            gate(RO_FAIRY_OCARINA_MM);
            row["category"] = "Items";
        }
        const int soulIndex = Rando::ExpandedSoulIndex(id);
        if (soulIndex >= 0) {
            gate(soulIndex < 4   ? RO_SHUFFLE_ANIMAL_SOULS
                 : soulIndex < 6 ? RO_SHUFFLE_MISC_SOULS
                                 : RO_SHUFFLE_NPC_SOULS);
            row["category"] = soulIndex < 4 ? "Animal Souls" : soulIndex < 6 ? "Misc. Souls" : "NPC Souls";
        }
        if (id == RI_SOUL_BOSS_MAJORA)
            row["excludeHunt"] = true;
        data["startingItems"].push_back(std::move(row));
    }
    for (int preset = 0; preset < 3; ++preset) {
        std::vector<RandoItemId> starting = { RI_PROGRESSIVE_SWORD, RI_SHIELD_HERO, RI_OCARINA, RI_SONG_TIME };
        if (preset == 1) {
            starting.push_back(RI_PROGRESSIVE_SWORD);
            starting.push_back(RI_SONG_SOARING);
        }
        auto names = nlohmann::json::array();
        for (auto id : starting)
            names.push_back(MM_ProfileCatalogText(Rando::StaticData::Items.at(id).spoilerName));
        data["presetStartingItems"][std::to_string(preset)] = std::move(names);
    }
    data["defaults"]["gRando.StartingItems"] = data["presetStartingItems"]["0"];
    MM_AppendProfileMenuMetadata(data);
    result = data.dump();
    return result.c_str();
}

// Transient profile setup: do not call the existing config setters because
// they Save() the complete app config. Inspection/copy never invokes this export.
static int MM_ApplyProfileRandoSettingsImpl(const char* raw, bool apply) {
    try {
        const auto input = nlohmann::json::parse(raw ? raw : "null");
        if (!input.is_object())
            return 0;
        nlohmann::json settings = nlohmann::json::object();
        for (const auto& [id, option] : Rando::StaticData::Options) {
            if (!option.cvar || !option.cvar[0])
                continue;
            const auto value = input.value(option.cvar, nlohmann::json(option.defaultValue));
            // Legacy terminal time is persisted as u32, read through signed integer CVars;
            // its source default is 350. Do not truncate it to an 8-bit option.
            if (!value.is_number_integer() || value.get<int64_t>() < 0 ||
                value.get<uint64_t>() > (id == RO_CLOCK_TERMINAL_TIME ? uint64_t(INT32_MAX) : uint64_t(UINT8_MAX)))
                return 0;
            const bool hidden = Combo_MM_IsHiddenAdultOption(id) && !Combo_MM_AdultResourcesReady();
            settings[option.cvar] = hidden ? nlohmann::json(option.defaultValue) : value;
        }
        for (const auto& trick : ComboRando::kMmTricks) {
            auto value = input.value(trick.key, nlohmann::json(0));
            if (!value.is_number_integer() || (value != 0 && value != 1))
                return 0;
            // A hidden trick still validates (old profiles list it) but is always stored off.
            settings[trick.key] = trick.publicOption ? std::move(value) : nlohmann::json(0);
        }
        auto excluded = input.value("gRando.ExcludedChecks", nlohmann::json::array());
        auto defaultStarting = nlohmann::json::array();
        for (auto id : { RI_PROGRESSIVE_SWORD, RI_SHIELD_HERO, RI_OCARINA, RI_SONG_TIME })
            defaultStarting.push_back(Rando::StaticData::Items.at(id).spoilerName);
        auto starting = input.value("gRando.StartingItems", defaultStarting);
        if (!excluded.is_array() || !starting.is_array())
            return 0;
        for (const auto& value : excluded) {
            if (!value.is_string())
                return 0;
            auto id = Rando::StaticData::GetCheckIdFromName(value.get<std::string>().c_str());
            if (id <= RC_UNKNOWN || id >= RC_MAX)
                return 0;
        }
        for (const auto& value : starting) {
            if (!value.is_string())
                return 0;
            auto id = Rando::StaticData::GetItemIdFromName(value.get<std::string>().c_str());
            if (id <= RI_UNKNOWN || id >= RI_MAX)
                return 0;
        }
        auto priority = input.value("gRando.SariaPriorityItems", nlohmann::json::array());
        if (!priority.is_array() || priority.size() > 16)
            return 0;
        for (const auto& value : priority) {
            if (!value.is_string())
                return 0;
            auto id = Rando::StaticData::GetItemIdFromName(value.get<std::string>().c_str());
            if (id <= RI_UNKNOWN || id >= RI_MAX)
                return 0;
        }
        if (!apply)
            return 1;
        for (const auto& value : settings.items())
            CVarSetInteger(value.key().c_str(), value.value().get<int>());
        auto config = Ship::Context::GetRawInstance()->GetConfig();
        config->SetBlock("CVars.gRando.ExcludedChecks", excluded);
        config->SetBlock("CVars.gRando.StartingItems", starting);
        config->SetBlock("CVars.gRando.SariaPriorityItems", priority);
        return 1;
    } catch (...) { return 0; }
}

extern "C" COMBO_EXPORT int MM_ApplyProfileRandoSettings(const char* raw) {
    return MM_ApplyProfileRandoSettingsImpl(raw, true);
}
extern "C" COMBO_EXPORT int MM_ValidateProfileRandoSettings(const char* raw) {
    return MM_ApplyProfileRandoSettingsImpl(raw, false);
}

// ComboShip: restore MM rando settings from a {cvar:value} snapshot. The reload/drop path calls this
// BEFORE MM_InitRandoSaveFile (which reads these CVars), so a dropped seed builds its MM save with the
// author's settings rather than the local ones.
extern "C" COMBO_EXPORT void MM_RestoreRandoSettings(const char* json) {
    if (!json)
        return;
    try {
        auto j = nlohmann::json::parse(json);
        for (const auto& trick : ComboRando::kMmTricks)
            CVarSetInteger(trick.key, 0);
        // Excluded checks: authoritative RC_-name array from the seed (skipped in the loop below). The
        // snapshot wins outright, so an absent list clears local exclusions (pre-GAP-7 spoilers).
        std::vector<RandoCheckId> excluded;
        if (j.contains("gRando.ExcludedChecks") && j["gRando.ExcludedChecks"].is_array()) {
            for (auto& n : j["gRando.ExcludedChecks"]) {
                auto rcid = Rando::StaticData::GetCheckIdFromName(n.get<std::string>().c_str());
                if (rcid > RC_UNKNOWN && rcid < RC_MAX)
                    excluded.push_back(rcid);
            }
        }
        Rando::SetExcludedChecksInConfig(excluded);
        // Starting items: authoritative array from the seed (handled here, skipped in the loop).
        if (j.contains("gRando.StartingItems") && j["gRando.StartingItems"].is_array()) {
            std::vector<RandoItemId> items;
            for (auto& n : j["gRando.StartingItems"]) {
                auto rid = Rando::StaticData::GetItemIdFromName(n.get<std::string>().c_str());
                if (rid > RI_UNKNOWN && rid < RI_MAX)
                    items.push_back(rid);
            }
            Rando::SetStartingItemsInConfig(items);
        }
        if (j.contains("gRando.SariaPriorityItems") && j["gRando.SariaPriorityItems"].is_array())
            Ship::Context::GetRawInstance()->GetConfig()->SetBlock("CVars.gRando.SariaPriorityItems",
                                                                   j["gRando.SariaPriorityItems"]);
        for (auto it = j.begin(); it != j.end(); ++it) {
            if (it.key() == "gRando.StartingItems" || it.key() == "gRando.ExcludedChecks" ||
                it.key() == "gRando.SariaPriorityItems")
                continue;
            // "" is the no-CVar sentinel of unreleased options (RO_UNRELEASED); never create that CVar.
            if (it.key().empty())
                continue;
            // Hidden tricks stay at the 0 written above, whatever the seed's snapshot says.
            if (ComboRando::IsHiddenMmTrick(it.key()))
                continue;
            if (it.value().is_string())
                CVarSetString(it.key().c_str(), it.value().get<std::string>().c_str());
            else
                CVarSetInteger(it.key().c_str(), it.value().get<int>());
        }
        if (!Combo_MM_AdultResourcesReady())
            for (const auto id : kComboHiddenAdultMmOptions)
                CVarClear(Rando::StaticData::Options[id].cvar);
    } catch (...) {}
}

static const std::unordered_map<std::string, RandoCheckId>& Combo_MM_CheckNameToCheckId();

// GeneratePools' rolled prices (id -> rupees), captured at dump so the oracle reset and the save init
// can re-apply them (both wipe to 0 = every CAN_AFFORD free). MM_SetCheckPrices swaps in spoiler's.
static std::unordered_map<uint32_t, uint16_t> sMMComboCheckPrices;

extern "C" COMBO_EXPORT void MM_SetCheckPrices(const char* json) {
    sMMComboCheckPrices.clear();
    if (!json)
        return;
    try {
        const auto& nameToId = Combo_MM_CheckNameToCheckId();
        auto j = nlohmann::json::parse(json);
        for (auto it = j.begin(); it != j.end(); ++it) {
            auto cit = nameToId.find(it.key());
            if (cit != nameToId.end())
                sMMComboCheckPrices[cit->second] = static_cast<uint16_t>(it.value().get<int>());
        }
    } catch (...) {}
}

static void Combo_MM_ApplyCheckPrices() {
    for (const auto& [id, price] : sMMComboCheckPrices)
        RANDO_SAVE_CHECKS[id].price = price;
}

// ComboShip: reuse EnGs.cpp's gossip-stone weight classification (same maps the runtime stone draw
// uses) so the cross-game hint layer's "no world bias" weighting stays consistent with MM's own.
extern std::unordered_map<RandoItemId, u32> riToWeight;
extern std::unordered_map<RandoItemType, u32> itemTypeToWeight;

extern "C" COMBO_EXPORT const char* MM_DumpRandoStaticData(void) {
    static std::string cached;

    nlohmann::json checks = nlohmann::json::array();
    nlohmann::json pool = nlohmann::json::array();
    auto beanSources = nlohmann::json::object();
    for (const auto& [id, check] : Rando::StaticData::Checks) {
        if (id == RC_UNKNOWN)
            continue;
        const auto source = Rando::BeanSourceForCheck(id, check.randoCheckType);
        beanSources[Rando::StaticData::GetCheckDisplayName(id)] = { { "license", source.license },
                                                                    { "renewable", source.renewable } };
    }
    nlohmann::json fixed = nlohmann::json::array();
    nlohmann::json items = nlohmann::json::array();
    nlohmann::json prices = nlohmann::json::object();

    // Seed MM's RNG so GeneratePools + PreplaceConfinedItems are reproducible per combo seed.
    if (sMMComboRandoSeedSet)
        Ship_Random_Seed(sMMComboRandoSeed);

    // Build a RandoSaveInfo from current CVars — same pattern as Menu.cpp RefreshMetrics().
    // Zero-init: it's a raw C struct, and garbage in randoSaveChecks[].price leaks into the price
    // capture while garbage finalSeed reseeds Ship_Random via the clock-shuffle starting-item branch
    // (StartingItems.cpp), making the whole dump nondeterministic per run.
    RandoSaveInfo saveInfo = {};
    saveInfo.finalSeed = (u32)sMMComboRandoSeed; // native OnFileCreate sets it before starting items
    for (auto& [id, opt] : Rando::StaticData::Options) {
        saveInfo.randoSaveOptions[id] = (uint32_t)CVarGetInteger(opt.cvar, opt.defaultValue);
    }
    auto startingItems = Rando::GetStartingItemsFromConfig();
    Rando::SetStartingItemsInSave(saveInfo, startingItems);

    std::vector<RandoCheckId> checkPool;
    std::vector<RandoItemId> itemPool;
    saveInfo.randoSaveOptions[RO_SHARED_OCARINA] = (gMMComboSharedMask & (1u << ComboRando::SF_OCARINA)) != 0;
    saveInfo.randoSaveOptions[RO_SHARED_SPELL_WIND] = (gMMComboSharedMask & (1u << ComboRando::SF_SPELL_WIND)) != 0;
    saveInfo.randoSaveOptions[RO_SHARED_SPELL_FIRE] = (gMMComboSharedMask & (1u << ComboRando::SF_SPELL_FIRE)) != 0;
    saveInfo.randoSaveOptions[RO_SHARED_SPELL_LOVE] = (gMMComboSharedMask & (1u << ComboRando::SF_SPELL_LOVE)) != 0;
    Combo_MM_ClearHiddenAdultOptions(saveInfo.randoSaveOptions);
    Rando::Logic::GeneratePools(saveInfo, checkPool, itemPool);

    // Capture only randomized purchase prices. Upstream also rolls unused prices
    // on ordinary checks and vanilla shelves; those are not part of the seed.
    sMMComboCheckPrices.clear();
    for (auto& [id, chk] : Rando::StaticData::Checks) {
        if (std::find(checkPool.begin(), checkPool.end(), id) == checkPool.end() ||
            (chk.randoCheckType != RCTYPE_SHOP && chk.randoCheckType != RCTYPE_TINGLE_SHOP))
            continue;
        uint16_t p = saveInfo.randoSaveChecks[id].price;
        if (p == 0) {
            continue;
        }
        sMMComboCheckPrices[id] = p;
        const std::string& cn = Rando::StaticData::GetCheckDisplayName(id); // ComboShip: friendly name
        if (!cn.empty())
            prices[cn] = p;
    }

    // Confine own-dungeon items via MM's own logic (writes RANDO_SAVE_CHECKS, shrinks both pools).
    std::vector<RandoCheckId> checkPoolBefore = checkPool;
    Rando::Logic::PreplaceConfinedItems(checkPool, itemPool);
    std::set<RandoCheckId> stillFillable(checkPool.begin(), checkPool.end());

    // GeneratePools under-fills (MM's own OnFileCreate pads junk); mirror it so every fillable check
    // gets a pool item instead of silently defaulting to vanilla.
    while (itemPool.size() < checkPool.size())
        itemPool.push_back(RI_JUNK);

    // RI_TRAP is RITYPE_LESSER but never gates logic — class it junk like OOT's traps.
    auto isAdvancement = [](const auto& it) {
        return it.randoItemType != RITYPE_JUNK && it.randoItemType != RITYPE_HEALTH && it.randoItemId != RI_TRAP;
    };

    // ComboShip: native category, so the cross fill can trim ONLY junk — `advancement` alone can't say
    // that (it lumps junk with hearts/traps). Unknown maps to "major" so it is never trimmable.
    auto categoryName = [](const auto& it) -> const char* {
        switch (it.randoItemType) {
            case RITYPE_JUNK:
                return "junk";
            case RITYPE_LESSER:
                return "lesser";
            case RITYPE_HEALTH:
                return "health";
            case RITYPE_BOSS_KEY:
                return "bossKey";
            case RITYPE_SMALL_KEY:
                return "smallKey";
            case RITYPE_SKULLTULA_TOKEN:
                return "token";
            case RITYPE_MAJOR:
                return "major";
            case RITYPE_MASK:
                return "mask";
            case RITYPE_STRAY_FAIRY:
                return "strayFairy";
            case RITYPE_MAX:
                break;
        }
        return "major"; // no default: a new RITYPE_ must warn, not silently become non-discardable
    };

    // Confined pre-placements -> fixed[] (removed checks = checkPoolBefore minus checkPool).
    for (RandoCheckId id : checkPoolBefore) {
        if (stillFillable.count(id))
            continue;
        auto chkIt = Rando::StaticData::Checks.find(id);
        if (chkIt == Rando::StaticData::Checks.end() || !chkIt->second.name || chkIt->second.name[0] == '\0')
            continue;
        auto iit = Rando::StaticData::Items.find(RANDO_SAVE_CHECKS[id].randoItemId);
        if (iit == Rando::StaticData::Items.end() || !iit->second.spoilerName || iit->second.spoilerName[0] == '\0')
            continue;
        // ComboShip: friendly check + item names for the normalized combo spoiler. These are genuinely
        // shuffled (PreplaceConfinedItems sets shuffled=true), so they stay hint targets.
        fixed.push_back({ { "check", Rando::StaticData::GetCheckDisplayName(id) },
                          { "item", Rando::StaticData::GetItemDisplayName(iit->first) },
                          { "advancement", isAdvancement(iit->second) },
                          { "hintable", true } });
    }

    // ComboShip: when boss remains aren't shuffled, GeneratePools drops RCTYPE_REMAINS checks entirely
    // (GeneratePools.cpp), so the Remains never reach the oracle — yet Moon/Majora access gates on
    // RemainsCount(). Emit each as a fixed placement of its vanilla remains so the fill/oracle credit it
    // once the boss-warp check is reachable (i.e. the temple is beaten). Mirrors the OOT vanilla-shop fix.
    if (saveInfo.randoSaveOptions[RO_SHUFFLE_BOSS_REMAINS] == RO_GENERIC_NO) {
        for (auto& [id, chk] : Rando::StaticData::Checks) {
            if (chk.randoCheckType != RCTYPE_REMAINS || !chk.name || chk.name[0] == '\0')
                continue;
            auto iit = Rando::StaticData::Items.find(chk.randoItemId);
            if (iit == Rando::StaticData::Items.end() || !iit->second.spoilerName || iit->second.spoilerName[0] == '\0')
                continue;
            // ComboShip: friendly check + item names for the normalized combo spoiler. Not shuffled, so
            // hints must never target these (native never hints a non-shuffled check).
            fixed.push_back({ { "check", Rando::StaticData::GetCheckDisplayName(id) },
                              { "item", Rando::StaticData::GetItemDisplayName(iit->first) },
                              { "advancement", true },
                              { "hintable", false } });
        }
    }

    // ComboShip: 5.0.0's per-house skulltula shuffle keeps 30-N tokens vanilla: GeneratePools marks
    // them shuffled=true with their own token in the (discarded) local saveInfo and drops them from
    // checkPool. Emit them as fixed so the oracle credits the tokens and the apply stamps them like
    // native (shuffled=true, so they stay hintable, mirroring native).
    const bool vanillaTokens = saveInfo.randoSaveOptions[RO_SHUFFLE_GOLD_SKULLTULAS] == RO_GENERIC_NO;
    if (vanillaTokens || saveInfo.randoSaveOptions[RO_SHUFFLE_GOLD_SKULLTULAS] == RO_GENERIC_YES) {
        for (auto& [id, chk] : Rando::StaticData::Checks) {
            if (chk.randoCheckType != RCTYPE_SKULL_TOKEN ||
                (!vanillaTokens && !saveInfo.randoSaveChecks[id].shuffled) || stillFillable.count(id))
                continue;
            auto iit = Rando::StaticData::Items.find(chk.randoItemId);
            if (iit == Rando::StaticData::Items.end() || !iit->second.spoilerName || iit->second.spoilerName[0] == '\0')
                continue;
            fixed.push_back({ { "check", Rando::StaticData::GetCheckDisplayName(id) },
                              { "item", Rando::StaticData::GetItemDisplayName(iit->first) },
                              { "advancement", isAdvancement(iit->second) },
                              { "hintable", !vanillaTokens } });
        }
    }

    // Vanilla returned frogs are progression too. Credit each only after its own check is
    // reachable; never grant all frogs merely because the reward is present in the pool.
    if (saveInfo.randoSaveOptions[RO_SHUFFLE_FROGS] == RO_GENERIC_NO) {
        for (const auto& [id, chk] : Rando::StaticData::Checks) {
            if (chk.randoCheckType != RCTYPE_FROG || !chk.name || !chk.name[0])
                continue;
            const auto item = Rando::StaticData::Items.find(chk.randoItemId);
            if (item == Rando::StaticData::Items.end() || !item->second.spoilerName || !item->second.spoilerName[0])
                continue;
            fixed.push_back({ { "check", Rando::StaticData::GetCheckDisplayName(id) },
                              { "item", Rando::StaticData::GetItemDisplayName(item->first) },
                              { "advancement", true },
                              { "hintable", false } });
        }
    }

    // Vanilla owl activations unlock actual Soaring sources, including sources whose
    // destinations belong to OoT in a mixed map. Credit each only after its statue check.
    if (saveInfo.randoSaveOptions[RO_SHUFFLE_OWL_STATUES] == RO_GENERIC_NO) {
        for (const auto& [id, chk] : Rando::StaticData::Checks) {
            if (chk.randoCheckType != RCTYPE_OWL || !chk.name || !chk.name[0])
                continue;
            const auto item = Rando::StaticData::Items.find(chk.randoItemId);
            if (item == Rando::StaticData::Items.end() || !item->second.spoilerName || !item->second.spoilerName[0])
                continue;
            fixed.push_back({ { "check", Rando::StaticData::GetCheckDisplayName(id) },
                              { "item", Rando::StaticData::GetItemDisplayName(item->first) },
                              { "advancement", true },
                              { "hintable", false } });
        }
    }

    // Fillable checks -> checks[] (name only; pool[] feeds the items).
    for (RandoCheckId id : checkPool) {
        auto chkIt = Rando::StaticData::Checks.find(id);
        if (chkIt == Rando::StaticData::Checks.end())
            continue;
        const std::string& cn = Rando::StaticData::GetCheckDisplayName(id); // ComboShip: friendly name
        if (cn.empty())
            continue;
        checks.push_back({ { "name", cn } });
    }

    // OoTMM transform.ts counts consumables originally supplied by randomized
    // renewable locations; solve.ts protects those copies from junk filling.
    // Keep the native IDs here and export the resulting placement constraint.
    // groups.ts CRITICAL_RENEWABLES also lists MM_MAGIC_BEAN, which is not junk there; bean renewal is
    // handled by beanSources (license/renewable per check) and the Palace salesman event instead.
    auto criticalRenewable = [](RandoItemId id) {
        return id == RI_SHIELD_HERO || id == RI_BOMBCHU_10 || id == RI_RED_POTION_REFILL ||
               id == RI_BLUE_POTION_REFILL || id == RI_MILK_REFILL;
    };
    std::unordered_map<RandoItemId, size_t> renewableJunks;
    for (const auto id : checkPool) {
        const auto& check = Rando::StaticData::Checks.at(id);
        if (criticalRenewable(check.randoItemId) && Rando::BeanSourceForCheck(id, check.randoCheckType).renewable) {
            ++renewableJunks[check.randoItemId];
        }
    }

    // Pool = the real free item pool (every settings-added item; confined items already removed).
    for (RandoItemId iid : itemPool) {
        auto it = Rando::StaticData::Items.find(iid);
        if (it == Rando::StaticData::Items.end() || !it->second.spoilerName || it->second.spoilerName[0] == '\0')
            continue;
        // ComboShip: friendly item name for the normalized combo spoiler.
        const bool renewable = renewableJunks[iid] > 0;
        if (renewable)
            --renewableJunks[iid];
        pool.push_back({ { "name", Rando::StaticData::GetItemDisplayName(iid) },
                         { "advancement", isAdvancement(it->second) || renewable },
                         { "criticalRenewable", renewable },
                         { "category", categoryName(it->second) } });
    }

    for (auto& [id, item] : Rando::StaticData::Items) {
        if (!item.spoilerName || item.spoilerName[0] == '\0')
            continue;
        // ComboShip: "name" is the friendly combo-spoiler key the grant/apply paths resolve.
        // "displayName" is the human string for toasts/shops in the OTHER game (suffixed there).
        // advancement drives whether a foreign item plays the held-up pickup animation.
        // ComboShip: "trap" lets the cross-world layer disguise a foreign trap in the other game.
        // ComboShip: "trickNames" are MM's curated fake names, so a foreign trap disguised as this
        // item can lie with a real near-miss name instead of a letter-doubled one.
        nlohmann::json entry = { { "name", Rando::StaticData::GetItemDisplayName(id) },
                                 { "id", static_cast<int>(id) },
                                 { "advancement", isAdvancement(item) },
                                 { "trap", id == RI_TRAP },
                                 { "trickNames", Rando::StaticData::GetTrickNames(id) } };
        if (item.name && item.name[0] != '\0') {
            entry["displayName"] = item.name;
        }
        // ComboShip: hint-weight class (same cascade EnGs.cpp's gossip-stone draw uses, minus the
        // per-check rcToWeight overrides, which need a check context). Cross-game hint gen uses this
        // to weight MM items the same way MM's own stones would.
        u32 weight = 1;
        if (riToWeight.contains(id)) {
            weight = riToWeight[id];
        } else if (itemTypeToWeight.contains(item.randoItemType)) {
            weight = itemTypeToWeight[item.randoItemType];
        }
        entry["weightClass"] = weight;
        items.push_back(std::move(entry));
    }

    // ComboShip: per-check hint-safe location name (GetLocationNameForHint(rc,false)) — the combo
    // hint layer's cross-game text composition needs the same "region" phrasing MM's own hints use.
    nlohmann::json locationHints = nlohmann::json::object();
    nlohmann::json checkMetadata = nlohmann::json::object();
    for (auto& [id, chk] : Rando::StaticData::Checks) {
        const std::string& cn = Rando::StaticData::GetCheckDisplayName(id); // ComboShip: friendly name
        if (cn.empty())
            continue;
        locationHints[cn] = Rando::StaticData::GetLocationNameForHint(id, false);
        checkMetadata[cn] = { { "id", static_cast<int>(id) },
                              { "enum", chk.name },
                              { "scene", static_cast<int>(chk.sceneId) },
                              { "type", static_cast<int>(chk.randoCheckType) },
                              { "flagType", static_cast<int>(chk.flagType) },
                              { "flag", chk.flag },
                              { "vanillaItem", static_cast<int>(chk.randoItemId) } };
    }

    // ComboShip: the two hint options that decide whether the combo hint layer's cross gossip pool is
    // even consumed (EnGs.cpp) — lets the pare-down gate skip requiredness work when both are off.
    nlohmann::json options = {
        { "RO_MM_SPELL_WIND", (uint32_t)saveInfo.randoSaveOptions[RO_MM_SPELL_WIND] },
        { "RO_MM_SPELL_FIRE", (uint32_t)saveInfo.randoSaveOptions[RO_MM_SPELL_FIRE] },
        { "RO_MM_SPELL_LOVE", (uint32_t)saveInfo.randoSaveOptions[RO_MM_SPELL_LOVE] },
        { "RO_MM_BOOTS_IRON", (uint32_t)saveInfo.randoSaveOptions[RO_MM_BOOTS_IRON] },
        { "RO_MM_BOOTS_HOVER", (uint32_t)saveInfo.randoSaveOptions[RO_MM_BOOTS_HOVER] },
        { "RO_MM_TUNIC_GORON", (uint32_t)saveInfo.randoSaveOptions[RO_MM_TUNIC_GORON] },
        { "RO_MM_TUNIC_ZORA", (uint32_t)saveInfo.randoSaveOptions[RO_MM_TUNIC_ZORA] },

        { "RO_CROSS_GAME_FARORE", (uint32_t)saveInfo.randoSaveOptions[RO_CROSS_GAME_FARORE] },
        { "RO_MM_CROSS_AGE", (uint32_t)saveInfo.randoSaveOptions[RO_MM_CROSS_AGE] },
        { "RO_HINTS_GOSSIP_STONES", (uint32_t)saveInfo.randoSaveOptions[RO_HINTS_GOSSIP_STONES] },
        { "RO_HINTS_PURCHASEABLE", (uint32_t)saveInfo.randoSaveOptions[RO_HINTS_PURCHASEABLE] }
    };

    // ComboShip: the set MM's own pickup rotation draws from, so the combo generator can bake a
    // cross-placed junk placeholder into an item MM itself would have handed the player.
    nlohmann::json junkPool = nlohmann::json::array();
    for (RandoItemId id : Rando::ComboJunkPool()) {
        junkPool.push_back(Rando::StaticData::GetItemDisplayName(id));
    }

    nlohmann::json sharedStartingItems = nlohmann::json::array();
    // Every explicitly configured starting item; the combo layer picks the shared
    // single-item families from it. Computed clock/key starts are left out so
    // they do not consume the generator RNG here.
    for (const auto item : startingItems)
        sharedStartingItems.push_back(Rando::StaticData::GetItemDisplayName(item));
    cached = nlohmann::json{
        { "sharedStartingItems", std::move(sharedStartingItems) },
        { "sharedButtonsEnabled", saveInfo.randoSaveOptions[RO_SHUFFLE_OCARINA_BUTTONS] != 0 },
        { "fairyOcarinaMm", saveInfo.randoSaveOptions[RO_FAIRY_OCARINA_MM] != 0 },
        { "checks", std::move(checks) },
        { "pool", std::move(pool) },
        { "fixed", std::move(fixed) },
        { "beanSources", std::move(beanSources) },
        { "items", std::move(items) },
        { "prices", std::move(prices) },
        { "locationHints", std::move(locationHints) },
        { "checkMetadata", std::move(checkMetadata) },
        { "options", std::move(options) },
        { "junkPool", std::move(junkPool) }
    }.dump();
    return cached.c_str();
}

// ComboShip: MM reachability oracle — headless logic-engine wrappers. The combined fill drives
// these to query "given owned items, which checks are reachable?"

static SaveContext sMM_OracleSavedContext;
static uint64_t sMM_OracleSavedRegionTime;
static ComboRando::Entrances::MmFlagDomain sMM_OracleSavedRegionalFlags;
// ComboShip: Reset runs once PER REACHABILITY QUERY (dozens per fill) but Restore only once at the
// end of the whole fill. Without this flag the second Reset snapshots the already-zeroed context,
// so Restore would write garbage (zeros) back into MM's live save after generation.
static bool sMM_OracleActive = false;
static bool sMM_OracleExternalBeans = false;
static bool sMM_OracleInventorySweepDone = false;
using Rando::Logic::gCurrentRegionTime;

// ComboShip (#61): cross-grant only set the trade item's obtained flag, leaving the shared trade
// slot's main item empty (item stuck as a rotatable offset, invisible to the tracker). Populate the
// main slot when empty, dormant-safe (no gPlayState), mirroring real pickup's Item_Give.
static void ComboGrantTradeSlot(u8 itemId) {
    if (itemId >= ARRAY_COUNT(gItemSlots) || gItemSlots[itemId] == SLOT_NONE) {
        return;
    }
    if (INV_CONTENT(itemId) == ITEM_NONE) {
        INV_CONTENT(itemId) = itemId;
    }
}

// Headless item-give: sets gSaveContext fields without ever touching gPlayState.
// Covers the save-context mutations that logic conditions read (INV_CONTENT, equipment,
// quest items, rando flags, dungeon items, week event regs). Derived from GiveItem.cpp.
static void GiveItemForOracle(RandoItemId ri) {
    int gearItem = ri == RI_BOOTS_IRON    ? ITEM_COMBO_BOOTS_IRON
                   : ri == RI_BOOTS_HOVER ? ITEM_COMBO_BOOTS_HOVER
                   : ri == RI_TUNIC_GORON ? ITEM_COMBO_TUNIC_GORON
                   : ri == RI_TUNIC_ZORA  ? ITEM_COMBO_TUNIC_ZORA
                                          : -1;
    if (gearItem >= 0) {
        auto& state = gSaveContext.save.shipSaveInfo.adultMm;
        const int group = ComboAdultGearInventory::Group(unsigned(gearItem));
        ComboAdultGearInventory::Grant(
            group == 2 ? state.gearTrade2Bits : state.gearTrade3Bits,
            gSaveContext.save.saveInfo.inventory.items[group == 2 ? SLOT_TRADE_KEY_MAMA : SLOT_TRADE_COUPLE],
            unsigned(gearItem));
        return;
    }

    if (ri == RI_MASK_ADULT || ri == RI_MASK_FIERCE_DEITY) {
        ComboAdultMaskInventory::Grant(gSaveContext.save.shipSaveInfo.adultMm.maskOwnedBits,
                                       INV_CONTENT(ITEM_MASK_FIERCE_DEITY), ri == RI_MASK_ADULT ? 1 : 0);
        return;
    }
    if (ri == RI_FARORES_WIND) {
        Combo_MMFaroreGrantItem();
        return;
    }
    if (ri == RI_DINS_FIRE) {
        Combo_MMSpellGrantItem(2);
        return;
    }
    if (ri == RI_NAYRUS_LOVE) {
        Combo_MMSpellGrantItem(1);
        return;
    }
    if (ri == RI_PROGRESSIVE_OCARINA) {
        GiveItemForOracle(Rando::ConvertItem(ri));
        return;
    }
    if (ri == RI_OCARINA || ri == RI_OCARINA_FAIRY) {
        Rando::Ocarinas::Grant(gSaveContext.save.shipSaveInfo.rando.ocarinaOwnedBits, INV_CONTENT(ITEM_OCARINA_OF_TIME),
                               ri == RI_OCARINA ? 2 : 1);
        return;
    }

    if (Rando::GrantMissingMmSoul(ri))
        return;
    if (Rando::GrantExpandedSoulItem(ri))
        return;
    switch (ri) {
        case RI_TRIFORCE_POWER:
        case RI_TRIFORCE_COURAGE:
        case RI_TRIFORCE_WISDOM:
            gSaveContext.save.shipSaveInfo.rando.namedTriforceMask |= 1u << (ri - RI_TRIFORCE_POWER);
            break;
        case RI_JUNK:
        case RI_NONE:
        case RI_TRAP:
            break;

        // Magic
        case RI_SINGLE_MAGIC:
            gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
            gSaveContext.save.saveInfo.playerData.magic = MAGIC_NORMAL_METER;
            SET_WEEKEVENTREG(WEEKEVENTREG_12_80);
            break;
        case RI_DOUBLE_MAGIC:
            gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
            gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = true;
            gSaveContext.save.saveInfo.playerData.magic = MAGIC_DOUBLE_METER;
            SET_WEEKEVENTREG(WEEKEVENTREG_12_80);
            break;
        case RI_DOUBLE_DEFENSE:
            gSaveContext.save.saveInfo.playerData.doubleDefense = true;
            gSaveContext.save.saveInfo.inventory.defenseHearts = 20;
            break;

        // Swords — set equipment value (logic checks GET_CUR_EQUIP_VALUE)
        case RI_SWORD_KOKIRI:
            SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_KOKIRI);
            break;
        case RI_SWORD_RAZOR:
            SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_RAZOR);
            break;
        case RI_SWORD_GILDED:
            SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_GILDED);
            break;

        // Shields are EQUIPMENT, not inventory-grid items (no gItemSlots entry), and logic reads the
        // equipped value (GET_CUR_EQUIP_VALUE) — e.g. Igos du Ikana needs the Mirror Shield. Real pickup
        // auto-equips (z_parameter.c); mirror that here (best shield wins). Do NOT touch INV_CONTENT — its
        // slot lookup is SLOT_NONE for shields and would write out of bounds.
        case RI_SHIELD_HERO:
        case RI_SHIELD_MIRROR: {
            u16 shieldEquip = (u16)(Rando::StaticData::Items[ri].itemId - ITEM_SHIELD_HERO + EQUIP_VALUE_SHIELD_HERO);
            if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) < shieldEquip)
                SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, shieldEquip);
            break;
        }

        // Bow tiers mirror ConvertItem -> Item_Give: duplicate/lower tiers become junk.
        // The oracle grants owned items directly, so never lower an already-owned quiver.
        case RI_BOW:
        case RI_QUIVER_40:
        case RI_QUIVER_50: {
            int tier = ri == RI_BOW ? 1 : ri == RI_QUIVER_40 ? 2 : 3;
            if (CUR_UPG_VALUE(UPG_QUIVER) < tier) {
                Inventory_ChangeUpgrade(UPG_QUIVER, tier);
                INV_CONTENT(ITEM_BOW) = ITEM_BOW;
                AMMO(ITEM_BOW) = CAPACITY(UPG_QUIVER, tier);
            }
            break;
        }

        // Bomb bags — set upgrade + inventory. Chu grant left unconditional: gen-time mask is the
        // loaded slot's, not the seed's, and MM logic never gates on chus alone (see GiveItem.cpp).
        case RI_BOMB_BAG_20:
            Inventory_ChangeUpgrade(UPG_BOMB_BAG, 1);
            INV_CONTENT(ITEM_BOMB) = ITEM_BOMB;
            INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
            AMMO(ITEM_BOMB) = 20;
            break;
        case RI_BOMB_BAG_30:
            Inventory_ChangeUpgrade(UPG_BOMB_BAG, 2);
            INV_CONTENT(ITEM_BOMB) = ITEM_BOMB;
            INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
            AMMO(ITEM_BOMB) = 30;
            break;
        case RI_BOMB_BAG_40:
            Inventory_ChangeUpgrade(UPG_BOMB_BAG, 3);
            INV_CONTENT(ITEM_BOMB) = ITEM_BOMB;
            INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
            AMMO(ITEM_BOMB) = 40;
            break;

        // Wallets
        case RI_WALLET_ADULT:
            Inventory_ChangeUpgrade(UPG_WALLET, 1);
            break;
        case RI_WALLET_GIANT:
            Inventory_ChangeUpgrade(UPG_WALLET, 2);
            break;
        case RI_WALLET_TYCOON:
            Inventory_ChangeUpgrade(UPG_WALLET, 3);
            break;

        // Heart pieces/containers — logic doesn't check these for reachability, but include for completeness
        case RI_HEART_CONTAINER:
            gSaveContext.save.saveInfo.playerData.healthCapacity += 0x10;
            break;
        case RI_HEART_PIECE:
            gSaveContext.save.saveInfo.playerData.healthCapacity += 0x10;
            break;

        // New bottles occupy a free slot, just like Item_Give. Writing the
        // generic inventory slot would replace an existing empty bottle with
        // Gold Dust and make reachability depend on the order of item grants.
        case RI_BOTTLE_EMPTY:
        case RI_BOTTLE_CHATEAU_ROMANI:
        case RI_BOTTLE_MILK:
        case RI_BOTTLE_GOLD_DUST:
        case RI_BOTTLE_RED_POTION: {
            for (int i = SLOT(ITEM_BOTTLE); i < SLOT(ITEM_BOTTLE) + 6; i++) {
                if (gSaveContext.save.saveInfo.inventory.items[i] == ITEM_NONE) {
                    gSaveContext.save.saveInfo.inventory.items[i] =
                        ri == RI_BOTTLE_RED_POTION ? ITEM_POTION_RED : Rando::StaticData::Items[ri].itemId;
                    break;
                }
            }
            break;
        }

        // Dungeon items
        case RI_WOODFALL_BOSS_KEY:
        case RI_WOODFALL_MAP:
        case RI_WOODFALL_COMPASS:
            SET_DUNGEON_ITEM(Rando::StaticData::Items[ri].itemId - ITEM_KEY_BOSS, DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE);
            break;
        case RI_SNOWHEAD_BOSS_KEY:
        case RI_SNOWHEAD_MAP:
        case RI_SNOWHEAD_COMPASS:
            SET_DUNGEON_ITEM(Rando::StaticData::Items[ri].itemId - ITEM_KEY_BOSS, DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE);
            break;
        case RI_GREAT_BAY_BOSS_KEY:
        case RI_GREAT_BAY_MAP:
        case RI_GREAT_BAY_COMPASS:
            SET_DUNGEON_ITEM(Rando::StaticData::Items[ri].itemId - ITEM_KEY_BOSS, DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE);
            break;
        case RI_STONE_TOWER_BOSS_KEY:
        case RI_STONE_TOWER_MAP:
        case RI_STONE_TOWER_COMPASS:
            SET_DUNGEON_ITEM(Rando::StaticData::Items[ri].itemId - ITEM_KEY_BOSS,
                             DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE);
            break;

        // Small keys. ComboShip: logic's KEY_COUNT reads rando.foundDungeonKeys, NOT inventory.dungeonKeys
        // (DUNGEON_KEY_COUNT), so bump both like the real GiveItem — else every KEY_COUNT gate stays 0 and
        // key-locked dungeon rooms (Stone Tower/Snowhead/Great Bay deep) are unreachable.
        case RI_WOODFALL_KEY_RING:
        case RI_SNOWHEAD_KEY_RING:
        case RI_GREAT_BAY_KEY_RING:
        case RI_STONE_TOWER_KEY_RING:
            Rando::GiveKeyRing(ri);
            break;
        case RI_WOODFALL_SMALL_KEY:
            Rando::AddSmallKey(DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE);
            break;
        case RI_SNOWHEAD_SMALL_KEY:
            Rando::AddSmallKey(DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE);
            break;
        case RI_GREAT_BAY_SMALL_KEY:
            Rando::AddSmallKey(DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE);
            break;
        case RI_STONE_TOWER_SMALL_KEY:
            Rando::AddSmallKey(DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE);
            break;
        // ComboShip: the oracle had no Skeleton Key case at all, so key-gated regions stayed unreachable
        // during fill. Same raise-both-counters body as Rando::GiveItem.
        case RI_SKELETON_KEY:
            for (auto& k : Rando::skeletonKeyCounts) {
                if (DUNGEON_KEY_COUNT(k.dungeonSceneIndex) < k.count) {
                    DUNGEON_KEY_COUNT(k.dungeonSceneIndex) = k.count;
                }
                if (gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys[k.dungeonSceneIndex] < k.count) {
                    gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys[k.dungeonSceneIndex] = k.count;
                }
            }
            break;

        // Stray fairies
        case RI_CLOCK_TOWN_STRAY_FAIRY:
            SET_WEEKEVENTREG(WEEKEVENTREG_08_80);
            break;
        case RI_WOODFALL_STRAY_FAIRY:
            gSaveContext.save.saveInfo.inventory.strayFairies[DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE]++;
            break;
        case RI_SNOWHEAD_STRAY_FAIRY:
            gSaveContext.save.saveInfo.inventory.strayFairies[DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE]++;
            break;
        case RI_GREAT_BAY_STRAY_FAIRY:
            gSaveContext.save.saveInfo.inventory.strayFairies[DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE]++;
            break;
        case RI_STONE_TOWER_STRAY_FAIRY:
            gSaveContext.save.saveInfo.inventory.strayFairies[DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE]++;
            break;

        // Rando-flag items (deeds, keys, letters, etc.) — also populate the shared trade slot (#61).
        case RI_MOONS_TEAR:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_MOONS_TEAR);
            ComboGrantTradeSlot(ITEM_MOONS_TEAR);
            break;
        case RI_DEED_LAND:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_DEED_LAND);
            ComboGrantTradeSlot(ITEM_DEED_LAND);
            break;
        case RI_DEED_SWAMP:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_DEED_SWAMP);
            ComboGrantTradeSlot(ITEM_DEED_SWAMP);
            break;
        case RI_DEED_MOUNTAIN:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_DEED_MOUNTAIN);
            ComboGrantTradeSlot(ITEM_DEED_MOUNTAIN);
            break;
        case RI_DEED_OCEAN:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_DEED_OCEAN);
            ComboGrantTradeSlot(ITEM_DEED_OCEAN);
            break;
        case RI_ROOM_KEY:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_ROOM_KEY);
            ComboGrantTradeSlot(ITEM_ROOM_KEY);
            break;
        case RI_LETTER_TO_MAMA:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_LETTER_TO_MAMA);
            ComboGrantTradeSlot(ITEM_LETTER_MAMA);
            break;
        case RI_LETTER_TO_KAFEI:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_LETTER_TO_KAFEI);
            ComboGrantTradeSlot(ITEM_LETTER_TO_KAFEI);
            break;
        case RI_PENDANT_OF_MEMORIES:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_PENDANT_OF_MEMORIES);
            ComboGrantTradeSlot(ITEM_PENDANT_OF_MEMORIES);
            break;
        case RI_POWDER_KEG:
            Flags_SetWeekEventReg(WEEKEVENTREG_HAS_POWDERKEG_PRIVILEGES);
            // ComboShip: logic gates on HAS_ITEM(ITEM_POWDER_KEG) (INV_CONTENT), not the privilege reg;
            // real GiveItem also does Item_Give, so grant the slot too or Milk Road/Ikana/cows never open.
            INV_CONTENT(ITEM_POWDER_KEG) = ITEM_POWDER_KEG;
            break;
        case RI_GREAT_SPIN_ATTACK:
            SET_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK);
            break;
        case RI_ABILITY_SWIM:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_SWIM);
            break;

        // ComboShip: owl-warp statues. Logic gates nearly every exit from the root region on
        // CAN_OWL_WARP, the primary entry into each cardinal cluster. The default case below would set
        // only INV_CONTENT and leave owlActivationFlags clear, so CAN_OWL_WARP stays false and the
        // oracle can't leave Clock Town. Sram_ActivateOwl is headless-safe (pure save-bit write).
        // Mirrors GiveItem.cpp RI_OWL_* cases.
        case RI_OWL_CLOCK_TOWN_SOUTH:
            Sram_ActivateOwl(OWL_WARP_CLOCK_TOWN);
            break;
        case RI_OWL_GREAT_BAY_COAST:
            Sram_ActivateOwl(OWL_WARP_GREAT_BAY_COAST);
            break;
        case RI_OWL_IKANA_CANYON:
            Sram_ActivateOwl(OWL_WARP_IKANA_CANYON);
            break;
        case RI_OWL_MILK_ROAD:
            Sram_ActivateOwl(OWL_WARP_MILK_ROAD);
            break;
        case RI_OWL_MOUNTAIN_VILLAGE:
            Sram_ActivateOwl(OWL_WARP_MOUNTAIN_VILLAGE);
            break;
        case RI_OWL_SNOWHEAD:
            Sram_ActivateOwl(OWL_WARP_SNOWHEAD);
            break;
        case RI_OWL_SOUTHERN_SWAMP:
            Sram_ActivateOwl(OWL_WARP_SOUTHERN_SWAMP);
            break;
        case RI_OWL_STONE_TOWER:
            Sram_ActivateOwl(OWL_WARP_STONE_TOWER);
            break;
        case RI_OWL_WOODFALL:
            Sram_ActivateOwl(OWL_WARP_WOODFALL);
            break;
        case RI_OWL_ZORA_CAPE:
            Sram_ActivateOwl(OWL_WARP_ZORA_CAPE);
            break;

        // Ocarina buttons
        case RI_OCARINA_BUTTON_A:
        case RI_OCARINA_BUTTON_C_DOWN:
        case RI_OCARINA_BUTTON_C_LEFT:
        case RI_OCARINA_BUTTON_C_RIGHT:
        case RI_OCARINA_BUTTON_C_UP:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_A + (ri - RI_OCARINA_BUTTON_A));
            break;

        // Songs (song double/inverted time)
        case RI_SONG_DOUBLE_TIME:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_SONG_DOUBLE_TIME);
            break;
        case RI_SONG_INVERTED_TIME:
            Flags_SetRandoInf(RANDO_INF_OBTAINED_SONG_INVERTED_TIME);
            break;

        // ComboShip: Goron Lullaby Intro. Its itemId (0x73) is outside the contiguous
        // ITEM_SONG_SONATA..SUN block the default case maps to quest items, so it needs its own flag.
        // ConvertItem(RI_PROGRESSIVE_LULLABY) returns the intro until QUEST_SONG_LULLABY_INTRO is set,
        // then the full lullaby — so without this flag the full lullaby is never granted and Snowhead
        // Temple (and the Moon) stay unreachable.
        case RI_SONG_LULLABY_INTRO:
            SET_QUEST_ITEM(QUEST_SONG_LULLABY_INTRO);
            break;

        // Clock items
        case RI_TIME_DAY_1:
        case RI_TIME_NIGHT_1:
        case RI_TIME_DAY_2:
        case RI_TIME_NIGHT_2:
        case RI_TIME_DAY_3:
        case RI_TIME_NIGHT_3: {
            int index = Rando::ClockItems::GetHalfDayIndexFromClockItem(ri);
            if (index != Rando::ClockItems::INVALID) {
                Flags_SetRandoInf(static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + index));
            }
            break;
        }
        case RI_TIME_PROGRESSIVE: {
            RandoItemId concrete = Rando::ConvertItem(RI_TIME_PROGRESSIVE);
            if (concrete != RI_JUNK)
                GiveItemForOracle(concrete);
            break;
        }

        // Souls
        case RI_SOUL_BOSS_GOHT:
        case RI_SOUL_BOSS_GYORG:
        case RI_SOUL_BOSS_MAJORA:
        case RI_SOUL_BOSS_ODOLWA:
        case RI_SOUL_BOSS_TWINMOLD:
            Flags_SetRandoInf(SOUL_RI_TO_RANDO_INF(ri));
            break;

        // Progressive items — convert then recurse
        case RI_PROGRESSIVE_MAGIC:
        case RI_PROGRESSIVE_BOW:
        case RI_PROGRESSIVE_BOMB_BAG:
        case RI_PROGRESSIVE_LULLABY:
        case RI_PROGRESSIVE_SWORD:
        case RI_PROGRESSIVE_WALLET:
            GiveItemForOracle(Rando::ConvertItem(ri));
            break;

        // Frogs
        case RI_FROG_BLUE:
            SET_WEEKEVENTREG(WEEKEVENTREG_33_01);
            break;
        case RI_FROG_CYAN:
            SET_WEEKEVENTREG(WEEKEVENTREG_32_40);
            break;
        case RI_FROG_PINK:
            SET_WEEKEVENTREG(WEEKEVENTREG_32_80);
            break;
        case RI_FROG_WHITE:
            SET_WEEKEVENTREG(WEEKEVENTREG_33_02);
            break;

        // GS tokens
        case RI_GS_TOKEN_SWAMP:
            SET_QUEST_ITEM(QUEST_QUIVER);
            Inventory_IncrementSkullTokenCount(SCENE_KINSTA1);
            break;
        case RI_GS_TOKEN_OCEAN:
            SET_QUEST_ITEM(QUEST_QUIVER);
            Inventory_IncrementSkullTokenCount(SCENE_KINDAN2);
            break;

        default: {
            // ComboShip: enemy souls (RI_SOUL_ENEMY_*) are flag-only items (itemId == ITEM_NONE), so
            // the inventory path below can't grant them; set their RANDO_INF like boss souls. Logic
            // gates on them (e.g. Great Bay Temple needs the Octorok soul to reverse the water flow),
            // so without this the temple never clears and the Moon stays locked. Mirrors GiveItem.cpp.
            if (ri >= RI_SOUL_ENEMY_ALIEN && ri <= RI_SOUL_ENEMY_WOLFOS) {
                Flags_SetRandoInf(SOUL_RI_TO_RANDO_INF(ri));
                break;
            }
            // Standard items: set the save-state that HAS_ITEM / CHECK_QUEST_ITEM read.
            // The expanded table explicitly maps only native inventory items and custom spells.
            // Songs/remains use quest flags; all other non-inventory IDs must stay SLOT_NONE.
            auto it = Rando::StaticData::Items.find(ri);
            if (it != Rando::StaticData::Items.end()) {
                u8 itemId = it->second.itemId;
                if (itemId >= ITEM_SONG_SONATA && itemId <= ITEM_SONG_SUN) {
                    SET_QUEST_ITEM(QUEST_SONG_SONATA + (itemId - ITEM_SONG_SONATA));
                } else if (itemId >= ITEM_REMAINS_ODOLWA && itemId <= ITEM_REMAINS_TWINMOLD) {
                    // ComboShip: boss remains are checked as quest items (RemainsCount() gates Moon
                    // access), not inventory slots, and their itemIds are out of gItemSlots range. Set
                    // the quest flag instead, else RemainsCount()==0 and the Moon trials are unreachable.
                    // ITEM_REMAINS_* and QUEST_REMAINS_* share the same order.
                    SET_QUEST_ITEM(QUEST_REMAINS_ODOLWA + (itemId - ITEM_REMAINS_ODOLWA));
                } else if (itemId != ITEM_NONE && itemId < ARRAY_COUNT(gItemSlots) && gItemSlots[itemId] != SLOT_NONE) {
                    INV_CONTENT(itemId) = itemId;
                }
            }
            break;
        }
    }
}

// Scoped search input only; live saves and inventory are never changed by this setter.
extern "C" COMBO_EXPORT int MM_SetOracleExternalBeanRenewal(uint8_t value) {
    if (!sMM_OracleActive || value > 1)
        return 0;
    sMM_OracleExternalBeans = value != 0;
    return 1;
}
extern "C" COMBO_EXPORT void Combo_MM_Rando_Reset(void) {
    sMM_OracleExternalBeans = false;
    // ComboShip: MM's region graph + static data are built by the eager boot
    // (MM_BootForCombo -> ShipInit::InitAll), so the oracle needs no lazy init here.
    if (!sMM_OracleActive) { // snapshot the REAL live context only on the first Reset of a fill
        memcpy(&sMM_OracleSavedContext, &gSaveContext, sizeof(SaveContext));
        sMM_OracleSavedRegionTime = gCurrentRegionTime;
        sMM_OracleSavedRegionalFlags = ComboRando::Entrances::mmFlagSource;
        sMM_OracleActive = true;
    }
    memset(&gSaveContext, 0, sizeof(SaveContext));
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHARED_OCARINA] =
        sMM_OracleSavedContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHARED_OCARINA];
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_FAIRY_OCARINA_MM] =
        sMM_OracleSavedContext.save.shipSaveInfo.rando.randoSaveOptions[RO_FAIRY_OCARINA_MM];
    // Zero is Fierce Deity in MM. A blank oracle must start as Human Link, not gain a free sword.
    gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
    gSaveContext.save.saveInfo.playerData.healthCapacity = 0x30;
    gSaveContext.save.saveInfo.playerData.health = 0x30;
    // Empty inventory slots are ITEM_NONE (0xFF), not 0: ITEM_OCARINA_OF_TIME is item id 0, so a
    // zeroed slot makes HAS_ITEM (an equality compare) report the ocarina owned and every song playable.
    memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE, sizeof(gSaveContext.save.saveInfo.inventory.items));
    if (!sMM_OracleInventorySweepDone) { // one-time guard against future zero-collision regressions
        sMM_OracleInventorySweepDone = true;
        for (auto& [id, item] : Rando::StaticData::Items) {
            u8 itemId = item.itemId;
            if (itemId != ITEM_NONE && itemId < ARRAY_COUNT(gItemSlots) && gItemSlots[itemId] != SLOT_NONE &&
                INV_CONTENT(itemId) == itemId) {
                SPDLOG_ERROR("MM oracle: empty context reports item {} owned; fill will over-reach", (int)itemId);
            }
        }
    }

    // ComboShip: the reachability logic reads RANDO_SAVE_OPTIONS (randoSaveOptions) and gates on
    // IS_RANDO (saveType == SAVETYPE_RANDO). The memset above wiped both, and nothing else repopulates
    // them in the headless oracle path. Without re-seeding here, reachability runs with every option at
    // 0 (dungeons closed, shuffles off), under-counts reachable checks, and the cross-world fill
    // dead-ends. Mirror the dump's CVar->options loop so reachability matches the generated pool.
    // (OOT's logic reads ctx->GetOption, which survives Reset; MM's reads gSaveContext, so it must be
    // re-seeded every query.)
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    gSaveContext.save.shipSaveInfo.rando.finalSeed = (u32)sMMComboRandoSeed; // clock-shuffle reseed source
    for (auto& [id, opt] : Rando::StaticData::Options) {
        gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[id] =
            (uint32_t)CVarGetInteger(opt.cvar, opt.defaultValue);
    }
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHARED_OCARINA] =
        (gMMComboSharedMask & (1u << ComboRando::SF_OCARINA)) != 0;
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHARED_SPELL_WIND] =
        (gMMComboSharedMask & (1u << ComboRando::SF_SPELL_WIND)) != 0;
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHARED_SPELL_FIRE] =
        (gMMComboSharedMask & (1u << ComboRando::SF_SPELL_FIRE)) != 0;
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHARED_SPELL_LOVE] =
        (gMMComboSharedMask & (1u << ComboRando::SF_SPELL_LOVE)) != 0;
    Combo_MM_ClearHiddenAdultOptions(gSaveContext.save.shipSaveInfo.rando.randoSaveOptions);
    // ComboShip (#136): combo owns the goal, so the hidden CVars above must not decide it here either.
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHUFFLE_TRIFORCE_PIECES] =
        gMMComboGoalHunt ? RO_GENERIC_YES : RO_GENERIC_NO;
    if (gMMComboGoalHunt) {
        gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_TRIFORCE_PIECES_REQUIRED] =
            (uint32_t)gMMComboGoalRequired;
        // #136: MM's half of the combined total; -1 = old seed, keep the CVar.
        if (gMMComboGoalPieces >= 0) {
            gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_TRIFORCE_PIECES_MAX] =
                (uint32_t)gMMComboGoalPieces;
        }
    }

    // Headless logic age has no model dependency. Full SaveContext snapshot/restore
    // above owns this state; the native runtime remains separately capability-gated.
    gSaveContext.save.linkAge = gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MM_STARTING_AGE] ? 0 : 1;
    gSaveContext.save.shipSaveInfo.adultMm.maskEnabled =
        gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MM_ADULT_MASK] != 0;
    gSaveContext.save.shipSaveInfo.adultMm.enabled =
        gSaveContext.save.shipSaveInfo.adultMm.maskEnabled ||
        gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MM_CROSS_AGE] != 0 || gSaveContext.save.linkAge == 0;

    // ComboShip: grant the seed's STARTING ITEMS into the oracle inventory. These aren't in the
    // shuffled pool (so SetOwnedItems never grants them), but logic depends on them: the default kit
    // (sword, shield, ocarina, Song of Time, plus computed buttons/swim/maps/souls). Without the
    // ocarina + buttons, CAN_PLAY_SONG is false, so no songs play and the oracle reports only the
    // song-free overworld reachable, dead-ending the fill. The real game uses GrantStartingItems(),
    // which needs gPlayState and is unsafe here, so reproduce it with the save-only GiveItemForOracle.
    // Must run after the option seeding above (GetComputedStartingItems reads randoSaveOptions); call
    // SetStartingItemsInSave first so its clock-shuffle branch sees the configured kit.
    {
        auto startingItems = Rando::GetStartingItemsFromConfig();
        Rando::SetStartingItemsInSave(gSaveContext.save.shipSaveInfo.rando, startingItems);
        auto computed = Rando::GetComputedStartingItems(gSaveContext.save.shipSaveInfo.rando);
        startingItems.insert(startingItems.begin(), computed.begin(), computed.end());
        for (RandoItemId si : startingItems) {
            GiveItemForOracle(Rando::ConvertItem(si));
        }
    }

    // The memset wiped check prices too; re-apply the rolled/spoiler set or every CAN_AFFORD gate
    // evaluates price==0 and shops/Tingle maps are free to the oracle.
    Combo_MM_ApplyCheckPrices();
    // Restore shelf shuffle identity erased by the oracle save reset. Native
    // potion events must not mistake randomized stock for its vanilla refill.
    for (const auto& [id, check] : Rando::StaticData::Checks) {
        if (check.randoCheckType == RCTYPE_SHOP && id != RC_CURIOSITY_SHOP_SPECIAL_ITEM && id != RC_BOMB_SHOP_ITEM_03 &&
            id != RC_BOMB_SHOP_ITEM_04_OR_CURIOSITY_SHOP_ITEM) {
            RANDO_SAVE_CHECKS[id].shuffled = RANDO_SAVE_OPTIONS[RO_SHUFFLE_SHOPS] != RO_GENERIC_NO;
        }
    }
    // GeneratePools leaves these shelves at their vanilla items when shops are
    // not shuffled. They still supply renewable ammo to the real game; clearing
    // them here must not erase those sources from generation's search.
    if (RANDO_SAVE_OPTIONS[RO_SHUFFLE_SHOPS] == RO_GENERIC_NO) {
        for (const auto& [id, check] : Rando::StaticData::Checks) {
            if (check.randoCheckType == RCTYPE_SHOP && id != RC_CURIOSITY_SHOP_SPECIAL_ITEM &&
                id != RC_BOMB_SHOP_ITEM_03 && id != RC_BOMB_SHOP_ITEM_04_OR_CURIOSITY_SHOP_ITEM) {
                RANDO_SAVE_CHECKS[id].randoItemId = check.randoItemId;
                RANDO_SAVE_CHECKS[id].price = 0;
            }
        }
    }
}

// ComboShip: name->id lookup maps for the oracle hot path, replacing per-name linear scans over
// StaticData that dominated fill time. StaticData is immutable after eager boot and the oracle runs
// single-threaded, so build-once function-local statics are safe.
// ComboShip: keyed on the FRIENDLY combo-spoiler names (GetItemDisplayName / GetCheckDisplayName), so
// the oracle, cross-item grant and price/apply paths all resolve the same normalized names the dump emits.
static const std::unordered_map<std::string, RandoItemId>& Combo_MM_SpoilerNameToItemId() {
    static const std::unordered_map<std::string, RandoItemId> map = [] {
        std::unordered_map<std::string, RandoItemId> m;
        for (auto& [id, item] : Rando::StaticData::Items) {
            const std::string& n = Rando::StaticData::GetItemDisplayName(id);
            if (!n.empty())
                m.emplace(n, id);
        }
        // Earlier .combosav placements and pinned OoTMM IDs remain resolvable
        // after changing only the human label from Time to Clock.
        const char* aliases[] = { "Time (Day 1)", "Time (Night 1)", "Time (Day 2)",     "Time (Night 2)",
                                  "Time (Day 3)", "Time (Night 3)", "Progressive Time", "MM_CLOCK1",
                                  "MM_CLOCK2",    "MM_CLOCK3",      "MM_CLOCK4",        "MM_CLOCK5",
                                  "MM_CLOCK6",    "MM_CLOCK" };
        for (const char* alias : aliases)
            m.emplace(alias, Rando::StaticData::GetItemIdFromName(alias));
        return m;
    }();
    return map;
}

static const std::unordered_map<std::string, RandoCheckId>& Combo_MM_CheckNameToCheckId() {
    static const std::unordered_map<std::string, RandoCheckId> map = [] {
        std::unordered_map<std::string, RandoCheckId> m;
        for (auto& [id, chk] : Rando::StaticData::Checks) {
            const std::string& n = Rando::StaticData::GetCheckDisplayName(id);
            if (!n.empty())
                m.emplace(n, id);
        }
        return m;
    }();
    return map;
}

// ComboShip: JSON array of MM rando checks the player has obtained, for the sphere-hint system.
// Reads RANDO_SAVE_CHECKS (in the MM save); safe to call while MM is dormant.
extern "C" COMBO_EXPORT const char* Combo_MM_GetObtainedChecks(void) {
    static std::string cached;
    nlohmann::json out = nlohmann::json::array();
    for (const auto& [name, id] : Combo_MM_CheckNameToCheckId()) {
        if (RANDO_SAVE_CHECKS[id].obtained)
            out.push_back(name);
    }
    cached = out.dump();
    return cached.c_str();
}

extern "C" COMBO_EXPORT void Combo_MM_Rando_SetOwnedItems(const char* itemNamesJson) {
    if (!itemNamesJson)
        return;
    try {
        auto items = nlohmann::json::parse(itemNamesJson);
        const auto& nameToId = Combo_MM_SpoilerNameToItemId();
        for (const auto& name : items) {
            auto it = nameToId.find(name.get<std::string>());
            if (it != nameToId.end()) {
                GiveItemForOracle(it->second);
            }
        }
    } catch (...) {}
}

// ComboShip: cross-game item delivery seam (issue #3). When the other game collects a check whose
// item belongs to MM, the launcher calls MM_GrantCrossItem to grant it straight into MM's resident
// save — even while MM is dormant. Delivers through the real give path (Rando::GiveItem), not the
// oracle, so capacity/ammo/multi-slot state is written faithfully; gComboDormantGive defers the
// play-dependent branches. Callers pass a concrete item (junk pre-resolved).
// True for the bottle-CONTENTS items, which are only obtainable with a free bottle.
static bool Combo_IsBottleRefill(RandoItemId rid) {
    switch (rid) {
        case RI_GOLD_DUST_REFILL:
        case RI_MILK_REFILL:
        case RI_CHATEAU_ROMANI_REFILL:
        case RI_FAIRY_REFILL:
        case RI_RED_POTION_REFILL:
        case RI_BLUE_POTION_REFILL:
        case RI_GREEN_POTION_REFILL:
            return true;
        default:
            return false;
    }
}

void Combo_MM_GiveDormantResolved(RandoItemId rid) {
    // ComboShip (#84): drop a bottle refill when no bottle is free. Callers convert first, so this is
    // a backstop — Item_Give's bottle-contents branch overwrites bottle #1. Keep it either way.
    if (Combo_IsBottleRefill(rid) && !Inventory_HasEmptyBottle()) {
        SPDLOG_INFO("[ComboShip] MM cross-grant: no empty bottle, dropping refill");
        return;
    }
    {
        // Scope-guard clears the flag even if GiveItem throws.
        struct FlagGuard {
            ~FlagGuard() {
                Rando::gComboDormantGive = false;
            }
        } flagGuard;
        Rando::gComboDormantGive = true;
        Rando::GiveItem(rid);
    }
    // ComboShip: rupees/magic land in transient accumulators (not in gSaveContext.save, applied on
    // the interface tick); flush them into the save so a dormant grant survives quitting before MM.
    if (gSaveContext.rupeeAccumulator != 0) {
        s16 total = gSaveContext.save.saveInfo.playerData.rupees + gSaveContext.rupeeAccumulator;
        gSaveContext.save.saveInfo.playerData.rupees = CLAMP(total, 0, (s16)CUR_CAPACITY(UPG_WALLET));
        gSaveContext.rupeeAccumulator = 0;
    }
    if (gSaveContext.magicToAdd != 0) {
        s16 total = gSaveContext.save.saveInfo.playerData.magic + gSaveContext.magicToAdd;
        gSaveContext.save.saveInfo.playerData.magic = CLAMP(total, 0, (s16)gSaveContext.magicCapacity);
        gSaveContext.magicToAdd = 0;
        gSaveContext.isMagicRequested = false;
    }
    if (gSaveContext.fileNum != 0xFF) {
        SaveManager_SaveCurrentForCombo(); // persist NOW
    }
}

extern "C" COMBO_EXPORT void MM_GrantCrossItem(const char* itemName) {
    if (!itemName)
        return;
    const auto& nameToId = Combo_MM_SpoilerNameToItemId();
    auto it = nameToId.find(itemName);
    if (it == nameToId.end()) {
        SPDLOG_WARN("[ComboShip] MM_GrantCrossItem: unknown MM item '{}'", itemName);
        return;
    }
    const RandoItemId placed = it->second;
    // ComboShip: convert like a native pickup does. MM's equipped shield value IS ownership, so an
    // already-owned item would otherwise downgrade it through vanilla Item_Give.
    RandoItemId rid = Rando::ConvertItem(placed);
    if (rid != placed) {
        SPDLOG_INFO("[ComboShip] MM_GrantCrossItem: '{}' not obtainable (already have / no slot), converted {} -> {}",
                    itemName, (int)placed, (int)rid);
    }
    // ComboShip: generation now bakes cross-placed junk into a real item, so this only catches an
    // older seed or a plando row that still names the placeholder. Red Rupee keeps those working.
    if (rid == RI_JUNK) {
        rid = RI_RUPEE_RED;
    }
    Combo_MM_GiveDormantResolved(rid);
    SPDLOG_INFO("[ComboShip] MM_GrantCrossItem: granted '{}' into MM save", itemName);
}

// ComboShip: mark a foreign MM check obtained without re-delivering — used on the NETWORK receive
// path so a client that gets a teammate's broadcast won't later physically collect the same check
// and double-deliver. Save-only (no grant), persisted immediately.
extern "C" COMBO_EXPORT void MM_MarkForeignObtained(const char* checkName) {
    if (!checkName)
        return;
    const auto& nameToId = Combo_MM_CheckNameToCheckId();
    auto it = nameToId.find(checkName);
    if (it == nameToId.end()) {
        SPDLOG_WARN("[ComboShip] MM_MarkForeignObtained: unknown MM check '{}'", checkName);
        return;
    }
    RANDO_SAVE_CHECKS[it->second].obtained = true;
    RANDO_SAVE_CHECKS[it->second].cycleObtained = true;
    RANDO_SAVE_CHECKS[it->second].eligible = false;
    if (gSaveContext.fileNum != 0xFF) {
        SaveManager_SaveCurrentForCombo();
    }
    SPDLOG_INFO("[ComboShip] MM_MarkForeignObtained: marked MM check '{}' collected", checkName);
}

// ComboShip: routing seams — the launcher registers DeliverCrossItem / MarkForeignObtained here so
// MM's foreign-check detection can hand an item to the OTHER game immediately (mirrors MM_SetAnchorSend).
extern "C" void (*gMMComboCrossDeliver)(int targetGame, const char* itemName, const char* srcCheckName) = nullptr;
extern "C" COMBO_EXPORT void MM_SetCrossDeliver(void (*cb)(int, const char*, const char*)) {
    gMMComboCrossDeliver = cb;
}
extern "C" void (*gMMComboMarkForeignObtained)(int srcGame, const char* checkName) = nullptr;
extern "C" COMBO_EXPORT void MM_SetMarkForeignObtained(void (*cb)(int, const char*)) {
    gMMComboMarkForeignObtained = cb;
}
// ComboShip (#164): combo Hint Tracker reveal sink. kind: 0 = cross gossipPool pick (poolIndex),
// 1 = native MM stone hint (key = check name, text = plain hint), 2 = NPC itemLocations hint (key = item).
extern "C" void (*gMMComboHintReveal)(int fileNum, int kind, int poolIndex, const char* key,
                                      const char* text) = nullptr;
extern "C" COMBO_EXPORT void MM_SetComboHintRevealCb(void (*cb)(int, int, int, const char*, const char*)) {
    gMMComboHintReveal = cb;
}
// ComboShip: end-gating seam (mirrors OOT). z_boss_07.c calls gComboFinalBossDefeated when Majora dies.
extern "C" int (*gComboFinalBossDefeated)(int game, int fileNum) = nullptr;
extern "C" COMBO_EXPORT void MM_SetFinalBossDefeatedCb(int (*cb)(int, int)) {
    gComboFinalBossDefeated = cb;
}

// ComboShip (#136): Triforce Hunt is ONE combined goal across both games, owned by the launcher.
// hunt=0 means the normal both-bosses goal; required is the combined piece count.
extern "C" int gMMComboGoalHunt = 0;
extern "C" int gMMComboGoalType = 0;
extern "C" int gMMComboGoalRequired = 0;
// This game's share of the combined piece total, forced at every save-option build site.
// -1 = unset (old seed), so MM's own slider decides.
extern "C" int gMMComboGoalPieces = -1;
extern "C" COMBO_EXPORT void MM_SetComboGoal(int hunt, int required, int pieces) {
    gMMComboGoalHunt = hunt ? 1 : 0;
    gMMComboGoalType = gMMComboGoalHunt;
    gMMComboGoalRequired = gMMComboGoalHunt ? required : 0;
    gMMComboGoalPieces = pieces < 0 ? -1 : (pieces > 100 ? 100 : pieces); // same 0..100 cap as OOT's
}
extern "C" COMBO_EXPORT void MM_SetComboGoalType(int type, int required, int pieces) {
    MM_SetComboGoal(type == 1, required, pieces);
    gMMComboGoalType = std::clamp(type, 0, 5);
}
// ComboShip: nothing when no rando save is loaded (SaveManager_MarkNoSaveLoaded after a failed load), so
// the combined Triforce overlay never counts the previous slot's MM pieces.
static bool Combo_MM_RandoSaveLoaded() {
    return gSaveContext.fileNum != 0xFF && gSaveContext.save.shipSaveInfo.saveType == SAVETYPE_RANDO;
}
extern "C" COMBO_EXPORT int MM_GetNamedTriforceMask(void) {
    return Combo_MM_RandoSaveLoaded() ? gSaveContext.save.shipSaveInfo.rando.namedTriforceMask : 0;
}
// ComboShip: Shared Items (OoTMM-style) — see combo/rando/SharedItems.h. MM has no gen-time settings
// that depend on the mask (only OOT's wallet force does), so only the runtime tier ABI lives here.
// gMMComboSharedMask mirrors the loaded slot's effective mask (SOH_SetComboSharedItems's MM twin) —
// vendored pokes (Bomb Bag / Bombchu Bag) read it through Combo_MM_BombchuBagShared so they never
// need the family header.
extern "C" int gMMComboSharedMask = 0;
extern "C" COMBO_EXPORT void MM_SetComboSharedItems(uint32_t mask) {
    gMMComboSharedMask = static_cast<int>(mask);
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHARED_OCARINA] =
        (mask & (1u << ComboRando::SF_OCARINA)) != 0;
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHARED_SPELL_WIND] =
        (mask & (1u << ComboRando::SF_SPELL_WIND)) != 0;
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHARED_SPELL_FIRE] =
        (mask & (1u << ComboRando::SF_SPELL_FIRE)) != 0;
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHARED_SPELL_LOVE] =
        (mask & (1u << ComboRando::SF_SPELL_LOVE)) != 0;
}
extern "C" int Combo_MM_BombchuBagShared(void) {
    return (gMMComboSharedMask >> ComboRando::SF_BOMBCHU_BAG) & 1;
}

extern "C" COMBO_EXPORT int MM_GetSharedTier(int family) try {
    if (family < 0 || family >= ComboRando::SF_COUNT)
        return 0;
    switch (static_cast<ComboRando::SharedFamily>(family)) {
        case ComboRando::SF_BOOTS_IRON:
            return ComboAdultGearInventory::Has(gSaveContext.save.shipSaveInfo.adultMm.gearTrade2Bits,
                                                ITEM_COMBO_BOOTS_IRON);
        case ComboRando::SF_BOOTS_HOVER:
            return ComboAdultGearInventory::Has(gSaveContext.save.shipSaveInfo.adultMm.gearTrade3Bits,
                                                ITEM_COMBO_BOOTS_HOVER);
        case ComboRando::SF_TUNIC_GORON:
            return ComboAdultGearInventory::Has(gSaveContext.save.shipSaveInfo.adultMm.gearTrade2Bits,
                                                ITEM_COMBO_TUNIC_GORON);
        case ComboRando::SF_TUNIC_ZORA:
            return ComboAdultGearInventory::Has(gSaveContext.save.shipSaveInfo.adultMm.gearTrade3Bits,
                                                ITEM_COMBO_TUNIC_ZORA);
        case ComboRando::SF_SPELL_WIND:
            return gSaveContext.save.shipSaveInfo.faroresWind.owned != 0;
        case ComboRando::SF_SPELL_FIRE:
            return gSaveContext.save.shipSaveInfo.spellFireOwned != 0;
        case ComboRando::SF_SPELL_LOVE:
            return gSaveContext.save.shipSaveInfo.spellLoveOwned != 0;
        case ComboRando::SF_OCARINA:
            return INV_CONTENT(ITEM_OCARINA_OF_TIME) == ITEM_OCARINA_OF_TIME
                       ? 2
                       : (INV_CONTENT(ITEM_OCARINA_OF_TIME) == ITEM_OCARINA_FAIRY ? 1 : 0);
        case ComboRando::SF_BOW:
            return CUR_UPG_VALUE(UPG_QUIVER);
        case ComboRando::SF_BOMB_BAG:
            return CUR_UPG_VALUE(UPG_BOMB_BAG);
        case ComboRando::SF_BOMBCHU_BAG:
            return INV_CONTENT(ITEM_BOMBCHU) != ITEM_NONE ? 1 : 0;
        case ComboRando::SF_MAGIC:
            return gSaveContext.save.saveInfo.playerData.isMagicAcquired +
                   gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired;
        case ComboRando::SF_WALLET:
            return CUR_UPG_VALUE(UPG_WALLET);
        case ComboRando::SF_HOOKSHOT:
            return INV_CONTENT(ITEM_HOOKSHOT) != ITEM_NONE ? 1 : 0;
        case ComboRando::SF_FIRE_ARROWS:
            return INV_CONTENT(ITEM_ARROW_FIRE) != ITEM_NONE ? 1 : 0;
        case ComboRando::SF_ICE_ARROWS:
            return INV_CONTENT(ITEM_ARROW_ICE) != ITEM_NONE ? 1 : 0;
        case ComboRando::SF_LIGHT_ARROWS:
            return INV_CONTENT(ITEM_ARROW_LIGHT) != ITEM_NONE ? 1 : 0;
        case ComboRando::SF_LENS:
            return INV_CONTENT(ITEM_LENS_OF_TRUTH) != ITEM_NONE ? 1 : 0;
        case ComboRando::SF_EPONAS_SONG:
            return CHECK_QUEST_ITEM(QUEST_SONG_EPONA) ? 1 : 0;
        case ComboRando::SF_SONG_OF_STORMS:
            return CHECK_QUEST_ITEM(QUEST_SONG_STORMS) ? 1 : 0;
        case ComboRando::SF_GORON_MASK:
            return INV_CONTENT(ITEM_MASK_GORON) != ITEM_NONE ? 1 : 0;
        case ComboRando::SF_ZORA_MASK:
            return INV_CONTENT(ITEM_MASK_ZORA) != ITEM_NONE ? 1 : 0;
        case ComboRando::SF_KEATON_MASK:
            return INV_CONTENT(ITEM_MASK_KEATON) != ITEM_NONE ? 1 : 0;
        case ComboRando::SF_BUNNY_HOOD:
            return INV_CONTENT(ITEM_MASK_BUNNY) != ITEM_NONE ? 1 : 0;
        case ComboRando::SF_MASK_OF_TRUTH:
            return INV_CONTENT(ITEM_MASK_TRUTH) != ITEM_NONE ? 1 : 0;
        case ComboRando::SF_OCARINA_BUTTON_A:
            return Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_A) ? 1 : 0;
        case ComboRando::SF_OCARINA_BUTTON_C_DOWN:
            return Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_DOWN) ? 1 : 0;
        case ComboRando::SF_OCARINA_BUTTON_C_RIGHT:
            return Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT) ? 1 : 0;
        case ComboRando::SF_OCARINA_BUTTON_C_LEFT:
            return Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_LEFT) ? 1 : 0;
        case ComboRando::SF_OCARINA_BUTTON_C_UP:
            return Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_UP) ? 1 : 0;
        case ComboRando::SF_SONG_OF_TIME:
            return CHECK_QUEST_ITEM(QUEST_SONG_TIME) ? 1 : 0;
        case ComboRando::SF_MIRROR_SHIELD:
            return GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) == EQUIP_VALUE_SHIELD_MIRROR ? 1 : 0;
        default:
            return 0;
    }
} catch (const std::exception& e) {
    SPDLOG_ERROR("[ComboShip] MM_GetSharedTier threw: {}", e.what());
    return 0;
} catch (...) {
    SPDLOG_ERROR("[ComboShip] MM_GetSharedTier threw a non-std exception");
    return 0;
}

extern "C" COMBO_EXPORT void MM_RaiseSharedTier(int family, int tier) try {
    if (family < 0 || family >= ComboRando::SF_COUNT)
        return;
    if (family == ComboRando::SF_OCARINA) {
        if (tier <= MM_GetSharedTier(family))
            return;
        if (tier == 1 && !gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_FAIRY_OCARINA_MM])
            return;
        const RandoItemId rid = tier >= 2 ? RI_OCARINA : RI_OCARINA_FAIRY;
        if (gPlayState == NULL)
            Combo_MM_GiveDormantResolved(rid);
        else {
            Rando::GiveItem(rid);
            SaveManager_SaveCurrentForCombo();
        }
        return;
    }
    const auto& def = ComboRando::SharedFamilyByIndex(family);
    tier = std::min(tier, def.mmTierCap);
    for (;;) {
        const int cur = MM_GetSharedTier(family);
        if (cur >= tier)
            return;
        // SF_BOMBCHU_BAG has no RandoItemId (MM has no bombchu-bag item) — poke the save directly.
        if (family == ComboRando::SF_BOMBCHU_BAG) {
            INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
            AMMO(ITEM_BOMBCHU) = CUR_CAPACITY(UPG_BOMB_BAG);
            if (gSaveContext.fileNum != 0xFF)
                SaveManager_SaveCurrentForCombo(); // persist dormant grants too (gPlayState is NULL then)
            if (MM_GetSharedTier(family) <= cur)
                return; // didn't raise — stop instead of looping forever
            continue;
        }
        RandoItemId base = RI_JUNK;
        switch (static_cast<ComboRando::SharedFamily>(family)) {
            case ComboRando::SF_BOOTS_IRON:
                base = RI_BOOTS_IRON;
                break;
            case ComboRando::SF_BOOTS_HOVER:
                base = RI_BOOTS_HOVER;
                break;
            case ComboRando::SF_TUNIC_GORON:
                base = RI_TUNIC_GORON;
                break;
            case ComboRando::SF_TUNIC_ZORA:
                base = RI_TUNIC_ZORA;
                break;
            case ComboRando::SF_BOW:
                base = RI_PROGRESSIVE_BOW;
                break;
            case ComboRando::SF_BOMB_BAG:
                base = RI_PROGRESSIVE_BOMB_BAG;
                break;
            case ComboRando::SF_MAGIC:
                base = RI_PROGRESSIVE_MAGIC;
                break;
            case ComboRando::SF_WALLET:
                base = RI_PROGRESSIVE_WALLET;
                break;
            case ComboRando::SF_HOOKSHOT:
                base = RI_HOOKSHOT;
                break;
            case ComboRando::SF_FIRE_ARROWS:
                base = RI_ARROW_FIRE;
                break;
            case ComboRando::SF_ICE_ARROWS:
                base = RI_ARROW_ICE;
                break;
            case ComboRando::SF_LIGHT_ARROWS:
                base = RI_ARROW_LIGHT;
                break;
            case ComboRando::SF_LENS:
                base = RI_LENS;
                break;
            case ComboRando::SF_EPONAS_SONG:
                base = RI_SONG_EPONA;
                break;
            case ComboRando::SF_SONG_OF_STORMS:
                base = RI_SONG_STORMS;
                break;
            case ComboRando::SF_GORON_MASK:
                base = RI_MASK_GORON;
                break;
            case ComboRando::SF_ZORA_MASK:
                base = RI_MASK_ZORA;
                break;
            case ComboRando::SF_KEATON_MASK:
                base = RI_MASK_KEATON;
                break;
            case ComboRando::SF_BUNNY_HOOD:
                base = RI_MASK_BUNNY;
                break;
            case ComboRando::SF_MASK_OF_TRUTH:
                base = RI_MASK_TRUTH;
                break;
            case ComboRando::SF_OCARINA_BUTTON_A:
                base = RI_OCARINA_BUTTON_A;
                break;
            case ComboRando::SF_OCARINA_BUTTON_C_DOWN:
                base = RI_OCARINA_BUTTON_C_DOWN;
                break;
            case ComboRando::SF_OCARINA_BUTTON_C_RIGHT:
                base = RI_OCARINA_BUTTON_C_RIGHT;
                break;
            case ComboRando::SF_OCARINA_BUTTON_C_LEFT:
                base = RI_OCARINA_BUTTON_C_LEFT;
                break;
            case ComboRando::SF_OCARINA_BUTTON_C_UP:
                base = RI_OCARINA_BUTTON_C_UP;
                break;
            case ComboRando::SF_SONG_OF_TIME:
                base = RI_SONG_TIME;
                break;
            case ComboRando::SF_MIRROR_SHIELD:
                base = RI_SHIELD_MIRROR;
                break;
            case ComboRando::SF_SPELL_WIND:
                base = RI_FARORES_WIND;
                break;
            case ComboRando::SF_SPELL_FIRE:
                base = RI_DINS_FIRE;
                break;
            case ComboRando::SF_SPELL_LOVE:
                base = RI_NAYRUS_LOVE;
                break;
            default:
                break;
        }
        // Default check id (RC_UNKNOWN) is safe here: none of these families gate on hasObtainedCheck.
        RandoItemId rid = Rando::ConvertItem(base);
        if (rid == RI_JUNK)
            return; // never substitutes junk — stop instead of granting the wrong thing
        if (gPlayState == NULL) {
            Combo_MM_GiveDormantResolved(rid);
        } else {
            Rando::GiveItem(rid);
            SaveManager_SaveCurrentForCombo();
        }
        if (MM_GetSharedTier(family) <= cur)
            return; // didn't raise — stop instead of looping forever
    }
} catch (const std::exception& e) { SPDLOG_ERROR("[ComboShip] MM_RaiseSharedTier threw: {}", e.what()); } catch (...) {
    SPDLOG_ERROR("[ComboShip] MM_RaiseSharedTier threw a non-std exception");
}

// Shared Items pokes: same shape as the #136 Triforce callbacks.
extern "C" void (*gMMComboSharedChanged)(int game, int fileNum) = nullptr;
extern "C" COMBO_EXPORT void MM_SetSharedChangedCb(void (*cb)(int, int)) {
    gMMComboSharedChanged = cb;
}
extern "C" void (*gMMComboSharedTick)(void) = nullptr;
extern "C" COMBO_EXPORT void MM_SetSharedTickCb(void (*cb)(void)) {
    gMMComboSharedTick = cb;
}

extern "C" COMBO_EXPORT int MM_GetTriforcePieceCount(void) {
    return Combo_MM_RandoSaveLoaded() ? gSaveContext.save.shipSaveInfo.rando.foundTriforcePieces : 0;
}
// The OTHER game's piece count, so pickup messages can show the combined progress.
extern "C" int (*gMMComboOtherTriforceCount)(void) = nullptr;
extern "C" COMBO_EXPORT void MM_SetOtherTriforceCountCb(int (*cb)(void)) {
    gMMComboOtherTriforceCount = cb;
}
// Poked after every piece grant (active or dormant); the launcher evaluates the combined total.
extern "C" void (*gMMComboTriforceProgress)(int game, int fileNum) = nullptr;
extern "C" COMBO_EXPORT void MM_SetTriforceProgressCb(void (*cb)(int, int)) {
    gMMComboTriforceProgress = cb;
}
// Goal reached: soul grant + completion hook run either way; only the active game gets the ending, a
// dormant MM persists. The save can throw, and the launcher calls this — nothing may cross the C-ABI.
extern "C" COMBO_EXPORT void MM_TriggerTriforceCredits(int dormant) try {
    if (!Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_MAJORA)) {
        Rando::GiveItem(RI_SOUL_BOSS_MAJORA);
    }
    GameInteractor_ExecuteOnGameCompletion();
    if (dormant) {
        if (gSaveContext.fileNum != 0xFF) {
            SaveManager_SaveCurrentForCombo();
        }
        return;
    }
    GameInteractor::Instance->events.emplace_back(GIEventTransition{ .entrance = ENTRANCE(TERMINA_FIELD, 0),
                                                                     .cutsceneIndex = 0xFFF7,
                                                                     .transitionTrigger = TRANS_TRIGGER_START,
                                                                     .transitionType = TRANS_TYPE_FADE_BLACK });
} catch (const std::exception& e) {
    SPDLOG_ERROR("[ComboShip] MM_TriggerTriforceCredits threw: {}", e.what());
} catch (...) { SPDLOG_ERROR("[ComboShip] MM_TriggerTriforceCredits threw a non-std exception"); }

#include "rando/SongTravelMM.h"
#include "rando/ForeignSongs.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include <atomic>
static std::atomic<int> sForeignSongRequest{ -1 }, sForeignSongReply{ -2 };
static std::atomic<bool> sForeignSongPerformed{ false };
static bool sNativeSongOpen = false;
static int sNativeSongSource = -1;
static std::string sNativeSongText;

extern "C" COMBO_EXPORT void MM_SetSongTravel(const char* json) {
    sForeignSongRequest = -1;
    sForeignSongReply = -2;
    sForeignSongPerformed = false;
    sNativeSongOpen = false;
    sNativeSongSource = -1;
    try {
        ComboRando::ActiveSongTravel() = ComboRando::SongTravelMap::parse(nlohmann::json::parse(json));
    } catch (...) { ComboRando::ActiveSongTravel() = {}; }
    ComboRando::MmSongArrivals() = 0;
}
static uint8_t sAdultOracleEntryAges = 0, sAdultOracleReachedAges = 0;
extern "C" COMBO_EXPORT void MM_SetSongArrivals(uint32_t mask) {
    ComboRando::MmSongArrivals() = mask & 0xffc0;
    ComboRando::MmSongAgeArrivals() = {};
    sAdultOracleEntryAges = 0;
}
extern "C" COMBO_EXPORT uint32_t MM_GetSongKnowledge() {
    return CHECK_QUEST_ITEM(QUEST_SONG_SOARING)
               ? uint32_t(gSaveContext.save.saveInfo.playerData.owlActivationFlags & 1023) << 6
               : 0;
}
extern "C" COMBO_EXPORT uint32_t MM_GetLiveSongKnowledge() {
    return IS_RANDO ? MM_GetSongKnowledge() : 0;
}
// Song ownership and unlocked landing sites are different facts. A learned
// Soaring song still has its normal performance when no owl has been unlocked.
extern "C" COMBO_EXPORT int MM_HasLiveSoaringSong() {
    return IS_RANDO && CHECK_QUEST_ITEM(QUEST_SONG_SOARING);
}
extern "C" COMBO_EXPORT void MM_SetForeignSongKnowledge(uint32_t mask) {
    ComboRando::MmForeignKnowledge() = mask;
}
static uint32_t MmForeignWarpSources(uint32_t known, bool logical = false) {
    uint32_t buttons = 0;
    const RandoInf infs[] = { RANDO_INF_OBTAINED_OCARINA_BUTTON_A, RANDO_INF_OBTAINED_OCARINA_BUTTON_C_DOWN,
                              RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT, RANDO_INF_OBTAINED_OCARINA_BUTTON_C_LEFT,
                              RANDO_INF_OBTAINED_OCARINA_BUTTON_C_UP };
    for (int i = 0; i < 5; i++)
        if (Flags_GetRandoInf(infs[i]))
            buttons |= 1u << i;
    return ComboRando::ForeignWarpSources(logical ? ComboRando::ActiveSongTravel().logicForeignSongs
                                                  : ComboRando::ActiveSongTravel().foreignSongs,
                                          known, HAS_PLAYABLE_OCARINA, buttons);
}
extern "C" COMBO_EXPORT uint32_t MM_GetSongSources() {
    uint32_t native = CAN_PLAY_SONG(SOARING) ? MM_GetSongKnowledge() : 0;
    return native | MmForeignWarpSources(ComboRando::MmForeignKnowledge(), true);
}
static uint32_t MmLiveSongSources(bool logical) {
    if (!IS_RANDO)
        return 0;
    auto fn = (uint32_t(*)())Combo_ResolveSym("soh", "SOH_GetLiveSongKnowledge");
    return (CAN_PLAY_SONG(SOARING) ? MM_GetSongKnowledge() : 0) | (fn ? MmForeignWarpSources(fn(), logical) : 0);
}

extern "C" COMBO_EXPORT uint32_t MM_GetLiveSongSources() {
    return MmLiveSongSources(false);
}
extern "C" COMBO_EXPORT uint32_t MM_GetLiveSongLogicSources() {
    return MmLiveSongSources(true);
}
extern "C" COMBO_EXPORT uint32_t MM_ForeignSongChoices() {
    int request = sForeignSongRequest.load();
    if (request < 0)
        return 0;
    return (MM_GetLiveSongSources() & (1u << request));
}
extern "C" COMBO_EXPORT int MM_ForeignSongDestination(int source) {
    return ComboRando::ActiveSongTravel().destination(source);
}
extern "C" int Combo_MMRecognizeForeignSong(const uint8_t* pitches, int count) {
    if (!IS_RANDO || !gPlayState || !ComboRando::ActiveSongTravel().foreignSongs ||
        gPlayState->msgCtx.ocarinaAction != OCARINA_ACTION_FREE_PLAY ||
        gPlayState->interfaceCtx.restrictions.songOfSoaring || sForeignSongRequest.load() >= 0)
        return 0;
    uint32_t allowed = MM_GetLiveSongSources() & 63;
    int song = ComboRando::RecognizeForeignSong(pitches, count, allowed);
    if (song < 0)
        return 0;
    sForeignSongReply = -2;
    sForeignSongPerformed = false;
    sForeignSongRequest = song;
    return 1;
}
extern "C" int Combo_MMForeignSongPerformanceActive() {
    return sForeignSongRequest.load() >= 0 && !sForeignSongPerformed.load();
}
extern "C" int Combo_MMForeignSongPlayback() {
    return sForeignSongRequest.load();
}
extern "C" void Combo_MMForeignSongPerformanceFinished() {
    sForeignSongPerformed = true;
}
static bool LoadOOTWarpMessage(int song, bool confirmation, CustomMessage::Entry& entry) {
    auto load = (int (*)(int, int, char*, int, int*))Combo_ResolveSym("soh", "SOH_GetWarpMessage");
    char source[1024];
    int type = 0;
    const int size = load ? load(song, confirmation, source, sizeof(source), &type) : 0;
    if (size <= 0)
        return false;
    std::string text;
    // The engines use different text control bytes. Preserve the native OoT
    // message's wording, spacing, colors and choices without interpreting it as MM text.
    static constexpr unsigned char colors[] = { 0, 1, 2, 3, 5, 6, 4, 7 };
    for (int i = 0; i < size; i++) {
        const auto ch = static_cast<unsigned char>(source[i]);
        switch (ch) {
            case 0x01:
                text.push_back(0x11);
                break; // newline
            case 0x02:
                text.push_back(char(0xBF));
                i = size;
                break; // end
            case 0x04:
                text.push_back(0x10);
                break; // box break
            case 0x05: // color
                if (++i >= size || static_cast<unsigned char>(source[i]) < 0x40 ||
                    static_cast<unsigned char>(source[i]) > 0x47)
                    return false;
                text.push_back(char(colors[static_cast<unsigned char>(source[i]) - 0x40]));
                break;
            case 0x06: // horizontal shift
                if (++i >= size)
                    return false;
                text.push_back(0x14);
                text.push_back(source[i]);
                break;
            case 0x08:
                text.push_back(0x17);
                break; // quick text
            case 0x09:
                text.push_back(0x18);
                break;
            case 0x0A:
                text.push_back(0x1A);
                break;
            case 0x0B:
                text.push_back(0x19);
                break;
            case 0x0E: // fade
                if (++i >= size)
                    return false;
                text.push_back(0x1C);
                text.push_back(source[i]);
                break;
            case 0x1A:
                break; // native MM messages cannot be skipped by default
            case 0x1B:
                text.push_back(char(0xC2));
                break; // two choices
            default:
                if (ch < 0x20)
                    return false;
                text.push_back(source[i]);
                break;
        }
    }
    entry.msg = std::move(text);
    entry.textboxType = type;
    entry.autoFormat = false;
    return true;
}
extern "C" void Combo_MMForeignSongPlayedText() {
    static constexpr const char* names[] = { "Minuet of Forest",  "Bolero of Fire",     "Serenade of Water",
                                             "Requiem of Spirit", "Nocturne of Shadow", "Prelude of Light" };
    int song = sForeignSongRequest.load();
    if (song < 0 || song >= 6 || !gPlayState)
        return;
    CustomMessage::Entry native;
    if (!LoadOOTWarpMessage(song, false, native)) {
        native = CustomMessage::LoadVanillaMessageTableEntry(0x1B7B);
        native.autoFormat = false;
        CustomMessage::Replace(&native.msg, "Song of Soaring", names[song]);
    }
    CustomMessage::SetActiveCustomMessage(native.msg, native);
    Message_ContinueTextbox(gPlayState, CUSTOM_MESSAGE_ID);
}
static void OpenNativeSongDialogue() {
    const int dest = ComboRando::ActiveSongTravel().destination(sNativeSongSource);
    sNativeSongOpen = true;
    AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
    CustomMessage::Entry native;
    if (dest != sForeignSongRequest.load() || !LoadOOTWarpMessage(sForeignSongRequest.load(), true, native)) {
        native = CustomMessage::LoadVanillaMessageTableEntry(0x1B93);
        native.autoFormat = false;
        CustomMessage::Replace(&native.msg, std::string(1, char(0xD4)), ComboRando::kSongLandingNames[dest]);
    }
    CustomMessage::StartTextbox(native.msg, native);
    gPlayState->msgCtx.ocarinaMode = 1;
}
static void UpdateNativeSongDialogue() {
    if (sForeignSongRequest.load() < 0 || !gPlayState || !sForeignSongPerformed.load())
        return;
    auto& input = *CONTROLLER1(&gPlayState->state);
    uint32_t choices = MM_ForeignSongChoices();
    if (!choices || (input.press.button & BTN_B)) {
        sForeignSongReply = -1;
        return;
    }
    if (!sNativeSongOpen) {
        // Do not let the final played note accept the confirmation, even with instant text.
        if (input.cur.button & (BTN_A | BTN_CUP | BTN_CDOWN | BTN_CLEFT | BTN_CRIGHT))
            return;
        for (int i = 0; i < 16; i++)
            if (choices & (1u << i)) {
                sNativeSongSource = i;
                break;
            }
        OpenNativeSongDialogue();
        return;
    }
    if (Message_GetState(&gPlayState->msgCtx) != TEXT_STATE_CHOICE)
        return;
    if (input.press.button & BTN_A) {
        Message_ShouldAdvance(gPlayState); // native selection sound
        sForeignSongReply = gPlayState->msgCtx.choiceIndex == 0 ? sNativeSongSource : -1;
        input.press.button &= ~BTN_A; // the engine must not also launch its own warp
    }
}
extern "C" int Combo_MMStartWarp(PlayState*, int, int);
static void ProcessForeignSongReply() {
    UpdateNativeSongDialogue();
    int reply = sForeignSongReply.exchange(-2);
    if (reply == -2)
        return;
    const uint32_t choices = MM_ForeignSongChoices();
    const int song = sForeignSongRequest.load();
    sForeignSongRequest = -1;
    sForeignSongPerformed = false;
    sNativeSongOpen = false;
    sNativeSongSource = -1;
    if (!gPlayState)
        return;
    AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
    Message_CloseTextbox(gPlayState);
    gPlayState->msgCtx.ocarinaMode = 4; // native ocarina END mode in both games
    if (reply < 0 || reply >= 16 || !(choices & (1u << reply)))
        return;
    const int dest = ComboRando::ActiveSongTravel().destination(reply);
    if (!Combo_MMStartWarp(gPlayState, dest, song))
        SPDLOG_ERROR("[ComboShip] Could not start OoT warp departure");
}

extern "C" uint32_t MM_LiveIncomingSongs() {
    if (Rando::Logic::evaluatingLiveTracker) {
        // The tracker shows what the active options allow now, including gameplay-only cross-game songs.
        auto fn = (uint32_t (*)())Combo_ResolveSym("soh", "SOH_GetLiveSongSources");
        return ComboRando::ActiveSongTravel().resolve((fn ? fn() : 0) | MM_GetLiveSongSources()) & 0xffc0;
    }
    auto fn = (uint32_t(*)())Combo_ResolveSym("soh", "SOH_GetLiveSongLogicSources");
    return ComboRando::ActiveSongTravel().resolveLogic((fn ? fn() : 0) | MM_GetLiveSongLogicSources()) & 0xffc0;
}
extern "C" COMBO_EXPORT int MM_TakeSongDeparture() {
    int d = sSongDeparture;
    sSongDeparture = -1;
    return d;
}
extern "C" COMBO_EXPORT void MM_SetSongArrival(int d) {
    sSongArrival = d >= 6 && d < 16 ? d : -1;
}
extern "C" int MM_ConsumeSongArrival() {
    const int d = sSongArrival;
    sSongArrival = -1;
    if (d < 6 || d >= 16)
        return -1;
    // A song landing uses the owl entrance's native arrival, not the idle
    // start mode that Player_Init selects when resuming a saved location.
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance = -1;
    return ComboRando::kMmSongEntrances[d - 6];
}
extern "C" void Combo_MMCompleteWarp(PlayState* play, int d) {
    if (d < 0 || d >= 16)
        return;
    if (d < 6) {
        sSongDeparture = d;
        play->transitionTrigger = TRANS_TRIGGER_OFF;
    } else {
        play->nextEntrance = ComboRando::kMmSongEntrances[d - 6];
        if (d == 13 && CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE))
            play->nextEntrance = ENTRANCE(SOUTHERN_SWAMP_CLEARED, 10);
        if (d == 9 && CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE))
            play->nextEntrance = ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 8);
        play->transitionTrigger = TRANS_TRIGGER_START;
        play->transitionType = TRANS_TYPE_FADE_BLACK;
    }
}
extern "C" int Combo_MMSongWarp(PlayState* play, int owl) {
    if (!IS_RANDO || !ComboRando::ActiveSongTravel().enabled || owl < 0 || owl >= 10)
        return 0;
    Combo_MMCompleteWarp(play, ComboRando::ActiveSongTravel().destination(owl + 6));
    return 1;
}
extern "C" COMBO_EXPORT void MM_SetOracleEntryAges(uint8_t mask) {
    sAdultOracleEntryAges = mask & 3;
}
extern "C" COMBO_EXPORT const char* Combo_MM_Rando_GetReachableChecks(void) {
    if (sMM_OracleExternalBeans && INV_CONTENT(ITEM_MAGIC_BEANS) == ITEM_MAGIC_BEANS)
        RANDO_EVENTS[RE_ACCESS_BEANS_REFILL] = true;
    ComboRando::MmSongOracle() = true;
    static std::string buf;
    struct RestoreOracleAge {
        u8 age;
        ~RestoreOracleAge() {
            gSaveContext.save.linkAge = age;
        }
    } restoreAge{ gSaveContext.save.linkAge };
    const bool cross = gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MM_CROSS_AGE] != 0;
    const bool clone = cross && (gSaveContext.save.shipSaveInfo.adultMm.maskOwnedBits & 2);
    uint8_t ages = (cross && sAdultOracleEntryAges)
                       ? sAdultOracleEntryAges
                       : (gSaveContext.save.linkAge <= 1 ? uint8_t(1u << gSaveContext.save.linkAge) : uint8_t(0));
    if (clone && ages)
        ages = 3;
    sAdultOracleReachedAges = ages;
    using Times = decltype(Rando::Logic::InitializeRegionTimeStates(RR_MAX));
    struct Frontier {
        std::set<RandoRegionId> reachable;
        Times times;
    };
    std::map<u8, Frontier> frontiers;
    for (u8 age = 0; age < 2; ++age)
        if (ages & (1u << age))
            frontiers.emplace(age, Frontier{ { RR_MAX }, Rando::Logic::InitializeRegionTimeStates(RR_MAX) });
    ComboRando::MmResourceScope resources;
    Rando::Logic::MmAccessScope access;
    std::set<std::pair<RandoEvent, std::function<bool()>>*> eventsInLogic;
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& [age, frontier] : frontiers) {
            gSaveContext.save.linkAge = age;
            auto& reachable = frontier.reachable;
            auto& timeStates = frontier.times;
            const size_t previous = reachable.size();
            for (auto region : std::set<RandoRegionId>(reachable))
                Rando::Logic::FindReachableRegions(region, reachable, timeStates);
            changed |= reachable.size() != previous;
            changed |= Rando::Logic::UpdateMmAccess();
            changed |= Rando::Logic::UpdateResourceSources(reachable, timeStates);
            for (auto region : reachable) {
                auto it = Rando::Logic::Regions.find(region);
                if (it == Rando::Logic::Regions.end())
                    continue;
                Rando::Logic::SetCurrentRegionTime(timeStates, region);
                for (auto& event : it->second.events)
                    if (!eventsInLogic.contains(&event) && Rando::Logic::TimeLogic::EvaluateConditionTimes(
                                                               event.second, Rando::Logic::gCurrentRegionTime)) {
                        RANDO_EVENTS[event.first]++;
                        eventsInLogic.insert(&event);
                        changed = true;
                    }
            }
        }
        // Pinned Adult Mask clones reached areas and their time state, not merely checks.
        if (clone)
            for (auto& [age, dst] : frontiers) {
                const auto& src = frontiers.at(1 - age);
                for (auto region : src.reachable) {
                    changed |= dst.reachable.insert(region).second;
                    auto it = src.times.find(region);
                    if (it == src.times.end())
                        continue;
                    auto existing = dst.times.find(region);
                    if (existing == dst.times.end()) {
                        dst.times.emplace(region, it->second);
                        changed = true;
                    } else {
                        auto merged = Rando::Logic::MergeTimeStates(existing->second, it->second);
                        if (merged.timeSlices != existing->second.timeSlices ||
                            merged.canStayOverTime != existing->second.canStayOverTime) {
                            existing->second = merged;
                            changed = true;
                        }
                    }
                }
            }
    }
    std::set<std::string> accessible;
    for (auto& [age, frontier] : frontiers) {
        gSaveContext.save.linkAge = age;
        for (auto region : frontier.reachable) {
            auto it = Rando::Logic::Regions.find(region);
            if (it == Rando::Logic::Regions.end())
                continue;
            Rando::Logic::SetCurrentRegionTime(frontier.times, region);
            for (auto& [check, condition] : it->second.checks)
                if (Rando::Logic::TimeLogic::EvaluateConditionTimes(condition.first,
                                                                    Rando::Logic::gCurrentRegionTime)) {
                    const auto& name = Rando::StaticData::GetCheckDisplayName(check);
                    if (!name.empty())
                        accessible.insert(name);
                }
        }
        if (frontier.reachable.contains(RR_MOON_MAJORAS_LAIR) && Rando::Logic::CanDefeatMajora())
            accessible.insert("Defeat Majora");
    }
    nlohmann::json out = nlohmann::json::array();
    for (const auto& name : accessible)
        out.push_back(name);
    ComboRando::MmSongOracle() = false;
    buf = out.dump();
    return buf.c_str();
}

extern "C" COMBO_EXPORT void Combo_MM_Rando_PlaceItem(const char* checkName, const char* itemName) {
    if (!checkName || !itemName)
        return;
    // ComboShip: map lookups replace nested name scans (runs once per committed check).
    auto chkIt = Combo_MM_CheckNameToCheckId().find(checkName);
    if (chkIt == Combo_MM_CheckNameToCheckId().end())
        return;
    auto itemIt = Combo_MM_SpoilerNameToItemId().find(itemName);
    if (itemIt == Combo_MM_SpoilerNameToItemId().end()) {
        // Empty is an oracle-only foreign reward. Do not let its original
        // vanilla stock masquerade as a local renewable potion or ammo source.
        if (!itemName[0] && sMM_OracleActive) {
            RANDO_SAVE_CHECKS[chkIt->second].randoItemId = RI_COMBO_FOREIGN;
            RANDO_SAVE_CHECKS[chkIt->second].shuffled = true;
        }
        return;
    }
    RANDO_SAVE_CHECKS[chkIt->second].randoItemId = itemIt->second;
    RANDO_SAVE_CHECKS[chkIt->second].shuffled = true;
}

extern "C" COMBO_EXPORT void Combo_MM_Rando_Restore(void) {
    sMM_OracleExternalBeans = false;
    if (!sMM_OracleActive) {
        return; // nothing snapshotted (double Restore / Restore without Reset)
    }
    memcpy(&gSaveContext, &sMM_OracleSavedContext, sizeof(SaveContext));
    gCurrentRegionTime = sMM_OracleSavedRegionTime;
    ComboRando::Entrances::mmFlagSource = sMM_OracleSavedRegionalFlags;
    sMM_OracleActive = false;
}
#endif // COMBO_BUILD — combo-only region opened above MM_LoadSaveForCombo

#ifdef COMBO_BUILD
// ComboShip: cross-game item-draw exports (MM_GetItemDrawInfo / MM_GetItemAnimDrawInfo). Bodies
// live in the combo-owned header so the vendored footprint stays this one include.
#include "ComboItemDrawMM.h"
#endif

#ifdef COMBO_BUILD
// ComboShip: MM analog of SOH_ExportMenu et al. comboui resolves these by GetProcAddress, ingests
// the CwMenu (combo/menu/ComboMenuABI.h), then invokes back by index.
namespace {
// Ensure mBenMenu exists; returns it (or nullptr if it couldn't be built).
std::shared_ptr<BenGui::BenMenu> Combo_EnsureBenMenu() {
    auto menu = BenGui::GetBenMenu();
    if (!menu) {
        BenGui::ActivateMenu();
        menu = BenGui::GetBenMenu();
    }
    // MM populates its menu tree (AddSettings/AddEnhancements/AddDevTools) and disabledMap in
    // BenMenu::InitElement() — NOT in the constructor. comboui owns the menu slot so the Gui loop
    // never Init()s this menu, leaving menuEntries empty and ExportComboMenu walking nothing (empty
    // MM tab). Init() here (idempotent) before any export/walk. OOT differs: it calls
    // AddMenuElements() explicitly at boot, so SOH_ExportMenu needs no Init.
    if (menu) {
        menu->Init();
    }
    // ComboShip: MM's rando Seed combobox reads Rando::Spoiler::spoilerOptions, empty until
    // RefreshOptions runs (it hasn't in this backgrounded context). Populate it on demand so the
    // always-available rando menu renders. RefreshOptions is idempotent.
    if (Rando::Spoiler::spoilerOptions.empty()) {
        Rando::Spoiler::RefreshOptions();
    }
    return menu;
}
} // namespace

// 2ship.dll has its own per-module ImGui GImGui — see combo/menu/ComboMenuSharedContext.h.

extern "C" COMBO_EXPORT const CwMenu* MM_ExportMenu(void) {
    ComboMenuContext::UseSharedImGuiContext();
    auto menu = Combo_EnsureBenMenu();
    return menu ? menu->ExportComboMenu() : nullptr;
}

extern "C" COMBO_EXPORT void MM_MenuInvokeCallback(int32_t i) {
    ComboMenuContext::UseSharedImGuiContext();
    // Menu code can load MM resources — scope MM's own RM, not the foreground game's (also in the
    // eval/draw/apply exports below; see combo/gui/ComboWidgetRender.h).
    Ship::ResourceManagerScope rmScope(Ship::CrossRMRegistry::Get("mm"));
    if (auto menu = Combo_EnsureBenMenu()) {
        menu->InvokeCallbackByIndex(i);
    }
}

// ComboShip: re-run the ShipInit func(s) registered for this CVar, mirroring 2Ship's native UIWidgets
// after a widget change — so settings/enhancements changed via the combo menu apply live instead of
// only on the next ShipInit::InitAll (MM boot / new save).
extern "C" COMBO_EXPORT void MM_MenuApplyCVarChange(const char* cvar) {
    Ship::ResourceManagerScope rmScope(Ship::CrossRMRegistry::Get("mm")); // ShipInit funcs load MM resources
    if (cvar && cvar[0])
        ShipInit::Init(cvar);
}

// ComboShip: combo-owned audio bridge entry point (combo/gui/ComboAudioBridge.cpp). The Shared tab's
// audio sliders are OOT's (gSettings.Volume.* int 0-100); the combo layer mirrors them into MM's float
// CVars and calls this to apply per-port volume live. MM applies volume via AudioSeq_SetPortVolumeScale
// (not ShipInit), so MM_MenuApplyCVarChange would not pick it up. gAudioCtx persists across transitions,
// so this is safe even when MM is not the foreground game.
extern "C" COMBO_EXPORT void MM_ApplyAudioVolume(int32_t seqPlayerIndex, float volume) {
    AudioSeq_SetPortVolumeScale((u8)seqPlayerIndex, volume);
}

// ComboShip: controller bindings live in the shared gSettings.Controllers.* CVars, so the Shared tab's
// (OOT) controls UI edits the same data MM reads. But each game's ControlDeck caches its mappings (it
// only re-reads on Init / this call), so a rebind made while MM was dormant is not picked up until MM
// reloads. The combo layer calls this when MM becomes the foreground game. Reloads all populated ports.
extern "C" COMBO_EXPORT void MM_ReloadControls(void) {
    auto controlDeck = Ship::Context::GetRawInstance()->GetControlDeck();
    if (!controlDeck)
        return;
    for (uint8_t port = 0; port < 4; ++port) {
        if (auto controller = controlDeck->GetControllerByPort(port))
            controller->ReloadAllMappingsFromConfig();
    }
}

// ComboShip: true when MM is the foreground game (queries comboui's ComboUI_GetForegroundGame, resolved
// once). BenMenu uses it to gate MM's live-world dev viewers — opening MM's tab while OOT is foreground
// must not draw against MM's dormant/swapped play state. comboui is always loaded under ComboShip; if it
// somehow isn't, default to true (draw) rather than hiding the tools.
bool Combo_MmIsForeground(void) {
    static int (*sFn)(void) = nullptr;
    static bool sTried = false;
    if (!sTried) {
        sTried = true;
        sFn = (int (*)(void))Combo_ResolveSym("comboui", "ComboUI_GetForegroundGame");
    }
    return sFn ? (sFn() == 1) : true;
}

// ComboShip (#127): MM's pause state, read by comboui's dormant-tracker gate (see
// SOH_IsPausedForCombo) — the dormant game's own pause state is stale.
extern "C" COMBO_EXPORT int MM_IsPausedForCombo(void) {
    return gPlayState != nullptr && gPlayState->pauseCtx.state > 0;
}

// ComboShip (#173): MM's play time is wall clock between flushes, so without these the hours spent in
// OOT get folded into filePlaytime at MM's next save. The launcher pauses/resumes on every swap.
static bool sComboPlaytimeRunning = false;

// Flushes the running interval into filePlaytime and stops counting. lastTimeLog == 0 means "never
// marked" (new file, z_sram_NES.c) and must not be treated as a timestamp — it would add ~57 years.
extern "C" COMBO_EXPORT void MM_ComboPausePlaytime(void) {
    if (sComboPlaytimeRunning && gSaveContext.shipSaveContext.lastTimeLog != 0 &&
        gSaveContext.save.shipSaveInfo.fileCompletedAt == 0) {
        uint64_t now = GetUnixTimestamp();
        if (now > gSaveContext.shipSaveContext.lastTimeLog) { // a backwards clock step would wrap
            gSaveContext.save.shipSaveInfo.filePlaytime += now - gSaveContext.shipSaveContext.lastTimeLog;
        }
        gSaveContext.shipSaveContext.lastTimeLog = now;
    }
    sComboPlaytimeRunning = false;
}

extern "C" COMBO_EXPORT void MM_ComboResumePlaytime(void) {
    gSaveContext.shipSaveContext.lastTimeLog = GetUnixTimestamp();
    sComboPlaytimeRunning = true;
}

// MM's half of the combo total, in ms. The live interval is added only while MM is the foreground
// game — otherwise the value would free-run on wall clock while MM is dormant.
extern "C" COMBO_EXPORT uint64_t MM_GetPlaytimeMs(void) {
    uint64_t total = gSaveContext.save.shipSaveInfo.filePlaytime;
    if (sComboPlaytimeRunning && gSaveContext.shipSaveContext.lastTimeLog != 0 &&
        gSaveContext.save.shipSaveInfo.fileCompletedAt == 0) {
        uint64_t now = GetUnixTimestamp();
        if (now > gSaveContext.shipSaveContext.lastTimeLog) {
            total += now - gSaveContext.shipSaveContext.lastTimeLog;
        }
    }
    return total;
}

extern "C" COMBO_EXPORT int32_t MM_MenuEvalDisabled(int32_t i, const char** outReason) {
    ComboMenuContext::UseSharedImGuiContext();
    Ship::ResourceManagerScope rmScope(Ship::CrossRMRegistry::Get("mm"));
    auto menu = Combo_EnsureBenMenu();
    return menu ? menu->EvalDisabledByIndex(i, outReason) : 0;
}

extern "C" COMBO_EXPORT void MM_MenuDrawCustom(int32_t i) {
    // comboui owns the active menu slot, so the Gui loop never drives MM's menu. A custom widget may read
    // THEME_COLOR (menuThemeIndex), which is set in UpdateElement(); skipping Update() makes ColorValues.at()
    // throw out_of_range (proven by the Phase 0 spike). So Init()+Update() before any custom draw.
    ComboMenuContext::UseSharedImGuiContext();
    Ship::ResourceManagerScope rmScope(Ship::CrossRMRegistry::Get("mm"));
    auto menu = Combo_EnsureBenMenu();
    if (menu) {
        menu->Init();
        menu->Update();
        menu->DrawCustomByIndex(i);
    }
}

extern "C" COMBO_EXPORT int32_t MM_MenuDrawWidget(int32_t i, int32_t width) {
    ComboMenuContext::UseSharedImGuiContext();
    Ship::ResourceManagerScope rmScope(Ship::CrossRMRegistry::Get("mm"));
    auto menu = Combo_EnsureBenMenu();
    if (menu) {
        menu->Init();
        menu->Update();
        return menu->DrawWidgetByIndex(i, width);
    }
    return 0;
}
#endif

// Helper to redirect the user to the boot screen in place of known console crash scenarios, and emits a notification
extern "C" bool Ship_HandleConsoleCrashAsReset() {
    // If fix crashes is on, return false and let fallback handling process in source
    if (CVarGetInteger("gEnhancements.Fixes.ConsoleCrashes", 1)) {
        return false;
    }

    std::reinterpret_pointer_cast<Ship::ConsoleWindow>(
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->GetGuiWindow("Console"))
        ->Dispatch("reset");

    Notification::Emit({
        .itemIcon = "__OTR__icon_item_24_static_yar/gQuestIconGoldSkulltulaTex",
        .message = "Crash prevented!",
        .remainingTime = 10.0f,
    });

    return true;
}

#ifdef COMBO_BUILD
extern "C" void Combo_MM_LoadFairyOcarinaPauseText(PlayState* play) {
    if (play != gPlayState || !gPlayState)
        return;
    auto entry = CustomMessage::LoadVanillaMessageTableEntry(0x1700);
    entry.icon = 0x4C; // pinned kaleido_scope.c uses the Ocarina-of-Time message icon
    entry.autoFormat = false;
    // Exact pinned MM Fairy description/control bytes, including the clear-color NUL.
    static constexpr char text[] = "\x01"
                                   "Fairy Ocarina\x11"
                                   "\x00"
                                   "This is a memento from\x11Saria.\x11\x12"
                                   "Play it with \xB0 and the four \xB2\x11"
                                   "Buttons. Press \xB1 to stop.\xBF";
    entry.msg.assign(text, sizeof(text) - 1);
    CustomMessage::LoadCustomMessageIntoFont(entry);
}
#endif

#ifdef COMBO_BUILD
#include "ComboFairyOcarinaHand.h"
#endif

#ifdef COMBO_BUILD
#include "ComboAdultMM.h"
#include "ComboAdultSettings.h"
#endif

#ifdef COMBO_BUILD
extern "C" void Combo_MM_CopyAdultProperties(PlayerAgeProperties* out) {
    *out = ComboAdultMM::BuildAdultProperties();
}
extern "C" int Combo_MM_IsAdult(void) {
    return ComboAdultMM::IsAdult(gSaveContext.save);
}
#endif

#ifdef COMBO_BUILD
#include "ComboAdultVoiceRecipe.h"
#endif

#ifdef COMBO_BUILD
#include "ComboAdultVoiceResources.h"
#endif

#ifdef COMBO_BUILD
// Native imported cl_maskoff is already the exact pinned10-frame header and player frame data.
extern "C" PlayerAnimationHeader* Combo_MM_AdultMaskOffAnimation(void) {
    return (PlayerAnimationHeader*)&gPlayerAnim_cl_maskoff;
}
extern "C" int Combo_MM_AdultResourcesReady(void);
using ComboAdultAgeCallback = int (*)(int, int);
static ComboAdultAgeCallback sAdultCanSetOotAge = nullptr, sAdultSetOotAge = nullptr;
extern "C" COMBO_EXPORT void MM_SetAdultCrossAgeBridge(ComboAdultAgeCallback canSet, ComboAdultAgeCallback set) {
    sAdultCanSetOotAge = canSet;
    sAdultSetOotAge = set;
}
extern "C" int Combo_MM_AdultCanSwapAge(void) {
    const auto& save = gSaveContext.save;
    const int file = gSaveContext.fileNum;
    if (file < 0 || file > 2 || !save.shipSaveInfo.adultMm.enabled || save.linkAge > 1 ||
        !Combo_MM_AdultResourcesReady())
        return 0;
    if (!save.shipSaveInfo.rando.randoSaveOptions[RO_MM_CROSS_AGE])
        return 1;
    return sAdultCanSetOotAge && sAdultSetOotAge && sAdultCanSetOotAge(file, 1 - save.linkAge);
}
extern "C" int Combo_MM_AdultSwapAge(PlayState* play) {
    (void)play;
    if (!Combo_MM_AdultCanSwapAge())
        return 0;
    const int file = gSaveContext.fileNum, age = 1 - gSaveContext.save.linkAge;
    const Save before = gSaveContext.save;
    if (!ComboAdultMM::SwapHumanLoadout(gSaveContext.save, age))
        return 0;
    if (gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MM_CROSS_AGE] && !sAdultSetOotAge(file, age)) {
        gSaveContext.save = before;
        return 0;
    }
    return 1;
}
#endif

#ifdef COMBO_BUILD
extern "C" int Combo_MM_AdultResourcesReady(void);
extern "C" SequenceData* Combo_MM_AdultSequenceForPath(const char* path) {
    if (!Combo_MM_AdultResourcesReady() || !path || !gSequenceMapSize || !gSequenceMap || !gSequenceMap[0] ||
        strcmp(path, gSequenceMap[0]))
        return nullptr;
    return ComboAdultMM::BuildAdultHybridSequence();
}
extern "C" SoundFont* Combo_MM_AdultFontForPath(const char* path) {
    if (!Combo_MM_AdultResourcesReady() || !path || !gFontMapSize || !gFontMap || !gFontMap[0] ||
        strcmp(path, gFontMap[0]))
        return nullptr;
    if (!ComboAdultMM::BuildAdultHybridSequence())
        return nullptr;
    return ComboAdultMM::BuildAdultHybridFont();
}
extern "C" void Combo_MM_AdultUpdateVoiceRouting(Player* player) {
    if (Combo_MM_AdultResourcesReady())
        ComboAdultMM::UpdateAdultVoiceRouting(player, Combo_MM_IsAdult());
}
#endif

#ifdef COMBO_BUILD
#include "ComboAdultModelResources.h"
#endif

#ifdef COMBO_BUILD
extern "C" void Combo_MM_ApplyAdultOverlayAge(int adult);
extern "C" COMBO_EXPORT int MM_GetEffectiveHumanAge(int fileNum) {
    if (fileNum < 0 || fileNum > 2 || fileNum != gSaveContext.fileNum)
        return -1;
    const auto& save = gSaveContext.save;
    if (!save.shipSaveInfo.adultMm.enabled)
        return 1;
    if (save.linkAge > 1)
        return -1;
    if (save.linkAge == 0 && !Combo_MM_AdultResourcesReady())
        return -1;
    return save.linkAge;
}
extern "C" COMBO_EXPORT int MM_CanFaroreTransitionAge(int fileNum, int age) {
    if (age < 0 || age > 1 || fileNum < 0 || fileNum > 2 || fileNum != gSaveContext.fileNum)
        return 0;
    const auto& state = gSaveContext.save.shipSaveInfo.adultMm;
    // Native legacy MM stores linkAge0 while using child human assets.
    if (!state.enabled)
        return age == 1;
    if (gSaveContext.save.linkAge > 1)
        return 0;
    if (age == 1 && gSaveContext.save.linkAge == 1)
        return 1;
    // The complete capability gate includes model/mask/spin/equipment callbacks, not just geometry.
    return Combo_MM_AdultResourcesReady() && ComboAdultMM::BuildAdultHybridSequence() != nullptr;
}
extern "C" COMBO_EXPORT int MM_ApplyFaroreTransitionAge(int fileNum, int age) {
    if (!MM_CanFaroreTransitionAge(fileNum, age))
        return 0;
    auto& save = gSaveContext.save;
    if (!save.shipSaveInfo.adultMm.enabled)
        return 1;
    if (save.linkAge != age && !ComboAdultMM::SwapHumanLoadout(save, age))
        return 0;
    Combo_MM_ApplyAdultModelTables(age == 0);
    Combo_MM_ApplyAdultOverlayAge(age == 0);
    // Player initialization consumes the new saved age/tables; voice pointers route on its update.
    return 1;
}
#endif

#ifdef COMBO_BUILD
// Shared Farore backend owns the age-specific native points; do not duplicate their save schema.
extern "C" int Combo_MMFaroreRefreshHumanRespawn(void);
extern "C" void Combo_MM_AdultRestoreFaroreRespawn(void) {
    Combo_MMFaroreRefreshHumanRespawn();
}
#endif

#include "ComboAdultGameplayKeepResources.h"

// Farore's pinned mode selection uses CFG_MM_MASK_ADULT, independent of startingAgeMm.
// Resource readiness controls transitions, never the saved age-mode policy.
extern "C" COMBO_EXPORT int MM_IsHumanAgeModeEnabled(int fileNum) {
    if (fileNum < 0 || fileNum > 2 || gSaveContext.fileNum != fileNum)
        return -1;
    return gSaveContext.save.shipSaveInfo.adultMm.maskEnabled ? 1 : 0;
}

#include "ComboAdultEponaResources.h"

#include "ComboAdultMaskResources.h"

#include "ComboAdultMaskInventory.h"
extern "C" COMBO_EXPORT int MM_GrantAdultMaskItem(int fileNum) {
    if (fileNum < 0 || fileNum > 2 || fileNum != gSaveContext.fileNum)
        return 0;
    return ComboAdultMaskInventory::Grant(gSaveContext.save.shipSaveInfo.adultMm.maskOwnedBits,
                                          gSaveContext.save.saveInfo.inventory.items[SLOT_MASK_FIERCE_DEITY], 1);
}

extern "C" COMBO_EXPORT int MM_GrantFierceDeityAdultVariant(int fileNum, int variant) {
    if (fileNum < 0 || fileNum > 2 || fileNum != gSaveContext.fileNum || variant < 0 || variant > 1)
        return 0;
    return ComboAdultMaskInventory::Grant(gSaveContext.save.shipSaveInfo.adultMm.maskOwnedBits,
                                          gSaveContext.save.saveInfo.inventory.items[SLOT_MASK_FIERCE_DEITY],
                                          unsigned(variant));
}

#include "ComboAdultMaskIconResources.h"

#include "ComboAdultMaskPauseNative.h"

#include "ComboAdultBootDrawResources.h"
#include "ComboHoverCircleResources.h"
#include "ComboAdultGearInventory.h"
#include "ComboAdultGearIconResources.h"
#include "ComboAdultGearGiResources.h"
#include "ComboAdultGearPauseNative.h"
#include "ComboAdultGearExtraIconNative.h"
extern "C" COMBO_EXPORT int MM_GrantAdultGearItem(int fileNum, int item) {
    if (fileNum < 0 || fileNum > 2 || fileNum != gSaveContext.fileNum)
        return 0;
    const int group = ComboAdultGearInventory::Group(unsigned(item));
    if (!group)
        return 0;
    auto& age = gSaveContext.save.shipSaveInfo.adultMm;
    return ComboAdultGearInventory::Grant(
        group == 2 ? age.gearTrade2Bits : age.gearTrade3Bits,
        gSaveContext.save.saveInfo.inventory.items[group == 2 ? SLOT_TRADE_KEY_MAMA : SLOT_TRADE_COUPLE],
        unsigned(item));
}

extern "C" void Combo_MM_ClearCustomMask(void) {
    gSaveContext.save.shipSaveInfo.adultMm.customMask = 0;
}

#ifdef COMBO_BUILD
// Geometry presence cannot authorize unfinished player/equipment/logic integration.
static constexpr bool kAdultMmGameplayIntegrationComplete = false;
extern "C" int Combo_MM_AdultResourcesReady(void) {
    if (!kAdultMmGameplayIntegrationComplete) {
#ifdef COMBO_ADULT_MM_RUNTIME_DIAGNOSTICS
        // Private validation requires an explicitly compiled diagnostic build AND a default-off runtime switch.
        if (!CVarGetInteger("gCombo.Debug.AdultMmRuntimeValidation", 0))
            return 0;
#else
        return 0;
#endif
    }
    return Combo_MM_AdultModelResourcesReady() && Combo_MM_AdultMaskResourcesReady() &&
           Combo_MM_AdultMaskIconsReady() && Combo_MM_AdultGameplayKeepResourcesReady() &&
           Combo_MM_AdultEponaResourcesReady() && Combo_MM_AdultBootDrawResourcesReady() &&
           Combo_MM_ChildBootResourcesReady() && Combo_MM_HoverCircleResourcesReady() &&
           Combo_MM_AdultGearIconsReady() && Combo_MM_AdultGearGiReady() && ComboAdultMM::BuildAdultHybridSequence();
}
// The combo layer ANDs a seed's faroreRules.crossAge with this before configuring either engine's Farore
// runtime or preflighting an entrance, so they agree with the MM save and oracle (RO_MM_CROSS_AGE kept off).
extern "C" COMBO_EXPORT int MM_AdultRuntimeReady(void) {
    return Combo_MM_AdultResourcesReady();
}
// Eligibility is checked before any legacy normalization or loadout mutation.
extern "C" int Combo_MM_AdultConfigureBoundSeed(int fileNum) {
    if (fileNum < 0 || fileNum > 2 || fileNum != gSaveContext.fileNum)
        return 0;
    auto& save = gSaveContext.save;
    const auto& options = save.shipSaveInfo.rando.randoSaveOptions;
    const int age = options[RO_MM_STARTING_AGE] ? 0 : 1;
    const bool mask = options[RO_MM_ADULT_MASK] != 0;
    const bool cross = options[RO_MM_CROSS_AGE] != 0;
    const bool gear = options[RO_MM_BOOTS_IRON] || options[RO_MM_BOOTS_HOVER] || options[RO_MM_TUNIC_GORON] ||
                      options[RO_MM_TUNIC_ZORA];
    if ((mask || cross || gear || age == 0 || save.shipSaveInfo.adultMm.enabled) && !Combo_MM_AdultResourcesReady())
        return 0;
    return ComboAdultMM::ConfigureSeedAge(save, age, mask, cross, gear);
}
#ifdef COMBO_ADULT_MM_RUNTIME_DIAGNOSTICS
// No options/grants are injected: root must supply an explicitly private bound seed fixture.
extern "C" COMBO_EXPORT int MM_DebugAdultConfigureBoundSeed(int fileNum) {
    if (!Combo_MM_AdultResourcesReady())
        return 0;
    return Combo_MM_AdultConfigureBoundSeed(fileNum);
}
extern "C" COMBO_EXPORT int MM_DebugAdultResourcesReady(void) {
    return Combo_MM_AdultResourcesReady();
}
#endif
#endif

#ifdef COMBO_BUILD
extern "C" int Combo_MM_AdultValidateLoadedSave(const Save* save) {
    if (!save)
        return 0;
    const auto& state = save->shipSaveInfo.adultMm;
    if (!state.enabled)
        return 1;
    return save->linkAge <= 1 && state.boots <= 2 && (state.tunic == 0 || state.tunic == 2 || state.tunic == 3) &&
           state.maskEnabled <= 1 && state.maskOwnedBits <= 3 &&
           !((state.gearTrade2Bits | state.gearTrade3Bits) & ~6u) && Combo_MM_AdultResourcesReady();
}
#endif

#ifdef COMBO_BUILD
// Called with the parsed local page, before assigning live SaveContext.
extern "C" int Combo_MM_AdultPrepareLoadedSave(Save* save) {
    if (!save)
        return 0;
    const auto& options = save->shipSaveInfo.rando.randoSaveOptions;
    const bool mask = options[RO_MM_ADULT_MASK] != 0, cross = options[RO_MM_CROSS_AGE] != 0;
    const bool gear = options[RO_MM_BOOTS_IRON] || options[RO_MM_BOOTS_HOVER] || options[RO_MM_TUNIC_GORON] ||
                      options[RO_MM_TUNIC_ZORA];
    const int age = options[RO_MM_STARTING_AGE] ? 0 : 1;
    if (!save->shipSaveInfo.adultMm.enabled && (mask || cross || gear || age == 0)) {
        if (!Combo_MM_AdultResourcesReady())
            return 0;
        if (!ComboAdultMM::ConfigureSeedAge(*save, age, mask, cross, gear))
            return 0;
    }
    return Combo_MM_AdultValidateLoadedSave(save);
}
#endif

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT uint32_t MM_GetOracleSongSourcesForAge(int age) {
    if (age < 0 || age > 1 || !(sAdultOracleReachedAges & (1u << age)))
        return 0;
    struct RestoreAge {
        u8 age;
        ~RestoreAge() {
            gSaveContext.save.linkAge = age;
        }
    } restore{ gSaveContext.save.linkAge };
    gSaveContext.save.linkAge = (u8)age;
    return MM_GetSongSources();
}
#endif

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT void MM_SetOracleSongAgeArrivals(uint32_t adult, uint32_t child) {
    ComboRando::MmSongAgeArrivals() = { true, { adult & 0xffc0, child & 0xffc0 } };
}
extern "C" COMBO_EXPORT uint8_t MM_GetOracleDefaultEntryAges(void) {
    return gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MM_STARTING_AGE] ? 1 : 2;
}
extern "C" COMBO_EXPORT bool MM_UseCrossAgeOracle(void) {
    return gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MM_CROSS_AGE] && Combo_MM_AdultResourcesReady();
}
#endif
