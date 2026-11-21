/**
 * @file bsp_fake.h
 * @brief observable fake BSP for unit tests
 */

#ifndef BSP_FAKE_H
#define BSP_FAKE_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp.h"

typedef struct {
    uint16_t pwm_a, pwm_b, adc_trig, period;
    bool     bridge_en;
    uint8_t  i2c_last[128];
    uint16_t i2c_len;
    uint8_t  tx[8192];
    uint32_t tx_len;
    uint8_t  rx[256];
    uint16_t rx_len, rx_pos;
} bsp_fake_t;

extern bsp_fake_t g_fake;

void fake_reset(void);
void fake_rx_inject(const char *s);

#endif /* BSP_FAKE_H */