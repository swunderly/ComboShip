// Deliberately fails an assertion; selfcheck_tests.py verifies the failure is
// reported on stderr with its expression, file and line, and exits nonzero at
// once (no Debug CRT or Windows Error Reporting dialog).
#include <cassert>

int main(int argc, char**) {
    const int checks = argc;
    assert(checks == 0 && "COMBO_SELFCHECK_EXPECTED_FAILURE");
    return 0;
}
