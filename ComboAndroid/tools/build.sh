#!/usr/bin/env bash
# Linux/WSL build of this repository. Supply a bootstrapped vcpkg and a workspace for build output.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
upstream="$(cd "$root/.." && pwd)"
vcpkg="$(realpath "${1:?Usage: build.sh VCPKG_ROOT WORKSPACE}")"
work="$(realpath "${2:?WORKSPACE is required}")"
: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to NDK 28.2.13676358}"
: "${ANDROID_HOME:?Set ANDROID_HOME to the Android SDK root}"
export ANDROID_NDK_ROOT="$ANDROID_NDK_HOME"
for program in cmake ninja python3 ccache javac; do command -v "$program" >/dev/null || { echo "Missing $program"; exit 1; }; done
"$vcpkg/vcpkg" install --triplet arm64-android zlib libpng libogg libvorbis opus opusfile
cmake -S "$upstream" -B "$work/build-android" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 -DANDROID_STL=c++_shared -DCOMBO_VCPKG_PREFIX="$vcpkg/installed/arm64-android" -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DENABLE_SCRIPTING=OFF -DUSE_OPENGLES=ON 2>&1 | tee "$work/android-configure.log"
cmake --build "$work/build-android" --target ComboShip -j3 -- -k 0 2>&1 | tee "$work/android-build.log"
# Native host ZAPD produces only support archives; it never consumes a game ROM.
cmake -S "$upstream" -B "$work/build-assets" -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_SCRIPTING=OFF -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache 2>&1 | tee "$work/assets-configure.log"
cmake --build "$work/build-assets" --target GenerateSohOtr Generate2ShipOtr -j3 2>&1 | tee "$work/assets-build.log"
python3 "$root/tools/stage_runtime.py" --upstream "$upstream" --native "$work/build-android" --sdl "$work/build-android/_deps/sdl2-src" --ndk "$ANDROID_NDK_HOME" --app "$root/app"
# Package from clean outputs: incremental packaging leaves megabytes of dead space in the APK.
rm -rf "$root/app/build/outputs/apk" "$root/app/build/intermediates/apk"
bash "$root/gradlew" -p "$root" :app:assembleDebug --no-daemon --stacktrace 2>&1 | tee "$work/android-package.log"
python3 "$root/tools/verify_apk.py" "$root/app/build/outputs/apk/debug/app-debug.apk"
mkdir -p "$work/apk-output"
version=$(sed -n "s/^[[:space:]]*versionName '\([^']*\)'.*/\1/p" "$root/app/build.gradle")
test -n "$version"
package="$work/apk-output/ComboShip-Android-$version-arm64.apk"
cp "$root/app/build/outputs/apk/debug/app-debug.apk" "$package"
"$ANDROID_HOME/build-tools/35.0.0/apksigner" verify --verbose --print-certs "$package" > "$work/apk-output/signature.txt"
(cd "$work/apk-output" && sha256sum *.apk > SHA256SUMS.txt)
