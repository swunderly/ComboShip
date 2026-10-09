# Build ComboShip Android

The overlay pins Varuuna/ComboShip to `94eb185e4abcc2d568aa8241fa02c43cdd86c439` and vcpkg to `10541e317a660f4165ba4ac2851ab54a8d4577b1`. Use a **fresh checkout** of that ComboShip revision. The patcher deliberately refuses existing modifications and repeat application. It never resets a checkout.

Prerequisites: Git, Python 3.10+, Java 17 or 21, Android SDK platform 35, build-tools 35.0.0, NDK 28.2.13676358, CMake 3.31.5, Ninja, and a bootstrapped vcpkg at the pinned revision. Windows also needs Visual Studio's C++ desktop workload; Linux needs the packages listed in the CI workflow and ccache. Allow several GB of disk space and substantial memory for the two game engines.

```sh
git clone https://github.com/Varuuna/ComboShip.git upstream
git -C upstream checkout 94eb185e4abcc2d568aa8241fa02c43cdd86c439
git clone https://github.com/microsoft/vcpkg.git vcpkg
git -C vcpkg checkout 10541e317a660f4165ba4ac2851ab54a8d4577b1
```

On Windows, bootstrap vcpkg and then run from a Visual Studio **x64 developer PowerShell**:

```powershell
./vcpkg/bootstrap-vcpkg.bat -disableMetrics
./ComboAndroid/tools/build.ps1 -Upstream ./upstream -Vcpkg ./vcpkg `
    -Workspace ./combo-build -AndroidSdk C:/Android/sdk -JavaHome C:/Java/jdk-21
