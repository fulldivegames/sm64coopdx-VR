#ifndef GFX_UI_SCISSOR_H
#define GFX_UI_SCISSOR_H
#include <stdbool.h>
#include <math.h>

static inline bool gfx_ui_scissor_tracks_canvas(const float p[4][4], bool fullScreen) {
    const bool flatTheater = p[2][0] == 0 && p[2][1] == 0 &&
        p[2][2] == 0 && p[2][3] == 0;
    // Resets may precede the next projection; do not use a stale HUD matrix.
    return !fullScreen || flatTheater;
}

// Project logical UI clipping bounds through the same per-eye matrix as
// the menu. Output is an NDC bounding rectangle; false means behind the eye.
static inline bool gfx_ui_project_scissor(const float p[4][4],
    float left, float bottom, float right, float top, float bounds[4]) {
    bounds[0] = bounds[1] = INFINITY;
    bounds[2] = bounds[3] = -INFINITY;
    for (unsigned i = 0; i < 4; i++) {
        const float x = (i & 1) ? right : left;
        const float y = (i & 2) ? top : bottom;
        const float w = x * p[0][3] + y * p[1][3] + p[3][3];
        if (!isfinite(w) || w <= 0.0001f) return false;
        const float nx = (x * p[0][0] + y * p[1][0] + p[3][0]) / w;
        const float ny = (x * p[0][1] + y * p[1][1] + p[3][1]) / w;
        if (!isfinite(nx) || !isfinite(ny)) return false;
        bounds[0] = fminf(bounds[0], nx);
        bounds[1] = fminf(bounds[1], ny);
        bounds[2] = fmaxf(bounds[2], nx);
        bounds[3] = fmaxf(bounds[3], ny);
    }
    return true;
}
#endif
