/**
 * @file ctrl_filter.c
 * @brief Filtered backward difference
 */

#include "ctrl_filter.h"

#include "ctrl_math.h"

void ctrl_diff_retune(ctrl_diff_t *f, float fc_hz, float ts_s){
    f->a = expf(-CTRL_TWO_PI * fc_hz * ts_s);
    f->ts = ts_s;
}

void ctrl_diff_init(ctrl_diff_t *f, float fc_hz, float ts_s){
    ctrl_diff_retune(f, fc_hz, ts_s);
    f->prev     = 0.0f;
    f->rate     = 0.0f;
    f->primed   = false;
}

float ctrl_diff_update(ctrl_diff_t *f, float x){
    if(!f->primed){
        f->prev     = x;
        f->primed   = true;
    }

    f->rate = f->a*f->rate + (1.0f - f->a) * (x - f->prev) / f->ts;
    f->prev = x;
    return  f->rate;
}