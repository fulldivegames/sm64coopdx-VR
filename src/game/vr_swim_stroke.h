#ifndef VR_SWIM_STROKE_H
#define VR_SWIM_STROKE_H
#include <stdbool.h>
#include <math.h>

// Position is torso-relative tracking space. Movement is pose-to-pose travel
// in metres (not instantaneous velocity), in tracking/game coordinates.
struct VrSwimSample { bool valid; float position[3], movement[3], worldMovement[3], forward[3]; };
struct VrSwimStroke {
    bool valid, armed, pulling, upward;
    float pullAxis[3], direction[3], travel, recovery, delivered;
};
struct VrSwimTracking { bool valid; float previous[3]; };
static inline bool vr_swim_tracking_delta(struct VrSwimTracking* s,
        bool valid, const float position[3], float delta[3]) {
    for (int i=0;i<3;i++) delta[i]=0;
    for (int i=0;i<3;i++) {
        if (!valid || !isfinite(position[i])) {
            *s=(struct VrSwimTracking){0}; return false;
        }
    }
    for (int i=0;i<3;i++) {
        if (s->valid) delta[i]=position[i]-s->previous[i];
        s->previous[i]=position[i];
    }
    s->valid=true;
    return true;
}
static inline float vr_swim_dot(const float a[3], const float b[3]) {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
static inline void vr_swim_stroke_reset(struct VrSwimStroke* s) {
    *s=(struct VrSwimStroke){0};
}
static inline float vr_swim_stroke_update(struct VrSwimStroke* s,
        const struct VrSwimSample* hand) {
    for (int i=0;i<3;i++) {
        if (!hand->valid || !isfinite(hand->position[i]) ||
            !isfinite(hand->movement[i]) || !isfinite(hand->worldMovement[i]) ||
            !isfinite(hand->forward[i])) {
            vr_swim_stroke_reset(s); return 0;
        }
    }
    const float* delta=hand->movement;
    if (!s->valid) { s->valid=true; return 0; }
    float distance=sqrtf(vr_swim_dot(delta,delta));
    if (distance>0.30f) { vr_swim_stroke_reset(s); return 0; }
    float worldTravel=sqrtf(vr_swim_dot(hand->worldMovement,hand->worldMovement));
    // A stationary controller supplies zero travel even if the head moves.
    // Do not gate a sampled pull on instantaneous end-of-frame speed.
    if (distance<0.001f || worldTravel<0.001f) return 0;
    float back=-vr_swim_dot(delta,hand->forward);
    float front=vr_swim_dot(hand->position,hand->forward);
    bool overhead=hand->position[1]>0.20f && -delta[1]>distance*0.9396926f;
    bool downward=-delta[1]>distance*0.35f &&
        (overhead || s->pulling || (front>0.10f && hand->position[1]>-0.20f));
    // Detect water being pushed backward/downward, not distance from a
    // guessed torso center. Side-of-body and curved strokes remain usable.
    // Forward recovery never produces thrust, even on an existing power phase.
    bool productive=back>distance*0.10f || (downward && back>=-distance*0.10f);
    if (!productive) {
        s->recovery+=distance;
        if (!s->pulling || s->recovery>=0.04f) {
            s->armed=true; s->pulling=false; s->upward=false;
            s->travel=0; s->delivered=0;
        }
        return 0;
    }
    if (!s->armed) {
        s->armed=true; s->travel=0; s->delivered=0;
    }
    s->recovery=0;
    s->travel+=distance;
    // A deliberate overhead downward pull swims upward even while looking
    // forward. Latch the intent for the power phase as the hand comes down.
    // position is shoulder-relative, so +0.20 is approximately head height.
    if (!s->pulling) {
        s->upward=overhead;
    }
    // Track a curved power phase from its beginning, not only after activation.
    // Otherwise a near-tangential middle section continually clears travel.
    s->pulling=true;
    for(int i=0;i<3;i++) {
        s->pullAxis[i]=delta[i]/distance;
        s->direction[i]=-hand->worldMovement[i]/worldTravel;
    }
    if (s->travel<0.04f) return 0;
    float total=fminf(s->travel-0.02f,0.65f);
    float impulse=fmaxf(0,total-s->delivered);
    s->delivered=total;
    return impulse;
}

static inline bool vr_swim_head_forward(const float q[4], float out[3]) {
    float n=0;
    for(int i=0;i<4;i++) { if(!isfinite(q[i])) return false; n+=q[i]*q[i]; }
    if(!isfinite(n) || n<0.000001f) return false;
    out[0]=-2*(q[0]*q[2]+q[3]*q[1])/n;
    out[1]=2*(q[3]*q[0]-q[1]*q[2])/n;
    out[2]=2*(q[0]*q[0]+q[1]*q[1])/n-1;
    return true;
}
#endif
