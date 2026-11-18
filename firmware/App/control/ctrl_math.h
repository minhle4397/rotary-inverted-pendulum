/**
 * @file ctrl_math.h
 * @brief small float helpers shared by the control layer
 */

#ifndef CTRL_MATH_H
#define CTRL_MATH_H

#include <math.h>

#define CTRL_PI      (3.14159265358979f)
#define CTRL_TWO_PI  (6.28318530717959f)

//degrees to radians
#define CTRL_DEG2RAD(d) ((d)*(CTRL_PI / 180.0f))
//radians to degrees
#define CTRL_RAD2DEG(r) ((r)*(180.0f/CTRL_PI))

//Clamp v to [lo, hi]
static inline float ctrl_clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

//sign() with sign(0) = +1
static inline float ctrl_sign1f(float v)
{
    return (v>=0.0f) ? 1.0f : -1.0f;
}

//Wrap an angle to [-pi, pi)
static inline float ctrl_wrap_pi(float a){
    float w= fmodf(a + CTRL_PI, CTRL_TWO_PI);
    if(w < 0.0f){
        w+= CTRL_TWO_PI;
    }
    return w - CTRL_PI;
}
#endif