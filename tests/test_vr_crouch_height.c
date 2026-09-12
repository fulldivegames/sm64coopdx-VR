#include <assert.h>
#include <stdio.h>
#include "../src/pc/controller/vr_crouch_height.h"
int main(void) {
    float identity[4]={0,0,0,1};
    assert(vr_crouch_nod_correction(identity)==0);
    for(int degrees=0;degrees<=90;degrees++) {
        float pitch=-degrees*0.01745329252f;
        for(int yawDegrees=0;yawDegrees<360;yawDegrees+=30) {
            float yaw=yawDegrees*0.01745329252f;
            float q[4]={cosf(yaw/2)*sinf(pitch/2),sinf(yaw/2)*cosf(pitch/2),
                -sinf(yaw/2)*sinf(pitch/2),cosf(yaw/2)*cosf(pitch/2)};
            float drop=0.12f*(1-cosf(pitch))-0.08f*sinf(pitch);
            float corrected=-drop+vr_crouch_nod_correction(q);
            assert(fabsf(corrected)<0.0001f); // neck-only nod: no crouch
            assert(fabsf((corrected-0.20f)+0.20f)<0.0001f); // actual crouch retained
            assert(vr_crouch_nod_correction(q)<=0.20001f);
            for(int i=0;i<4;i++)q[i]*=-2;
            assert(fabsf(vr_crouch_nod_correction(q)-drop)<0.0001f);
        }
    }
    float invalid[4]={NAN,0,0,1};assert(vr_crouch_nod_correction(invalid)==0);
    puts("PASS: downward nod compensation, yaw/sign invariance, real crouch retained, bounded correction");
}
