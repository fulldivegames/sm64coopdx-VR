#include <assert.h>
#include <stdio.h>
#include "../src/pc/controller/vr_jump_gesture.h"

static double now;
static int step(struct VrWallJumpGesture* g, int hand, float y, float speed,
        bool window, bool heldA) {
    struct VrJumpSample s[2]={{true,true,-.5f,0,0},{true,true,-.5f,0,0}};
    s[hand].y=y; s[hand].upSpeed=speed;
    now+=1.0/30.0;
    return vr_wall_jump_update(g,s,true,true,window,heldA,now);
}
int main(void) {
    for(int hand=0;hand<2;hand++) {
        struct VrWallJumpGesture g={0};
        assert(!step(&g,hand,-.5f,0,false,false));
        assert(!step(&g,hand,-.4f,1.5f,false,false));
        assert(!step(&g,hand,-.1f,1.5f,false,false));
        assert(g.pending); // A completed air gesture alone cannot jump.
        assert(step(&g,hand,-.1f,0,true,true)==-1); // Release held ground-jump A.
        assert(step(&g,hand,-.1f,0,true,false)==1);
        struct VrJumpGesture jump={0};
        vr_jump_gesture_continue_airborne(&jump,&g.stroke,now);
        struct VrJumpSample hold[2]={{true,true,-.5f,0,0},{true,true,-.5f,0,0}};
        hold[hand].y=-.1f;
        // Full ascent: held A persists beyond the recognizer's 0.2s timeout.
        assert(vr_jump_gesture_update(&jump,hold,true,false,now+.4));
        assert(vr_jump_gesture_update(&jump,hold,true,false,now+.7));
        hold[hand].y=-.25f;
        assert(!vr_jump_gesture_update(&jump,hold,true,false,now+.75));
        vr_jump_gesture_continue_airborne(&jump,&g.stroke,now);
        hold[hand].y=-.1f; hold[hand].held=false;
        assert(!vr_jump_gesture_update(&jump,hold,true,false,now+.1));
        vr_jump_gesture_continue_airborne(&jump,&g.stroke,now);
        hold[hand].held=true;
        assert(!vr_jump_gesture_update(&jump,hold,true,true,now+.1));
        assert(!step(&g,hand,-.1f,0,true,false)); // No repeat.
        g=(struct VrWallJumpGesture){0};
        assert(!step(&g,hand,-.5f,0,false,false));
        assert(!step(&g,hand,-.4f,1.5f,false,false));
        assert(!step(&g,hand,-.1f,1.5f,false,false));
        now+=.3;
        assert(!step(&g,hand,-.1f,0,true,false)); // Old strokes cannot kick.
        g.pending=true;
        struct VrJumpSample s[2]={0};
        assert(!vr_wall_jump_update(&g,s,true,false,true,false,now));
        assert(!g.pending); // Landing discards airborne input.
        g.pending=true;
        assert(!vr_wall_jump_update(&g,s,false,true,true,false,now));
        assert(!g.pending); // Menus/death/disabled reset.
    }
    puts("PASS: either-hand wall kicks, sustained full jump, lowering/releasing/landing ends hold, collision-window buffer, held-A edge, expiration and resets");
}
