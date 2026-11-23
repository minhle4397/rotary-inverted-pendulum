/**
 * @file    test_motor_current.c
 * @brief   Motor driver (sign-magnitude PWM, ADC trigger) and current-sense conversion (uses bsp_fake).
 */
#include "board_config.h"
#include "bsp_fake.h"
#include "current_sense.h"
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

/* ---- current_sense ---- */

static void cs_setup(current_sense_t *cs, motor_t *m)
{
    fake_reset();
    motor_init(m, VBUS, +1);
    motor_enable(m, true);
    current_sense_init(cs, BOARD_IS_R_OHM, BOARD_IS_KILIS, BOARD_ADC_VREF_V, 4095u, BOARD_IS_FAULT_RAW,
                       BOARD_IS_MIN_DUTY);
}

static void test_cs_scale(void)
{
    current_sense_t cs;
    motor_t         m;
    cs_setup(&cs, &m);
    UT_NEAR(cs.amps_per_lsb, 4.76e-3, 1e-5);
}

static void test_cs_positive_uses_half_a(void)
{
    current_sense_t cs;
    motor_t         m;
    cs_setup(&cs, &m);
    motor_set_voltage(&m, 6.0f);
    const current_meas_t r = current_sense_update(&cs, 100u, 2000u, &m);
    UT_CHECK(r.valid && !r.driver_fault);
    UT_NEAR(r.amps, 100.0 * cs.amps_per_lsb, 1e-5);
}

static void test_cs_negative_uses_half_b_and_sign(void)
{
    current_sense_t cs;
    motor_t         m;
    cs_setup(&cs, &m);
    motor_set_voltage(&m, -6.0f);
    const current_meas_t r = current_sense_update(&cs, 2000u, 100u, &m);
    UT_CHECK(r.valid && !r.driver_fault);
    UT_NEAR(r.amps, -100.0 * cs.amps_per_lsb, 1e-5);
}

static void test_cs_low_duty_invalid(void)
{
    current_sense_t cs;
    motor_t         m;
    cs_setup(&cs, &m);
    motor_set_voltage(&m, 0.5f); /* duty ~4 % < 10 % */
    const current_meas_t r = current_sense_update(&cs, 100u, 0u, &m);
    UT_CHECK(!r.valid);
    UT_NEAR(r.amps, 0.0, 1e-9);
}

static void test_cs_disabled_invalid(void)
{
    current_sense_t cs;
    motor_t         m;
    cs_setup(&cs, &m);
    motor_set_voltage(&m, 6.0f);
    motor_enable(&m, false);
    const current_meas_t r = current_sense_update(&cs, 100u, 0u, &m);
    UT_CHECK(!r.valid);
}

static void test_cs_fault_on_either_channel(void)
{
    current_sense_t cs;
    motor_t         m;
    cs_setup(&cs, &m);
    motor_set_voltage(&m, 6.0f);
    current_meas_t r = current_sense_update(&cs, 100u, BOARD_IS_FAULT_RAW, &m); /* idle half faulted */
    UT_CHECK(r.driver_fault && !r.valid);
    UT_NEAR(r.amps, 0.0, 1e-9);
    r = current_sense_update(&cs, BOARD_IS_FAULT_RAW - 1u, 0u, &m);
    UT_CHECK(!r.driver_fault && r.valid);
    r = current_sense_update(&cs, BOARD_IS_FAULT_RAW, 0u, &m);
    UT_CHECK(r.driver_fault);
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
    UT_RUN(test_cs_scale);
    UT_RUN(test_cs_positive_uses_half_a);
    UT_RUN(test_cs_negative_uses_half_b_and_sign);
    UT_RUN(test_cs_low_duty_invalid);
    UT_RUN(test_cs_disabled_invalid);
    UT_RUN(test_cs_fault_on_either_channel);
    UT_REPORT();
}
