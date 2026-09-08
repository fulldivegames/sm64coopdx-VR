#ifndef DJUI_HUD_BOUNDS_H
#define DJUI_HUD_BOUNDS_H
#include <stdbool.h>
#include <math.h>
// Top-left logical coordinates before VR spread/projection. No render state.
static inline bool djui_hud_rect_outside(float x, float y, float w, float h,
        float screenW, float screenH) {
    return fmaxf(x, x + w) <= 0 || fminf(x, x + w) >= screenW ||
           fmaxf(y, y + h) <= 0 || fminf(y, y + h) >= screenH;
}
#endif
