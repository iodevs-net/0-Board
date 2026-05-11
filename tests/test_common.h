#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>

static int g_test_failures = 0;
static int g_test_count = 0;

#define TASSERT(cond, msg) do { \
    g_test_count++; \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL [%d] %s\n", g_test_count, msg); \
        g_test_failures++; \
    } else { \
        printf("  OK   [%d] %s\n", g_test_count, msg); \
    } \
} while(0)

#define TEST_REPORT() do { \
    printf("\n%d tests, %d failures\n", g_test_count, g_test_failures); \
    return g_test_failures > 0 ? 1 : 0; \
} while(0)

#endif
