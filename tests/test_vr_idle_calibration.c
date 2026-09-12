#include <assert.h>
#include <stdio.h>
#include "../src/game/vr_idle_calibration.h"
int main(void) {
 struct VrIdleCalibration s={0};
 assert(!vr_idle_calibration_ready(&s,10,false));
 assert(!vr_idle_calibration_ready(&s,11,true));
 for(int i=0;i<20;i++)assert(!vr_idle_calibration_ready(&s,11,true));
 assert(!vr_idle_calibration_ready(&s,12,true));
 assert(!vr_idle_calibration_ready(&s,13,true));
 assert(vr_idle_calibration_ready(&s,14,true));
 assert(!vr_idle_calibration_ready(&s,15,false));
 assert(!vr_idle_calibration_ready(&s,16,true));
 assert(!vr_idle_calibration_ready(&s,100,true));
 puts("PASS: four distinct idle ticks, no airborne/Propeller samples, render-rate calls cannot advance calibration");
}
