/**
 * @file    test_strfmt.c
 * @brief   String builder, integer/float/hex formatting and float parsing.
 */
#include "strfmt.h"
#include "utest.h"

#include <string.h>

#define CAP 64u

static void test_init_empty(void)
{
    char buf[CAP];
    sbuf_t sb;
    memset(buf, 'x', sizeof buf);
    sb_init(&sb, buf, sizeof buf);
    UT_CHECK(sb.len == 0u);
    UT_CHECK(strcmp(buf, "") == 0);
}

static void test_char_and_str(void)
{
    char buf[CAP];
    sbuf_t sb;
    sb_init(&sb, buf, sizeof buf);
    sb_str(&sb, "ab");
    sb_char(&sb, 'c');
    UT_CHECK(strcmp(buf, "abc") == 0);
    UT_CHECK(sb.len == 3u);
}

static void test_truncation_keeps_terminator(void)
{
    char buf[4];
    sbuf_t sb;
    sb_init(&sb, buf, sizeof buf);
    sb_str(&sb, "abcdef");
    UT_CHECK(strcmp(buf, "abc") == 0);
    UT_CHECK(sb.len == 3u);
    sb_char(&sb, 'z'); /* still full: ignored */
    UT_CHECK(strcmp(buf, "abc") == 0);
}

static void test_zero_and_one_capacity(void)
{
    char buf[1] = {'x'};
    sbuf_t sb;
    sb_init(&sb, buf, 1u);
    sb_str(&sb, "abc");
    UT_CHECK(buf[0] == '\0');
    UT_CHECK(sb.len == 0u);

    char guard = 'g';
    sb_init(&sb, &guard, 0u); /* cap 0: must not touch the buffer */
    sb_str(&sb, "abc");
    UT_CHECK(guard == 'g');
    UT_CHECK(sb.len == 0u);
}

