#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
typedef int16_t s16;
typedef int32_t s32;
typedef float f32;
typedef struct { float x, y, z; } Mtx;
typedef struct { int flags; Mtx *matrix; } Gfx;
enum { G_MTX_MODELVIEW=0, G_MTX_MUL=0, G_MTX_LOAD=2, G_MTX_PUSH=4 };
static Gfx commands[64], *gDisplayListHead, *sPowerMeterDisplayListPos;
static Mtx matrix, *sPowerMeterMtx;
static struct { int x,y; } sPowerMeterHUD = {140,166};
static float sPowerMeterPrevY = 166;
static bool active;
static int projections;
static bool vr_is_active(void) { return active; }
static void *alloc_display_list(unsigned n) { (void)n; return &matrix; }
static void create_dl_vr_hud_matrix(void) { projections++; }
static int vr_hud_group_x(int x,int anchor) { (void)anchor;return x; }
static int vr_hud_group_y(int y,int anchor) { (void)anchor;return y; }
static void guTranslate(Mtx *m,float x,float y,float z) { m->x=x;m->y=y;m->z=z; }
static float delta_interpolate_f32(float a,float b,float t) { return a+(b-a)*t; }
static void render_power_meter_health_segment(int n) { (void)n; }
#define VIRTUAL_TO_PHYSICAL(m) (m)
#define gSPMatrix(cmd,m,f) do { Gfx *c=(cmd);c->matrix=(m);c->flags=(f); } while(0)
#define gSPSaveState(cmd,...) ((void)(cmd))
#define gSPClearGeometryMode(cmd,...) ((void)(cmd))
#define gDPSetEnvColor(cmd,...) ((void)(cmd))
#define gSPDisplayList(cmd,...) ((void)(cmd))
#define gSPLoadState(cmd,...) ((void)(cmd))
#define gSPPopMatrix(cmd,...) ((void)(cmd))
#include "../build/test_vr_power_meter.inc"
int main(void) {
    for(int mode=0;mode<2;mode++) {
        active=mode;projections=0;gDisplayListHead=commands;
        render_dl_power_meter(8);
        assert(projections==mode);
        assert(commands[0].flags==(G_MTX_PUSH|(mode?G_MTX_LOAD:G_MTX_MUL)));
        assert(matrix.x==140 && matrix.y==166);
        sPowerMeterHUD.y=200;
        patch_hud_interpolated(.5f);
        assert(commands[0].flags==(G_MTX_PUSH|(mode?G_MTX_LOAD:G_MTX_MUL)));
        assert(matrix.x==140 && matrix.y==183);
        sPowerMeterHUD.y=166;
    }
    puts("power meter: isolated VR projection/modelview, matching interpolation, unchanged flat flags PASS");
}
