/**
 * @file encoder.h
 * @brief Quadrature encoder: 16-bit hardware counter -> 32-bit position -> angle.
 */

#ifndef ENCODER_H
#define ENCODER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct{
    int32_t     count;          //extended position [counts]
    uint16_t    last_raw;       //previous hardware counter 
    int8_t      dir;            //+1/-1 wiring direction
    bool        primed;         //first sample received
    float       rad_per_count;  //2*pi /cpr
    float       offset_rad;     //angle at count = 0
} encoder_t;

/**
 * @param dir           +1 or -1 (board_config.h)
 * @param cpr           counts per revol (x4)
 * @param offset_rad    angle reported at power-up (count 0), e.g. pi for hanging pendulum
 */

void encoder_init(encoder_t *e, int8_t dir, uint32_t cpr, float offset_rad);
//feed the current hardware counter. The first call only latches the reference.
void encoder_update(encoder_t *e, uint16_t raw);
//unwrapped angle [rad] = offset + dir * count * 2pi/cpr
float encoder_angle(const encoder_t *e);
int32_t encoder_count(const encoder_t *e);
//define the current position as angle_rad (calibration)
void encoder_rezero(encoder_t *e, float angle_rad);
#endif
