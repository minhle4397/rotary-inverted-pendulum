/**
 * @file    test_ctrl_filter.c
 * @brief   Filtered backward difference: w[k] = a*w[k-1] + (1-a)*(x[k]-x[k-1])/Ts.
 */
#include "ctrl_filter.h"
#include "ctrl_math.h"
#include "utest.h"

#define FC 10.0f
#define TS 0.001f

static void test_init_coefficient(void)
{
    ctrl_diff_t f;
    ctrl_diff_init(&f, FC, TS);
    UT_NEAR(f.a, expf(-CTRL_TWO_PI * FC * TS), 1e-7);
    UT_NEAR(f.ts, TS, 0.0);
    UT_CHECK(!f.primed);
    UT_NEAR(f.rate, 0.0, 0.0);
}

static void test_first_sample_returns_zero(void)
{
    ctrl_diff_t f;
    ctrl_diff_init(&f, FC, TS);
    /* Even with a large initial value there must be no derivative spike. */
    UT_NEAR(ctrl_diff_update(&f, 123.0f), 0.0, 0.0);
    UT_CHECK(f.primed);
}

static void test_constant_input_gives_zero(void)
{
    ctrl_diff_t f;
    ctrl_diff_init(&f, FC, TS);
    for (int k = 0; k < 200; k++) {
        UT_NEAR(ctrl_diff_update(&f, 5.0f), 0.0, 1e-9);
    }
}

static void test_ramp_converges_to_slope(void)
{
    ctrl_diff_t f;
    ctrl_diff_init(&f, FC, TS);
    const float v = 2.5f; /* units/s */
    float w       = 0.0f;
    for (int k = 0; k < 1000; k++) {
        w = ctrl_diff_update(&f, v * (float)k * TS);
    }
    UT_NEAR(w, v, 1e-3);
}

static void test_negative_ramp(void)
{
    ctrl_diff_t f;
    ctrl_diff_init(&f, FC, TS);
    const float v = -4.0f;
    float w       = 0.0f;
    for (int k = 0; k < 1000; k++) {
        w = ctrl_diff_update(&f, v * (float)k * TS);
    }
    UT_NEAR(w, v, 1e-3);
}

static void test_ramp_step_response_matches_closed_form(void)
{
    /* After priming at k=0, ramp of slope v: w[n] = v * (1 - a^n). */
    ctrl_diff_t f;
    ctrl_diff_init(&f, FC, TS);
    const float v = 1.0f;
    const float a = f.a;

    float an = 1.0f;
    for (int n = 0; n < 100; n++) {
        const float w = ctrl_diff_update(&f, v * (float)n * TS);
        UT_NEAR(w, v * (1.0f - an), 1e-4);
        an *= a;
    }
}

static void test_high_cutoff_is_raw_derivative(void)
{
    ctrl_diff_t f;
    ctrl_diff_init(&f, 1.0e6f, TS); /* a underflows to 0 */
    UT_NEAR(f.a, 0.0, 0.0);
    (void)ctrl_diff_update(&f, 0.0f);
    UT_NEAR(ctrl_diff_update(&f, 0.5f), 0.5f / TS, 1e-2);
    UT_NEAR(ctrl_diff_update(&f, 0.5f), 0.0, 1e-9);
}

static void test_lower_cutoff_is_slower(void)
{
    ctrl_diff_t slow, fast;
    ctrl_diff_init(&slow, 1.0f, TS);
    ctrl_diff_init(&fast, 50.0f, TS);
    UT_CHECK(slow.a > fast.a);

    float ws = 0.0f, wf = 0.0f;
    for (int k = 0; k < 20; k++) {
        ws = ctrl_diff_update(&slow, (float)k * TS);
        wf = ctrl_diff_update(&fast, (float)k * TS);
    }
    UT_CHECK(wf > ws); /* fast filter is closer to the true slope 1.0 */
    UT_CHECK(wf < 1.0f);
}

static void test_attenuates_alternating_noise(void)
{
    /* +-1 jitter has raw derivative of 2/Ts = 2000; the filter must reduce it a lot. */
    ctrl_diff_t f;
    ctrl_diff_init(&f, FC, TS);
    float peak = 0.0f;
    for (int k = 0; k < 500; k++) {
        const float w = ctrl_diff_update(&f, (k & 1) ? 1.0f : -1.0f);
        if (k > 200 && fabsf(w) > peak) {
            peak = fabsf(w);
        }
    }
    UT_CHECK(peak < 0.1f * (2.0f / TS));
}

static void test_retune_keeps_state(void)
{
    ctrl_diff_t f;
    ctrl_diff_init(&f, FC, TS);
    for (int k = 0; k < 50; k++) {
        (void)ctrl_diff_update(&f, (float)k * TS);
    }
    const float prev   = f.prev;
    const float rate   = f.rate;
    const bool primed  = f.primed;
    const float old_a  = f.a;

    ctrl_diff_retune(&f, 2.0f * FC, 2.0f * TS);
    UT_NEAR(f.prev, prev, 0.0);
    UT_NEAR(f.rate, rate, 0.0);
    UT_CHECK(f.primed == primed);
    UT_NEAR(f.ts, 2.0f * TS, 0.0);
    UT_NEAR(f.a, expf(-CTRL_TWO_PI * 2.0f * FC * 2.0f * TS), 1e-7);
    UT_CHECK(f.a != old_a);
}

static void test_retune_new_ts_scales_derivative(void)
{
    ctrl_diff_t f;
    ctrl_diff_init(&f, FC, TS);
    for (int k = 0; k < 10; k++) {
        (void)ctrl_diff_update(&f, 0.0f);
    }
    ctrl_diff_retune(&f, FC, 0.01f);
    float w = 0.0f;
    for (int k = 1; k < 2000; k++) {
        w = ctrl_diff_update(&f, 3.0f * (float)k * 0.01f); /* slope 3 at the new Ts */
    }
    UT_NEAR(w, 3.0, 1e-2);
}

int main(void)
{
    UT_RUN(test_init_coefficient);
    UT_RUN(test_first_sample_returns_zero);
    UT_RUN(test_constant_input_gives_zero);
    UT_RUN(test_ramp_converges_to_slope);
    UT_RUN(test_negative_ramp);
    UT_RUN(test_ramp_step_response_matches_closed_form);
    UT_RUN(test_high_cutoff_is_raw_derivative);
    UT_RUN(test_lower_cutoff_is_slower);
    UT_RUN(test_attenuates_alternating_noise);
    UT_RUN(test_retune_keeps_state);
    UT_RUN(test_retune_new_ts_scales_derivative);
    UT_REPORT();
}
