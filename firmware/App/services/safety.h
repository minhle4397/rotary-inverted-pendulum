/**
 * @file safety.h
 * @brief Software safety monitor
 * The hardware E-stop cuts the bridge independently of software
 * This module adds software protections and REPORTS the E-stop:
 *
 *   ESTOP       INH node low while the bridge should be enabled
 *   ARM_LIMIT   |theta| > 45 deg
 *   OVERCURRENT |i| > SAFETY_I_TRIP_A for SAFETY_I_TRIP_TICKS
 *   DRIVER      BTN8982 IS fault current (over-temperature/over-current/UV)
 *   OVERSPEED   implausible velocity (encoder glitch, runaway)
 *   OVERRUN     control tick execution time above budget
 *
 * Limits are compile-time constants on purpose: safety limits are not
 * tunable through the UART.
 */

#ifndef SAFETY_H
#define SAFETY_H

#include <stdbool.h>
#include <stdint.h>

#define SAFETY_FAULT_ESTOP       (1u << 0)
#define SAFETY_FAULT_ARM_LIMIT   (1u << 1)
#define SAFETY_FAULT_OVERCURRENT (1u << 2)
#define SAFETY_FAULT_DRIVER      (1u << 3)
#define SAFETY_FAULT_OVERSPEED   (1u << 4)
#define SAFETY_FAULT_OVERRUN     (1u << 5)

typedef struct {
    float    theta_trip;   // rad
    float    i_trip;       // A
    uint16_t i_trip_ticks; // ticks
    float    dtheta_max;   // rad/s
    float    dalpha_max;   // rad/s
    uint32_t exec_max_us;  // us
} safety_cfg_t;

typedef struct {
    bool     motor_active; // bridge enabled and settled
    bool     inh_high;     // inh sense pin (E-stop chain closed)
    float    theta;
    float    dtheta;
    float    dalpha;
    float    current;
    bool     current_valid;
    bool     driver_fault;
    uint32_t exec_us; // duration of the previous tick
} safety_in_t;

typedef struct {
    safety_cfg_t cfg;
    uint16_t     oc_ticks;
    uint32_t     latched;
} safety_t;

void safety_init(safety_t *s, const safety_cfg_t *cfg);
// Evaluate all checks, latch new faults, return the latched set
uint32_t safety_check(safety_t *s, const safety_in_t *in);
void     safety_clear(safety_t *s);
uint32_t safety_faults(const safety_t *s);
// Short name of the lowest set fault bit (for LCD / log)
const char *safety_fault_name(uint32_t faults);

#endif