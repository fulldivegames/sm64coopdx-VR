#include <assert.h>
#include <stdio.h>
#include "sm64.h"
#include "game/mario.h"
#include "../build/test_vr_swing_ascent.inc"
int main(void) {
    struct MarioState m={0};
    m.action=ACT_WALL_KICK_AIR; m.vel[1]=62; m.flags=MARIO_UNKNOWN_08;
    assert(should_strengthen_gravity_for_jump_ascent(&m)); // Soft release.
    m.input=INPUT_A_DOWN;
    assert(!should_strengthen_gravity_for_jump_ascent(&m)); // Full A jump.
    m.input=0; m.flags &= ~MARIO_UNKNOWN_08;
    assert(!should_strengthen_gravity_for_jump_ascent(&m)); // Hard release.
    assert(m.input==0); // No held input leaks to another action.
    m.flags |= MARIO_UNKNOWN_08; // Native next airborne action resets this.
    assert(should_strengthen_gravity_for_jump_ascent(&m));
    puts("PASS: hard swing matches held-A ascent cutoff; soft release and subsequent jumps retain native control");
}
