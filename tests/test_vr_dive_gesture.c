#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/game/vr_dive_gesture.h"
int main(void) {
    for(int yaw=0;yaw<360;yaw+=5) {
        for(int el=-90;el<=90;el++) {
            float a=el*.01745329252f, heading=yaw*.01745329252f;
            float velocity[3]={-cosf(a)*sinf(heading),sinf(a),-cosf(a)*cosf(heading)};
            if(el<70) assert(vr_dive_horizontal_punch(velocity));
            if(el>70) assert(!vr_dive_horizontal_punch(velocity));
        }
    }
    const float invalid[3]={NAN,0,0}, zero[3]={0,0,0};
    assert(!vr_dive_horizontal_punch(invalid) && !vr_dive_horizontal_punch(zero));
    puts("PASS: original dive directions retained, only upward angles above 70 degrees excluded");
}
