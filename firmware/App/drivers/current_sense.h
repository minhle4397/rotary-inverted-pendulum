/**
 * @file current_sense.h
 * @brief BTN8982TA IS pin -> motor current
 * I_load = (raw * Vref / 4095) / R_IS * kILIS
 *
 * Only the high-side MOSFET of the PWM-ing half-bridge carries the load
 * current during the on-time, so the value of the ACTIVE half-bridge is
 * used, signed with the voltage direction. At small duty the on-time is too
 * short for the IS output to settle -> measurement flagged invalid.
 * A saturated IS channel (fault current 4..6.5 mA) means a driver fault.
 */

#ifndef CURRENT_SENSE_H
#define CURRENT_SENSE_H

#include <stdbool.h>
#include <stdint.h>

#include "motor.h"

typedef struct {
    float    amps_per_lsb;
    uint16_t fault_raw;
    float    min_duty;
} current_sense_t;

typedef struct {
    float amps;         // signed motor current [A]
    bool  valid;        // false when duty too small / motor off
    bool  driver_fault; // IS fault current detected on a channel
} current_meas_t;

void current_sense_init(current_sense_t *cs, float r_is, float k_ilis, float vref, uint16_t full_scale,
                        uint16_t fault_raw, float min_duty);
// raw_a / raw_b = ADC of the IS pin of half-bridge A / B (apply BOARD_IS_SWAP before calling)
current_meas_t current_sense_update(const current_sense_t *cs, uint16_t raw_a, uint16_t raw_b, const motor_t *m);

#endif /* CURRENT_SENSE_H */
