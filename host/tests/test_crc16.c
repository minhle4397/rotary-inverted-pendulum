/**
 * @file    test_crc16.c
 * @brief   CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no reflection, no final XOR).
 */
#include "crc16.h"
#include "utest.h"

#include <string.h>

static void test_check_value(void)
{
    /* Standard check value of CRC-16/CCITT-FALSE. */
    const char *s = "123456789";
    UT_CHECK(crc16_update(CRC16_INIT, (const uint8_t *)s, strlen(s)) == 0x29B1u);
}

static void test_empty_returns_init(void)
{
    const uint8_t dummy = 0u;
    UT_CHECK(crc16_update(CRC16_INIT, &dummy, 0u) == CRC16_INIT);
    UT_CHECK(crc16_update(0x1234u, &dummy, 0u) == 0x1234u);
}

static void test_single_byte(void)
{
    const uint8_t a = (uint8_t)'A';
    UT_CHECK(crc16_update(CRC16_INIT, &a, 1u) == 0xB915u);
}

static void test_chunked_equals_oneshot(void)
{
    const uint8_t data[] = {0x01u, 0x02u, 0x03u, 0xFFu, 0x00u, 0x80u, 0x7Fu, 0xA5u};
    const uint16_t whole = crc16_update(CRC16_INIT, data, sizeof data);

    for (size_t split = 0u; split <= sizeof data; split++) {
        uint16_t c = CRC16_INIT;
        c          = crc16_update(c, data, split);
        c          = crc16_update(c, data + split, sizeof data - split);
        UT_CHECK(c == whole);
    }
}

static void test_append_crc_gives_zero_residue(void)
{
    /* Appending the CRC (big-endian) makes the CRC of the whole frame 0. */
    uint8_t frame[10] = {0xDEu, 0xADu, 0xBEu, 0xEFu, 0x12u, 0x34u, 0x56u, 0x78u};
    const uint16_t c  = crc16_update(CRC16_INIT, frame, 8u);
    frame[8]          = (uint8_t)(c >> 8);
    frame[9]          = (uint8_t)(c & 0xFFu);
    UT_CHECK(crc16_update(CRC16_INIT, frame, sizeof frame) == 0u);
}

static void test_detects_single_bit_flip(void)
{
    uint8_t data[16];
    for (size_t i = 0u; i < sizeof data; i++) {
        data[i] = (uint8_t)(i * 17u + 3u);
    }
    const uint16_t good = crc16_update(CRC16_INIT, data, sizeof data);

    for (size_t byte = 0u; byte < sizeof data; byte++) {
        for (unsigned bit = 0u; bit < 8u; bit++) {
            data[byte] ^= (uint8_t)(1u << bit);
            UT_CHECK(crc16_update(CRC16_INIT, data, sizeof data) != good);
            data[byte] ^= (uint8_t)(1u << bit);
        }
    }
}

int main(void)
{
    UT_RUN(test_check_value);
    UT_RUN(test_empty_returns_init);
    UT_RUN(test_single_byte);
    UT_RUN(test_chunked_equals_oneshot);
    UT_RUN(test_append_crc_gives_zero_residue);
    UT_RUN(test_detects_single_bit_flip);
    UT_REPORT();
}
