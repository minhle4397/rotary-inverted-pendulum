/**
 * @file    button.c
 * @brief   Integrator debouncing
 */

#include "button.h"
#include <stdbool.h>
#include <stdint.h>

void button_init(button_t *b, uint8_t threshold){
    b->integrator = 0u;
    b->threshold  = (threshold == 0u) ? 1u : threshold;
    b->pressed    = false;
}

button_event_t button_update(button_t *b, bool raw_pressed){
    if(raw_pressed){
        if(b->integrator < b->threshold){
            b->integrator++;
        }
    }
    else if(b->integrator > 0u){
        b->integrator--;
    }
    else{
        //stays released
    }

    if(!b->pressed && (b->integrator >= b->threshold)){
        b->pressed = true;
        return BUTTON_EV_PRESSED;
    }
    if(b->pressed && (b->integrator == 0u)){
        b->pressed = false;
        return BUTTON_EV_RELEASED;
    }
    return BUTTON_EV_NONE;
}

bool button_is_pressed(const button_t *b){
    return b->pressed;
}