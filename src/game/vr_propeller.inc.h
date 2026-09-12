// Local power-up state; included after the common power-up/reset helpers.
#define VR_PROPELLER_PICKUP_GROUND_OFFSET 0.0f
#define VR_PROPELLER_PICKUP_GRAVITY 2.0f
#define VR_PROPELLER_PICKUP_FALL_SPEED 40.0f
#define VR_PROPELLER_DURATION (60 * 30)
static struct Object* sVrPropellerPickupObject;
static f32 sVrPropellerPickupVelocityY;
static bool sVrPropellerPickupLanded;
static u16 sVrPropellerPickupAge;
static u16 sVrPropellerTimer;
static bool sVrPropellerBurstUsed, sVrPropellerMusic;
static s16 sVrPropellerLevel = -1, sVrPropellerArea = -1;
static s16 sVrPropellerPickupLevel = -1, sVrPropellerPickupArea = -1;
static u32 sVrPropellerTick = (u32)-1;

bool vr_special_moves_propeller_active(void) {
    return sVrPropellerTimer != 0 && vr_is_active() &&
        sVrPropellerLevel == gCurrLevelNum && sVrPropellerArea == gCurrAreaIndex;
}

bool vr_special_moves_propeller_can_burst(struct MarioState* m) {
    if (!m || m->playerIndex != 0 || !vr_special_moves_propeller_active() ||
        sVrPropellerBurstUsed || m->heldObj || m->health <= 0x100) return false;
    // Eligibility is airtime, not jump type/height/speed. This includes short
    // hops, water exits, slope jumps, knockback and lava rebound actions.
    // Never interrupt a submerged action, scripted entrance, death or cutscene.
    return (m->action & ACT_FLAG_AIR) && !(m->action & ACT_FLAG_INTANGIBLE) &&
        (m->action & ACT_GROUP_MASK) == ACT_GROUP_AIRBORNE &&
        !vr_hand_interaction_action_is_death(m->action);
}

void vr_special_moves_propeller_recharge(struct MarioState* m) {
    if (m && m->playerIndex == 0 && vr_special_moves_propeller_active())
        sVrPropellerBurstUsed = false;
}

void vr_special_moves_propeller_stomp_burst(struct MarioState* m) {
    // Called only after an accepted top attack on a native stompable enemy.
    // A successful stomp may relaunch even after this airtime's manual burst.
    if (!m || m->playerIndex != 0 || !vr_special_moves_propeller_active() ||
        m->heldObj || m->health <= 0x100) return;
    sVrPropellerBurstUsed = true;
    set_mario_action(m, ACT_TWIRLING, VR_PROPELLER_ACTION_ARG);
    m->vel[1] = 75.0f;
    m->twirlYaw = 0;
    m->angleVel[1] = 0x1800;
    m->flags &= ~MARIO_UNKNOWN_08;
    m->input &= ~INPUT_A_PRESSED;
}

static void vr_special_moves_reset_propeller(void) {
    struct MarioState* m = &gMarioStates[0];
    if (sVrPropellerMusic && !(m->flags & MARIO_SPECIAL_CAPS)) stop_cap_music();
    sVrPropellerMusic = false;
    sVrPropellerTimer = 0;
    sVrPropellerBurstUsed = false;
    sVrPropellerLevel = sVrPropellerArea = -1;
    if (m->action == ACT_TWIRLING && (m->actionArg & VR_PROPELLER_ACTION_ARG)) {
        set_mario_action(m, ACT_FREEFALL, 0);
    }
}

static bool vr_special_moves_grant_propeller(void) {
    if (!vr_is_active() || !vr_special_moves_online_allowed() ||
        gMarioStates[0].health <= 0x100 ||
        vr_hand_interaction_action_is_death(gMarioStates[0].action)) return false;
    vr_special_moves_replace_powerup();
    sVrPropellerTimer = VR_PROPELLER_DURATION;
    sVrPropellerBurstUsed = false;
    sVrPropellerLevel = gCurrLevelNum;
    sVrPropellerArea = gCurrAreaIndex;
    play_sound(SOUND_MENU_POWER_METER, gGlobalSoundSource);
    return true;
}

