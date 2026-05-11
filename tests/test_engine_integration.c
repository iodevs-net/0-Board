// SPDX-License-Identifier: MIT — see LICENSE file
//
// Engine integration/contract tests — null guards, zero keysym, no-XTest.

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include "test_common.h"
#include "engine.h"

// Replicate struct Engine so we can set use_xtest in tests.
// engine.h only forward-declares `typedef struct Engine Engine`.
struct Engine {
    Display *display;
    bool use_xtest;
    int event_delay_us;
    bool owns_display;
};

int main(void) {
    printf("=== engine integration tests ===\n");

    // 1. Null engine guard
    int r = engine_send_key_ex(NULL, XK_a, true, 0);
    TASSERT(r == -1, "null engine returns -1");

    // Check if X display is available — skip X-dependent tests if not
    Display *test_dpy = XOpenDisplay(NULL);
    if (!test_dpy) {
        printf("  SKIP: no X display available, skipping X-dependent tests\n");
        TEST_REPORT();
    }
    XCloseDisplay(test_dpy);

    // 2. Zero keysym guard
    {
        EngineConfig cfg = { .display = NULL, .use_xtest = true };
        Engine *e = engine_create(&cfg);
        TASSERT(e != NULL, "engine created with null display");

        r = engine_send_key_ex(e, 0, true, 0);
        TASSERT(r == -1, "zero keysym returns -1");

        engine_destroy(e);
    }

    // 3. No XTest guard
    {
        EngineConfig cfg = { .display = NULL, .use_xtest = true };
        Engine *e = engine_create(&cfg);
        TASSERT(e != NULL, "engine created for no-xtest test");

        e->use_xtest = false;
        r = engine_send_key_ex(e, XK_a, true, 0);
        TASSERT(r == -1, "no xtest returns -1");

        engine_destroy(e);
    }

    // 4. engine_flush with null — should not crash
    engine_flush(NULL);
    TASSERT(1, "engine_flush(NULL) did not crash");

    // 5. engine_destroy with null — should not crash
    engine_destroy(NULL);
    TASSERT(1, "engine_destroy(NULL) did not crash");

    TEST_REPORT();
}
