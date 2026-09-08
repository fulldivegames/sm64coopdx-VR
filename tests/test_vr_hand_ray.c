#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine/surface_load.h"
#include "engine/math_util.h"
#include "pc/lua/smlua_hooks.h"
SpatialPartitionCell gStaticSurfacePartition[NUM_CELLS][NUM_CELLS];
SpatialPartitionCell gDynamicSurfacePartition[NUM_CELLS][NUM_CELLS];
s16 gCheckingSurfaceCollisionsForCamera;
static int hooks;
bool smlua_call_event_hooks_HOOK_ON_FIND_SURFACE_ON_RAY(Vec3f orig, Vec3f dir,
        struct Surface** surface, Vec3f hit) {
    (void)orig; (void)dir; (void)surface; (void)hit; ++hooks; return false;
}
int main(void) {
    // The ray starts in x<0's partition, then hits a wall wholly in x>0's.
    // Its 120-unit movement is shorter than the old cell sampling step.
    struct Surface wall = {0};
    vec3s_set(wall.vertex1, 80, -100, -100);
    vec3s_set(wall.vertex2, 80, 100, -100);
    vec3s_set(wall.vertex3, 80, 0, 100);
    wall.normal.x = -1; wall.originOffset = 80;
    wall.lowerY = -100; wall.upperY = 100;
    struct SurfaceNode node = {.next = NULL, .surface = &wall};
    const int cellX = LEVEL_BOUNDARY_MAX / CELL_SIZE;
    const int cellZ = LEVEL_BOUNDARY_MAX / CELL_SIZE;
    gStaticSurfacePartition[cellZ][cellX][SPATIAL_PARTITION_WALLS].next = &node;
    Vec3f origin = {-20, 0, 0}, delta = {120, 0, 0}, hit;
    struct Surface* surface;
    find_surface_on_ray(origin, delta, &surface, hit, 2);
    assert(surface == NULL); // reproduce the skipped-cell failure
    hooks = 0;
    find_surface_on_hand_ray(origin, delta, &surface, hit);
    assert(surface == &wall && fabsf(hit[0] - 80) < 0.01f && hooks == 1);
    origin[0] = 100; delta[0] = -120;
    find_surface_on_hand_ray(origin, delta, &surface, hit);
    assert(surface == &wall); // opposite traversal direction
    delta[0] = 0;
    find_surface_on_hand_ray(origin, delta, &surface, hit);
    assert(surface == NULL); // zero movement does not normalize NaN
    puts("PASS: reproduced old skipped-cell miss; hand ray hits in both directions");
    return 0;
}
