#include <assert.h>
#include <stdio.h>
#include "../src/pc/gfx/gfx_ui_scissor.h"
int main(void) {
    float p[4][4] = {{2.0f/320,0,0,0},{0,2.0f/240,0,0},
                     {0,0,1,0},{-1,-1,0,1}};
    float left[4], right[4];
    assert(!gfx_ui_scissor_tracks_canvas(p, true));
    // A HUD occupying the middle half of an eye must retain its canvas edge.
    // A meter moved above logical y=240 is hidden, but y=200 remains visible.
    float hud[4][4] = {{1.0f/320,0,0,0},{0,1.0f/240,0,0},
                      {0,0,-.1f,0},{-.5f,-.5f,0,1}};
    assert(!gfx_ui_scissor_tracks_canvas(hud, true));
    assert(gfx_ui_project_scissor(hud,0,0,320,240,right));
    assert(300*hud[1][1]+hud[3][1] > right[3]);
    assert(200*hud[1][1]+hud[3][1] < right[3]);
    float world[4][4] = {{1,0,0,0},{0,1,0,0},{0,0,-1,-1},{0,0,-1,0}};
    assert(!gfx_ui_scissor_tracks_canvas(world,true));
    assert(gfx_ui_project_scissor(p, 0, 0, 160, 240, left));
    assert(fabsf(left[0]+1) < 0.00001f && fabsf(left[2]) < 0.00001f);
    assert(fabsf(left[1]+1) < 0.00001f && fabsf(left[3]-1) < 0.00001f);
    // Opposite eye offsets must move clipping by the same amount as the UI.
    p[3][0] += 0.1f;
    assert(gfx_ui_project_scissor(p, 0, 0, 160, 240, right));
    assert(fabsf(right[0]-left[0]-0.1f) < 0.00001f);
    assert(fabsf(right[2]-left[2]-0.1f) < 0.00001f);
    // A rotated plane requires all four corners to compute its bounds.
    p[0][0]=0; p[0][1]=2.0f/320; p[1][0]=-2.0f/240; p[1][1]=0;
    p[3][0]=1; p[3][1]=-1;
    assert(gfx_ui_project_scissor(p, 0, 0, 320, 240, left));
    assert(fabsf(left[0]+1)<0.00001f && fabsf(left[3]-1)<0.00001f);
    p[3][3]=-1;
    assert(!gfx_ui_project_scissor(p, 0, 0, 320, 240, left));
    p[3][3]=NAN;
    assert(!gfx_ui_project_scissor(p, 0, 0, 320, 240, left));
    puts("PASS: UI clipping bounds, per-eye offsets, rotation, invalid projection");
}
