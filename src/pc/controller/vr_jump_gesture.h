#ifndef VR_JUMP_GESTURE_H
#define VR_JUMP_GESTURE_H
#include <stdbool.h>
#include <math.h>

// Tracking-space metres, relative to the headset (not Mario's animated hands).
struct VrJumpSample { bool valid, held; float y, upSpeed, lateralSpeedSquared; };
struct VrJumpMotionSample { bool valid; float position[3]; double time; };
static inline bool vr_jump_pose_velocity(struct VrJumpMotionSample *previous,
        const float position[3], bool valid, double now, struct VrJumpSample *sample) {
    if (!valid || !isfinite(now) || !isfinite(position[0]) ||
        !isfinite(position[1]) || !isfinite(position[2])) {
        *previous=(struct VrJumpMotionSample){0}; return false;
    }
    double dt=now-previous->time;
    bool measured=previous->valid && dt>=0.008 && dt<=0.100;
    float x=position[0]-previous->position[0];
    float y=position[1]-previous->position[1];
    float z=position[2]-previous->position[2];
    // Never turn a tracking discontinuity into a gesture.
    if (measured && x*x+y*y+z*z>0.25f) measured=false;
    if (measured) {
        sample->upSpeed=y/(float)dt;
        sample->lateralSpeedSquared=(x*x+z*z)/(float)(dt*dt);
    }
    for (unsigned i=0;i<3;i++) previous->position[i]=position[i];
    previous->time=now; previous->valid=true;
    return measured;
}
struct VrJumpHand {
    bool valid, armed, stroke;
    float base, peak, peakSpeed;
    double strokeTime;
};
struct VrJumpGesture {
    struct VrJumpHand hand[2];
    unsigned activeHand; // Bitmask: 1 = left, 2 = right, 3 = both.
    bool airborne;
    double jumpTime;
    unsigned reservedHands;
};

static inline void vr_jump_gesture_reset(struct VrJumpGesture* gesture) {
    *gesture = (struct VrJumpGesture){0};
}

static inline bool vr_jump_gesture_update(struct VrJumpGesture* g,
        const struct VrJumpSample sample[2], bool allowed, bool grounded, double now) {
    if (!allowed || !isfinite(now)) { vr_jump_gesture_reset(g); return false; }
    g->reservedHands = 0;
    bool landingRelease = false;
    if (g->activeHand) {
        if (!grounded) g->airborne = true;
        landingRelease = grounded && g->airborne;
        if (landingRelease || now - g->jumpTime > 2.0 ||
            (!g->airborne && now - g->jumpTime > 0.20)) {
            g->activeHand = 0;
        } else {
            for (unsigned i = 0; i < 2; ++i) {
                if (!(g->activeHand & (1U << i))) continue;
                struct VrJumpHand* h = &g->hand[i];
                h->peak = fmaxf(h->peak, sample[i].y);
                if (!sample[i].valid || !sample[i].held || !isfinite(sample[i].y) ||
                    h->peak - sample[i].y >= 0.10f) {
                    g->activeHand &= ~(1U << i);
                }
            }
            g->reservedHands |= g->activeHand;
        }
    }
    // Evaluate both hands against the same pre-frame state, so the left hand
    // triggering first does not suppress a simultaneous right-hand gesture.
    const bool jumpAlreadyActive = g->activeHand != 0;
    for (unsigned i = 0; i < 2; ++i) {
        struct VrJumpHand* h = &g->hand[i];
        const struct VrJumpSample* s = &sample[i];
        if (!s->valid || !s->held || !isfinite(s->y) || !isfinite(s->upSpeed) ||
            !isfinite(s->lateralSpeedSquared)) {
            *h = (struct VrJumpHand){0};
            g->activeHand &= ~(1U << i);
            continue;
        }
        if (!h->valid) {
            *h = (struct VrJumpHand){.valid=true, .armed=true, .base=s->y, .peak=s->y};
            continue;
        }
        if (!h->armed) {
            if (h->peak - s->y < 0.14f) continue;
            h->armed = true;
            h->base = s->y;
        }
        if (!grounded || landingRelease || jumpAlreadyActive) {
            h->base = s->y; h->stroke = false; h->peakSpeed = 0;
            continue;
        }
        if (s->y < h->base || s->upSpeed < -0.30f ||
            (h->stroke && now - h->strokeTime > 0.40)) {
            h->base = s->y; h->stroke = false; h->peakSpeed = 0;
        }
        // Require every pre-jump sample to stay within 15 degrees of vertical.
        // tan(15 degrees)^2: horizontal speed must be <= 0.268 * upward speed.
        // Reset the stroke if it curves forward; a prior upward sample must not
        // let a subsequent forward punch complete the gesture.
        bool straightUp = s->upSpeed > 0.0f && s->lateralSpeedSquared >= 0.0f &&
            s->lateralSpeedSquared <= s->upSpeed * s->upSpeed * 0.07179677f;
        if (!straightUp) {
            h->base = s->y; h->stroke = false; h->peakSpeed = 0;
            continue;
        }
        if (s->upSpeed >= 0.60f) {
            if (!h->stroke) { h->stroke = true; h->strokeTime = now; }
            h->peakSpeed = fmaxf(h->peakSpeed, s->upSpeed);
        }
        if (!h->stroke) { h->base = s->y; continue; }
        g->reservedHands |= 1U << i;
        if (h->peakSpeed >= 0.85f && s->y - h->base >= 0.12f && s->y >= -0.15f) {
            h->armed = false; h->stroke = false; h->peak = s->y;
            g->activeHand |= 1U << i; g->airborne = false; g->jumpTime = now;
        }
    }
    return g->activeHand != 0;
}
// Separate airborne recognizer: never feeds an ordinary midair/landing jump.
static inline void vr_jump_gesture_continue_airborne(struct VrJumpGesture *jump,
        const struct VrJumpGesture *stroke, double now) {
    *jump = *stroke;
    jump->airborne = true;
    jump->jumpTime = now;
    jump->reservedHands |= jump->activeHand;
}

