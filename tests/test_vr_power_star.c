#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
typedef uint32_t u32;
typedef uint16_t u16;
typedef int16_t s16;
typedef float f32, Vec3f[3];
enum { ACTIVE_FLAG_ACTIVE=1, MARIO_SPECIAL_CAPS=14, MODEL_STAR=1, MODEL_SPARKLES_ANIMATION=2,
 MOD_AUDIO_CHANNEL_MUSIC=1, SEQ_PLAYER_LEVEL=0, SOUND_MENU_POWER_METER=1, SOUND_ACTION_HIT_2=2,
 VR_FIRE_FLOWER_PICKUP_GRACE_FRAMES=12, VR_CONTROLLER_COUNT=2, PLAY_MODE_PAUSED=1,
 OBJ_MOVE_HIT_WALL=1, OBJ_MOVE_MASK_ON_GROUND=2, INT_STATUS_INTERACTED=1, INT_STATUS_TOUCHED_BOB_OMB=2,
 INTERACT_BOUNCE_TOP=1, INTERACT_BOUNCE_TOP2=2, INTERACT_HIT_FROM_BELOW=4, INTERACT_BULLY=8,
 INTERACT_DAMAGE=16, INTERACT_KOOPA=32, INT_FAST_ATTACK_OR_SHELL=32, VR_POWER_STAR_CHOMP_EXPLODE=3,
 ACTIVE_FLAG_UNK10=16, OBJ_MOVE_MASK_IN_WATER=4 };
#define SYS_MAX_PATH 1024
#define LOG_ERROR(...) ((void)0)
struct Object { unsigned activeFlags,oInteractType,oInteractStatus,oMoveFlags; int oIntangibleTimer,oAction;
 float oPosX,oPosY,oPosZ,oVelY,oForwardVel,oGravity,oBounciness,oWallHitboxRadius,oFloorHeight;
 int oMoveAngleYaw,oFaceAngleYaw; const int* behavior;
 struct { struct {Vec3f pos,prevPos,cameraToObject;} gfx;} header; };
struct MarioState {int playerIndex, health,hurtCounter; unsigned flags,action;
 struct Object *marioObj,*heldObj,*interactObj; Vec3f pos;};
struct VrControllerState {int unused;};
struct ModAudio {int unused;};
static int bhvStaticObject[1],bhvSparkle[1],bhvChainChomp[1],bhvBobomb[1];
static struct Object player={.activeFlags=1}, objects[64],*gCurrentObject;
static struct MarioState gMarioStates[1];
static int gCurrLevelNum=1,gCurrAreaIndex=1,sCurrPlayMode,gGlobalSoundSource[3];
static u32 gGlobalTimer;
static bool configVrSpecialFireFlowerMusic=true, configVrPowerStarLongTimer, active=true,online=true,dead,loadFails;
static int spawns, sparkles, loads, plays, stops, attacks;
static float stageVolume=1;
static float musicVolume=1, waterLevel=-11000;
static int gainChanges;
static bool movePickup;
static struct {float floorLowerLimit;} gLevelValues={-11000};
static float find_water_level(float x,float z){return waterLevel;}
static struct ModAudio music;
static bool vr_is_active(void){return active;}
static bool vr_special_moves_online_allowed(void){return online;}
static bool vr_hand_interaction_action_is_death(unsigned a){return dead;}
static void vr_special_moves_reset_power_star(void);
static void vr_special_moves_replace_powerup(void){vr_special_moves_reset_power_star();gMarioStates[0].flags=0;}
static void set_sequence_player_volume(int p,float v){stageVolume=v;}
static void audio_stream_stop(struct ModAudio*a){stops++;}
static struct ModAudio* audio_stream_load_path(const char*p){loads++;assert(strstr(p,"power_star.mp3"));return loadFails?NULL:&music;}
static void audio_stream_set_volume_channel(struct ModAudio*a,int channel){assert(channel==MOD_AUDIO_CHANNEL_MUSIC);}
static void audio_stream_set_volume(struct ModAudio*a,float gain){musicVolume=gain;gainChanges++;}
static void audio_stream_set_looping(struct ModAudio*a,bool loop){assert(loop);}
static void audio_stream_play(struct ModAudio*a,bool r,float v){plays++;}
static void stop_cap_music(void){}
static const char* sys_resource_path(void){return "resources";}
static void play_sound(int s,void*p){}
static struct Object* spawn_object(struct Object*p,int model,const int*b){
 if(model==MODEL_SPARKLES_ANIMATION){sparkles++;return NULL;}
 assert(spawns<64);struct Object*o=&objects[spawns++];*o=(struct Object){.activeFlags=1,.behavior=b};return o;}
