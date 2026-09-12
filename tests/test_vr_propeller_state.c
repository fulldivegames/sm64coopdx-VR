#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
typedef uint32_t u32;
typedef uint16_t u16;
typedef int16_t s16;
typedef float f32;
enum { ACT_GROUP_MASK=0xF00, ACT_GROUP_MOVING=0x100, ACT_GROUP_STATIONARY=0x200,
 ACT_GROUP_AIRBORNE=0, ACT_GROUP_SUBMERGED=0x300, ACT_GROUP_CUTSCENE=0x400,
 ACT_FLAG_INTANGIBLE=0x10000,
 ACT_FLAG_AIR=0x4000, ACT_JUMP=ACT_FLAG_AIR|1, ACT_DOUBLE_JUMP=ACT_FLAG_AIR|2,
 ACT_TRIPLE_JUMP=ACT_FLAG_AIR|3, ACT_BACKFLIP=ACT_FLAG_AIR|4, ACT_SIDE_FLIP=ACT_FLAG_AIR|5,
 ACT_LONG_JUMP=ACT_FLAG_AIR|6, ACT_FREEFALL=ACT_FLAG_AIR|7, ACT_WALL_KICK_AIR=ACT_FLAG_AIR|8,
 ACT_TOP_OF_POLE_JUMP=ACT_FLAG_AIR|9, ACT_TWIRLING=ACT_FLAG_AIR|10,
 ACT_WATER_JUMP=ACT_FLAG_AIR|11, ACT_STEEP_JUMP=ACT_FLAG_AIR|12,
 ACT_LAVA_BOOST=ACT_FLAG_AIR|13, ACT_JUMP_KICK=ACT_FLAG_AIR|14,
 ACT_SOFT_BONK=ACT_FLAG_AIR|15,
 INPUT_A_PRESSED=1, MARIO_SPECIAL_CAPS=0xF0, MARIO_UNKNOWN_08=8, ACTIVE_FLAG_ACTIVE=1,
 MODEL_VR_PROPELLER_MUSHROOM=257, SOUND_MENU_POWER_METER=1, VR_PROPELLER_ACTION_ARG=0x100 };
