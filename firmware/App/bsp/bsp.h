/**
 * @file bsp.h
 * @brief Board support package interface - the only boundary to hardware
 * Implementation
 * bsp_stm32f1.c        target, built on the STM32CubeMX generated HAL handles
 * host/sil/bsp_sim.c   software-in-the-loop: the same firmware runs on a PC
 *                      against a nonlinear plant model
 * host/tests/bsp_fake.c unit tests
 * Everything above this layer (drivers, services, control, app) is portable C
 * and never includes a HAL header.
 */

#ifndef BSP_H
#define BSP_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*bsp_tick_cb_t)(void);

// lifecycle
// Start encoders, PWM, ADC+DMA and UART RX. Bridge stays DISABLED
void bsp_init(void);
// Start the 1kHz control interrupt (TIM3) and call cb from it.
void bsp_tick_start(bsp_tick_cb_t cb);

// sensors
uint16_t bsp_enc_arm_raw(void);  // TIM4 counter (theta)
uint16_t bsp_enc_pend_raw(void); // TIM2 counter (alpha)
// Mean of the IS1/IS2 ADC samples taken since the previous call [LSB]
void bsp_current_raw(uint16_t *is_a, uint16_t *is_b);

// power stage
uint16_t bsp_pwm_period(void); //< ARR + 1 (3600 @ 20 kHz)
// Compare values of half-bridge A (PA8) and B (PA11); period = 100%
void bsp_pwm_set(uint16_t cmp_a, uint16_t cmp_b);
// ADC trigger instant inside the PWM period (TIM1 CH3 compare)
void bsp_adc_trigger_set(uint16_t cmp);
// BTN8982 INH through the E-stop chain (PB0)
void bsp_bridge_enable(bool enable);
// Reads the INH node (PB1): true = bridge enabled AND E-stop closed
bool bsp_bridge_inhibit_sense(void);

// HMI
bool bsp_btn_start_raw(void); // true = pressed (active low on PCB)
bool bsp_btn_stop_raw(void);
void bsp_led_set(bool on);
bool bsp_i2c_write(uint8_t addr7, const uint8_t *buf, uint16_t len);

// Non-blocking DMA transmit. buf must stay valid until bsp_uart_tx_busy() is false
bool bsp_uart_tx(const uint8_t *buf, uint16_t len);
bool bsp_uart_tx_busy(void);
// Copy received bytes (circular DMA) into buf, returns the count
uint16_t bsp_uart_rx(uint8_t *buf, uint16_t max);

// time
uint32_t bsp_millis(void);
void     bsp_delay_ms(uint32_t ms);
uint32_t bsp_cycles(void); //<dwt cycle counter
uint32_t bsp_cycles_per_us(void);

// system
void     bsp_watchdog_kick(void);
uint32_t bsp_irq_lock(void);
void     bsp_irq_unlock(uint32_t key);

#endif