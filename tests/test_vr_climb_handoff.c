#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
typedef unsigned int u32;
typedef float Vec3f[3];
static bool sVrFirstPersonAnchorValid;
static u32 sVrFirstPersonAnchorTimestamp, gGlobalTimer;
static Vec3f sVrFirstPersonAnchorPrev, sVrFirstPersonAnchor;
#include "../build/test_vr_climb_handoff.inc"

int main(void) {
    Vec3f previous = { 100, 200, -300 }, current = { 110, 220, -330 };
    sVrFirstPersonAnchorValid = true;
    gGlobalTimer = sVrFirstPersonAnchorTimestamp = 100;
    for (int i = 0; i < 3; i++) {
        sVrFirstPersonAnchorPrev[i] = 10;
        sVrFirstPersonAnchor[i] = 20;
    }
    vr_rebase_first_person_climb_anchor(previous, current);
    for (int i = 0; i < 3; i++) {
        // Every subframe stays on the same trajectory when the separate
        // offset disappears. No temporary return to the pre-climb root.
        for (int frame = 0; frame <= 4; frame++) {
            float t = frame * .25f;
            float before = 10 + 10*t + previous[i] + (current[i]-previous[i])*t;
            float after = sVrFirstPersonAnchorPrev[i] +
                (sVrFirstPersonAnchor[i]-sVrFirstPersonAnchorPrev[i])*t;
            assert(before == after);
        }
    }
    sVrFirstPersonAnchorTimestamp = 99;
    for (int i = 0; i < 3; i++) sVrFirstPersonAnchor[i] = 20;
    vr_rebase_first_person_climb_anchor(previous, current);
    for (int i = 0; i < 3; i++) assert(sVrFirstPersonAnchor[i] == 20 + previous[i]);
    sVrFirstPersonAnchorTimestamp = 90;
    vr_rebase_first_person_climb_anchor(previous, current);
    for (int i = 0; i < 3; i++) assert(sVrFirstPersonAnchor[i] == 20 + previous[i]);
    sVrFirstPersonAnchorValid = false;
    sVrFirstPersonAnchorTimestamp = 100;
    vr_rebase_first_person_climb_anchor(previous, current);
    for (int i = 0; i < 3; i++) assert(sVrFirstPersonAnchor[i] == 20 + previous[i]);
    puts("climb handoff: current tick, previous tick, stale and invalid histories PASS");
}
