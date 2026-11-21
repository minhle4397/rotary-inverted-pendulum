/**
 * @file bsp_fake.c
 * @brief Fake BSP for unit tests: records outputs, injects inputs
 */

#include "bsp_fake.h"

#include <string.h>

bsp_fake_t g_fake;

void fake_reset(void)
{
    memset(&g_fake, 0, sizeof g_fake);
    g_fake.period = 3600u;
}

void fake_rx_inject(const char *s)
{
    const size_t n = strlen(s);
    for (size_t i = 0; i < n && g_fake.rx_len < sizeof g_fake.rx; i++) {
        g_fake.rx[g_fake.rx_len++] = (uint8_t)s[i];
    }
}

void bsp_init(void)
{
}
void bsp_tick_start(bsp_tick_cb_t cb)
{
    (void)cb;
}
uint16_t bsp_enc_arm_raw(void)
{
    return 0;
}
uint16_t bsp_enc_pend_raw(void)
{
    return 0;
}
void bsp_current_raw(uint16_t *a, uint16_t *b)
{
    *a = 0;
    *b = 0;
}
uint16_t bsp_pwm_period(void)
{
    return g_fake.period ? g_fake.period : 3600u;
}
void bsp_pwm_set(uint16_t a, uint16_t b)
{
    g_fake.pwm_a = a;
    g_fake.pwm_b = b;
}
void bsp_adc_trigger_set(uint16_t c)
{
    g_fake.adc_trig = c;
}
void bsp_bridge_enable(bool en)
{
    g_fake.bridge_en = en;
}
bool bsp_bridge_inhibit_sense(void)
{
    return g_fake.bridge_en;
}
bool bsp_btn_start_raw(void)
{
    return false;
}
bool bsp_btn_stop_raw(void)
{
    return false;
}
void bsp_led_set(bool on)
{
    (void)on;
}

bool bsp_i2c_write(uint8_t addr7, const uint8_t *buf, uint16_t len)
{
    (void)addr7;
    g_fake.i2c_len = (len > sizeof g_fake.i2c_last) ? (uint16_t)sizeof g_fake.i2c_last : len;
    memcpy(g_fake.i2c_last, buf, g_fake.i2c_len);
    return true;
}

bool bsp_uart_tx_busy(void)
{
    return false;
}

bool bsp_uart_tx(const uint8_t *buf, uint16_t len)
{
    if (g_fake.tx_len + len <= sizeof g_fake.tx) {
        memcpy(&g_fake.tx[g_fake.tx_len], buf, len);
        g_fake.tx_len += len;
    }
    return true;
}

uint16_t bsp_uart_rx(uint8_t *buf, uint16_t max)
{
    uint16_t n = 0;
    while (g_fake.rx_pos < g_fake.rx_len && n < max) {
        buf[n++] = g_fake.rx[g_fake.rx_pos++];
    }
    return n;
}

uint32_t bsp_millis(void)
{
    return 0u;
}
void bsp_delay_ms(uint32_t ms)
{
    (void)ms;
}
uint32_t bsp_cycles(void)
{
    return 0u;
}
uint32_t bsp_cycles_per_us(void)
{
    return 72u;
}
void bsp_watchdog_kick(void)
{
}
uint32_t bsp_irq_lock(void)
{
    return 0u;
}
void bsp_irq_unlock(uint32_t key)
{
    (void)key;
}
