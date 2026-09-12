#ifndef VR_IDLE_CALIBRATION_H
#define VR_IDLE_CALIBRATION_H
#include <stdbool.h>
#include <stdint.h>
struct VrIdleCalibration { uint32_t tick; unsigned frames; };
static inline bool vr_idle_calibration_ready(struct VrIdleCalibration* s, uint32_t tick, bool eligible) {
    if (!eligible) {s->frames=0;s->tick=tick;return false;}
    if (!s->frames || tick != s->tick) {
        s->frames = s->frames && tick == s->tick + 1 ? s->frames + 1 : 1;
        if (s->frames > 4) s->frames=4;
        s->tick=tick;
    }
    return s->frames >= 4;
}
#endif
