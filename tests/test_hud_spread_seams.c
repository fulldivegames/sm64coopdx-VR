#include <assert.h>
#include <math.h>
#include <stdio.h>
static float position(float x, float center, float scale) {
    return center + (x-center)*scale;
}
int main(void) {
    for (int percent=80; percent<=200; ++percent) {
        float scale=percent/100.f;
        for(int resolution=1; resolution<=4; ++resolution) {
            float center=160.f*resolution, x=109.f*resolution;
            float width=32.f*resolution;
            float left=position(x,center,scale);
            float right=position(x+width,center,scale);
            assert(fabsf(left+width*scale-right)<.0002f);
            // Inner pie keeps its quarter-width inset, with no new overlap.
            float pie=position(x+16.f*resolution,center,scale);
            assert(fabsf(pie-left-16.f*resolution*scale)<.0002f);
        }
    }
    puts("PASS: meter seams and pie insets at 80..200% spread, four resolutions");
}
