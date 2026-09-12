#include <assert.h>
#include <stdio.h>
#include "../src/pc/controller/vr_jump_gesture.h"
static double now;
static bool step(struct VrJumpGesture* g, bool ground, float left, float right, float lv, float rv) {
    struct VrJumpSample s[2] = {{true,true,left,lv,0}, {true,true,right,rv,0}};
    now += 1.0 / 30.0;
    return vr_jump_gesture_update(g,s,true,ground,now);
}
int main(void) {
    // PC runtime velocity may be absent: position-derived vertical strokes
    // still recognize, and a tracking gap cannot produce a false jump.
    const int rates[]={30,60,72,90,120};
    for (unsigned rate=0;rate<sizeof(rates)/sizeof(rates[0]);++rate) {
        const int hz=rates[rate];
        struct VrJumpMotionSample motion={0}; struct VrJumpGesture pc={0};
        bool fired=false;
        for(int i=0;i<hz/2;i++) {
            float t=(float)i/hz;
            float pos[3]={0,-.5f+1.2f*t,0};
            struct VrJumpSample samples[2]={{false,true,pos[1],0,0},{0}};
            samples[0].valid=vr_jump_pose_velocity(&motion,pos,true,t,&samples[0]);
            fired |= vr_jump_gesture_update(&pc,samples,true,true,t);
        }
        assert(fired);
        float pos[3]={0,2,0}; struct VrJumpSample sample={0};
        assert(!vr_jump_pose_velocity(&motion,pos,true,3,&sample));
        assert(!vr_jump_pose_velocity(&motion,pos,false,3.03,&sample));
    }
    // Irregular PC frame intervals must retain held-A without creating
    // another jump. Tracking loss and impossible position snaps must cancel.
    {
        struct VrJumpMotionSample motion={0}; struct VrJumpGesture pc={0};
        const double intervals[]={.022,.044,.029,.038};
        double t=0; bool fired=false;
        for(int i=0;i<45;i++) {
            t+=intervals[i%4];
            float pos[3]={0,fminf(-.5f+1.5f*(float)t,.15f),0};
            struct VrJumpSample samples[2]={{false,true,pos[1],0,0},{0}};
            samples[0].valid=vr_jump_pose_velocity(&motion,pos,true,t,&samples[0]);
            bool down=vr_jump_gesture_update(&pc,samples,true,!fired,t);
            if(fired) assert(down);
            fired |= down;
        }
        assert(fired);
        float pos[3]={0,3,0}; struct VrJumpSample s={0};
        assert(!vr_jump_pose_velocity(&motion,pos,true,t+.03,&s));
        pos[0]=NAN;
        assert(!vr_jump_pose_velocity(&motion,pos,true,t+.06,&s));
        assert(!motion.valid);
    }
    struct VrJumpGesture g={0};
    assert(!step(&g,true,-.5f,-.5f,0,0));
    assert(!step(&g,true,-.4f,-.5f,1.5f,0));
    assert(step(&g,true,-.1f,-.5f,1.5f,0));
    assert(g.activeHand==1 && (g.reservedHands&1));
    // Hold raised: continuous A through ascent, no repeated jump edges.
    for(int i=0;i<10;i++) assert(step(&g,false,-.1f,-.5f,0,0));
    // Landing must release A even if the hand is still raised.
    assert(!step(&g,true,-.1f,-.5f,0,0));
    for(int i=0;i<5;i++) assert(!step(&g,true,-.1f,-.5f,0,0));
    // Alternate to right, then lower it for a short jump.
    assert(!step(&g,true,-.1f,-.4f,0,1.5f));
    assert(step(&g,true,-.1f,-.1f,0,1.5f));
    assert(g.activeHand==2);
    assert(!step(&g,false,-.5f,-.3f,-1,-1));
    assert(!step(&g,false,-.5f,-.5f,0,-1));
    assert(!step(&g,true,-.5f,-.5f,0,0));
    // Third gesture on the left supplies a new edge to native combo logic.
    assert(!step(&g,true,-.4f,-.5f,1.5f,0));
    assert(step(&g,true,-.1f,-.5f,1.5f,0));
    struct VrJumpSample s[2]={{true,true,-.1f,0,0},{true,true,-.5f,0,0}};
    assert(!vr_jump_gesture_update(&g,s,false,false,now)); // Menu/death/climb gate.
    assert(g.activeHand==0);
    // Air punches must never queue a jump for landing.
    assert(!step(&g,false,-.5f,-.5f,0,0));
    assert(!step(&g,false,-.4f,-.5f,2,0));
    assert(!step(&g,false,.2f,-.5f,2,0));
    assert(!step(&g,true,.2f,-.5f,0,0));
    // Slow raising and sideways punches are not jump gestures.
    vr_jump_gesture_reset(&g);
    for(int i=0;i<20;i++) assert(!step(&g,true,-.5f+i*.04f,-.5f,.3f,0));
    vr_jump_gesture_reset(&g);
    for(int i=0;i<20;i++) {
        s[0]=(struct VrJumpSample){true,true,-.5f+i*.04f,1,9};
        now+=1.0/30.0;
        assert(!vr_jump_gesture_update(&g,s,true,true,now));
    }
    // Restrictive cone: accept 14 degrees, reject 16/30/45 degrees from up.
    const float angles[] = {14, 16, 30, 45};
    for (unsigned a=0; a<4; ++a) {
        vr_jump_gesture_reset(&g);
        assert(!step(&g,true,-.5f,-.5f,0,0));
        float lateral = 1.5f * tanf(angles[a] * 0.01745329252f);
        bool fired = false;
        for (int i=1; i<=4; ++i) {
            s[0]=(struct VrJumpSample){true,true,-.5f+i*.1f,1.5f,lateral*lateral};
            now+=1.0/30.0;
            fired |= vr_jump_gesture_update(&g,s,true,true,now);
        }
        assert(fired == (a==0));
    }
    // Starting vertically cannot latch permission for a diagonal finish.
    vr_jump_gesture_reset(&g);
    assert(!step(&g,true,-.5f,-.5f,0,0));
    assert(!step(&g,true,-.4f,-.5f,1.5f,0));
    s[0]=(struct VrJumpSample){true,true,-.1f,1.5f,1.0f};
    now+=1.0/30.0;
    assert(!vr_jump_gesture_update(&g,s,true,true,now));
    assert(g.reservedHands==0);
    // Loss of tracking/recenter requires fresh motion, never a position snap.
    vr_jump_gesture_reset(&g);
    assert(!step(&g,true,-.5f,-.5f,0,0));
    assert(!step(&g,true,-.4f,-.5f,2,0));
    assert(step(&g,true,0,-.5f,2,0));
    s[0].valid=false;
    assert(!vr_jump_gesture_update(&g,s,true,false,now));
    assert(!step(&g,true,.5f,-.5f,2,0));
    vr_jump_gesture_reset(&g);
    assert(!step(&g,true,.5f,-.5f,2,0));
    s[0].held=false;
    assert(!vr_jump_gesture_update(&g,s,true,true,now));
    // Both hands produce one jump, held until both hands release/lower.
    vr_jump_gesture_reset(&g);
    assert(!step(&g,true,-.5f,-.5f,0,0));
    assert(!step(&g,true,-.4f,-.4f,1.5f,1.5f));
    assert(step(&g,true,-.1f,-.1f,1.5f,1.5f));
    assert(g.activeHand==3 && g.reservedHands==3);
    assert(step(&g,false,-.3f,-.1f,-1,0));
    assert(g.activeHand==2);
    assert(!step(&g,false,-.3f,-.3f,0,-1));
    assert(!step(&g,true,-.3f,-.3f,0,0));
    puts("PASS: one/two-hand jumps, full/short hold, landing release, alternating gestures, directional thresholds and tracking recovery");
}
