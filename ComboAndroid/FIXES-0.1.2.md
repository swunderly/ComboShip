# ComboShip Android 0.1.2 — text and new-profile fixes

Install `ComboShip-Android-0.1.2-dev-arm64.apk` over 0.1.1. It uses the same app identity and signing certificate; the emulator update preserved the existing imported ROMs, game archives, settings and linked saves. No re-extraction is required for these fixes.

## Changes

- Fixed the renderer's address checks for Android ARM64 tagged pointers. OoT stores character asset paths in heap-allocated font buffers. The desktop address limit rejected pointers with an Android top-byte tag, causing those paths to be treated as raw image data. Both the texture-signature and display-list filepath guards now check the address portion while preserving the original tagged pointer for resource access. This addresses a concrete code defect consistent with the reported blocks in name entry, saved names and dialogue. Physical-device confirmation is still pending. [Android's tagged-pointer documentation](https://source.android.com/docs/security/test/tagged-pointers) describes this platform behavior.
- New empty files open randomizer setup directly. ComboShip deliberately supports only the combined randomizer, so the old OoT quest carousel had one option despite displaying navigation arrows. Back returns to the file list; cancelling name entry returns through randomizer setup. The four top-level settings tabs retain upstream organization.

## Validation

- A regression test using the production pointer-check function failed before the fix when given a tagged address. After the fix, 1,288 cases pass in the Android ARM64 configuration and 1,288 in the desktop configuration. It covers all 256 top-byte tags, low/invalid/odd addresses, and preservation of the original pointer passed to the resource manager. Run `python ComboAndroid/tests/check_pointer_guards.py /path/to/upstream` from a Visual Studio x64 developer shell, or provide `--cxx clang++`/`--cxx g++`.
- Both engines, shared renderer, launcher and combined UI compiled. Android assembly and lint passed with zero errors. APK verification passed for eight ARM64 libraries, entry points, dependencies, 16 KiB ELF alignment and 9,036 support-file hashes. No ROM or game-derived archive filenames are packaged.
- Replayed the overlay on a fresh pinned upstream checkout. All 37 modified upstream files match the compiled source after newline normalization.
- Installed over 0.1.1 on the existing Android 15 emulator. Verified readable saved names and death count, direct entry into randomizer setup, Back to the file list, readable name-entry characters, entering a name, and cancelling name entry without creating a save. Loaded an existing MM save, used the Clock Tower portal to OoT Market, and read an OoT Navi dialogue with blue, white and red text.
- The installed APK hash matches the delivery APK. APK v2 signature verified. The emulator uses x86_64 with ARM64 translation and does not reproduce a physical ARM64 device's tagged heap behavior; the synthetic pointer test covers that case, while the phone retest remains necessary.

## Phone retest

1. Open a new empty profile. It should go straight to **Start Randomizer / Generate New Randomizer Seed / Open Randomizer Settings**, with no quest-logo carousel. Check **B** returns to the file list.
2. Start the randomizer and inspect the name-entry alphabet, numbers, punctuation and entered name.
3. Open your existing profile and check its displayed name and death count.
4. Talk to an OoT NPC and advance through several dialogue pages. Check letters, spaces and colored words. Check an MM dialogue too.

Report any remaining corruption with a screenshot, your device model and Android version. Saves, ROMs, local test logs and screenshots are excluded from the source archive.

APK size: 44,187,103 bytes.

SHA-256: `ad1af7a74a13d6c37c313b31c6f946c6b1dc95d5b31c1909bc3e6c4b8990e712`
