#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
typedef float f32;
typedef uint32_t u32;
typedef float Vec3f[3];
enum { ACT_GRABBED = 1, ACT_THROWN_FORWARD, ACT_THROWN_BACKWARD };
struct Object { struct { struct { Vec3f pos; uint32_t skipInterpolationTimestamp; } gfx; } header; };
struct CameraState { Vec3f pos; };
struct MarioState { int action; struct Object *marioObj; Vec3f pos; struct CameraState *statusForCamera; };
static struct MarioState gMarioStates[1];
static bool sVrFirstPersonAnchorValid, gRenderingInterpolated, active = true;
static uint32_t sVrFirstPersonAnchorTimestamp, gGlobalTimer = 1;
static Vec3f sVrFirstPersonAnchor, sVrFirstPersonAnchorPrev;
static float gRenderingDelta;
static bool vr_is_active(void) { return active; }
static void vec3f_copy(float *a, const float *b) { memcpy(a,b,sizeof(Vec3f)); }
static void delta_interpolate_vec3f(float *o,const float *a,const float *b,float t) {
    for(int i=0;i<3;i++) o[i]=a[i]+(b[i]-a[i])*t;
}
#include "../build/test_vr_enemy_camera.inc"
int main(void) {
    struct Object obj = {0}; struct CameraState camera = {0};
    struct MarioState *m = gMarioStates;
    m->marioObj=&obj; m->statusForCamera=&camera;
    Vec3f out;
    m->pos[0]=10;
    vr_get_first_person_anchor(out); assert(out[0]==10);
    ++gGlobalTimer; m->action=ACT_GRABBED;
    obj.header.gfx.pos[0]=100; obj.header.gfx.pos[1]=200;
    vr_get_first_person_anchor(out);
    // Holder moves after an early physics tracking query.
    obj.header.gfx.pos[0]=200; obj.header.gfx.pos[1]=400;
    vr_refresh_enemy_camera_anchor();
    assert(sVrFirstPersonAnchorPrev[0]==10);
    assert(camera.pos[0]==200 && camera.pos[1]==400);
    vr_refresh_enemy_camera_anchor(); assert(sVrFirstPersonAnchorPrev[0]==10);
    gRenderingInterpolated=true;
    ++gGlobalTimer; // display_and_vsync happens before render interpolation.
    for(int i=0;i<=10;i++) {
        gRenderingDelta=i/10.f; vr_get_first_person_anchor(out);
        assert(fabsf(out[0]-(10+190*gRenderingDelta))<0.001f);
    }
    // Release changes source, not the interpolation history.
    gRenderingInterpolated=false; m->action=ACT_THROWN_FORWARD;
    m->pos[0]=220; m->pos[1]=430;
    vr_refresh_enemy_camera_anchor();
    assert(sVrFirstPersonAnchorPrev[0]==200 && sVrFirstPersonAnchor[0]==220);
    ++gGlobalTimer; m->action=ACT_THROWN_BACKWARD; m->pos[0]=240;
    vr_refresh_enemy_camera_anchor(); assert(sVrFirstPersonAnchor[0]==240);
    // Warps collapse history; non-finite mod attachment positions fall back.
    ++gGlobalTimer; obj.header.gfx.skipInterpolationTimestamp=gGlobalTimer;
    m->action=ACT_GRABBED; obj.header.gfx.pos[0]=NAN;
    vr_refresh_enemy_camera_anchor();
    assert(sVrFirstPersonAnchorPrev[0]==240 && sVrFirstPersonAnchor[0]==240);
    active=false; m->pos[0]=999; vr_refresh_enemy_camera_anchor();
    assert(sVrFirstPersonAnchor[0]==240);
    puts("Enemy camera: carried XYZ, render interpolation, late holder update, throw handoff, warp and flat-screen isolation passed.");
}
