#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../src/pc/gfx/gfx_ui_quad_clip.h"

int main(void) {
    for (int angle = 0; angle < 360; angle += 15) {
        for (int eye = -1; eye <= 1; eye += 2) {
            struct GfxVertex v[4] = {0}, original[4];
            const float x[4] = {-1,1,1,-1}, y[4] = {-1,-1,1,1};
            float a = angle * 0.01745329252f;
            for (int i = 0; i < 4; ++i) {
                v[i].x = .5f*(x[i]*cosf(a)-y[i]*sinf(a)) + eye*.05f;
                v[i].y = .5f*(x[i]*sinf(a)+y[i]*cosf(a));
                v[i].z = .1f*x[i]; v[i].w = 1+.15f*x[i];
                v[i].u = (x[i]+1)*32; v[i].v = (1-y[i])*32;
            }
            memcpy(original,v,sizeof(v));
            gfx_ui_clip_quad(v,0,0,0,0);
            assert(memcmp(original,v,sizeof(v)) == 0);
            // Cut a quarter from the local left: position and UV must move
            // together along the rotated/perspective edge, in both eyes.
            gfx_ui_clip_quad(v,.25f,0,0,0);
            assert(fabsf(v[0].x-(.75f*original[0].x+.25f*original[1].x))<1e-6f);
            assert(fabsf(v[0].y-(.75f*original[0].y+.25f*original[1].y))<1e-6f);
            assert(fabsf(v[0].w-(.75f*original[0].w+.25f*original[1].w))<1e-6f);
            assert(fabsf(v[0].u-16)<1e-6f && v[0].v==64);
            assert(v[1].x==original[1].x && v[1].u==64);
            assert(v[0].clip_rej==0 && v[3].clip_rej==0);
            // The following fully visible glyph must remain completely intact.
            memcpy(v,original,sizeof(v));
            gfx_ui_clip_quad(v,0,0,0,0);
            assert(memcmp(original,v,sizeof(v))==0);
        }
    }
    puts("UI glyph clipping: rotation, perspective, both eyes, UV alignment, fully visible glyphs passed.");
}
