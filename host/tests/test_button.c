/**
 * @file    test_button.c
 * @brief   Integrator debouncing with press/release events.
 */
#include "button.h"
#include "utest.h"

#define TH 3u

/* Feed `n` identical samples; return the number of PRESSED / RELEASED events seen. */
static void feed(button_t *b, bool raw, unsigned n, unsigned *pressed, unsigned *released)
{
    for (unsigned i = 0u; i < n; i++) {
        const button_event_t ev = button_update(b, raw);
        if (ev == BUTTON_EV_PRESSED) {
            (*pressed)++;
        } else if (ev == BUTTON_EV_RELEASED) {
            (*released)++;
        }
    }
}

static void test_init_released(void)
{
    button_t b;
    button_init(&b, TH);
    UT_CHECK(!button_is_pressed(&b));
    UT_CHECK(button_update(&b, false) == BUTTON_EV_NONE);
}

static void test_press_after_threshold(void)
{
    button_t b;
    button_init(&b, TH);

    /* First TH-1 samples: still released, no event. */
    for (unsigned i = 0u; i < TH - 1u; i++) {
        UT_CHECK(button_update(&b, true) == BUTTON_EV_NONE);
        UT_CHECK(!button_is_pressed(&b));
    }
    /* TH-th sample fires PRESSED exactly once. */
    UT_CHECK(button_update(&b, true) == BUTTON_EV_PRESSED);
    UT_CHECK(button_is_pressed(&b));
    UT_CHECK(button_update(&b, true) == BUTTON_EV_NONE);
}

static void test_release_after_threshold(void)
{
    button_t b;
    button_init(&b, TH);
    unsigned p = 0u, r = 0u;
    feed(&b, true, 10u, &p, &r);
    UT_CHECK(button_is_pressed(&b));

    /* Needs TH released samples to drain the integrator to 0. */
    for (unsigned i = 0u; i < TH - 1u; i++) {
        UT_CHECK(button_update(&b, false) == BUTTON_EV_NONE);
        UT_CHECK(button_is_pressed(&b));
    }
    UT_CHECK(button_update(&b, false) == BUTTON_EV_RELEASED);
    UT_CHECK(!button_is_pressed(&b));
    UT_CHECK(button_update(&b, false) == BUTTON_EV_NONE);
}

static void test_short_glitch_ignored(void)
{
    button_t b;
    button_init(&b, TH);
    unsigned p = 0u, r = 0u;

    /* Alternating noise never reaches the threshold. */
    for (unsigned i = 0u; i < 50u; i++) {
        feed(&b, true, TH - 1u, &p, &r);
        feed(&b, false, TH - 1u, &p, &r);
    }
    UT_CHECK(p == 0u);
    UT_CHECK(r == 0u);
    UT_CHECK(!button_is_pressed(&b));
}

static void test_bounce_while_pressing(void)
{
    button_t b;
    button_init(&b, TH);
    unsigned p = 0u, r = 0u;

    /* Bouncy edge followed by a stable press: exactly one PRESSED event. */
    const bool pattern[] = {true, false, true, true, false, true, true, true, true, true};
    for (unsigned i = 0u; i < sizeof pattern / sizeof pattern[0]; i++) {
        feed(&b, pattern[i], 1u, &p, &r);
    }
    UT_CHECK(p == 1u);
    UT_CHECK(r == 0u);
    UT_CHECK(button_is_pressed(&b));
}

static void test_integrator_saturates(void)
{
    button_t b;
    button_init(&b, TH);
    unsigned p = 0u, r = 0u;

    /* Holding for a long time must not delay the release (integrator is capped at TH). */
    feed(&b, true, 1000u, &p, &r);
    UT_CHECK(p == 1u);
    feed(&b, false, TH, &p, &r);
    UT_CHECK(r == 1u);
    UT_CHECK(!button_is_pressed(&b));
}

static void test_zero_threshold_clamped(void)
{
    button_t b;
    button_init(&b, 0u); /* clamped to 1 */
    UT_CHECK(b.threshold == 1u);
    UT_CHECK(button_update(&b, true) == BUTTON_EV_PRESSED);
    UT_CHECK(button_update(&b, false) == BUTTON_EV_RELEASED);
}

static void test_repeated_cycles(void)
{
    button_t b;
    button_init(&b, TH);
    unsigned p = 0u, r = 0u;
    for (unsigned i = 0u; i < 5u; i++) {
        feed(&b, true, 10u, &p, &r);
        feed(&b, false, 10u, &p, &r);
    }
    UT_CHECK(p == 5u);
    UT_CHECK(r == 5u);
}

int main(void)
{
    UT_RUN(test_init_released);
    UT_RUN(test_press_after_threshold);
    UT_RUN(test_release_after_threshold);
    UT_RUN(test_short_glitch_ignored);
    UT_RUN(test_bounce_while_pressing);
    UT_RUN(test_integrator_saturates);
    UT_RUN(test_zero_threshold_clamped);
    UT_RUN(test_repeated_cycles);
    UT_REPORT();
}
