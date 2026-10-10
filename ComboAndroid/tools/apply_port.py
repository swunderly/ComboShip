#!/usr/bin/env python3
"""Apply narrow Android patches to a locked ComboShip checkout, never to game data."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
PIN = '94eb185e4abcc2d568aa8241fa02c43cdd86c439'
def replace(root, relative, old, new, count=1):
    path=root/relative
    text=path.read_text(encoding='utf-8')
    actual=text.count(old)
    if actual != count: raise RuntimeError(f'{relative}: expected {count} patch anchors, found {actual}. Upstream drift.')
    path.write_text(text.replace(old,new), encoding='utf-8', newline='\n')
def apply(root,overlay,final_integration=True):
    marker=root/'.comboship-android-patched.json'
    if marker.exists(): raise RuntimeError('Already patched. Use a fresh checkout; modifications are never reset.')
    if not (root/'.git').exists(): raise RuntimeError('A pinned git checkout is required.')
    head=subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD'],text=True).strip()
    if head != PIN: raise RuntimeError(f'Expected {PIN}, got {head}')
    if subprocess.check_output(['git','-C',str(root),'status','--porcelain','--untracked-files=no'],text=True).strip():
        raise RuntimeError('Refusing to patch tracked local modifications.')
    if (root/'ComboAndroid').exists(): raise RuntimeError('Overlay destination already exists.')
    shutil.copytree(overlay,root/'ComboAndroid',ignore=shutil.ignore_patterns('build','.gradle','.cxx','__pycache__','local.properties','*.apk','assets','jniLibs','sdl-java'))
    for relative in ['CMakeLists.txt','soh/CMakeLists.txt','mm/CMakeLists.txt','OTRExporter/CMakeLists.txt']:
        replace(root,relative,'set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)','if(WIN32)\n    set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)\nendif()')
    replace(root,'CMakeLists.txt','# Shared libultraship (in Combo directory)','include("${CMAKE_SOURCE_DIR}/ComboAndroid/cmake/AndroidDependencies.cmake")\n\n# Shared libultraship (in Combo directory)')
    exporter='add_subdirectory(OTRExporter ${CMAKE_BINARY_DIR}/OTRExporter)'
    replace(root,'CMakeLists.txt',exporter,exporter+'\nfind_package(PNG REQUIRED)\n# Exporter consumes engine XML/ZIP headers; inherit actual target dependencies.\ntarget_link_libraries(OTRExporter PUBLIC libultraship PNG::PNG)')
    replace(root,'combo/CMakeLists.txt','add_executable(ComboShip ${COMBO_SOURCES})','''if(ANDROID)
    add_library(ComboShip SHARED ${COMBO_SOURCES}
        ${CMAKE_SOURCE_DIR}/ComboAndroid/app/src/main/cpp/AndroidEntry.cpp
        ${CMAKE_SOURCE_DIR}/ComboAndroid/app/src/main/cpp/RuntimePaths.cpp)
    set_target_properties(ComboShip PROPERTIES OUTPUT_NAME comboship)
    set_target_properties(comboui PROPERTIES PREFIX "lib")
    target_link_libraries(ComboShip PRIVATE libultraship android log)
    target_link_options(ComboShip PRIVATE "-Wl,-z,global")
    target_link_options(libultraship PRIVATE "-Wl,-z,global")
    target_include_directories(ComboShip PRIVATE ${CMAKE_SOURCE_DIR}/ComboAndroid/app/src/main/cpp)
else()
    add_executable(ComboShip ${COMBO_SOURCES})
endif()''')
    replace(root,'combo/ComboShip.cpp','int main(int argc, char** argv) {','''#ifdef __ANDROID__
int ComboAndroid_RunDesktopMain(int argc, char** argv) {
#else
int main(int argc, char** argv) {
#endif''')
    replace(root,'combo/ComboShip.cpp',"    std::string path = std::strchr(name, '/') ? name : std::string(\"./\") + name;",'''#ifdef __ANDROID__
    const char* directory = std::getenv("COMBOSHIP_NATIVE_LIB_DIR");
    if (!directory || std::strchr(name, '/')) return nullptr;
    std::string path = std::string(directory) + "/" + name;
#else
    std::string path = std::strchr(name, '/') ? name : std::string("./") + name;
#endif''')
    replace(root,'combo/ComboShip.cpp','    const char* comboUiDll = "comboui.so";','''#ifdef __ANDROID__
    const char* comboUiDll = "libcomboui.so";
#else
    const char* comboUiDll = "comboui.so";
#endif''')
    for signature in ['std::string Context::GetAppBundlePath() {','std::string Context::GetAppDirectoryPath(const std::string& appName) {']:
        replace(root,'libultraship/src/ship/Context.cpp',signature,signature+'''
#if defined(__ANDROID__) && defined(COMBO_BUILD)
    const char* comboRoot = std::getenv("COMBOSHIP_DATA_DIR");
    if (comboRoot && comboRoot[0] == '/') return comboRoot;
#endif''')
    replace(root,'libultraship/src/fast/Fast3dWindow.cpp','#ifdef __linux__','#if defined(__linux__) && !defined(__ANDROID__)')
    replace(root,'libultraship/src/ship/port/mobile/MobileImpl.cpp','            state->ClearText();','            if (state) state->ClearText();')
    picker='combo/gui/ComboFilePicker.h'
    replace(root,picker,'#else\n#include "portable-file-dialogs.h"','#elif !defined(__ANDROID__)\n#include "portable-file-dialogs.h"')
    replace(root,picker,'#else\n    static const bool sAvailable','#elif defined(__ANDROID__)\n    return false; // Setup uses the Android document picker.\n#else\n    static const bool sAvailable')
    replace(root,picker,'#else\n    (void)winFilter;','#elif defined(__ANDROID__)\n    (void)title; (void)winFilter; (void)pfdFilters;\n    return {};\n#else\n    (void)winFilter;')
    replace(root,'OTRExporter/OTRExporter/ExporterArchiveOTR.cpp','printf(fullPath.c_str());','printf("%s", fullPath.c_str());')
    for game in ['soh','mm']:
        for package in ['Ogg','Vorbis','Opus','OpusFile']:
            old=f'    find_package({package} REQUIRED)'
            new=f'    if(ANDROID)\n        find_package({package} CONFIG REQUIRED)\n    else()\n{old}\n    endif()'
            replace(root,f'{game}/CMakeLists.txt',old,new)
        with (root/game/'CMakeLists.txt').open('a', encoding='utf-8', newline='\n') as out:
            out.write('''
if(ANDROID)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types;-Wno-int-conversion;-Wno-implicit-int>)
endif()
''')
    subprocess.run(['git','-C',str(root),'apply','--check',str(root/'ComboAndroid/patches/mm-qol-backports.patch')],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(root/'ComboAndroid/patches/mm-qol-backports.patch')],check=True)
    runtime_patch=root/'ComboAndroid/patches/android-runtime.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(runtime_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(runtime_patch)],check=True)
    import sys
    subprocess.run([sys.executable,str(root/'ComboAndroid/tools/backport_fork_fixes.py')],cwd=root,check=True)
    session_patch=root/'ComboAndroid/patches/session-fixes.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(session_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(session_patch)],check=True)
    startup_patch=root/'ComboAndroid/patches/android-startup.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(startup_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(startup_patch)],check=True)
    tracker_patch=root/'ComboAndroid/patches/check-tracker.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(tracker_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(tracker_patch)],check=True)
    experience_patch=root/'ComboAndroid/patches/unified-experience.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(experience_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(experience_patch)],check=True)
    randomizer_patch=root/'ComboAndroid/patches/randomizer-menu.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(randomizer_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(randomizer_patch)],check=True)
    multiplayer_patch=root/'ComboAndroid/patches/multiplayer.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(multiplayer_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(multiplayer_patch)],check=True)
    seen_patch=root/'ComboAndroid/patches/mm-seen-checks.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(seen_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(seen_patch)],check=True)
    audio_patch=root/'ComboAndroid/patches/android-audio-latency.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(audio_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(audio_patch)],check=True)
    visibility_patch=root/'ComboAndroid/patches/mm-check-visibility.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(visibility_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(visibility_patch)],check=True)
    rules_patch=root/'ComboAndroid/patches/gameplay-rules.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(rules_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(rules_patch)],check=True)
    song_patch=root/'ComboAndroid/patches/song-travel.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(song_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(song_patch)],check=True)
    dialogue_patch=root/'ComboAndroid/patches/native-warp-dialogue.patch'
    subprocess.run(['git','-C',str(root),'apply','--check',str(dialogue_patch)],check=True)
    subprocess.run(['git','-C',str(root),'apply',str(dialogue_patch)],check=True)
    # Ordered 0.1.21 components, followed by the narrow launcher/runtime integration.
    # Context whitespace may differ between Windows CRLF checkouts and LF-generated files;
    # identifiers/code still match, and a clean pinned replay is compared with the built source.
    component_patches = [
        'song-performance.patch', 'uniform-editors.patch', 'combined-logic-guarantees.patch',
        'boss-event-path-hints.patch', 'mm-verified-tricks.patch', 'fd-native-traversal.patch',
        'unified-tricks-ui.patch', 'tracker-knowledge.patch', 'mm-enhancement-logic.patch',
        'oot-shop-hours.patch', 'oot-capability-rules.patch',
    ]
    if final_integration:
        component_patches.extend(['backlog-integration.patch', 'editor-restorations.patch', 'remaining-additions.patch', 'navigation-song-travel.patch', 'menu-refresh.patch', 'cross-warp-presentation.patch', 'oot-skulltula-observation.patch', 'oot-shop-knowledge.patch', 'native-warp-fidelity.patch', 'tracker-reachability-refresh.patch', 'tracker-age-planning.patch', 'logic-parity-improvements.patch', 'oot-resource-logic.patch', 'supported-logic-fixes.patch', 'mm-resource-parity.patch', 'full-parity.patch', 'mm-save-compatibility.patch', 'tracker-knowledge-persistence.patch', 'push-block-persistence.patch'])
    if final_integration:
        component_patches.append('warp-polish.patch')
        component_patches.append('feature-expansion.patch')
        component_patches.append('feature-expansion-0220.patch')
        component_patches.append('mask-quest-parity.patch')
        component_patches.append('mask-quest-repair.patch')
        component_patches.append('menu-captions-layout.patch')
        component_patches.append('menu-options-safety.patch')
        component_patches.append('menu-rules-placement.patch')
        component_patches.append('starting-song-layout.patch')
        component_patches.append('mm-shop-observations.patch')
        component_patches.append('oot-shop-visibility.patch')
        component_patches.append('menu-reopen-cache.patch')
        component_patches.append('menu-polish-cache.patch')
        component_patches.append('mm-expanded-souls-public.patch')
        component_patches.append('farore-wind-public.patch')
    for name in component_patches:
        patch=root/'ComboAndroid/patches'/name
        subprocess.run(['git','-C',str(root),'apply','--ignore-space-change','--check',str(patch)],check=True)
        subprocess.run(['git','-C',str(root),'apply','--ignore-space-change',str(patch)],check=True)
    changed=subprocess.check_output(['git','-C',str(root),'diff','--name-only'],text=True).splitlines()
    changed+=subprocess.check_output(['git','-C',str(root),'ls-files','--others','--exclude-standard','--','combo','libultraship/include/ship/window/gui','mm/2s2h/DeveloperTools/OwlStatueEditor.h','soh/soh/Enhancements/Restorations/ShadowDoorUnload.h','soh/soh/Enhancements/randomizer/age_change_areas.h','soh/soh/Enhancements/CrossWarp','mm/src/overlays/actors/ovl_Combo_Song_Warp','mm/src/overlays/actors/ovl_Combo_Magic_Fire/z_combo_magic_fire.c','mm/src/overlays/actors/ovl_Combo_Magic_Fire/z_combo_magic_fire.h','mm/src/overlays/actors/ovl_Combo_Magic_Fire/sSphereVtx.vtx.inc','mm/src/overlays/actors/ovl_Combo_Magic_Dark/z_combo_magic_dark.c','mm/src/overlays/actors/ovl_Combo_Magic_Dark/z_combo_magic_dark.h','mm/src/overlays/actors/ovl_Combo_Magic_Dark/sDiamondVtx.vtx.inc','mm/2s2h/Rando/Spells.h','mm/2s2h/Rando/Spells.cpp','mm/src/overlays/actors/ovl_player_actor/ComboSpellPlayer.inc.h','combo/rando/SpellRules.h','mm/2s2h/Rando','mm/2s2h/Enhancements','soh/soh/Enhancements/randomizer/silver_rupees','mm/src/overlays/actors/ovl_En_Wallmas','mm/src/overlays/actors/ovl_Combo_Magic_Wind','mm/src/overlays/actors/ovl_player_actor/ComboFarorePlayer.inc.h','mm/src/code/ComboFarorePointer.c'],text=True).splitlines()
    changed+=subprocess.check_output(['git','-C',str(root),'ls-files','--others','--exclude-standard','--',
        'soh/soh/Enhancements/randomizer/KokiriRupeePolicy.h',
        'soh/soh/Enhancements/randomizer/KokiriGrassPolicy.h'],text=True).splitlines()
    manifest={'upstream':PIN,'patchedFiles':{name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in changed}}
    marker.write_text(json.dumps(manifest,indent=2)+'\n')
    print(f'Patched {len(changed)} upstream files; retained both games and combined randomizer.')
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('checkout',type=Path)
    parser.add_argument('--overlay',type=Path,default=Path(__file__).resolve().parents[1])
    args=parser.parse_args()
    apply(args.checkout.resolve(),args.overlay.resolve())

