/**
 * @file button.h
 * @brief Push button debouncing (integrator) with press/release events.
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>
#include <stdint.h>

typedef enum{
    BUTTON_EV_NONE = 0,
    BUTTON_EV_PRESSED,
    BUTTON_EV_RELEASED
} button_event_t;

typedef struct{
    uint8_t integrator;
    uint8_t threshold;
    bool    pressed;
} button_t;

void            button_init(button_t *b, uint8_t threshold);
button_event_t  button_update(button_t *b, bool raw_pressed);
bool            button_is_pressed(const button_t *b);

#endif