static void obj_scale(struct Object*o,float scale){assert(scale==.5f||scale==.4f);}
static void obj_update_gfx_pos_and_angle(struct Object*o){o->header.gfx.pos[0]=o->oPosX;o->header.gfx.pos[1]=o->oPosY;o->header.gfx.pos[2]=o->oPosZ;}
static unsigned random_u16(void){return 0;}
static void vec3f_copy(float*a,const float*b){memcpy(a,b,12);}
static void vr_special_moves_delete_object(struct Object**o){if(*o)(*o)->activeFlags=0;*o=NULL;}
static void cur_obj_update_floor_and_walls(void){}
static void cur_obj_move_standard(int a){if(movePickup)gCurrentObject->oPosY+=gCurrentObject->oVelY;}
static bool vr_get_stabilized_headset_world_position(float*p,bool a){return false;}
static bool vr_get_controller_state(unsigned h,struct VrControllerState*s){return false;}
static bool vr_get_controller_world_fist_reach_target_from_state(unsigned h,struct VrControllerState*s,float*p,float*v){return false;}
static bool obj_has_behavior(struct Object*o,const int*b){return o->behavior==b;}
static void attack_object(struct MarioState*m,struct Object*o,unsigned t){assert(t==INT_FAST_ATTACK_OR_SHELL);attacks++;o->oInteractStatus|=INT_STATUS_INTERACTED;}
static void network_send_object(struct Object*o){}
#include "../src/game/vr_power_star.inc.h"
static void tick(void){gGlobalTimer++;vr_special_moves_update_power_star(gMarioStates);}
int main(void){
 gMarioStates[0]=(struct MarioState){.health=0x880,.marioObj=&player};
 assert(vr_special_moves_grant_power_star()&&sVrPowerStarTimer==900);
 tick();assert(plays==1&&loads==1&&stageVolume==0);
 unsigned remaining=sVrPowerStarTimer;vr_special_moves_update_power_star(gMarioStates);assert(sVrPowerStarTimer==remaining);
 sCurrPlayMode=PLAY_MODE_PAUSED;tick();assert(sVrPowerStarTimer==remaining);sCurrPlayMode=0;
 for(int i=0;i<60;i++)tick();assert(plays==1&&loads==1&&sparkles<=32);
 assert(gainChanges==1&&musicVolume==1); // no redundant gain calls during steady playback
 configVrSpecialFireFlowerMusic=false;tick();assert(!sVrPowerStarMusicPlaying&&stageVolume==1);
 configVrSpecialFireFlowerMusic=true;tick();assert(plays==2);
 gMarioStates[0].flags=MARIO_SPECIAL_CAPS;tick();assert(!sVrPowerStarMusicPlaying&&stageVolume==1);
 gMarioStates[0].flags=0;tick();assert(plays==3);
 struct Object enemy={.activeFlags=1,.oInteractType=INTERACT_BOUNCE_TOP};
 assert(vr_special_moves_power_star_attack(gMarioStates,&enemy)&&attacks==1);
 assert(!vr_special_moves_power_star_attack(gMarioStates,&enemy));
 enemy=(struct Object){.activeFlags=1,.behavior=bhvBobomb};
 assert(vr_special_moves_power_star_attack(gMarioStates,&enemy)&&(enemy.oInteractStatus&INT_STATUS_TOUCHED_BOB_OMB));
 enemy=(struct Object){.activeFlags=1,.behavior=bhvChainChomp};
 assert(vr_special_moves_power_star_attack(gMarioStates,&enemy)&&enemy.oAction==VR_POWER_STAR_CHOMP_EXPLODE);
 enemy=(struct Object){.activeFlags=1};assert(!vr_special_moves_power_star_attack(gMarioStates,&enemy));
 sVrPowerStarTimer=121;tick();assert(musicVolume==1&&vr_special_moves_power_star_fade()==1);
 for(int i=0;i<60;i++)tick();assert(fabsf(musicVolume-.5f)<.0001f);
 assert(vr_special_moves_power_star_active()); // fade is a warning, not early cancellation
 sVrPowerStarTimer=1;tick();assert(!vr_special_moves_power_star_active()&&stageVolume==1);
 configVrPowerStarLongTimer=true;assert(vr_special_moves_grant_power_star()&&sVrPowerStarTimer==1800);
 configVrPowerStarLongTimer=false;tick();assert(sVrPowerStarTimer==1799);
 dead=true;tick();assert(!sVrPowerStarTimer);dead=false;
 assert(vr_special_moves_spawn_cheat_power_star());assert(!vr_special_moves_spawn_cheat_power_star());
 assert(sVrPowerStarPickup->oPosY==480);
 struct Object*p=sVrPowerStarPickup;
 assert(p->activeFlags&ACTIVE_FLAG_UNK10);
 p->oPosX=1000;p->oFloorHeight=-100;waterLevel=100;movePickup=true;
 p->oPosY=105;p->oVelY=-12;tick();assert(p->oPosY==100&&p->oVelY==22);
 assert(!(p->oMoveFlags&OBJ_MOVE_MASK_IN_WATER));
 p->oPosY=80;p->oVelY=-12;tick();assert(p->oPosY==68&&p->oVelY==-12); // submerged: no teleport
 p->oFloorHeight=150;p->oPosY=105;p->oVelY=-12;tick();assert(p->oPosY==93); // floor above water wins
 waterLevel=-11000;p->oFloorHeight=-12000;p->oPosY=-10995;p->oVelY=-12;
 tick();assert(p->oPosY==-11007); // no-water sentinel is not a surface
 movePickup=false;p->oMoveFlags=OBJ_MOVE_MASK_ON_GROUND;p->oPosY=0;p->oVelY=0;
 tick();assert(p->oVelY==22&&!(p->oMoveFlags&OBJ_MOVE_MASK_ON_GROUND)); // native ground, lava and sand floors
 struct Object*old=sVrPowerStarPickup;gCurrAreaIndex++;tick();assert(!sVrPowerStarPickup&&old->activeFlags);
 assert(vr_special_moves_grant_power_star());gCurrAreaIndex++;tick();assert(!sVrPowerStarTimer);
 sVrPowerStarMusic=NULL;sVrPowerStarMusicLoadAttempted=false;loadFails=true;
 assert(vr_special_moves_grant_power_star());tick();assert(stageVolume==1&&!sVrPowerStarMusicPlaying);
 int attempted=loads;tick();assert(loads==attempted);online=false;tick();assert(!sVrPowerStarTimer);
 puts("PASS: Power Star timer, pause/stereo, expiry, area/death reset, bounded pickup/particles, music priority/failure, native contact attacks.");
}
