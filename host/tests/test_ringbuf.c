/**
 * @file    test_ringbuf.c
 * @brief   Byte ring buffer (SPSC): init, write all-or-nothing, read, wrap-around.
 */
#include "ringbuf.h"
#include "utest.h"

#include <string.h>

#define SIZE 8u

static void test_init_empty(void)
{
    uint8_t mem[SIZE];
    ringbuf_t rb;
    ringbuf_init(&rb, mem, SIZE);
    UT_CHECK(ringbuf_used(&rb) == 0u);
    UT_CHECK(ringbuf_free(&rb) == SIZE);
}

static void test_write_then_read(void)
{
    uint8_t mem[SIZE];
    ringbuf_t rb;
    ringbuf_init(&rb, mem, SIZE);

    const uint8_t in[3] = {1u, 2u, 3u};
    uint8_t out[8]      = {0};
    UT_CHECK(ringbuf_write(&rb, in, 3u));
    UT_CHECK(ringbuf_used(&rb) == 3u);
    UT_CHECK(ringbuf_free(&rb) == SIZE - 3u);

    UT_CHECK(ringbuf_read(&rb, out, sizeof out) == 3u);
    UT_CHECK(memcmp(in, out, 3u) == 0);
    UT_CHECK(ringbuf_used(&rb) == 0u);
}

static void test_read_empty_returns_zero(void)
{
    uint8_t mem[SIZE];
    ringbuf_t rb;
    ringbuf_init(&rb, mem, SIZE);
    uint8_t out[4];
    UT_CHECK(ringbuf_read(&rb, out, sizeof out) == 0u);
}

static void test_write_exactly_full(void)
{
    uint8_t mem[SIZE];
    ringbuf_t rb;
    ringbuf_init(&rb, mem, SIZE);

    const uint8_t in[SIZE] = {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u};
    uint8_t out[SIZE];
    UT_CHECK(ringbuf_write(&rb, in, SIZE));
    UT_CHECK(ringbuf_free(&rb) == 0u);
    UT_CHECK(ringbuf_used(&rb) == SIZE);
    UT_CHECK(ringbuf_read(&rb, out, SIZE) == SIZE);
    UT_CHECK(memcmp(in, out, SIZE) == 0);
}

static void test_write_all_or_nothing(void)
{
    uint8_t mem[SIZE];
    ringbuf_t rb;
    ringbuf_init(&rb, mem, SIZE);

    const uint8_t in[SIZE + 1u] = {0};
    UT_CHECK(!ringbuf_write(&rb, in, SIZE + 1u));
    UT_CHECK(ringbuf_used(&rb) == 0u);

    const uint8_t six[6] = {1u, 2u, 3u, 4u, 5u, 6u};
    UT_CHECK(ringbuf_write(&rb, six, 6u));
    UT_CHECK(!ringbuf_write(&rb, six, 3u)); /* only 2 free */
    UT_CHECK(ringbuf_used(&rb) == 6u);      /* unchanged */
}

static void test_partial_read(void)
{
    uint8_t mem[SIZE];
    ringbuf_t rb;
    ringbuf_init(&rb, mem, SIZE);

    const uint8_t in[5] = {10u, 20u, 30u, 40u, 50u};
    uint8_t out[5];
    UT_CHECK(ringbuf_write(&rb, in, 5u));
    UT_CHECK(ringbuf_read(&rb, out, 2u) == 2u);
    UT_CHECK(out[0] == 10u && out[1] == 20u);
    UT_CHECK(ringbuf_used(&rb) == 3u);
    UT_CHECK(ringbuf_read(&rb, out, 5u) == 3u);
    UT_CHECK(out[0] == 30u && out[1] == 40u && out[2] == 50u);
}

static void test_wrap_around(void)
{
    uint8_t mem[SIZE];
    ringbuf_t rb;
    ringbuf_init(&rb, mem, SIZE);

    uint8_t out[SIZE];
    const uint8_t a[6] = {1u, 2u, 3u, 4u, 5u, 6u};
    UT_CHECK(ringbuf_write(&rb, a, 6u));
    UT_CHECK(ringbuf_read(&rb, out, 5u) == 5u); /* head=6, tail=5 */

    const uint8_t b[6] = {7u, 8u, 9u, 10u, 11u, 12u}; /* wraps past end of mem */
    UT_CHECK(ringbuf_write(&rb, b, 6u));
    UT_CHECK(ringbuf_used(&rb) == 7u);
    UT_CHECK(ringbuf_read(&rb, out, sizeof out) == 7u);

    const uint8_t expect[7] = {6u, 7u, 8u, 9u, 10u, 11u, 12u};
    UT_CHECK(memcmp(out, expect, 7u) == 0);
}

static void test_counter_overflow_16bit(void)
{
    /* head/tail are free-running 16-bit counters: must survive wrap at 65536. */
    uint8_t mem[SIZE];
    ringbuf_t rb;
    ringbuf_init(&rb, mem, SIZE);

    uint8_t next_w = 0u;
    uint8_t next_r = 0u;
    for (unsigned iter = 0u; iter < 30000u; iter++) {
        uint8_t in[3];
        for (unsigned k = 0u; k < 3u; k++) {
            in[k] = next_w++;
        }
        UT_CHECK(ringbuf_write(&rb, in, 3u));

        uint8_t out[3];
        UT_CHECK(ringbuf_read(&rb, out, 3u) == 3u);
        for (unsigned k = 0u; k < 3u; k++) {
            if (out[k] != next_r) {
                UT_CHECK(out[k] == next_r); /* record once, then stop */
                return;
            }
            next_r++;
        }
    }
    UT_CHECK(ringbuf_used(&rb) == 0u);
}

int main(void)
{
    UT_RUN(test_init_empty);
    UT_RUN(test_write_then_read);
    UT_RUN(test_read_empty_returns_zero);
    UT_RUN(test_write_exactly_full);
    UT_RUN(test_write_all_or_nothing);
    UT_RUN(test_partial_read);
    UT_RUN(test_wrap_around);
    UT_RUN(test_counter_overflow_16bit);
    UT_REPORT();
}
