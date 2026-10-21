# Build ComboShip Android

This repository is the complete source: Varuuna/ComboShip (forked at `94eb185e4abcc2d568aa8241fa02c43cdd86c439`) with the Android port committed on top. Build it directly; nothing is patched in at build time. vcpkg is pinned to `10541e317a660f4165ba4ac2851ab54a8d4577b1`.

Prerequisites: Git, Python 3.10+, Java 17 or 21, Android SDK platform 35, build-tools 35.0.0, NDK 28.2.13676358, CMake 3.31.5, Ninja, and a bootstrapped vcpkg at the pinned revision. Windows also needs Visual Studio's C++ desktop workload; Linux needs the packages listed in the CI workflow and ccache. Allow several GB of disk space and substantial memory for the two game engines.

```sh
git clone -b android-source https://github.com/swunderly/ComboShip.git
git clone https://github.com/microsoft/vcpkg.git vcpkg
git -C vcpkg checkout 10541e317a660f4165ba4ac2851ab54a8d4577b1
```

## Full build

On Windows, bootstrap vcpkg and then run from a Visual Studio **x64 developer PowerShell**:

```powershell
./vcpkg/bootstrap-vcpkg.bat -disableMetrics
./ComboShip/ComboAndroid/tools/build.ps1 -Vcpkg ./vcpkg -Workspace ./combo-build `
    -AndroidSdk C:/Android/sdk -JavaHome C:/Java/jdk-21
```

On Linux/WSL, export `ANDROID_HOME`, `ANDROID_NDK_HOME` and `JAVA_HOME`, bootstrap vcpkg, create the output directory, then run:

```sh
./vcpkg/bootstrap-vcpkg.sh -disableMetrics
mkdir -p combo-build
bash ComboShip/ComboAndroid/tools/build.sh ./vcpkg ./combo-build
```

The pipeline compiles eight ARM64 shared libraries, uses a native host ZAPD to generate `soh.o2r` and `2ship.o2r` from repository support assets, merges the extractor XML trees, validates ELF dependencies, entry points and 16 KiB page alignment, then builds and signs a debug APK with the pinned Gradle wrapper. No ROM is consumed during the build. The APK and its checksum are written to `apk-output` inside the workspace.

## Rebuilding after a change

Rerun only the steps after the change, in the existing build directories:

1. `cmake --build <workspace>/build-android --target ComboShip`
2. `python ComboAndroid/tools/stage_runtime.py --upstream . --native <workspace>/build-android --sdl <workspace>/build-android/_deps/sdl2-src --ndk <ndk> --app ComboAndroid/app` (records the source commit in `build-info.json`)
3. Delete `ComboAndroid/app/build/outputs/apk` and `ComboAndroid/app/build/intermediates/apk`, then `gradlew -p ComboAndroid :app:assembleDebug`. Packaging on top of an old APK can leave megabytes of dead space in it.
4. `python ComboAndroid/tools/verify_apk.py ComboAndroid/app/build/outputs/apk/debug/app-debug.apk`. It rejects an APK with more than 1 MiB of space that no zip entry points at, and one whose libraries were built with the dev-only CMake option `COMBO_ENTRANCE_DEV` or `COMBO_MQ_WALK_DEV`.

A developer APK for the Master Quest emulator walk is built the same way with `-DCOMBO_MQ_WALK_DEV=ON` in its own build directory. It adds the console commands `mq_walk`, `mq_tp`, `mq_wait`, `mq_status`, `mq_log` and `mq_stop`, and an adb command file that `tools/mq_walk_driver.py` writes (see `DEVICE_TESTS.md`). `verify_apk.py` refuses it, so it can never be a release.

Development APKs use the builder's Android debug signing key. An APK from a different builder or CI has a different signature and will not install over an existing app. Keep the signing key for updates, and export saves before changing installation or signing identity. Private keys are not in the repository.

## Tests

From the repository root:

```sh
python ComboAndroid/tests/run_tests.py
```

This configures, builds and runs every host test (C++ tests, Python harnesses that compile extracted production code, and the Java ROM-import and storage-copy tests), then prints a pass/fail/skip summary; the exit code is nonzero on any failure. On Windows it finds Visual Studio automatically; on Linux it uses the system `c++`. Third-party headers (nlohmann json, Dear ImGui, spdlog) come from the `_deps` folder of a sibling CMake build, or `--deps`. Useful options: `-R <regex>` to run some tests, `--fresh` for a clean build directory (default `../build-tests`), `--sdl <prefix>` to include the audio latency harness, `--timeout <seconds>` per test (default 300). Tests that cannot run on the current platform are listed as skipped with the reason.

On Windows, every test program reports a failed assertion on stderr and exits nonzero instead of opening a crash dialog. The `selfcheck_failed_assert_reports_and_exits` test checks this.

## On-device instrumentation tests

The Android touch-overlay regression suite uses real multi-pointer MotionEvents and test-only preferences. After staging the runtime, run it on a dedicated emulator or device:

```sh
./ComboAndroid/gradlew -p ComboAndroid :app:assembleDebug :app:assembleDebugAndroidTest :app:lintDebug
adb install -r ComboAndroid/app/build/outputs/apk/debug/app-debug.apk
adb install -r ComboAndroid/app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb shell am instrument -w -r org.comboship.android.test/org.comboship.android.TouchOverlayInstrumentation
```

Use `gradlew.bat` on Windows and `adb -s SERIAL` when several devices are connected. A successful run reports `PASS: 58 touch-overlay checks` and `INSTRUMENTATION_CODE: -1`. The test APK is a separate package and is not part of the delivered app. It checks input and drawing without launching either game or reading game data.

The storage instrumentation is opt-in: build `:app:assembleDebugAndroidTest -PtestRunner=org.comboship.android.StorageInstrumentation`, install the app and test APK on a disposable emulator, grant its all-files permission, then run `adb shell am instrument -w -e disposableEmulator true org.comboship.android.test/org.comboship.android.StorageInstrumentation`. It refuses non-emulator devices and an existing visible ComboShip folder.

Device tests, including real extraction and cross-game saves, are tracked in [DEVICE_TESTS.md](DEVICE_TESTS.md).
