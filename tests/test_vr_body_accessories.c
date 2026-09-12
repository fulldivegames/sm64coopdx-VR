// Compile the actual attachment helper with a recording renderer. Tests pose
// inheritance/visibility/state only; visual model fit still needs headset QA.
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
typedef float Mat4[4][4];
typedef float Vec3f[3];
typedef int16_t Vec3s[3],s16;
typedef int32_t s32;
typedef int Gfx;
#define MATRIX_STACK_SIZE 8
#define MARIO_ANIM_PART_HEAD 4
#define MARIO_ANIM_PART_TORSO 3
#define GRAPH_NODE_TYPE_GENERATED_LIST 42
#define LAYER_OPAQUE 1
struct MarioState {void* marioObj;} gMarioStates[2], *gCurGraphNodeMarioState;
struct Body {s32 currAnimPart;} body, *gCurMarioBodyState=&body;
struct GraphNode {int type; struct GraphNode* prev;};
typedef void (*GraphNodeFunc)(void);
struct GraphNodeRotation {struct GraphNode node;};
struct GraphNodeGenerated {struct {struct GraphNode node; void (*func)(void);} fnNode;};
static void geo_mario_head_rotation(void) {}
static void* gCurGraphNodeHeldObject;
static void* gCurGraphNodeProcessingObject;
static bool sVrCharacterMenuScene, active=true, propeller, hammer, hidden;
static Mat4 gMatStack[MATRIX_STACK_SIZE],gMatStackPrev[MATRIX_STACK_SIZE];
static int gMatStackIndex,drawCount;
static uint32_t gGlobalTimer=50;
static uint32_t sVrPaintingExitHatHand=2,sVrHeldHelmet;
#define VR_CONTROLLER_COUNT 2
static const Gfx vr_hammer_helmet_dl[1]={4},mario_propeller_zipper_dl[1]={5};
static const Gfx vr_propeller_helmet_dl[1]={1},vr_propeller_rotor_dl[1]={2},vr_hammer_shell_dl[1]={3};
static Mat4 draws[4],previousDraws[4];
static bool vr_is_active(void) {return active;}
static bool vr_special_moves_propeller_active(void) {return propeller;}
static bool vr_special_moves_hammer_suit_active(void) {return hammer;}
static bool vr_hide_local_first_person_mario_part(void) {return hidden;}
static void mtxf_identity(Mat4 m) {memset(m,0,sizeof(Mat4));for(int i=0;i<4;++i)m[i][i]=1;}
static void mtxf_mul(Mat4 d,Mat4 a,Mat4 b) {
    Mat4 t={0};for(int i=0;i<4;++i)for(int j=0;j<4;++j)for(int k=0;k<4;++k)t[i][j]+=a[i][k]*b[k][j];
    memcpy(d,t,sizeof(t));
}
static void mtxf_rotate_xyz_and_translate(Mat4 m,Vec3f p,Vec3s a) {
    mtxf_identity(m);float t=a[0]*(6.283185307f/65536.f);
    m[1][1]=m[2][2]=cosf(t);m[1][2]=sinf(t);m[2][1]=-sinf(t);
    memcpy(m[3],p,sizeof(Vec3f));
}
static bool increment_mat_stack(void) {++gMatStackIndex;return true;}
static void geo_append_display_list(void* dl,int layer) {
    assert(dl && layer==LAYER_OPAQUE && drawCount<4);
    memcpy(draws[drawCount],gMatStack[gMatStackIndex],sizeof(Mat4));
    memcpy(previousDraws[drawCount++],gMatStackPrev[gMatStackIndex],sizeof(Mat4));
}
#include "../src/game/vr_body_accessories.inc.h"
int main(void) {
    int object;gMarioStates[0].marioObj=&object;
    // Native Mario does NOT set GRAPH_RENDER_PLAYER: its optional animation
    // offset is disabled. Object/body ownership must suffice without this cache.
    gCurGraphNodeProcessingObject=&object;gCurGraphNodeMarioState=NULL;
    mtxf_identity(gMatStack[0]);mtxf_identity(gMatStackPrev[0]);
    gMatStack[0][3][0]=100;gMatStackPrev[0][3][0]=-20;
    body.currAnimPart=MARIO_ANIM_PART_HEAD;propeller=true;
    Mat4 original;memcpy(original,gMatStack[0],sizeof(Mat4));
    vr_append_propeller_helmet();assert(drawCount==2 && gMatStackIndex==0);
    assert(draws[0][3][0]==100 && previousDraws[0][3][0]==-20);
    assert(draws[1][3][0]==418 && previousDraws[1][3][0]==298);
    assert(memcmp(original,gMatStack[0],sizeof(Mat4))==0);
    vr_append_propeller_helmet();assert(drawCount==2); // no duplicate head/LOD
    drawCount=0;sVrPropellerHeadAttached=false;hidden=true;
    vr_append_propeller_helmet();assert(drawCount==0); // local first person
    hidden=false;sVrPropellerHeadAttached=false;
    int remote;gCurGraphNodeProcessingObject=&remote;
    vr_append_propeller_helmet();assert(drawCount==0); // never another player
    gCurGraphNodeProcessingObject=&object;gCurGraphNodeHeldObject=&object;
    vr_append_propeller_helmet();assert(drawCount==0); // never held skeleton
    gCurGraphNodeHeldObject=NULL;active=false;
    vr_append_propeller_helmet();assert(drawCount==0);
    active=true;body.currAnimPart=MARIO_ANIM_PART_HEAD;
    struct GraphNodeGenerated callback={.fnNode={.node={.type=42},.func=geo_mario_head_rotation}};
    struct GraphNodeRotation rotation={.node={.prev=&callback.fnNode.node}};
    assert(vr_is_head_rotation_node(&rotation));
    callback.fnNode.func=NULL;assert(!vr_is_head_rotation_node(&rotation));
    propeller=false;hammer=true;drawCount=0;sVrPropellerHeadAttached=false;
    vr_append_propeller_helmet();assert(drawCount==1);
    drawCount=0;sVrPropellerHeadAttached=false;sVrHeldHelmet=2;sVrPaintingExitHatHand=0;
    vr_append_propeller_helmet();assert(drawCount==0);
    sVrPaintingExitHatHand=2;sVrHeldHelmet=0;body.currAnimPart=MARIO_ANIM_PART_TORSO;
    vr_append_hammer_back_shell();assert(drawCount==1 && gMatStackIndex==0);
    puts("PASS: native Mario head attachment without player flag, endpoints, visibility, ownership and rotation callback");
}