```

On Linux/WSL, export `ANDROID_HOME`, `ANDROID_NDK_HOME`, and `JAVA_HOME`, bootstrap vcpkg, create the output directory, then run:

```sh
./vcpkg/bootstrap-vcpkg.sh -disableMetrics
mkdir -p combo-build
bash ComboAndroid/tools/build.sh ./upstream ./vcpkg ./combo-build
```

The pipeline compiles eight ARM64 shared libraries, uses a native host ZAPD to generate `soh.o2r` and `2ship.o2r` from repository support assets, merges the extractor XML trees, validates ELF dependencies/entry points and 16 KiB page alignment, then builds and signs a debug APK with the pinned Gradle wrapper. No ROM is consumed during the build. The APK and checksum are written to `apk-output` inside the workspace.

For an interrupted build, continue the failed CMake/Gradle command in its existing build directory. Do not rerun the patcher on the modified checkout. Regenerate staged files with `stage_runtime.py` after changes to native code. The staging tool replaces only its generated app assets, SDL Java, and JNI library directories.

Tests on Linux: `bash ComboAndroid/tools/run_tests.sh`. The CMake input test also runs with MSVC on Windows; POSIX path/session-lock tests require Linux. The Java ROM, record-lock and verified-storage-copy tests are platform independent. The full Android Java build validates against API 35.

The Android touch-overlay regression suite uses real multi-pointer MotionEvents and test-only preferences. After staging the runtime, run it on a dedicated compatible emulator or device:

```sh
./ComboAndroid/gradlew -p ComboAndroid :app:assembleDebug :app:assembleDebugAndroidTest :app:lintDebug
adb install -r ComboAndroid/app/build/outputs/apk/debug/app-debug.apk
adb install -r ComboAndroid/app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb shell am instrument -w -r org.comboship.android.test/org.comboship.android.TouchOverlayInstrumentation
```

Use `gradlew.bat` on Windows and `adb -s SERIAL` when multiple devices are connected. A successful run reports `PASS: 58 touch-overlay checks` and `INSTRUMENTATION_CODE: -1`. The test APK is a separate package and is not included in the delivered app. The suite checks input and drawing behavior without launching either game or accessing game data; live gameplay checks remain part of device validation.

For the tracker's visibility, transition and filter tests, configure these CMake tests with `-DCOMBO_SOURCE_DIR=/absolute/path/to/patched/upstream/combo`, build `check_tracker_tests`, and run `ctest -R shared_check_tracker --output-on-failure` in the test build directory. The production header under test is installed by `patches/check-tracker.patch`. Both game adapters and the UI are included in normal native builds and export verification.

For combined multiplayer protocol tests, also pass `-DCOMBO_JSON_INCLUDE_DIR=/absolute/path/to/json/single_include`, using the native build's nlohmann JSON dependency. Build `multiplayer_protocol_tests`, then run `ctest -R multiplayer_protocol --output-on-failure`. The tests use the production `network/ComboAnchorProtocol.h` installed by `patches/multiplayer.patch`; they cover paired saves, the stock server's cached response, seed isolation, queued message routing and per-game duplicate journals. Runtime networking still needs integration testing with two clients and an Anchor server.

For feature navigation, build `navigation_tests` in the same configured test project and run `ctest -R feature_navigation --output-on-failure`. This covers page aliases across engines, replacement of native tracker pages, shared settings and fallback visibility for new upstream features. `patches/unified-experience.patch` applies after the tracker patch. The test can also be compiled with the NDK for a local Android emulator; keep assertions enabled.

Development APKs use the builder's Android debug signing key. An APK from a different builder or CI may have a different signature. Keep the signing key locally for updates, and export saves before changing installation/signing identity. Private keys are not in the source archive.

Source replay is checked against a fresh pinned checkout. Device tests, including actual extraction and cross-game saves, are tracked in [DEVICE_TESTS.md](DEVICE_TESTS.md).

## Audio regression checks (0.1.16)

From an x64 Visual Studio developer shell on Windows, run `python ComboAndroid/tests/audio_latency_tests.py --source /path/to/patched/ComboShip --sdl /path/to/vcpkg/installed/x64-windows-static-md --build-dir /path/to/test-output`. The harness compiles the actual queue/conversion methods and both producers against real SDL2, with a simulated hardware queue. It verifies continuous resampling, source-frame accounting, signal frequency and both game schedules; it does not measure acoustic latency.

The included storage instrumentation is opt-in: build `:app:assembleDebugAndroidTest -PtestRunner=org.comboship.android.StorageInstrumentation`, install the app and test APK on a disposable emulator, grant its all-files permission, then run `adb shell am instrument -w -e disposableEmulator true org.comboship.android.test/org.comboship.android.StorageInstrumentation`. It refuses non-emulator devices and an existing visible ComboShip folder. The default instrumentation runner remains the touch-control tests.

## MM visibility regression checks (0.1.17)

Run `python ComboAndroid/tests/mm_check_visibility_tests.py --source /path/to/patched/ComboShip --build-dir /path/to/test-output --compiler cl` from an x64 Visual Studio developer shell (use `c++` elsewhere). It compiles the production observation gate against controlled camera and collision adapters, checking clipping, wide/narrow viewports, drawn model positioning, invisible scale, transitions, screen fills, pause overlays and blocked/clear sight lines. Complement it with `mm_check_knowledge_tests.py` for save compatibility and observation persistence. Live scene tests are needed to validate real collision geometry; these host tests do not simulate rendering.

## Gameplay rules and screen geometry (0.1.18)

The existing CMake test project also builds `gameplay_rules_tests` and `screen_fit_tests` when configured with the patched `COMBO_SOURCE_DIR` and `COMBO_JSON_INCLUDE_DIR` above. Run `ctest --output-on-failure` to include persistence, malformed-rule imports, equipment prerequisites, viewport geometry, navigation, tracker, multiplayer and input regressions. Keep assertions enabled.

Run `python ComboAndroid/tests/progression_rules_tests.py --source /path/to/patched/ComboShip --json-include /path/to/json/single_include --build-dir /path/to/test-output --compiler cl` from an x64 Visual Studio developer shell (use `c++` elsewhere). This compiles extracted production well-search, Fierce Deity sword and chick-recruitment code against controlled engine fixtures. Run `mm_check_knowledge_tests.py` with the same arguments for knowledge-source persistence and merge regressions. Neither harness replaces an actual gameplay route or full-seed playthrough.
