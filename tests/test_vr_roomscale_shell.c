#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef float f32;
typedef int s32;
typedef float Vec3f[3];
#define ACT_FLAG_AIR 1
#define ACT_FLAG_SWIMMING 2
#define ACT_FLAG_RIDING_SHELL 4
#define ACT_FREEFALL ACT_FLAG_AIR
#define ACT_SPAWN_SPIN_AIRBORNE 100
#define ACT_SPAWN_NO_SPIN_AIRBORNE 101
#define ACT_RIDING_SHELL_FALL (ACT_FLAG_AIR | ACT_FLAG_RIDING_SHELL)
#define VR_CAMERA_MODE_FIRST_PERSON 1
#define max(a,b) ((a)>(b)?(a):(b))
struct Surface {int unused;};
struct WallCollisionData {int numWalls;};
struct Object {float oPosX,oPosY,oPosZ; struct {struct {Vec3f pos;} gfx;} header;};
struct MarioState {struct Object* marioObj; unsigned action; Vec3f pos; struct Surface* floor; float floorHeight;};
static struct Surface terrain, gWaterSurfacePseudoFloor, sonic;
static float ground=-500, water=0;
static int configVrCameraMode=1;
static bool vr_is_active(void){return true;}
static void vr_reset_roomscale_body_tracking(void){}
static bool allowed=true,committed;
static void vr_absorb_spawn_tracking(void){committed=true;}
static bool vr_hand_interaction_roomscale_action_allowed(struct MarioState*m){return allowed;}
static bool vr_get_roomscale_body_displacement(Vec3f p){p[0]=10;p[1]=p[2]=0;return true;}
static void vec3f_copy(float*d,const float*s){memcpy(d,s,sizeof(Vec3f));}
static bool mario_is_sonic_water_run_floor(struct Surface*s){return s==&sonic;}
static void resolve_and_return_wall_collisions_data(Vec3f p,float a,float b,struct WallCollisionData*w){}
static float find_floor(float x,float y,float z,struct Surface**s){*s=&terrain;return ground;}
static float find_water_level(float x,float z){return water;}
static float find_ceil(float x,float y,float z,struct Surface**s){*s=0;return 20000;}
static void mario_update_wall(struct MarioState*m,struct WallCollisionData*w){}
static void set_mario_action(struct MarioState*m,unsigned a,int arg){m->action=a;}
static void vr_commit_roomscale_body_displacement(Vec3f p){committed=true;}
#include "../build/test_vr_roomscale_shell.inc"
int main(void){
 struct Object o={0}; struct MarioState m={.marioObj=&o,.action=ACT_FLAG_RIDING_SHELL,.floor=&terrain};
 // Shoreline and deep water must both retain the water plane and ride action.
 for(int i=0;i<20;i++) {ground=i? -500:-60; vr_hand_interaction_update_roomscale_body(&m); assert(m.pos[1]==0 && m.action==ACT_FLAG_RIDING_SHELL && m.floor==&gWaterSurfacePseudoFloor);}
 // A real dry ledge uses shell fall, never ordinary freefall.
 water=-11000; vr_hand_interaction_update_roomscale_body(&m); assert(m.action==ACT_RIDING_SHELL_FALL);
 // Ordinary walking and Sonic support retain their existing behavior.
 m.action=0;m.floor=&terrain;vr_hand_interaction_update_roomscale_body(&m);assert(m.action==ACT_FREEFALL);
 water=0;m.action=0;m.floor=&sonic;vr_hand_interaction_update_roomscale_body(&m);assert(m.action==0 && m.floor==&sonic && m.pos[1]==0);
 float oldX=m.pos[0];allowed=false;committed=false;
 vr_hand_interaction_update_roomscale_body(&m);assert(!committed && m.pos[0]==oldX);
 m.action=ACT_SPAWN_SPIN_AIRBORNE;
 vr_hand_interaction_update_roomscale_body(&m);assert(committed && m.pos[0]==oldX);
 puts("PASS: actual room-scale solver preserves shell water riding, shell ledge fall, walking and Sonic water support");
}
