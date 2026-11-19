/**
 * @file board_config.h
 * @brief Board-level constants of the RIP controller PCB
 * Everything that depends on the WIRING (directions, sense resistors, I2C
 * address) lives here.
 * Pin assignment is done in STM32CubeMX and reaches the code
 * only through the CubeMX user labels used in bsp_stm32f1.c.
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define FW_VERSION_STR "1.0.0"

/** encoder
 * AMT102-V, DIP switch set to 2048 P/R -> 8192 counts/rev in x4 mode.
 * Note: on the PCB channel A of both encoders goes to timer CH2 (PB7, PB3),
 * so the hardware count direction is probably inverted -> [VERIFY].
 */
#define BOARD_ENC_ARM_DIR  (+1) // VERIFY +1 or -1
#define BOARD_ENC_PEND_DIR (+1) // VERIFY +1 or -1

/** power stage
 * 2 x BTN8982TA, sign-magnitude PWM 20 kHz (TIM1 CH1 = PA8 half-bridge A,
 * TIM1 CH4 = PA11 half-bridge B). Positive voltage must give positive dtheta.
 */
#define BOARD_MOTOR_DIR      (+1) // VERIFY +1 or -1
#define BOARD_VBUS_NOMINAL_V (12.0f)

/** current sense
 * I_load = V_IS / R_IS * kILIS -> 4.76 mA/LSB
 */
#define BOARD_IS_R_OHM             (3300.0f)  // 3k3 ohms
#define BOARD_IS_KILIS             (19500.0f) //[VERIFY] calibrate, datasheet 14000..25000
#define BOARD_ADC_VREF_V           (3.3f)     // 3.3V reference for ADC
#define BOARD_ADC_FULL_SCALE       (4095.0f)  // 12-bit ADC
#define BOARD_IS_FAULT_RAW         (3900u)    // BTN8982TA FAULT current saturates the ADC
#define BOARD_IS_SWAP              (0)        // [VERIFY] 1 if IS1 (PA2) belongs to half-bridge B
#define BOARD_IS_MIN_DUTY          (0.10f)    // below this the IS sample is not trusted
#define BOARD_ADC_SAMPLES_PER_TICK (20u)      // 20kHz PWM/ 1kHz control
#define BOARD_ADC_TRIG_MIN_CNT     (72u)      // >= 1 us after the PWM rising edge

/** hmi
 */
#define BOARD_LCD_I2C_ADDR         (0x27u) // PCF8574 backpack: 0x27 or 0x3F [VERIFY]
#define BOARD_BTN_DEBOUNCE_SAMPLES (3u)    // x10ms

/** application
 */
#define APP_START_WINDOW_DEG (10.0f) // START only if |theta| < this
#define APP_ARMING_TICKS     (10u)   // 10 ms bridge enable before E-stop check

/** safety
 * Arm limit 45 deg comes from rip_params_autogen.h (RIP_S_THETA_TRIP)
 */
#define SAFETY_I_TRIP_A     (3.5f)  // < stall current 3.8 A
#define SAFETY_I_TRIP_TICKS (50u)   // 50 ms above the limit
#define SAFETY_DTHETA_MAX   (15.0f) // rad/s, design max 5.35 rad/s
#define SAFETY_DALPHA_MAX   (60.0f) // rad/s, design max 12.9 rad/s
#define SAFETY_EXEC_MAX_US  (800u)  // control tick budget 1000 us

/** telemetry
 */
#define TLM_DEFAULT_DECIMATION (1u) // 1 = every tick (1kHz, ~44 kB/s)

#endif