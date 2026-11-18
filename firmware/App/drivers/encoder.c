/**
 * @file encoder.c
 * @brief Quadrature encoder position extension (16->32bit)
 */

#include "encoder.h"
#include <stdint.h>

void encoder_init(encoder_t *e, int8_t dir, uint32_t cpr, float offset_rad)
{
    e->count            = 0;
    e->last_raw         = 0u;
    e->dir              = (dir < 0) ? (int8_t)-1 : (int8_t)1;
    e->primed           = false;
    e->rad_per_count    = 6.28318530717959f / (float)cpr; 
    e->offset_rad       = offset_rad;
}

void encoder_update(encoder_t *e, uint16_t raw){
    if(!e->primed){
        e->last_raw = raw;
        e->primed   = true;
        return;
    }
    const int16_t delta = (int16_t)(uint16_t)(raw - e->last_raw);
    e->last_raw = raw;
    e->count   += (int32_t)e->dir * (int32_t)delta;
}

float encoder_angle(const encoder_t *e){
    return e->offset_rad + (float)e->count *e->rad_per_count;
}

int32_t encoder_count(const encoder_t *e){
    return e->count;
}

void encoder_rezero(encoder_t *e, float angle_rad)
{
    e->count        = 0;
    e->offset_rad   = angle_rad;
}