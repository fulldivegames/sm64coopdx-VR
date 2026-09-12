#ifndef VR_DIVE_STROKE_H
#define VR_DIVE_STROKE_H
#include <stdbool.h>
#include <math.h>
struct VrDiveStroke { bool valid, fired; float start[3]; bool airborne; };
static inline bool vr_dive_stroke_update(struct VrDiveStroke* s, const float p[3],
        const float v[3], const float forward[2], bool allowed, bool airborne) {
    if (!allowed) { *s=(struct VrDiveStroke){0}; return false; }
    for(int i=0;i<3;i++) if(!isfinite(p[i])||!isfinite(v[i])) {
        *s=(struct VrDiveStroke){0}; return false;
    }
    float speed=v[0]*forward[0]+v[2]*forward[1];
    float lateral=v[0]*forward[1]-v[2]*forward[0];
    // Existing motion naturally starts a new stroke on reversal. No added
    // settle timer, hand-lowering gesture, or grip-release requirement.
    if(!s->valid || s->airborne!=airborne || speed<=0.15f) {
        *s=(struct VrDiveStroke){.valid=true,.airborne=airborne};
        for(int i=0;i<3;i++)s->start[i]=p[i];
        return false;
    }
    float cone=airborne?1.0f:2.04f; // 45 / 55 degrees around horizontal forward.
    if(lateral*lateral+v[1]*v[1]>speed*speed*cone) {
        for(int i=0;i<3;i++)s->start[i]=p[i];
        return false;
    }
    float dx=p[0]-s->start[0],dy=p[1]-s->start[1],dz=p[2]-s->start[2];
    float travel=dx*forward[0]+dz*forward[1];
    float side=dx*forward[1]-dz*forward[0];
    if(s->fired || speed<(airborne?1.65f:1.20f) || travel<(airborne?0.20f:0.13f) ||
        side*side+dy*dy>travel*travel*cone) return false;
    s->fired=true;
    return true;
}
#endif
