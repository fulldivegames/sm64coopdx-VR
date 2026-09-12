#include <assert.h>
#include <stdio.h>
#include "../src/game/vr_propeller_motion.h"
#include "../src/pc/controller/vr_jump_gesture.h"
int main(void) {
    float v=75;
    for(int i=0;i<60;i++) v=vr_propeller_vertical_velocity(v,false);
    assert(v==-10);
    for(int cycle=0;cycle<10;cycle++) {
        for(int i=0;i<20;i++) v=vr_propeller_vertical_velocity(v,true);
        assert(v==-60);
        for(int i=0;i<20;i++) { v=vr_propeller_vertical_velocity(v,false); assert(v<0); }
        assert(v==-10);
    }
    assert(vr_propeller_tornado_spin(true)==3*vr_propeller_tornado_spin(false));
    assert(vr_propeller_tornado_opacity(true)==255);
    assert(vr_propeller_tornado_opacity(false)==191);
    assert(vr_propeller_vertical_velocity(NAN,true)==0);
    assert(vr_propeller_vertical_velocity(INFINITY,false)==0);
    for(int hand=0;hand<2;hand++) {
        struct VrJumpGesture grounded={0};
        struct VrJumpSample raised[2]={{true,true,-.5f,0,0},{true,true,-.5f,0,0}};
        vr_jump_gesture_update(&grounded,raised,true,true,0);
        raised[hand].y=-.4f;raised[hand].upSpeed=1.5f;
        vr_jump_gesture_update(&grounded,raised,true,true,.03);
        raised[hand].y=-.1f;
        assert(vr_jump_gesture_update(&grounded,raised,true,true,.06));
        struct VrWallJumpGesture burst={0};
        for(int i=0;i<2;i++) burst.stroke.hand[i]=grounded.hand[i];
        raised[hand].y=.1f;
        assert(!vr_wall_jump_update(&burst,raised,true,true,true,false,.09));
        raised[hand].y=.3f;
        assert(!vr_wall_jump_update(&burst,raised,true,true,true,false,.12));
        raised[hand].y=-.5f;raised[hand].upSpeed=-1;
        vr_wall_jump_update(&burst,raised,true,true,true,false,.15);
        raised[hand].y=-.4f;raised[hand].upSpeed=1.5f;
        vr_wall_jump_update(&burst,raised,true,true,true,false,.18);
        raised[hand].y=-.1f;
        assert(vr_wall_jump_update(&burst,raised,true,true,true,false,.21)==1);
        struct VrLandingJumpGesture g={0};
        struct VrJumpSample s[2]={{true,true,-.5f,0,0},{true,true,-.5f,0,0}};
        assert(!vr_landing_jump_update(&g,s,true,false,true,false,1));
        s[hand].y=-.4f;s[hand].upSpeed=1.5f;
        assert(!vr_landing_jump_update(&g,s,true,false,true,false,1.03));
        s[hand].y=-.1f;
        assert(!vr_landing_jump_update(&g,s,true,false,true,false,1.06));
        assert(g.pending && g.hands==(1U<<hand));
        struct VrLandingJumpGesture expired=g;
        assert(!vr_landing_jump_update(&expired,s,true,true,false,false,1.181));
        assert(vr_landing_jump_update(&g,s,true,true,false,true,1.09)==-1);
        assert(vr_landing_jump_update(&g,s,true,true,false,false,1.12)==1);
        assert(!vr_landing_jump_update(&g,s,true,true,false,false,1.15));
        for(int i=0;i<10;i++) assert(!vr_landing_jump_update(&g,s,true,true,false,false,1.2+i*.03));
        g.pending=true;g.time=2;
        assert(!vr_landing_jump_update(&g,s,false,true,false,false,2.03));
        assert(!g.pending);
    }
    puts("PASS: reversible hold/release descent; 3x tornado; both-hand 120ms landing buffer, expiry, release edge, no held repeat");
}
