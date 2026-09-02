#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <math.h>
#include <stdio.h>

typedef struct
{
    unsigned int passed;
    unsigned int failed;
} test_summary_t;

static void Test_CheckNear(
    test_summary_t *summary,
    const char *name,
    float measured,
    float expected,
    float absolute_tolerance)
{
    float absolute_error = fabsf(measured - expected);
    float denominator = fabsf(expected);
    float relative_error = (denominator > 1.0e-20f)
                               ? (absolute_error / denominator)
                               : absolute_error;
    int passed = isfinite(measured) && (absolute_error <= absolute_tolerance);

    printf("%-34s measured=% .9f expected=% .9f abs_err=% .3e rel_err=% .3e %s\n",
           name,
           (double)measured,
           (double)expected,
           (double)absolute_error,
           (double)relative_error,
           passed ? "PASS" : "FAIL");
    if (passed)
    {
        ++summary->passed;
    }
    else
    {
        ++summary->failed;
    }
}

static void Test_CheckU32(
    test_summary_t *summary,
    const char *name,
    unsigned int measured,
    unsigned int expected)
{
    unsigned int absolute_error = (measured > expected)
                                      ? (measured - expected)
                                      : (expected - measured);
    int passed = (measured == expected);

    printf("%-34s measured=%u expected=%u abs_err=%u rel_err=%s %s\n",
           name,
           measured,
           expected,
           absolute_error,
           (expected == 0U) ? "N/A" : ((absolute_error == 0U) ? "0" : ">0"),
           passed ? "PASS" : "FAIL");
    if (passed)
    {
        ++summary->passed;
    }
    else
    {
        ++summary->failed;
    }
}

#endif /* TEST_HELPERS_H */
