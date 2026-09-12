#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
typedef unsigned int u32;
typedef float f32;
typedef float Vec3f[3];
enum { VR_CONTROLLER_COUNT=2, VR_PHYSICAL_CLIMB_NONE=0,
       INPUT_NONZERO_ANALOG=1, INPUT_ZERO_MOVEMENT=2 };
#define VR_CLIMB_SWING_RELEASE_MIN_SPEED 110.0f
struct Controller { float stickX,stickY,stickMag; };
struct MarioState { unsigned input; struct Controller *controller; };
static unsigned sVrPhysicalClimbHandoffGraceFrames, sVrPhysicalClimbHandoffGraceTimestamp;
static unsigned gGlobalTimer, sVrPhysicalClimbType;
static bool sVrPhysicalClimbPendingSwingRelease, sVrPhysicalClimbPendingVelocityValid;
static bool configVrSwingClimbRelease;
static bool sVrPhysicalClimbHands[2], sVrGripPressed[2];
static Vec3f sVrPhysicalClimbPendingVelocity;
static int released, synced;
static void vr_hand_interaction_release_physical_climb(struct MarioState *m,
        const Vec3f velocity, bool swing) {
    (void)m;(void)velocity;(void)swing;released++;
    sVrPhysicalClimbHandoffGraceFrames=0;
}
static void vr_hand_interaction_sync_climb_collider_to_headset(struct MarioState *m) {
    (void)m;synced++;
}
#include "../build/test_vr_swing_handoff.inc"
static void reset(float speed) {
    released=synced=0;
    sVrPhysicalClimbHandoffGraceFrames=3;
    sVrPhysicalClimbHandoffGraceTimestamp=gGlobalTimer=10;
    sVrPhysicalClimbType=1;
    sVrPhysicalClimbPendingSwingRelease=true;
    sVrPhysicalClimbPendingVelocityValid=true;
    configVrSwingClimbRelease=true;
    for(int i=0;i<2;i++) sVrPhysicalClimbHands[i]=sVrGripPressed[i]=false;
    sVrPhysicalClimbPendingVelocity[0]=speed;
    sVrPhysicalClimbPendingVelocity[1]=sVrPhysicalClimbPendingVelocity[2]=0;
}
int main(void) {
    struct Controller c={1,1,1};
    struct MarioState m={INPUT_NONZERO_ANALOG,&c};
    reset(110);vr_hand_interaction_update_physical_climb_handoff(&m);
    assert(released==1 && synced==0); // no extra game tick, even at threshold
    for(int hand=0;hand<2;hand++) {
        reset(300);sVrPhysicalClimbHands[hand]=sVrGripPressed[hand]=true;
        vr_hand_interaction_update_physical_climb_handoff(&m);
        assert(!released && !sVrPhysicalClimbHandoffGraceFrames);
    }
    reset(109);vr_hand_interaction_update_physical_climb_handoff(&m);
    assert(!released && sVrPhysicalClimbHandoffGraceFrames==3);
    for(int i=0;i<3;i++) { gGlobalTimer++;vr_hand_interaction_update_physical_climb_handoff(&m); }
    assert(released==1 && synced==2);
    reset(300);configVrSwingClimbRelease=false;
    vr_hand_interaction_update_physical_climb_handoff(&m);assert(!released);
    reset(300);sVrPhysicalClimbPendingVelocityValid=false;
    vr_hand_interaction_update_physical_climb_handoff(&m);assert(!released);
    reset(NAN);vr_hand_interaction_update_physical_climb_handoff(&m);assert(!released);
    reset(INFINITY);vr_hand_interaction_update_physical_climb_handoff(&m);assert(!released);
    puts("PASS: immediate swing; either-hand handoff preserved; gentle/disabled/invalid release grace preserved");
}
