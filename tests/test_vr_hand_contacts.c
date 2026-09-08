// Include the implementation to exercise the exact private contact solver.
// Link-time garbage collection discards unrelated game/renderer entry points.
#include "../src/game/vr_hand_interaction.c"
#include <assert.h>

unsigned int configVrGloveSize = 70;
bool configVrExperimentalClimbableColliders = false;
SpatialPartitionCell gStaticSurfacePartition[NUM_CELLS][NUM_CELLS];
SpatialPartitionCell gDynamicSurfacePartition[NUM_CELLS][NUM_CELLS];
s16 gCheckingSurfaceCollisionsForCamera;
bool smlua_call_event_hooks_HOOK_ON_FIND_SURFACE_ON_RAY(Vec3f orig, Vec3f dir,
        struct Surface** surface, Vec3f hit) {
    (void)orig; (void)dir; (void)surface; (void)hit; return false;
}

int main(void) {
    struct Surface floor = {0}, wall = {0};
    vec3s_set(floor.vertex1, -1000, 0, -1000);
    vec3s_set(floor.vertex2, 1000, 0, -1000);
    vec3s_set(floor.vertex3, 0, 0, 1000);
    floor.normal.y = 1;
    vec3s_set(wall.vertex1, 0, -1000, -1000);
    vec3s_set(wall.vertex2, 0, 1000, -1000);
    vec3s_set(wall.vertex3, 0, 0, 1000);
    wall.normal.x = 1;
    const float radius = fminf(vr_hand_interaction_fist_radius(), VR_HAND_COLLISION_RADIUS_MAX);
    for (int reverse = 0; reverse < 2; ++reverse) {
        struct VrHandCollisionState state = {0};
        state.rawPositionValid = true;
        vr_hand_interaction_set_constraint(&state, reverse ? &wall : &floor);
        vr_hand_interaction_set_constraint(&state, reverse ? &floor : &wall);
        Vec3f target = {-80, -80, 0};
        vr_hand_interaction_apply_contacts(&state, target);
        assert(fabsf(target[0] - radius) < .001f);
        assert(fabsf(target[1] - radius) < .001f);
        assert(state.constraintActive && state.secondaryActive);
        target[0] = -100; target[1] = -100; target[2] = 50;
        vr_hand_interaction_apply_contacts(&state, target);
        assert(target[0] >= radius && target[1] >= radius && target[2] == 50);
        vec3f_set(target, 100, 100, 0);
        vr_hand_interaction_apply_contacts(&state, target);
        assert(!state.constraintActive && !state.secondaryActive);
        assert(target[0] == 100 && target[1] == 100);
    }
    struct Surface secondWall = {0};
    vec3s_set(secondWall.vertex1, -1000, -1000, 0);
    vec3s_set(secondWall.vertex2, 1000, -1000, 0);
    vec3s_set(secondWall.vertex3, 0, 1000, 0);
    secondWall.normal.z = 1;
    for (int order = 0; order < 3; ++order) {
        struct VrHandCollisionState state = {0};
        struct Surface* faces[] = {&floor, &wall, &secondWall};
        state.rawPositionValid = true;
        for (int step = 0; step < 12; ++step) {
            vr_hand_interaction_set_constraint(&state, faces[(step + order) % 3]);
            if (step < 2) continue;
            Vec3f target = {-80, -90, -100};
            vr_hand_interaction_apply_contacts(&state, target);
            assert(state.constraintActive && state.secondaryActive && state.tertiaryActive);
            for (int axis = 0; axis < 3; ++axis) assert(fabsf(target[axis] - radius) < .001f);
        }
        Vec3f target = {100, 100, 100};
        vr_hand_interaction_apply_contacts(&state, target);
        assert(!state.constraintActive && !state.secondaryActive && !state.tertiaryActive);
    }
    struct Surface bevel = {0};
    vec3s_set(bevel.vertex1, -1000, 1000, -1000);
    vec3s_set(bevel.vertex2, 1000, -1000, -1000);
    vec3s_set(bevel.vertex3, 0, 0, 1000);
    bevel.normal.x = bevel.normal.y = sqrtf(.5f);
    struct VrHandCollisionState slopeState = {0};
    slopeState.rawPositionValid = true;
    vr_hand_interaction_set_constraint(&slopeState, &floor);
    vr_hand_interaction_set_constraint(&slopeState, &secondWall);
    vr_hand_interaction_set_constraint(&slopeState, &bevel);
    Vec3f slopeTarget = {-90, -90, -90};
    vr_hand_interaction_apply_contacts(&slopeState, slopeTarget);
    assert(vr_hand_interaction_surface_distance(&bevel, slopeTarget) >= radius - .001f);
    assert(slopeTarget[1] >= radius - .001f && slopeTarget[2] >= radius - .001f);
    // Discover both walls through the real spatial query, not preinstalled
    // constraints: a floor slide must not stop after finding only one wall.
    wall.lowerY = secondWall.lowerY = -1000;
    wall.upperY = secondWall.upperY = 1000;
    struct SurfaceNode nodeZ = {.next = NULL, .surface = &secondWall};
    struct SurfaceNode nodeX = {.next = &nodeZ, .surface = &wall};
    const int cell = LEVEL_BOUNDARY_MAX / CELL_SIZE;
    for (int x = cell - 1; x <= cell; ++x)
        for (int z = cell - 1; z <= cell; ++z)
            gStaticSurfacePartition[z][x][SPATIAL_PARTITION_WALLS].next = &nodeX;
    struct VrHandCollisionState sweepState = {0};
    sweepState.rawPositionValid = true;
    vec3f_set(sweepState.previousCollisionPosition, 100, radius, 100);
    vr_hand_interaction_set_constraint(&sweepState, &floor);
    Vec3f cornerTarget = {-100, -80, -100};
    vr_hand_interaction_apply_contacts(&sweepState, cornerTarget);
    assert(vr_hand_interaction_sweep_contacts(&sweepState, cornerTarget, radius) != NULL);
    for (int axis = 0; axis < 3; ++axis) assert(cornerTarget[axis] >= radius - .001f);
    assert(sweepState.constraintActive && sweepState.secondaryActive && sweepState.tertiaryActive);
    puts("PASS: two/three-face corners, repeated contact orders, slope intersection, tangential slide, release");
    return 0;
}
