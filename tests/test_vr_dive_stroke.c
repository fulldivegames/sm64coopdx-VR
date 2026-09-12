#include <assert.h>
#include <stdio.h>
#include "../src/pc/controller/vr_dive_stroke.h"
static bool stroke(float distance,float speed,bool air,float y,float vy) {
    struct VrDiveStroke s={0};float f[2]={0,-1},p[3]={0},v[3]={0,vy,-speed};
    assert(!vr_dive_stroke_update(&s,p,v,f,true,air));
    p[2]=-distance;p[1]=y;return vr_dive_stroke_update(&s,p,v,f,true,air);
}
int main(void) {
    assert(stroke(.24f,2,true,0,0));
    assert(!stroke(.12f,2,true,0,0));
    assert(!stroke(.24f,1.3f,true,0,0));
    assert(stroke(.15f,1.3f,false,0,0));
    assert(!stroke(.24f,2,true,.30f,3));
    assert(!stroke(.24f,-2,true,0,0));
    assert(!stroke(0,3,true,0,0)); // jump/room-scale movement shared with HMD
    struct VrDiveStroke s={0};float p[3]={0},v[3]={0,0,-2},f[2]={0,-1};
    vr_dive_stroke_update(&s,p,v,f,true,true);p[2]=-.24f;
    assert(!vr_dive_stroke_update(&s,p,v,f,false,true)); // jump owns swing
    assert(!s.valid);
    puts("PASS: deliberate forward stroke, strict air thresholds, lighter ground stroke, jump cancellation");
}
