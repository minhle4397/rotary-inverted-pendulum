/**
 * @file    test_telemetry.c
 * @brief   Telemetry framing, decimation, text splitting and overflow (uses bsp_fake).
 */
#include <string.h>

#include "board_config.h"
#include "bsp_fake.h"
#include "crc16.h"
#include "telemetry.h"
#include "utest.h"

static tlm_sample_t sample(void)
{
    tlm_sample_t s;
    memset(&s, 0, sizeof s);
    s.tick      = 0x01020304u;
    s.theta     = 1.0f;
    s.app_state = 7u;
    s.ctrl_mode = 9u;
    s.faults    = 0x0A0Bu;
    s.exec_us   = 0x0C0Du;
    return s;
}

/* Check SOF, LEN, TYPE and CRC of the frame starting at p; return its total size (0 if bad). */
static size_t check_frame(const uint8_t *p, uint8_t type, uint8_t len)
{
    if (p[0] != TLM_SOF0 || p[1] != TLM_SOF1 || p[2] != len || p[3] != type) {
        return 0u;
    }
    const uint16_t crc = crc16_update(CRC16_INIT, &p[2], (size_t)len + 2u);
    const uint16_t got = (uint16_t)(p[4u + len] | (p[5u + len] << 8));
    return (crc == got) ? ((size_t)len + TLM_FRAME_OVERHEAD) : 0u;
}

static void test_pack_layout(void)
{
    const tlm_sample_t s = sample();
    uint8_t            b[TLM_SAMPLE_LEN];
    UT_CHECK(tlm_pack_sample(&s, b) == TLM_SAMPLE_LEN);
    UT_CHECK(b[0] == 0x04u && b[1] == 0x03u && b[2] == 0x02u && b[3] == 0x01u); /* tick LE */
    UT_CHECK(b[4] == 0x00u && b[5] == 0x00u && b[6] == 0x80u && b[7] == 0x3Fu); /* 1.0f LE */
    UT_CHECK(b[32] == 7u && b[33] == 9u);
    UT_CHECK(b[34] == 0x0Bu && b[35] == 0x0Au);
    UT_CHECK(b[36] == 0x0Du && b[37] == 0x0Cu);
}

static void test_encode_frame(void)
{
    const uint8_t payload[3] = {1u, 2u, 3u};
    uint8_t       f[16];
    UT_CHECK(tlm_encode_frame(TLM_TYPE_TEXT, payload, 3u, f) == 3u + TLM_FRAME_OVERHEAD);
    UT_CHECK(check_frame(f, TLM_TYPE_TEXT, 3u) == 9u);
    UT_CHECK(f[4] == 1u && f[5] == 2u && f[6] == 3u);
}

static void test_encode_empty_payload(void)
{
    const uint8_t none = 0u; /* valid pointer: memcpy(NULL, 0) is undefined */
    uint8_t       f[8];
    UT_CHECK(tlm_encode_frame(TLM_TYPE_TEXT, &none, 0u, f) == TLM_FRAME_OVERHEAD);
    UT_CHECK(check_frame(f, TLM_TYPE_TEXT, 0u) == TLM_FRAME_OVERHEAD);
}

static void test_push_and_poll(void)
{
    const tlm_sample_t s = sample();
    fake_reset();
    telemetry_init();
    UT_CHECK(telemetry_decimation() == TLM_DEFAULT_DECIMATION);
    telemetry_push_sample(&s);
    UT_CHECK(g_fake.tx_len == 0u); /* nothing sent before poll */
    telemetry_poll();
    UT_CHECK(g_fake.tx_len == TLM_SAMPLE_LEN + TLM_FRAME_OVERHEAD);
    UT_CHECK(check_frame(g_fake.tx, TLM_TYPE_SAMPLE, (uint8_t)TLM_SAMPLE_LEN) == g_fake.tx_len);
    telemetry_poll(); /* empty ring: no more output */
    UT_CHECK(g_fake.tx_len == TLM_SAMPLE_LEN + TLM_FRAME_OVERHEAD);
}

static void test_decimation(void)
{
    const tlm_sample_t s = sample();
    fake_reset();
    telemetry_init();
    telemetry_set_decimation(0u);
    UT_CHECK(telemetry_decimation() == 0u);
    for (int i = 0; i < 10; i++) {
        telemetry_push_sample(&s);
    }
    telemetry_poll();
    UT_CHECK(g_fake.tx_len == 0u);

    telemetry_set_decimation(3u);
    for (int i = 0; i < 6; i++) {
        telemetry_push_sample(&s);
    }
    telemetry_poll();
    UT_CHECK(g_fake.tx_len == 2u * (TLM_SAMPLE_LEN + TLM_FRAME_OVERHEAD));
}

static void test_text_short(void)
{
    fake_reset();
    telemetry_init();
    telemetry_send_text("hello");
    telemetry_poll();
    UT_CHECK(g_fake.tx_len == 5u + TLM_FRAME_OVERHEAD);
    UT_CHECK(check_frame(g_fake.tx, TLM_TYPE_TEXT, 5u) == g_fake.tx_len);
    UT_CHECK(memcmp(&g_fake.tx[4], "hello", 5u) == 0);
}

static void test_text_long_is_split(void)
{
    char text[451];
    memset(text, 'x', 450u);
    text[450] = '\0';
    fake_reset();
    telemetry_init();
    telemetry_send_text(text);
    telemetry_poll();
    /* 200 + 200 + 50 bytes -> 3 frames */
    UT_CHECK(g_fake.tx_len == 450u + 3u * TLM_FRAME_OVERHEAD);
    size_t off = check_frame(g_fake.tx, TLM_TYPE_TEXT, 200u);
    UT_CHECK(off != 0u);
    size_t n = check_frame(&g_fake.tx[off], TLM_TYPE_TEXT, 200u);
    UT_CHECK(n != 0u);
    off += n;
    n = check_frame(&g_fake.tx[off], TLM_TYPE_TEXT, 50u);
    UT_CHECK(n != 0u);
    UT_CHECK(off + n == g_fake.tx_len);
}

static void test_overflow_counts_dropped(void)
{
    const tlm_sample_t s = sample();
    fake_reset();
    telemetry_init();
    UT_CHECK(telemetry_dropped() == 0u);
    for (int i = 0; i < 100; i++) { /* ring 4096 / 44 -> 93 frames fit */
        telemetry_push_sample(&s);
    }
    UT_CHECK(telemetry_dropped() == 7u);
    /* everything that fit is still valid and drains in whole frames */
    size_t total = 0u;
    for (int i = 0; i < 20; i++) {
        telemetry_poll();
        total = g_fake.tx_len;
    }
    UT_CHECK(total == 93u * (TLM_SAMPLE_LEN + TLM_FRAME_OVERHEAD));
}

int main(void)
{
    UT_RUN(test_pack_layout);
    UT_RUN(test_encode_frame);
    UT_RUN(test_encode_empty_payload);
    UT_RUN(test_push_and_poll);
    UT_RUN(test_decimation);
    UT_RUN(test_text_short);
    UT_RUN(test_text_long_is_split);
    UT_RUN(test_overflow_counts_dropped);
    UT_REPORT();
}
