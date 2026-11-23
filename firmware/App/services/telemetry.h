/**
 * @file    telemetry.h
 * @brief   Binary telemetry + text messages over USART1 (DMA), framed with CRC.
 *
 * Frame (little-endian):
 *   0xA5 0x5A | LEN (payload bytes) | TYPE | PAYLOAD[LEN] | CRC16 (LEN..PAYLOAD)
 *
 * TYPE 0x01 SAMPLE (38 bytes):
 *   u32 tick, f32 theta, f32 alpha, f32 dtheta, f32 dalpha, f32 voltage,
 *   f32 current, f32 energy, u8 app_state, u8 ctrl_mode, u16 faults, u16 exec_us
 * TYPE 0x02 TEXT: ASCII, no terminator
 *
 * The same decoder (sim/rip_sim/telemetry.py) is used for the SIL output and
 * for the live serial monitor (tools/rip_monitor.py).
 *
 * Threading: telemetry_push_sample() from the control ISR, everything else
 * from the background loop.
 *
 * Layer   : SERVICE (uses bsp.h for the UART)
 */

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stddef.h>
#include <stdint.h>

#define TLM_SOF0           (0xA5u)
#define TLM_SOF1           (0x5Au)
#define TLM_TYPE_SAMPLE    (0x01u)
#define TLM_TYPE_TEXT      (0x02u)
#define TLM_SAMPLE_LEN     (38u)
#define TLM_MAX_PAYLOAD    (200u)
#define TLM_FRAME_OVERHEAD (6u)

typedef struct {
    uint32_t tick;
    float    theta;
    float    alpha;
    float    dtheta;
    float    dalpha;
    float    voltage;
    float    current;
    float    energy;
    uint8_t  app_state;
    uint8_t  ctrl_mode;
    uint16_t faults;
    uint16_t exec_us;
} tlm_sample_t;

void telemetry_init(void);
// 0 = samples off, n = one sample every n control ticks
void     telemetry_set_decimation(uint16_t n);
uint16_t telemetry_decimation(void);
// ISR context. Applies decimation, drops the frame if the buffer is full
void telemetry_push_sample(const tlm_sample_t *s);
// Background context. Long context is split into several frames
void telemetry_send_text(const char *text);
// Background context: move buffered bytes to the UART DMA
void     telemetry_poll(void);
uint32_t telemetry_dropped(void);

// pure helpers (unit-tested)
size_t tlm_pack_sample(const tlm_sample_t *s, uint8_t *out);
size_t tlm_encode_frame(uint8_t type, const uint8_t *payload, uint8_t len, uint8_t *out);

#endif /* TELEMETRY_H */
