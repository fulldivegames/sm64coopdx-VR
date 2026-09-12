#ifndef VR_TRACKING_MATH_H
#define VR_TRACKING_MATH_H
#include <math.h>
#include <stdbool.h>

// OpenXR xyzw orientation -> horizontal reference orientation. Do not call
// atan2f here: SM64 exports that name with its non-libm angle convention!
// Half-angle identities avoid the symbol collision on both PC and Quest.
static inline bool vr_tracking_yaw_reference(const float q[4], float out[4]) {
    float norm=0;
    for(int i=0;i<4;i++) {
        if(!isfinite(q[i])) return false;
        norm+=q[i]*q[i];
    }
    if(!isfinite(norm) || norm<0.000001f) return false;
    float sine=2.0f*(q[3]*q[1]+q[0]*q[2])/norm;
    float cosine=1.0f-2.0f*(q[0]*q[0]+q[1]*q[1])/norm;
    float length=sqrtf(sine*sine+cosine*cosine);
    // Looking exactly vertically has no forward heading. Leave the supplied
    // reference intact until a horizontal heading is available.
    if(length<0.0001f) return false;
    sine/=length; cosine=fmaxf(-1.0f,fminf(1.0f,cosine/length));
    out[0]=out[2]=0;
    if(cosine>=0) {
        out[3]=sqrtf((1.0f+cosine)*0.5f);
        out[1]=sine/(2.0f*out[3]);
    } else {
        out[1]=copysignf(sqrtf((1.0f-cosine)*0.5f),sine);
        out[3]=sine/(2.0f*out[1]);
    }
    return true;
}
#endif
