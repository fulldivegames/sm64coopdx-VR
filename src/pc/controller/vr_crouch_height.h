#ifndef VR_CROUCH_HEIGHT_H
#define VR_CROUCH_HEIGHT_H
#include <math.h>

// Estimate the eye drop from nodding around the neck, in meters. This is
// deliberately local to crouch detection: never change camera/body tracking.
// A yaw-only reference preserves world up on both PC and Quest. XR forward
// is -Z; looking down gives a negative forward-Y component.
static inline float vr_crouch_nod_correction(const float q[4]) {
    float length2=q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3];
    if (!isfinite(length2) || length2 < 0.000001f) return 0.0f;
    float down=2.0f*(q[1]*q[2]-q[3]*q[0])/length2;
    down=fminf(1.0f,fmaxf(0.0f,down));
    // Approximate eye-to-neck offsets: 12 cm above, 8 cm in front.
    // Bounded to 20 cm at straight down; body lowering still counts normally.
    return 0.12f*(1.0f-sqrtf(fmaxf(0.0f,1.0f-down*down)))+0.08f*down;
}
#endif
