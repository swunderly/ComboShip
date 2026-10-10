/* Linked into every test executable on Windows: CMake test targets through
 * CMakeLists.txt, harness programs through hostcxx.build(). A failed assert(),
 * abort() or crash must print to stderr and exit nonzero at once. By default
 * the Debug CRT opens modal dialogs and Windows Error Reporting may wait for
 * minutes, which hangs ctest. Assertions stay enabled (/UNDEBUG); only the
 * reporting changes. Valid as C or C++; a no-op on other platforms. */
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <crtdbg.h>
#include <stdlib.h>

static void ComboNoCrashDialogs(void) {
    /* Debug CRT reports (failed _ASSERT, STL checks, "abort() has been called"). */
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    /* assert(): print "Assertion failed: expr, file f, line n" to stderr. */
    _set_error_mode(_OUT_TO_STDERR);
    /* abort(): no message box and no Windows Error Reporting round trip. */
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    /* Crashes: no "has stopped working" or critical-error boxes. */
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
}

#if defined(__cplusplus)
[[maybe_unused]] static const int comboNoCrashDialogs = (ComboNoCrashDialogs(), 0);
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) void (*comboNoCrashDialogs)(void) = ComboNoCrashDialogs;
#else
__attribute__((constructor)) static void ComboNoCrashDialogsAtStartup(void) { ComboNoCrashDialogs(); }
#endif
#else
typedef int ComboNoCrashDialogsUnused; /* keep the translation unit non-empty */
#endif
