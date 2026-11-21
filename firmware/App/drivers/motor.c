/**
 * @file motor.c
 * @brief Sign-magnitude PWM for 2 x BTN8982TA
 */

#include "motor.h"

#include <math.h>

#include "board_config.h"
#include "bsp.h"

static void apply(motor_t *m, float v_model)
{
    const float v_hw = v_model * (float)m->dir; // wiring correction
    float       duty = fabsf(v_hw) / m->vbus;
    if (duty > 1.0f) {
        duty = 1.0f;
    }
    const uint16_t cmp = (uint16_t)((duty * (float)m->period) + 0.5f);

    if (v_hw >= 0.0f) {
        bsp_pwm_set(cmp, 0u);
        m->active = MOTOR_HALF_A;
    } else {
        bsp_pwm_set(0u, cmp);
        m->active = MOTOR_HALF_B;
    }

    // sample the IS current in the middle of the on-time
    uint16_t trig = (uint16_t)(cmp / 2u);
    if (trig < BOARD_ADC_TRIG_MIN_CNT) {
        trig = BOARD_ADC_TRIG_MIN_CNT;
    }
    bsp_adc_trigger_set(trig);

    m->duty  = duty;
    m->v_cmd = (duty * m->vbus) * ((v_model >= 0.0f) ? 1.0f : -1.0f);
}

void motor_init(motor_t *m, float vbus, int8_t dir)
{
    m->vbus    = vbus;
    m->dir     = (dir < 0) ? (int8_t)-1 : (int8_t)1;
    m->period  = bsp_pwm_period();
    m->enabled = false;
    m->v_cmd   = 0.0f;
    m->duty    = 0.0f;
    m->active  = MOTOR_HALF_A;
    apply(m, 0.0f);
    bsp_bridge_enable(false);
}

void motor_enable(motor_t *m, bool enable)
{
    if (!enable) {
        apply(m, 0.0f);
    }
    if (enable != m->enabled) {
        bsp_bridge_enable(enable);
        m->enabled = enable;
    }
}

void motor_set_voltage(motor_t *m, float volts)
{
    apply(m, m->enabled ? volts : 0.0f);
}

float motor_voltage(const motor_t *m)
{
    return m->v_cmd;
}

float motor_duty(const motor_t *m)
{
    return m->duty;
}

motor_half_t motor_active_half(const motor_t *m)
{
    return m->active;
}

bool motor_is_enabled(const motor_t *m)
{
    return m->enabled;
}