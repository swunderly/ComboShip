#!/usr/bin/env python3
"""Package real ARM64 libraries and no-ROM support archives into the Android app."""
import argparse,hashlib,json,re,shutil,struct,subprocess,zipfile
from pathlib import Path
from verify_native import verify_runtime, EXPORTS
from patch_sdl import patch as patch_sdl
REQUIRED={'libcomboship.so','libsoh.so','lib2ship.so','libcomboui.so','libultraship.so','libSDL2.so','libSDL2_net.so','libc++_shared.so'}
def arm64(data,name):
    if len(data)<64 or data[:6]!=b'\x7fELF\x02\x01' or struct.unpack_from('<H',data,18)[0]!=183:
        raise RuntimeError(f'{name} is not a little-endian ARM64 ELF library')
    offset=struct.unpack_from('<Q',data,32)[0]
    entry_size,count=struct.unpack_from('<HH',data,54)
    if entry_size<56 or offset+entry_size*count>len(data):raise RuntimeError(f'{name}: malformed program headers')
    for i in range(count):
        base=offset+i*entry_size
        if struct.unpack_from('<I',data,base)[0]==1:
            align=struct.unpack_from('<Q',data,base+48)[0]
            if align<16384:raise RuntimeError(f'{name}: PT_LOAD alignment {align} below 16 KiB')
def copy_tree(source,dest):
    if not source.is_dir():raise RuntimeError(f'Missing directory: {source}')
    shutil.copytree(source,dest,dirs_exist_ok=True)
