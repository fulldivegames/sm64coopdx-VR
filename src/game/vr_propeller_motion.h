#ifndef VR_PROPELLER_MOTION_H
#define VR_PROPELLER_MOTION_H
#include <stdbool.h>
#include <math.h>

// Simulation-tick velocities. Crouch is held, never latched: letting go brakes
// a fast descent back to the slow glide without another burst or upward impulse.
static inline float vr_propeller_vertical_velocity(float velocity, bool crouch) {
    if (!isfinite(velocity)) return 0.0f;
    if (crouch) return fmaxf(-60.0f, velocity - 8.0f);
    if (velocity > 0.0f) return velocity - 3.0f;
    if (velocity < -10.0f) return fminf(-10.0f, velocity + 8.0f);
    return fmaxf(-10.0f, velocity - 2.0f);
}
static inline unsigned vr_propeller_tornado_opacity(bool fast) { return fast ? 255 : 191; }
static inline int vr_propeller_tornado_spin(bool fast) { return fast ? 0x4800 : 0x1800; }
#endif
