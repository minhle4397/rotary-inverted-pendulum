/**
 * @file telemetry.c
 * @brief Framed telemetry over UART DMA
 */

#include "telemetry.h"

#include <stdint.h>
#include <string.h>

#include "board_config.h"
#include "bsp.h"
#include "crc16.h"
#include "ringbuf.h"

#define TLM_RING_SIZE (4096u)
#define TLM_TX_CHUNK  (512u)

static uint8_t           s_ring_mem[TLM_RING_SIZE];
static ringbuf_t         s_ring;
static uint8_t           s_tx_buf[TLM_TX_CHUNK]; // DMA source, untouched while busy
static volatile uint16_t s_decimation;
static uint16_t          s_dec_cnt;
static volatile uint32_t s_dropped;

// pure helpers
static size_t put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
    return 4u;
}

static size_t put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    return 2u;
}

static size_t put_f32(uint8_t *p, float f)
{
    uint32_t v;
    memcpy(&v, &f, sizeof v); // IEEE-754 bit pattern, endianness handled by put_u32
    return put_u32(p, v);
}

size_t tlm_pack_sample(const tlm_sample_t *s, uint8_t *out)
{
    size_t n = 0u;
    n += put_u32(&out[n], s->tick);
    n += put_f32(&out[n], s->theta);
    n += put_f32(&out[n], s->alpha);
    n += put_f32(&out[n], s->dtheta);
    n += put_f32(&out[n], s->dalpha);
    n += put_f32(&out[n], s->voltage);
    n += put_f32(&out[n], s->current);
    n += put_f32(&out[n], s->energy);
    out[n++] = s->app_state;
    out[n++] = s->ctrl_mode;
    n += put_u16(&out[n], s->faults);
    n += put_u16(&out[n], s->exec_us);
    return n;
}

size_t tlm_encode_frame(uint8_t type, const uint8_t *payload, uint8_t len, uint8_t *out)
{
    out[0] = TLM_SOF0;
    out[1] = TLM_SOF1;
    out[2] = len;
    out[3] = type;
    memcpy(&out[4], payload, len);
    const uint16_t crc = crc16_update(CRC16_INIT, &out[2], (size_t)len + 2u);
    (void)put_u16(&out[4u + len], crc);
    return (size_t)len + TLM_FRAME_OVERHEAD;
}

// service
void telemetry_init(void)
{
    ringbuf_init(&s_ring, s_ring_mem, (uint16_t)TLM_RING_SIZE);
    s_decimation = TLM_DEFAULT_DECIMATION;
    s_dec_cnt    = 0u;
    s_dropped    = 0u;
}

void telemetry_set_decimation(uint16_t n)
{
    s_decimation = n;
}

uint16_t telemetry_decimation(void)
{
    return s_decimation;
}

void telemetry_push_sample(const tlm_sample_t *s)
{
    const uint16_t dec = s_decimation;
    if (dec == 0u) {
        return;
    }
    if (++s_dec_cnt < dec) {
        return;
    }
    s_dec_cnt = 0u;

    uint8_t payload[TLM_SAMPLE_LEN];
    uint8_t frame[TLM_SAMPLE_LEN + TLM_FRAME_OVERHEAD];
    (void)tlm_pack_sample(s, payload);
    const size_t n = tlm_encode_frame(TLM_TYPE_SAMPLE, payload, (uint8_t)TLM_SAMPLE_LEN, frame);
    if (!ringbuf_write(&s_ring, frame, (uint16_t)n)) {
        s_dropped++;
    }
}

void telemetry_send_text(const char *text)
{
    size_t len = strlen(text);
    while (len > 0u) {
        const uint8_t chunk = (uint8_t)((len > TLM_MAX_PAYLOAD) ? TLM_MAX_PAYLOAD : len);
        uint8_t       frame[TLM_MAX_PAYLOAD + TLM_FRAME_OVERHEAD];
        const size_t  n = tlm_encode_frame(TLM_TYPE_TEXT, (const uint8_t *)text, chunk, frame);
        // second producer (background) -> block the ISR producer while writing
        const uint32_t key = bsp_irq_lock();
        const bool     ok  = ringbuf_write(&s_ring, frame, (uint16_t)n);
        bsp_irq_unlock(key);
        if (!ok) {
            s_dropped++;
        }
        text += chunk;
        len -= chunk;
    }
}

void telemetry_poll(void)
{
    if (bsp_uart_tx_busy() || (ringbuf_used(&s_ring) == 0u)) {
        return;
    }
    const uint16_t n = ringbuf_read(&s_ring, s_tx_buf, (uint16_t)TLM_TX_CHUNK);
    if (n > 0u) {
        (void)bsp_uart_tx(s_tx_buf, n);
    }
}

uint32_t telemetry_dropped(void)
{
    return s_dropped;
}