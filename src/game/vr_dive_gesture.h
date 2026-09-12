#ifndef VR_DIVE_GESTURE_H
#define VR_DIVE_GESTURE_H
#include <stdbool.h>
#include <math.h>

// Tracking references are yaw-only, so their Y axis remains world-up.
// Original unrestricted two-punch trigger, except near-vertical upward jumps.
static inline bool vr_dive_horizontal_punch(const float velocity[3]) {
    for (int i=0;i<3;i++) if (!isfinite(velocity[i])) return false;
    float horizontal2=velocity[0]*velocity[0]+velocity[2]*velocity[2];
    float speed2=horizontal2+velocity[1]*velocity[1];
    if (!isfinite(speed2) || speed2<0.0001f) return false;
    // sin(70 degrees)^2. Downward, sideways and backward punches retain the
    // original behavior; jumping starts at 75 degrees above horizontal.
    return velocity[1]<=0 || velocity[1]*velocity[1]<=0.88302222f*speed2;
}
#endif