def stage(upstream,native,sdl,ndk,app):
    if not (app/'build.gradle').is_file() or not (app/'src/main/AndroidManifest.xml').is_file():
        raise RuntimeError('Expected the ComboAndroid app source directory')
    assets=app/'src/main/assets';libs=app/'src/main/jniLibs/arm64-v8a';java=app/'src/main/sdl-java'
    for generated in (assets,libs,java):
        if not generated.resolve().is_relative_to(app.resolve()) or generated.is_symlink():
            raise RuntimeError(f'Generated destination escapes app directory: {generated}')
        if generated.exists():shutil.rmtree(generated)
        generated.mkdir(parents=True)
    candidates={}
    for source in sorted(native.rglob('*.so')):
        if source.is_file():
            digest=hashlib.sha256(source.read_bytes()).hexdigest()
            if source.name in candidates and candidates[source.name][1]!=digest:raise RuntimeError(f'Conflicting libraries named {source.name}')
            candidates[source.name]=(source,digest)
    prebuilts=list((ndk/'toolchains/llvm/prebuilt').glob('*/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so'))
    if len(prebuilts)!=1:raise RuntimeError('Expected exactly one host NDK runtime')
    cxx=prebuilts[0]
    candidates['libc++_shared.so']=(cxx,None)
    missing=REQUIRED-candidates.keys()
    if missing:raise RuntimeError(f'Missing real native game libraries: {sorted(missing)}')
    for name,(source,_) in candidates.items():
        arm64(source.read_bytes(),name);shutil.copy2(source,libs/name)
    launcher=(upstream/'combo/ComboShip.cpp').read_text(encoding='utf-8')
    exported={name:sorted(set(re.findall(r'GetSym\('+module+r',\s*"([^"]+)"',launcher)))
        for module,name in [('sohModule','libsoh.so'),('mmModule','lib2ship.so'),('comboUIModule','libcomboui.so')]}
    for name, symbols in EXPORTS.items():
        exported[name]=sorted(set(exported.get(name,[])) | symbols)
    verify_runtime({p.name:p.read_bytes() for p in libs.glob('*.so')},exported)
    (assets/'native-exports.json').write_text(json.dumps(exported,indent=2)+'\n',encoding='utf-8')
    pin='94eb185e4abcc2d568aa8241fa02c43cdd86c439'
    patched=subprocess.check_output(['git','-C',str(upstream),'diff',pin,'--name-only'],text=True).splitlines()
    patched+=subprocess.check_output(['git','-C',str(upstream),'ls-files','--others','--exclude-standard','--','combo','libultraship/include/ship/window/gui','mm/2s2h/DeveloperTools/OwlStatueEditor.h','soh/soh/Enhancements/Restorations/ShadowDoorUnload.h','soh/soh/Enhancements/randomizer/age_change_areas.h','soh/soh/Enhancements/CrossWarp','mm/src/overlays/actors/ovl_Combo_Song_Warp','mm/src/overlays/actors/ovl_Combo_Magic_Fire/z_combo_magic_fire.c','mm/src/overlays/actors/ovl_Combo_Magic_Fire/z_combo_magic_fire.h','mm/src/overlays/actors/ovl_Combo_Magic_Fire/sSphereVtx.vtx.inc','mm/src/overlays/actors/ovl_Combo_Magic_Dark/z_combo_magic_dark.c','mm/src/overlays/actors/ovl_Combo_Magic_Dark/z_combo_magic_dark.h','mm/src/overlays/actors/ovl_Combo_Magic_Dark/sDiamondVtx.vtx.inc','mm/2s2h/Rando/Spells.h','mm/2s2h/Rando/Spells.cpp','mm/src/overlays/actors/ovl_player_actor/ComboSpellPlayer.inc.h','combo/rando/SpellRules.h','mm/src/overlays/actors/ovl_Combo_Magic_Wind','mm/src/overlays/actors/ovl_player_actor/ComboFarorePlayer.inc.h','mm/src/code/ComboFarorePointer.c','mm/src/overlays/actors/ovl_En_Wallmas','mm/2s2h/Enhancements/Songs/CrossWarpAudio.cpp','mm/2s2h/Enhancements/Songs/CrossWarpAudio.h'],text=True).splitlines()
    patched+=subprocess.check_output(['git','-C',str(upstream),'ls-files','--others','--exclude-standard','--','soh/soh/Enhancements/randomizer/silver_rupees','mm/2s2h/Rando','mm/2s2h/Enhancements'],text=True).splitlines()
    patched+=subprocess.check_output(['git','-C',str(upstream),'ls-files','--others','--exclude-standard','--',
        'soh/soh/Enhancements/randomizer/KokiriRupeePolicy.h',
        'soh/soh/Enhancements/randomizer/KokiriGrassPolicy.h',
        'soh/soh/ProfileOotStartingLayout.inc', 'mm/2s2h/ProfileMmLayout.inc'],text=True).splitlines()
    provenance={'upstream':pin,'androidApi':28,'ndk':'28.2.13676358','abi':'arm64-v8a',
        'deviceTested':False,'patchedFiles':{p:hashlib.sha256((upstream/p).read_bytes()).hexdigest() for p in patched}}
    (assets/'build-info.json').write_text(json.dumps(provenance,indent=2)+'\n',encoding='utf-8')
    copy_tree(sdl/'android-project/app/src/main/java',java)
    patch_sdl(java)
    runtime=assets/'runtime';runtime.mkdir()
    for game,archive in [('soh','soh.o2r'),('mm','2ship.o2r')]:
        source=upstream/game/archive
        if not source.is_file():raise RuntimeError(f'Missing no-ROM archive {source}; run host archive generators')
        with zipfile.ZipFile(source) as z:
            if z.testzip() is not None:raise RuntimeError(f'Invalid support archive {source}')
            if game=='soh' and 'shaders/opengl/default.shader.glsl' not in z.namelist():raise RuntimeError('Missing renderer shader')
        shutil.copy2(source,runtime/archive)
        # Exactly match the native ComboShip POST_BUILD flat merged extractor layout.
        copy_tree(upstream/game/'assets/extractor',runtime/'assets')
        copy_tree(upstream/game/'assets/xml',runtime/'assets/xml')
    shutil.copy2(upstream/'gamecontrollerdb.txt',runtime/'gamecontrollerdb.txt')
    for path in runtime.rglob('*'):
        if not path.is_file():continue
        if path.suffix.lower() in {'.z64','.v64','.n64','.elf','.exe','.dll','.so'}:raise RuntimeError(f'Unexpected ROM/executable asset: {path}')
        if path.suffix.lower() in {'.o2r','.otr'} and path.name not in {'soh.o2r','2ship.o2r'}:raise RuntimeError(f'Game-derived archive must not ship: {path}')
    records=[{'path':p.relative_to(runtime).as_posix(),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(runtime.rglob('*')) if p.is_file()]
    (assets/'support-manifest.json').write_text(json.dumps({'version':1,'upstream':'94eb185e4abcc2d568aa8241fa02c43cdd86c439','files':records},indent=2)+'\n')
    notices=assets/'licenses';notices.mkdir()
    for root,label in ((upstream,'ComboShip'),(upstream/'libultraship','libultraship'),(upstream/'soh','Shipwright'),(upstream/'mm','2Ship'),(sdl,'SDL2')):
        for path in root.glob('*'):
            if path.is_file() and (path.name.upper().startswith('LICENSE') or path.name.upper().startswith('COPYING')):shutil.copy2(path,notices/(label+'-'+path.name))
    for dependency in sorted((native/'_deps').glob('*-src')):
        for path in dependency.iterdir():
            if path.is_file() and path.name.upper().startswith(('LICENSE','COPYING','NOTICE')):
                shutil.copy2(path,notices/(dependency.name+'-'+path.name))
    cache=(native/'CMakeCache.txt').read_text(encoding='utf-8')
    for line in cache.splitlines():
        if line.startswith('COMBO_VCPKG_PREFIX:'):
            prefix=Path(line.split('=',1)[1])
            for copyright in (prefix/'share').glob('*/copyright'):
                shutil.copy2(copyright,notices/('vcpkg-'+copyright.parent.name+'-copyright.txt'))
    shutil.copy2(app.parent/'LICENSE',notices/'ComboAndroid-LICENSE')
    copy_tree(app.parent/'licenses',notices/'ComboAndroid-dependencies')
    print(f'Staged {len(candidates)} ARM64 libraries, {sum(map(len,exported.values()))} game callbacks, and {len(records)} no-ROM support files.')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('upstream','native','sdl','ndk','app'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();stage(a.upstream.resolve(),a.native.resolve(),a.sdl.resolve(),a.ndk.resolve(),a.app.resolve())
