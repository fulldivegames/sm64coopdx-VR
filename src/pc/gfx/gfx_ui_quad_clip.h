#ifndef GFX_UI_QUAD_CLIP_H
#define GFX_UI_QUAD_CLIP_H
#include <stdint.h>
#include "gfx.h"

// DJUI quads are bottom-left, bottom-right, top-right, top-left.
// Clip in their local rectangle, then interpolate homogeneous coordinates.
// Screen-space min/max clipping breaks rotated and perspective menu planes.
static inline void gfx_ui_clip_quad(struct GfxVertex *vertices,
        float left, float top, float right, float bottom) {
    if (left == 0 && top == 0 && right == 0 && bottom == 0) return;
    struct GfxVertex original[4];
    for (int i = 0; i < 4; ++i) original[i] = vertices[i];
    for (int i = 0; i < 4; ++i) {
        float s = (i == 0 || i == 3) ? left : 1.0f - right;
        float t = i < 2 ? 1.0f - bottom : top;
        float weights[4] = {(1-s)*t, s*t, s*(1-t), (1-s)*(1-t)};
        struct GfxVertex *v = &vertices[i];
        v->x = v->y = v->z = v->w = v->u = v->v = 0;
        for (int j = 0; j < 4; ++j) {
            v->x += original[j].x * weights[j];
            v->y += original[j].y * weights[j];
            v->z += original[j].z * weights[j];
            v->w += original[j].w * weights[j];
            v->u += original[j].u * weights[j];
            v->v += original[j].v * weights[j];
        }
        v->clip_rej = 0;
        if (v->x < -v->w) v->clip_rej |= 1;
        if (v->x >  v->w) v->clip_rej |= 2;
        if (v->y < -v->w) v->clip_rej |= 4;
        if (v->y >  v->w) v->clip_rej |= 8;
        if (v->z < -v->w) v->clip_rej |= 16;
        if (v->z >  v->w) v->clip_rej |= 32;
    }
}
#endif
