/**
 * @file    test_motor_current.c
 * @brief   Motor driver: sign-magnitude PWM and ADC trigger placement (uses bsp_fake).
 */
#include "board_config.h"
#include "bsp_fake.h"
#include "motor.h"
#include "utest.h"

#define VBUS 12.0f

static void test_init_safe(void)
{
    motor_t m;
    fake_reset();
    g_fake.bridge_en = true;
    motor_init(&m, VBUS, +1);
    UT_CHECK(!motor_is_enabled(&m));
    UT_CHECK(!g_fake.bridge_en);
    UT_CHECK(g_fake.pwm_a == 0u && g_fake.pwm_b == 0u);
    UT_CHECK(m.period == 3600u);
}

static void test_disabled_ignores_command(void)
{
    motor_t m;
    fake_reset();
    motor_init(&m, VBUS, +1);
    motor_set_voltage(&m, 6.0f);
    UT_CHECK(g_fake.pwm_a == 0u && g_fake.pwm_b == 0u);
    UT_NEAR(motor_voltage(&m), 0.0f, 1e-6);
}

static void test_positive_uses_half_a(void)
{
    motor_t m;
    fake_reset();
    motor_init(&m, VBUS, +1);
    motor_enable(&m, true);
    UT_CHECK(g_fake.bridge_en);
    motor_set_voltage(&m, 6.0f); /* 50 % */
    UT_CHECK(g_fake.pwm_a == 1800u && g_fake.pwm_b == 0u);
    UT_CHECK(motor_active_half(&m) == MOTOR_HALF_A);
    UT_NEAR(motor_duty(&m), 0.5, 1e-6);
    UT_NEAR(motor_voltage(&m), 6.0, 1e-5);
}

static void test_negative_uses_half_b(void)
{
    motor_t m;
    fake_reset();
    motor_init(&m, VBUS, +1);
    motor_enable(&m, true);
    motor_set_voltage(&m, -3.0f); /* 25 % */
    UT_CHECK(g_fake.pwm_a == 0u && g_fake.pwm_b == 900u);
    UT_CHECK(motor_active_half(&m) == MOTOR_HALF_B);
    UT_NEAR(motor_voltage(&m), -3.0, 1e-5);
}

static void test_dir_inverts_hardware_not_model(void)
{
    motor_t m;
    fake_reset();
    motor_init(&m, VBUS, -1);
    motor_enable(&m, true);
    motor_set_voltage(&m, 6.0f);
    UT_CHECK(g_fake.pwm_a == 0u && g_fake.pwm_b == 1800u);
    UT_NEAR(motor_voltage(&m), 6.0, 1e-5); /* model sign preserved */
}

static void test_saturation(void)
{
    motor_t m;
    fake_reset();
    motor_init(&m, VBUS, +1);
    motor_enable(&m, true);
    motor_set_voltage(&m, 50.0f);
    UT_CHECK(g_fake.pwm_a == 3600u);
    UT_NEAR(motor_duty(&m), 1.0, 1e-6);
    UT_NEAR(motor_voltage(&m), VBUS, 1e-4);
    motor_set_voltage(&m, -50.0f);
    UT_CHECK(g_fake.pwm_b == 3600u);
    UT_NEAR(motor_voltage(&m), -VBUS, 1e-4);
}

static void test_adc_trigger_mid_on_time(void)
{
    motor_t m;
    fake_reset();
    motor_init(&m, VBUS, +1);
    motor_enable(&m, true);
    motor_set_voltage(&m, 6.0f);
    UT_CHECK(g_fake.adc_trig == 900u); /* 1800 / 2 */
    motor_set_voltage(&m, -3.0f);
    UT_CHECK(g_fake.adc_trig == 450u);
}

static void test_adc_trigger_clamped_to_min(void)
{
    motor_t m;
    fake_reset();
    motor_init(&m, VBUS, +1);
    motor_enable(&m, true);
    motor_set_voltage(&m, 0.0f);
    UT_CHECK(g_fake.adc_trig == BOARD_ADC_TRIG_MIN_CNT);
    motor_set_voltage(&m, 0.1f); /* cmp = 30 -> trig 15 < min */
    UT_CHECK(g_fake.adc_trig == BOARD_ADC_TRIG_MIN_CNT);
}

static void test_disable_forces_zero(void)
{
    motor_t m;
    fake_reset();
    motor_init(&m, VBUS, +1);
    motor_enable(&m, true);
    motor_set_voltage(&m, 9.0f);
    motor_enable(&m, false);
    UT_CHECK(!g_fake.bridge_en);
    UT_CHECK(g_fake.pwm_a == 0u && g_fake.pwm_b == 0u);
    UT_NEAR(motor_voltage(&m), 0.0f, 1e-6);
    motor_set_voltage(&m, 9.0f);
    UT_CHECK(g_fake.pwm_a == 0u);
}

int main(void)
{
    UT_RUN(test_init_safe);
    UT_RUN(test_disabled_ignores_command);
    UT_RUN(test_positive_uses_half_a);
    UT_RUN(test_negative_uses_half_b);
    UT_RUN(test_dir_inverts_hardware_not_model);
    UT_RUN(test_saturation);
    UT_RUN(test_adc_trigger_mid_on_time);
    UT_RUN(test_adc_trigger_clamped_to_min);
    UT_RUN(test_disable_forces_zero);
    UT_REPORT();
}
