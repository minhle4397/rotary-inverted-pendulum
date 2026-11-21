/**
 * @file safety.c
 * @brief Latched software safety monitor
 */

#include "safety.h"

#include <math.h>
#include <stdint.h>

void safety_init(safety_t *s, const safety_cfg_t *cfg)
{
    s->cfg      = *cfg;
    s->oc_ticks = 0u;
    s->latched  = 0u;
}

uint32_t safety_check(safety_t *s, const safety_in_t *in)
{
    uint32_t f = 0u;

    if (in->exec_us > s->cfg.exec_max_us) {
        f |= SAFETY_FAULT_OVERRUN;
    }

    if (in->motor_active) {
        if (!in->inh_high) {
            f |= SAFETY_FAULT_ESTOP;
        }
        if (fabsf(in->theta) > s->cfg.theta_trip) {
            f |= SAFETY_FAULT_ARM_LIMIT;
        }
        if ((fabsf(in->dtheta) > s->cfg.dtheta_max) || (fabsf(in->dalpha) > s->cfg.dalpha_max)) {
            f |= SAFETY_FAULT_OVERSPEED;
        }
        if (in->driver_fault) {
            f |= SAFETY_FAULT_DRIVER;
        }
        if (in->current_valid && (fabsf(in->current) > s->cfg.i_trip)) {
            if (s->oc_ticks < s->cfg.i_trip_ticks) {
                s->oc_ticks++;
            }
        } else if (s->oc_ticks > 0u) {
            s->oc_ticks--;
        } else {
            // no over-current
        }
        if (s->oc_ticks >= s->cfg.i_trip_ticks) {
            f |= SAFETY_FAULT_OVERCURRENT;
        }
    } else {
        s->oc_ticks = 0u;
    }

    s->latched |= f;
    return s->latched;
}

void safety_clear(safety_t *s)
{
    s->latched  = 0u;
    s->oc_ticks = 0u;
}

uint32_t safety_faults(const safety_t *s)
{
    return s->latched;
}

const char *safety_fault_name(uint32_t faults)
{
    if ((faults & SAFETY_FAULT_ESTOP) != 0u) {
        return "E-STOP";
    }
    if ((faults & SAFETY_FAULT_ARM_LIMIT) != 0u) {
        return "ARM LIMIT";
    }
    if ((faults & SAFETY_FAULT_OVERCURRENT) != 0u) {
        return "OVERCURRENT";
    }
    if ((faults & SAFETY_FAULT_DRIVER) != 0u) {
        return "DRIVER";
    }
    if ((faults & SAFETY_FAULT_OVERSPEED) != 0u) {
        return "OVERSPEED";
    }
    if ((faults & SAFETY_FAULT_OVERRUN) != 0u) {
        return "OVERRUN";
    }
    return "OK";
}