static bool vr_special_moves_spawn_propeller_pickup(struct Object* parent,
        f32 x, f32 y, f32 z, f32 velocityY) {
    if (parent == NULL || (sVrPropellerPickupObject &&
        (sVrPropellerPickupObject->activeFlags & ACTIVE_FLAG_ACTIVE))) return false;
    sVrPropellerPickupObject = spawn_object(parent, MODEL_VR_PROPELLER_MUSHROOM, bhvStaticObject);
    if (!sVrPropellerPickupObject) return false;
    sVrPropellerPickupObject->oPosX = x;
    sVrPropellerPickupObject->oPosY = y;
    sVrPropellerPickupObject->oPosZ = z;
    sVrPropellerPickupObject->oInteractType = 0;
    obj_scale(sVrPropellerPickupObject, 1.0f);
    obj_update_gfx_pos_and_angle(sVrPropellerPickupObject);
    sVrPropellerPickupVelocityY = velocityY;
    sVrPropellerPickupLanded = false;
    sVrPropellerPickupAge = 0;
    sVrPropellerPickupLevel = gCurrLevelNum;
    sVrPropellerPickupArea = gCurrAreaIndex;
    return true;
}

bool vr_special_moves_spawn_cheat_propeller(void) {
    struct MarioState* m = &gMarioStates[0];
    if (!vr_is_active() || !vr_special_moves_online_allowed() || !m->marioObj) return false;
    vr_special_moves_delete_object(&sVrPropellerPickupObject);
    return vr_special_moves_spawn_propeller_pickup(m->marioObj,
        m->pos[0], m->pos[1] + 480.0f, m->pos[2], 0.0f);
}

#include "vr_propeller_pickup.inc.h"

static void vr_special_moves_update_propeller(struct MarioState* m) {
    if (sVrPropellerTick == gGlobalTimer) return;
    sVrPropellerTick = gGlobalTimer;
    // Old areas free their object pool. Never dereference/delete that recycled
    // pointer in the new area (it can now belong to an unrelated object).
    if (sVrPropellerPickupLevel != gCurrLevelNum || sVrPropellerPickupArea != gCurrAreaIndex) {
        sVrPropellerPickupObject = NULL;
    }
    if (!vr_is_active() || !vr_special_moves_online_allowed() ||
        vr_hand_interaction_action_is_death(m->action) || m->health <= 0x100) {
        vr_special_moves_reset_propeller();
        vr_special_moves_delete_object(&sVrPropellerPickupObject);
        return;
    }
    if (sVrPropellerTimer && !vr_special_moves_propeller_active()) vr_special_moves_reset_propeller();
    if (sCurrPlayMode == PLAY_MODE_PAUSED) return;
    if (sVrPropellerTimer && --sVrPropellerTimer == 0) vr_special_moves_reset_propeller();
    vr_special_moves_update_propeller_pickup(m);
    if (!vr_special_moves_propeller_active()) return;
    bool grounded = !(m->action & ACT_FLAG_AIR) && m->floor &&
        fabsf(m->pos[1] - m->floorHeight) <= 5.0f &&
        ((m->action & ACT_GROUP_MASK) == ACT_GROUP_MOVING ||
         (m->action & ACT_GROUP_MASK) == ACT_GROUP_STATIONARY);
    if (grounded || (m->action & ACT_GROUP_MASK) == ACT_GROUP_SUBMERGED)
        vr_special_moves_propeller_recharge(m);
    if ((m->input & INPUT_A_PRESSED) && vr_special_moves_propeller_can_burst(m)) {
        vr_special_moves_propeller_stomp_burst(m);
    }
    bool music = configVrSpecialFireFlowerMusic && !configVrAlternatePowerUpMusic &&
        !(m->flags & MARIO_SPECIAL_CAPS);
    if (music) {
        if (sVrPropellerTimer > 60) play_cap_music(SEQUENCE_ARGS(4, gLevelValues.wingCapSequence));
        else if (sVrPropellerTimer == 60) fadeout_cap_music();
    } else if (sVrPropellerMusic) stop_cap_music();
    sVrPropellerMusic = music;
}
