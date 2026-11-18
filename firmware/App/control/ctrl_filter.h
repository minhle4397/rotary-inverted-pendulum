/**
 * @file ctrl_filter.h
 * @brief Filtered backward difference (velocity estimator)
 * w[k] = a*w[k-1] + (1-a)*(x[k] - x[k-1])/Ts ,   a = exp(-2*pi*fc*Ts)
 */

#ifndef CTRL_FILTER_H
#define CTRL_FILTER_H

#include <stdbool.h>

typedef struct{
    float a;     //filter pole exp(-2*pi*fc*Ts)
    float ts;    //sample time [s]
    float prev;  //previous input sample
    float rate;  //filtered derivative 
    bool primed; //true after the first sample
} ctrl_diff_t;

//Initialize for cut-off fc_hz and sample time ts [s]
void ctrl_diff_init(ctrl_diff_t *f, float fc_hz, float ts_s);

//Change fc/ts without losing the current state
void ctrl_diff_retune(ctrl_diff_t *f, float fc_hz, float ts_s);

//Feed one sample, returns the filtered derivative. The first call returns 0
float ctrl_diff_update(ctrl_diff_t *f, float x);

#endif