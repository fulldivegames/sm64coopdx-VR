#include <assert.h>
#include <stdio.h>
#include "../src/pc/controller/vr_punch_travel.h"
int main(void) {
    float start[3]={0,0,0};
    // Raising a fist with tiny forward motion is not a 20cm punch.
    for(int i=1;i<=30;i++) {
        float p[3]={.01f*sinf(i),i*.02f,.03f};
        assert(vr_punch_travel(start,p,true)<.05f);
    }
    float forward[3]={0,.02f,.25f}, side[3]={.25f,0,0};
    float down[3]={0,-.25f,0}, up[3]={0,.25f,0};
    assert(vr_punch_travel(start,forward,true)>.20f);
    assert(vr_punch_travel(start,side,true)>.20f);
    assert(vr_punch_travel(start,down,true)>.20f);
    assert(vr_punch_travel(start,up,false)>.20f);
    for(int i=0;i<100;i++) {
        float jitter[3]={0,0,(i%2 ? .03f : -.03f)};
        assert(vr_punch_travel(start,jitter,false)<.05f);
    }
    float invalid[3]={NAN,0,0}; assert(vr_punch_travel(start,invalid,true)==0);
    puts("PASS: fist raising/jitter rejected; forward, sideways, downward and jump-disabled uppercuts preserved");
}
