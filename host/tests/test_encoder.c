/**
 * @file    test_encoder.c
 * @brief   16-bit counter extension, direction, calibration.
 */
#include "ctrl_math.h"
#include "encoder.h"
#include "utest.h"

static void test_first_sample_is_reference(void)
{
    encoder_t e;
    encoder_init(&e, 1, 8192u, CTRL_PI);
    encoder_update(&e, 40000u);
    UT_CHECK(encoder_count(&e) == 0);
    UT_NEAR(encoder_angle(&e), CTRL_PI, 1e-6);
}

static void test_wrap_forward_backward(void)
{
    encoder_t e;
    encoder_init(&e, 1, 8192u, 0.0f);
    encoder_update(&e, 65530u);
    encoder_update(&e, 4u); /* +10 across the wrap */
    UT_CHECK(encoder_count(&e) == 10);
    encoder_update(&e, 65520u); /* -20 across the wrap */
    UT_CHECK(encoder_count(&e) == -10);
}

static void test_many_revolutions(void)
{
    encoder_t e;
    encoder_init(&e, 1, 8192u, 0.0f);
    uint16_t raw = 0u;
    encoder_update(&e, raw);
    for (int i = 0; i < 100; i++) { /* 100 x 1000 counts = 12.2 rev, wraps the 16-bit counter */
        raw = (uint16_t)(raw + 1000u);
        encoder_update(&e, raw);
    }
    UT_CHECK(encoder_count(&e) == 100000);
    UT_NEAR(encoder_angle(&e), 100000.0 * 2.0 * M_PI / 8192.0, 1e-2);
}

static void test_direction_and_rezero(void)
{
    encoder_t e;
    encoder_init(&e, -1, 8192u, 0.0f);
    encoder_update(&e, 100u);
    encoder_update(&e, 2148u); /* +2048 raw = quarter turn, inverted */
    UT_NEAR(encoder_angle(&e), -M_PI / 2.0, 1e-5);
    encoder_rezero(&e, CTRL_PI);
    UT_NEAR(encoder_angle(&e), CTRL_PI, 1e-6);
    encoder_update(&e, 2149u);
    UT_CHECK(encoder_count(&e) == -1);
}

int main(void)
{
    UT_RUN(test_first_sample_is_reference);
    UT_RUN(test_wrap_forward_backward);
    UT_RUN(test_many_revolutions);
    UT_RUN(test_direction_and_rezero);
    UT_REPORT();
}