static void test_i32(void)
{
    char buf[CAP];
    sbuf_t sb;

    sb_init(&sb, buf, sizeof buf);
    sb_i32(&sb, 0, 0u);
    UT_CHECK(strcmp(buf, "0") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_i32(&sb, 42, 5u);
    UT_CHECK(strcmp(buf, "   42") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_i32(&sb, -42, 5u);
    UT_CHECK(strcmp(buf, "  -42") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_i32(&sb, 123456, 3u); /* width smaller than the number: no truncation */
    UT_CHECK(strcmp(buf, "123456") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_i32(&sb, INT32_MAX, 0u);
    UT_CHECK(strcmp(buf, "2147483647") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_i32(&sb, INT32_MIN, 0u);
    UT_CHECK(strcmp(buf, "-2147483648") == 0);
}

static void test_hex(void)
{
    char buf[CAP];
    sbuf_t sb;

    sb_init(&sb, buf, sizeof buf);
    sb_hex(&sb, 0xBEEFu, 4u);
    UT_CHECK(strcmp(buf, "BEEF") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_hex(&sb, 0xAu, 4u); /* zero padded */
    UT_CHECK(strcmp(buf, "000A") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_hex(&sb, 0x12345678u, 2u); /* keeps the low digits */
    UT_CHECK(strcmp(buf, "78") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_hex(&sb, 0xDEADBEEFu, 8u);
    UT_CHECK(strcmp(buf, "DEADBEEF") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_hex(&sb, 0xFFu, 0u);
    UT_CHECK(strcmp(buf, "") == 0);
}

static void test_pad_within_capacity(void)
{
    char buf[CAP];
    sbuf_t sb;
    sb_init(&sb, buf, sizeof buf);
    sb_str(&sb, "ab");
    sb_pad(&sb, 6u);
    UT_CHECK(strcmp(buf, "ab    ") == 0);
    sb_pad(&sb, 3u); /* already longer: no-op */
    UT_CHECK(sb.len == 6u);
}

static void test_f32_basic(void)
{
    char buf[CAP];
    sbuf_t sb;

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, 1.25f, 2u, 0u);
    UT_CHECK(strcmp(buf, "1.25") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, -1.5f, 1u, 6u);
    UT_CHECK(strcmp(buf, "  -1.5") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, 0.0f, 3u, 0u);
    UT_CHECK(strcmp(buf, "0.000") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, 3.14159f, 2u, 0u);
    UT_CHECK(strcmp(buf, "3.14") == 0);
}

static void test_f32_rounding_and_carry(void)
{
    char buf[CAP];
    sbuf_t sb;

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, 0.999f, 2u, 0u); /* carries into the integer part */
    UT_CHECK(strcmp(buf, "1.00") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, 2.5f, 0u, 0u); /* no decimal point, round half up */
    UT_CHECK(strcmp(buf, "3") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, 9.96f, 1u, 0u);
    UT_CHECK(strcmp(buf, "10.0") == 0);
}

static void test_f32_width(void)
{
    char buf[CAP];
    sbuf_t sb;

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, 3.5f, 2u, 8u);
    UT_CHECK(strcmp(buf, "    3.50") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, 12345.5f, 1u, 3u); /* narrower than the value: no truncation */
    UT_CHECK(strcmp(buf, "12345.5") == 0);
}

static void test_f32_negative_rounds_to_zero_keeps_width(void)
{
    char buf[CAP];
    sbuf_t sb;
    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, -0.001f, 2u, 5u);
    UT_CHECK(strcmp(buf, " 0.00") == 0); /* no "-0.00", column alignment preserved */
    UT_CHECK(sb.len == 5u);
}

static void test_f32_decimals_clamped(void)
{
    char buf[CAP];
    sbuf_t sb;
    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, 1.0f, 20u, 0u); /* clamped to 6 */
    UT_CHECK(strcmp(buf, "1.000000") == 0);
}

static void test_f32_nan_and_out_of_range(void)
{
    char buf[CAP];
    sbuf_t sb;
    const float nanv = nanf("");

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, nanv, 2u, 0u);
    UT_CHECK(strcmp(buf, "nan") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, nanv, 2u, 6u);
    UT_CHECK(strcmp(buf, "   nan") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, 3.0e9f, 2u, 0u);
    UT_CHECK(strcmp(buf, "nan") == 0);

    sb_init(&sb, buf, sizeof buf);
    sb_f32(&sb, -3.0e9f, 2u, 0u);
    UT_CHECK(strcmp(buf, "nan") == 0);
}

static void test_build_telemetry_line(void)
{
    char buf[CAP];
    sbuf_t sb;
    sb_init(&sb, buf, sizeof buf);
    sb_str(&sb, "T=");
    sb_i32(&sb, 120, 4u);
    sb_char(&sb, ' ');
    sb_f32(&sb, -0.5f, 2u, 6u);
    sb_char(&sb, ' ');
    sb_hex(&sb, 0x29B1u, 4u);
    UT_CHECK(strcmp(buf, "T= 120  -0.50 29B1") == 0);
}

static void test_parse_valid(void)
{
    float v = 0.0f;
    UT_CHECK(str_to_f32("-1.25", &v));
    UT_NEAR(v, -1.25, 1e-6);
    UT_CHECK(str_to_f32("3", &v));
    UT_NEAR(v, 3.0, 0.0);
    UT_CHECK(str_to_f32("2e-3", &v));
    UT_NEAR(v, 0.002, 1e-7);
    UT_CHECK(str_to_f32("+7", &v));
    UT_NEAR(v, 7.0, 0.0);
    UT_CHECK(str_to_f32(".5", &v));
    UT_NEAR(v, 0.5, 1e-6);
    UT_CHECK(str_to_f32("5.", &v));
    UT_NEAR(v, 5.0, 0.0);
    UT_CHECK(str_to_f32("1E3", &v));
    UT_NEAR(v, 1000.0, 1e-3);
    UT_CHECK(str_to_f32("1e+2", &v));
    UT_NEAR(v, 100.0, 1e-4);
    UT_CHECK(str_to_f32("0", &v));
    UT_NEAR(v, 0.0, 0.0);
    UT_CHECK(str_to_f32("123.456", &v));
    UT_NEAR(v, 123.456, 1e-3);
}

static void test_parse_invalid_leaves_output(void)
{
    const char *bad[] = {"", "-", "+", ".", "abc", "1.5x", "1e", "1e-", " 1", "1 ", "--1", "1..2", "e5"};
    for (size_t i = 0u; i < sizeof bad / sizeof bad[0]; i++) {
        float v = 99.0f;
        UT_CHECK(!str_to_f32(bad[i], &v));
        UT_NEAR(v, 99.0, 0.0);
    }
}

static void test_format_then_parse_roundtrip(void)
{
    const float values[] = {0.0f, 1.5f, -2.25f, 12.345f, -100.001f, 0.125f};
    for (size_t i = 0u; i < sizeof values / sizeof values[0]; i++) {
        char buf[CAP];
        sbuf_t sb;
        float back = 0.0f;
        sb_init(&sb, buf, sizeof buf);
        sb_f32(&sb, values[i], 3u, 0u);
        UT_CHECK(str_to_f32(buf, &back));
        UT_NEAR(back, values[i], 1e-3);
    }
}

int main(void)
{
    UT_RUN(test_init_empty);
    UT_RUN(test_char_and_str);
    UT_RUN(test_truncation_keeps_terminator);
    UT_RUN(test_zero_and_one_capacity);
    UT_RUN(test_i32);
    UT_RUN(test_hex);
    UT_RUN(test_pad_within_capacity);
    UT_RUN(test_f32_basic);
    UT_RUN(test_f32_rounding_and_carry);
    UT_RUN(test_f32_width);
    UT_RUN(test_f32_negative_rounds_to_zero_keeps_width);
    UT_RUN(test_f32_decimals_clamped);
    UT_RUN(test_f32_nan_and_out_of_range);
    UT_RUN(test_build_telemetry_line);
    UT_RUN(test_parse_valid);
    UT_RUN(test_parse_invalid_leaves_output);
    UT_RUN(test_format_then_parse_roundtrip);
    UT_REPORT();
}
