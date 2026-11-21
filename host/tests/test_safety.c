/**
 * @file    test_safety.c
 * @brief   Latched software safety monitor.
 */
#include "safety.h"
#include "utest.h"

static safety_cfg_t cfg(void)
{
    const safety_cfg_t c = {
        .theta_trip   = 0.785f,
        .i_trip       = 5.0f,
        .i_trip_ticks = 3u,
        .dtheta_max   = 20.0f,
        .dalpha_max   = 50.0f,
        .exec_max_us  = 800u,
    };
    return c;
}

static safety_in_t good(void)
{
    const safety_in_t in = {
        .motor_active  = true,
        .inh_high      = true,
        .theta         = 0.0f,
        .dtheta        = 0.0f,
        .dalpha        = 0.0f,
        .current       = 0.0f,
        .current_valid = true,
        .driver_fault  = false,
        .exec_us       = 100u,
    };
    return in;
}

static void test_ok(void)
{
    safety_t           s;
    const safety_cfg_t c  = cfg();
    const safety_in_t  in = good();
    safety_init(&s, &c);
    UT_CHECK(safety_check(&s, &in) == 0u);
    UT_CHECK(safety_faults(&s) == 0u);
    UT_CHECK(safety_fault_name(0u)[0] == 'O');
}

static void test_estop(void)
{
    safety_t           s;
    const safety_cfg_t c  = cfg();
    safety_in_t        in = good();
    safety_init(&s, &c);
    in.inh_high = false;
    UT_CHECK((safety_check(&s, &in) & SAFETY_FAULT_ESTOP) != 0u);
}

static void test_arm_limit(void)
{
    safety_t           s;
    const safety_cfg_t c  = cfg();
    safety_in_t        in = good();
    safety_init(&s, &c);
    in.theta = -0.9f;
    UT_CHECK((safety_check(&s, &in) & SAFETY_FAULT_ARM_LIMIT) != 0u);
}

static void test_overspeed(void)
{
    safety_t           s;
    const safety_cfg_t c  = cfg();
    safety_in_t        in = good();
    safety_init(&s, &c);
    in.dalpha = -60.0f;
    UT_CHECK((safety_check(&s, &in) & SAFETY_FAULT_OVERSPEED) != 0u);
    safety_clear(&s);
    in        = good();
    in.dtheta = 25.0f;
    UT_CHECK((safety_check(&s, &in) & SAFETY_FAULT_OVERSPEED) != 0u);
}

static void test_driver_fault(void)
{
    safety_t           s;
    const safety_cfg_t c  = cfg();
    safety_in_t        in = good();
    safety_init(&s, &c);
    in.driver_fault = true;
    UT_CHECK((safety_check(&s, &in) & SAFETY_FAULT_DRIVER) != 0u);
}

static void test_overrun_even_when_inactive(void)
{
    safety_t           s;
    const safety_cfg_t c  = cfg();
    safety_in_t        in = good();
    safety_init(&s, &c);
    in.motor_active = false;
    in.exec_us      = 900u;
    UT_CHECK((safety_check(&s, &in) & SAFETY_FAULT_OVERRUN) != 0u);
}

static void test_inactive_ignores_motor_checks(void)
{
    safety_t           s;
    const safety_cfg_t c  = cfg();
    safety_in_t        in = good();
    safety_init(&s, &c);
    in.motor_active = false;
    in.inh_high     = false;
    in.theta        = 3.0f;
    UT_CHECK(safety_check(&s, &in) == 0u);
}

static void test_overcurrent_needs_consecutive_ticks(void)
{
    safety_t           s;
    const safety_cfg_t c  = cfg();
    safety_in_t        in = good();
    safety_init(&s, &c);
    in.current = 6.0f;
    UT_CHECK(safety_check(&s, &in) == 0u);                              /* 1 */
    UT_CHECK(safety_check(&s, &in) == 0u);                              /* 2 */
    UT_CHECK((safety_check(&s, &in) & SAFETY_FAULT_OVERCURRENT) != 0u); /* 3 */
}

static void test_overcurrent_spike_decays(void)
{
    safety_t           s;
    const safety_cfg_t c  = cfg();
    safety_in_t        in = good();
    safety_init(&s, &c);
    in.current = 6.0f;
    (void)safety_check(&s, &in);
    (void)safety_check(&s, &in);
    in.current = 0.0f;
    for (int i = 0; i < 5; i++) {
        UT_CHECK(safety_check(&s, &in) == 0u);
    }
    in.current = 6.0f;
    UT_CHECK(safety_check(&s, &in) == 0u);
}

static void test_current_invalid_not_counted(void)
{
    safety_t           s;
    const safety_cfg_t c  = cfg();
    safety_in_t        in = good();
    safety_init(&s, &c);
    in.current       = 99.0f;
    in.current_valid = false;
    for (int i = 0; i < 10; i++) {
        UT_CHECK(safety_check(&s, &in) == 0u);
    }
}

static void test_latch_and_clear(void)
{
    safety_t           s;
    const safety_cfg_t c  = cfg();
    safety_in_t        in = good();
    safety_init(&s, &c);
    in.driver_fault = true;
    (void)safety_check(&s, &in);
    in = good();
    UT_CHECK((safety_check(&s, &in) & SAFETY_FAULT_DRIVER) != 0u); /* stays latched */
    safety_clear(&s);
    UT_CHECK(safety_faults(&s) == 0u);
    UT_CHECK(safety_check(&s, &in) == 0u);
}

static void test_fault_name_priority(void)
{
    UT_CHECK(safety_fault_name(SAFETY_FAULT_ESTOP | SAFETY_FAULT_OVERRUN)[0] == 'E');
    UT_CHECK(safety_fault_name(SAFETY_FAULT_OVERRUN)[0] == 'O');
    UT_CHECK(safety_fault_name(SAFETY_FAULT_DRIVER)[0] == 'D');
}

int main(void)
{
    UT_RUN(test_ok);
    UT_RUN(test_estop);
    UT_RUN(test_arm_limit);
    UT_RUN(test_overspeed);
    UT_RUN(test_driver_fault);
    UT_RUN(test_overrun_even_when_inactive);
    UT_RUN(test_inactive_ignores_motor_checks);
    UT_RUN(test_overcurrent_needs_consecutive_ticks);
    UT_RUN(test_overcurrent_spike_decays);
    UT_RUN(test_current_invalid_not_counted);
    UT_RUN(test_latch_and_clear);
    UT_RUN(test_fault_name_priority);
    UT_REPORT();
}