struct VrWallJumpGesture {
    struct VrJumpGesture stroke;
    bool down, pending;
    double time;
};
// -1 requests an A release edge; +1 requests a wall-kick press; 0 does neither.
static inline int vr_wall_jump_update(struct VrWallJumpGesture* g,
        const struct VrJumpSample sample[2], bool allowed, bool airborne,
        bool wallWindow, bool aDown, double now) {
    if (!allowed || !airborne || !isfinite(now)) {
        *g=(struct VrWallJumpGesture){0}; return 0;
    }
    bool down=vr_jump_gesture_update(&g->stroke,sample,true,true,now);
    if(down && !g->down) { g->pending=true; g->time=now; }
    g->down=down;
    if(g->pending && (now-g->time>0.18 || now<g->time)) g->pending=false;
    if(!g->pending || !wallWindow) return 0;
    if(aDown) return -1;
    g->pending=false;
    return 1;
}
// A completed upward stroke just before landing may be used for 120 ms.
// This never emits an airborne jump and cannot replay a held fist on landing.
struct VrLandingJumpGesture {
    struct VrJumpGesture stroke;
    bool down, pending;
    double time;
    unsigned hands;
};
static inline int vr_landing_jump_update(struct VrLandingJumpGesture* g,
        const struct VrJumpSample sample[2], bool allowed, bool grounded,
        bool descending, bool aDown, double now) {
    if (!allowed || !isfinite(now)) { *g=(struct VrLandingJumpGesture){0}; return 0; }
    if (g->pending && (now < g->time || now - g->time > 0.12)) g->pending = false;
    if (!grounded) {
        bool down = vr_jump_gesture_update(&g->stroke, sample, true, true, now);
        if (down && !g->down && descending) {
            g->pending = true; g->time = now; g->hands = g->stroke.activeHand;
        }
        g->down = down;
        return 0;
    }
    if (g->pending) {
        if (aDown) return -1;
        g->pending = false;
        return 1;
    }
    *g=(struct VrLandingJumpGesture){0};
    return 0;
}
#endif
