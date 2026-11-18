/**
 * @file    utest.h
 * @brief   Minimal unit-test helpers (no external dependency).
 */
#ifndef UTEST_H
#define UTEST_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int ut_failures = 0;
static int ut_checks   = 0;

#define UT_CHECK(cond)                                                                \
    do {                                                                              \
        ut_checks++;                                                                  \
        if (!(cond)) {                                                                \
            ut_failures++;                                                            \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
        }                                                                             \
    } while (0)

#define UT_NEAR(a, b, tol)                                                                          \
    do {                                                                                            \
        ut_checks++;                                                                                \
        const double ut_a = (double)(a), ut_b = (double)(b);                                        \
        if (!(fabs(ut_a - ut_b) <= (double)(tol))) {                                                \
            ut_failures++;                                                                          \
            fprintf(stderr, "  FAIL %s:%d: %s = %.9g, expected %.9g (tol %g)\n", __FILE__, __LINE__, \
                    #a, ut_a, ut_b, (double)(tol));                                                 \
        }                                                                                           \
    } while (0)

#define UT_RUN(fn)                    \
    do {                              \
        printf("[ RUN  ] %s\n", #fn); \
        fn();                         \
    } while (0)

#define UT_REPORT()                                                                          \
    do {                                                                                     \
        printf("%d checks, %d failures -> %s\n", ut_checks, ut_failures,                     \
               ut_failures ? "FAILED" : "PASSED");                                           \
        return ut_failures ? EXIT_FAILURE : EXIT_SUCCESS;                                    \
    } while (0)

#endif /* UTEST_H */