#define SEQUENCE_ARGS(a,b) ((a)+(b))
struct Object { unsigned activeFlags,oInteractType; float oPosX,oPosY,oPosZ; };
struct MarioState {
 int playerIndex,health; unsigned action,actionArg,input,flags;
 struct Object *marioObj,*heldObj; void *floor; float pos[3],vel[3],floorHeight;
 int twirlYaw,angleVel[3];
};
static struct MarioState gMarioStates[1];
static struct {int wingCapSequence;} gLevelValues;
static int gCurrLevelNum=1,gCurrAreaIndex=1,bhvStaticObject[1],gGlobalSoundSource[1];
static u32 gGlobalTimer;
enum { PLAY_MODE_PAUSED=2 };
static int sCurrPlayMode;
static bool configVrSpecialFireFlowerMusic=true,configVrAlternatePowerUpMusic;
static bool active=true,online=true,dead;
static int replaced,stops,plays;
static struct Object object={.activeFlags=1},pickup;
static void vr_special_moves_reset_propeller(void);
static bool vr_is_active(void){return active;}
static bool vr_special_moves_online_allowed(void){return online;}
static bool vr_hand_interaction_action_is_death(unsigned action){(void)action;return dead;}
static void stop_cap_music(void){stops++;}
static void play_cap_music(int x){(void)x;plays++;}
static void fadeout_cap_music(void){}
static void play_sound(int x,void*y){(void)x;(void)y;}
static int set_mario_action(struct MarioState*m,unsigned a,unsigned arg){m->action=a;m->actionArg=arg;return 1;}
static void vr_special_moves_replace_powerup(void){replaced++;vr_special_moves_reset_propeller();}
static void vr_special_moves_delete_object(struct Object**o){if(*o)(*o)->activeFlags=0;*o=NULL;}
static struct Object* spawn_object(struct Object*p,int model,void*b){(void)p;(void)b;assert(model==257);pickup.activeFlags=1;return &pickup;}
static void obj_scale(struct Object*o,float f){(void)o;(void)f;}
static void obj_update_gfx_pos_and_angle(struct Object*o){(void)o;}
#include "../build/test_vr_propeller_state.inc"
static void tick(void){gGlobalTimer++;vr_special_moves_update_propeller(&gMarioStates[0]);}
int main(void){
 struct MarioState*m=gMarioStates;
 *m=(struct MarioState){.health=0x880,.marioObj=&object,.floor=&object,.action=ACT_GROUP_MOVING};
 assert(vr_special_moves_grant_propeller());assert(replaced==1&&sVrPropellerTimer==1800);
 sCurrPlayMode=PLAY_MODE_PAUSED;tick();assert(sVrPropellerTimer==1800);sCurrPlayMode=0;
 m->input=INPUT_A_PRESSED;tick();assert(m->action==ACT_GROUP_MOVING); // Ground press not burst.
 m->action=ACT_JUMP;tick();assert(m->action==ACT_TWIRLING&&m->vel[1]==75&&m->actionArg==0x100);
 assert(!m->input&&sVrPropellerBurstUsed);
 unsigned remaining=sVrPropellerTimer;
 vr_special_moves_update_propeller(m);assert(sVrPropellerTimer==remaining); // Stereo-safe timer.
 m->action=ACT_FREEFALL;m->input=1;tick();assert(m->action==ACT_FREEFALL); // No repeat in same airtime.
 m->action=ACT_GROUP_MOVING;tick();assert(!sVrPropellerBurstUsed);
 m->action=ACT_JUMP;tick();assert(m->action==ACT_TWIRLING);
 // A valid enemy stomp relaunches at exactly the normal burst strength without
 // extending the power-up timer; a second manual air burst remains forbidden.
 unsigned stompTimer=sVrPropellerTimer;m->vel[1]=-60;
 vr_special_moves_propeller_stomp_burst(m);
 assert(m->vel[1]==75&&m->action==ACT_TWIRLING&&m->actionArg==0x100);
 assert(sVrPropellerTimer==stompTimer&&sVrPropellerBurstUsed);
 // One activation on any ordinary airborne action, with no height/speed gate.
 unsigned actions[]={ACT_JUMP,ACT_WATER_JUMP,ACT_STEEP_JUMP,ACT_LAVA_BOOST,ACT_JUMP_KICK,ACT_SOFT_BONK,ACT_FREEFALL};
 for(unsigned i=0;i<sizeof(actions)/sizeof(actions[0]);++i) {
  m->action=actions[i];m->pos[1]=.01f;m->vel[1]=.01f;
  vr_special_moves_propeller_recharge(m);
  assert(vr_special_moves_propeller_can_burst(m));
  m->input=INPUT_A_PRESSED;tick();assert(m->action==ACT_TWIRLING&&sVrPropellerBurstUsed);
  m->action=actions[i];m->input=INPUT_A_PRESSED;tick();assert(m->action==actions[i]);
 }
 // Water contact recharges without firing underwater; exiting restores one use.
 m->action=ACT_GROUP_SUBMERGED;m->input=INPUT_A_PRESSED;tick();
 assert(!sVrPropellerBurstUsed&&m->action==ACT_GROUP_SUBMERGED);
 m->action=ACT_WATER_JUMP;tick();assert(m->action==ACT_TWIRLING&&sVrPropellerBurstUsed);
 vr_special_moves_propeller_recharge(m);
 m->action=ACT_FLAG_AIR|ACT_GROUP_CUTSCENE;assert(!vr_special_moves_propeller_can_burst(m));
 m->action=ACT_JUMP|ACT_FLAG_INTANGIBLE;assert(!vr_special_moves_propeller_can_burst(m));
 m->action=ACT_JUMP;dead=true;assert(!vr_special_moves_propeller_can_burst(m));dead=false;
 struct MarioState remote=*m;remote.playerIndex=1;sVrPropellerBurstUsed=true;
 vr_special_moves_propeller_recharge(&remote);assert(sVrPropellerBurstUsed);
 m->action=ACT_TWIRLING;m->actionArg=VR_PROPELLER_ACTION_ARG;
 configVrAlternatePowerUpMusic=true;int oldStops=stops;tick();assert(stops>oldStops&&!sVrPropellerMusic);
 configVrAlternatePowerUpMusic=false;tick();assert(sVrPropellerMusic&&plays>0);
 sVrPropellerTimer=1;tick();assert(!vr_special_moves_propeller_active()&&m->action==ACT_FREEFALL);
 assert(vr_special_moves_grant_propeller());m->action=ACT_TWIRLING;m->actionArg=0;
 vr_special_moves_reset_propeller();assert(m->action==ACT_TWIRLING&&m->actionArg==0); // Ordinary twirl untouched.
 assert(vr_special_moves_grant_propeller());dead=true;tick();assert(!sVrPropellerTimer);dead=false;
 assert(vr_special_moves_grant_propeller());gCurrAreaIndex=2;tick();assert(!sVrPropellerTimer);
 assert(vr_special_moves_spawn_cheat_propeller());assert(sVrPropellerPickupObject);
 assert(!vr_special_moves_spawn_propeller_pickup(&object,0,0,0,0)); // Bounded pool.
 gCurrAreaIndex=3;tick();assert(!sVrPropellerPickupObject&&pickup.activeFlags); // No stale-pointer deletion.
 assert(vr_special_moves_grant_propeller());online=false;tick();assert(!sVrPropellerTimer);
 puts("PASS: short hops, water exits, slopes/lava/bonk airtime, one burst per contact, water recharge, death/cutscene guards, timer/music/pickup state");
}
