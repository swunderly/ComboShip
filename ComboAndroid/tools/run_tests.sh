#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cmake -S "$root/tests" -B "$root/build/host-tests" -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build "$root/build/host-tests" --parallel 2
ctest --test-dir "$root/build/host-tests" --output-on-failure
mkdir -p "$root/build/java-tests"
javac -d "$root/build/java-tests" "$root/app/src/main/java/org/comboship/android/RomImporter.java" "$root/app/src/main/java/org/comboship/android/DataLock.java" "$root/tests/RomImporterTest.java"
java -cp "$root/build/java-tests" org.comboship.android.RomImporterTest
javac -cp "$root/build/java-tests" -d "$root/build/java-tests" "$root/app/src/main/java/org/comboship/android/StorageCopy.java" "$root/tests/StorageCopyTest.java"
java -cp "$root/build/java-tests" org.comboship.android.StorageCopyTest
python3 -m py_compile "$root"/tools/*.py
