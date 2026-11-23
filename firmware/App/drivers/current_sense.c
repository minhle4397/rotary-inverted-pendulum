/**
 * @file current_sense.c
 * @brief BTN8982TA current sense conversion
 */

#include "current_sense.h"
#include <stdint.h>

#include "motor.h"

void current_sense_init(current_sense_t *cs, float r_is, float k_ilis, float vref, uint16_t full_scale,
                        uint16_t fault_raw, float min_duty)
{
    cs->amps_per_lsb = (vref / (float)full_scale) / r_is * k_ilis; // 4.76 mA/LSB
    cs->fault_raw    = fault_raw;
    cs->min_duty     = min_duty;
}

current_meas_t current_sense_update(const current_sense_t *cs, uint16_t raw_a, uint16_t raw_b, const motor_t *m)
{
    current_meas_t r;
    r.driver_fault      = (raw_a >= cs->fault_raw) || (raw_b >= cs->fault_raw);
    const uint16_t raw  = (motor_active_half(m) == MOTOR_HALF_A) ? raw_a : raw_b;
    const float    sign = (motor_voltage(m) >= 0.0f) ? 1.0f : -1.0f;

    r.valid = motor_is_enabled(m) && (motor_duty(m) >= cs->min_duty) && !r.driver_fault;
    r.amps  = r.valid ? sign * (float)raw * cs->amps_per_lsb : 0.0f;
    return r;
}
