#ifndef VR_PUNCH_TRAVEL_H
#define VR_PUNCH_TRAVEL_H
#include <stdbool.h>
#include <math.h>
// Net displacement prevents small reversals from accumulating into a punch.
// When jumping is enabled, raising a fist with a small horizontal wobble
// must not become a punch before the strict vertical jump recognizer fires.
static inline float vr_punch_travel(const float start[3], const float end[3], bool jumping) {
    float x=end[0]-start[0], y=end[1]-start[1], z=end[2]-start[2];
    float horizontal=x*x+z*z;
    if (!isfinite(horizontal) || !isfinite(y)) return 0.0f;
    if (jumping && y>0.0f && y*y>horizontal) return sqrtf(horizontal);
    return sqrtf(horizontal+y*y);
}
#endif
