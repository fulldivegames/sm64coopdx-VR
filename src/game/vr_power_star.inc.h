// Shared Quest/PC Power Star. No new per-frame world scans or shaders.
static u16 sVrPowerStarTimer;
static s16 sVrPowerStarLevel = -1, sVrPowerStarArea = -1;
static u32 sVrPowerStarTick = (u32)-1;
static struct Object* sVrPowerStarPickup;
static s16 sVrPowerStarPickupLevel = -1, sVrPowerStarPickupArea = -1;
static u16 sVrPowerStarPickupAge;
static struct ModAudio* sVrPowerStarMusic;
static bool sVrPowerStarMusicPlaying, sVrPowerStarMusicLoadAttempted;
static f32 sVrPowerStarMusicGain = -1.0f;

f32 vr_special_moves_power_star_fade(void) {
    return sVrPowerStarTimer >= 120 ? 1.0f : (f32)sVrPowerStarTimer / 120.0f;
}

bool vr_special_moves_power_star_active(void) {
    return sVrPowerStarTimer && vr_is_active() &&
        vr_special_moves_online_allowed() &&
        sVrPowerStarLevel == gCurrLevelNum && sVrPowerStarArea == gCurrAreaIndex;
}

static void vr_power_star_stop_music(void) {
    if (!sVrPowerStarMusicPlaying) return;
    if (sVrPowerStarMusic) audio_stream_stop(sVrPowerStarMusic);
    sVrPowerStarMusicPlaying = false;
    sVrPowerStarMusicGain = -1.0f;
    set_sequence_player_volume(SEQ_PLAYER_LEVEL, 1.0f);
}

static void vr_special_moves_reset_power_star(void) {
    vr_power_star_stop_music();
    sVrPowerStarTimer = 0;
    sVrPowerStarLevel = sVrPowerStarArea = -1;
}

static void vr_power_star_music(struct MarioState* m) {
    bool wanted = vr_special_moves_power_star_active() && configVrSpecialFireFlowerMusic &&
        !(m->flags & MARIO_SPECIAL_CAPS);
    if (!wanted) { vr_power_star_stop_music(); return; }
    if (!sVrPowerStarMusicLoadAttempted) {
        char path[SYS_MAX_PATH];
        snprintf(path, sizeof(path), "%s/sonic_shoes/power_star.mp3", sys_resource_path());
        sVrPowerStarMusicLoadAttempted = true;
        sVrPowerStarMusic = audio_stream_load_path(path);
        if (!sVrPowerStarMusic) LOG_ERROR("Power Star music unavailable: %s", path);
    }
    // Never silence the stage when the dedicated track failed to load.
    if (!sVrPowerStarMusic) return;
    if (!sVrPowerStarMusicPlaying) {
        stop_cap_music();
        audio_stream_set_volume_channel(sVrPowerStarMusic, MOD_AUDIO_CHANNEL_MUSIC);
        audio_stream_set_looping(sVrPowerStarMusic, true);
        audio_stream_play(sVrPowerStarMusic, true, 1.0f);
        sVrPowerStarMusicPlaying = true;
    }
    const f32 gain = vr_special_moves_power_star_fade();
    if (gain != sVrPowerStarMusicGain) {
        audio_stream_set_volume(sVrPowerStarMusic, gain);
        sVrPowerStarMusicGain = gain;
    }
    set_sequence_player_volume(SEQ_PLAYER_LEVEL, 0.0f);
}

static bool vr_special_moves_grant_power_star(void) {
    struct MarioState* m = &gMarioStates[0];
    if (!vr_is_active() || !vr_special_moves_online_allowed() || m->health <= 0x100 ||
        vr_hand_interaction_action_is_death(m->action)) return false;
    vr_special_moves_replace_powerup();
    m->hurtCounter = 0;
    sVrPowerStarTimer = (configVrPowerStarLongTimer ? 60 : 30) * 30;
    sVrPowerStarLevel = gCurrLevelNum;
    sVrPowerStarArea = gCurrAreaIndex;
    play_sound(SOUND_MENU_POWER_METER, gGlobalSoundSource);
    return true;
}

static bool vr_special_moves_spawn_power_star_pickup(struct Object* parent, f32 x, f32 y, f32 z) {
    if (!parent || (sVrPowerStarPickup && sVrPowerStarPickupLevel == gCurrLevelNum &&
        sVrPowerStarPickupArea == gCurrAreaIndex &&
        (sVrPowerStarPickup->activeFlags & ACTIVE_FLAG_ACTIVE))) return false;
    struct Object* p = spawn_object(parent, MODEL_STAR, bhvStaticObject);
    if (!p) return false;
    sVrPowerStarPickup = p;
    sVrPowerStarPickupLevel = gCurrLevelNum;
    sVrPowerStarPickupArea = gCurrAreaIndex;
    sVrPowerStarPickupAge = 0;
    p->oPosX = x; p->oPosY = y; p->oPosZ = z;
    p->oVelY = 20.0f;
    p->oForwardVel = 8.0f;
    p->oMoveAngleYaw = random_u16();
    p->oGravity = -2.0f;
    p->oBounciness = 0.0f;
    p->oWallHitboxRadius = 20.0f;
    p->oInteractType = 0; // Never a course star / save-file reward.
    // Water is handled below as a bounce plane; skip the native underwater
    // solver's water query and buoyancy so there is only one query per tick.
    p->activeFlags |= ACTIVE_FLAG_UNK10;
    obj_scale(p, 0.5f);
    obj_update_gfx_pos_and_angle(p);
    return true;
}

bool vr_special_moves_spawn_cheat_power_star(void) {
    struct MarioState* m = &gMarioStates[0];
    return vr_is_active() && vr_special_moves_online_allowed() &&
        vr_special_moves_spawn_power_star_pickup(m->marioObj, m->pos[0], m->pos[1]+480, m->pos[2]);
}

static void vr_power_star_sparkle(struct Object* parent, f32 x, f32 y, f32 z) {
    struct Object* p = spawn_object(parent, MODEL_SPARKLES_ANIMATION, bhvSparkle);
    if (!p) return;
    p->oPosX = x; p->oPosY = y; p->oPosZ = z;
    obj_scale(p, 0.4f);
    obj_update_gfx_pos_and_angle(p);
    vec3f_copy(p->header.gfx.prevPos, p->header.gfx.pos);
}

static bool vr_power_star_near(const Vec3f a, const Vec3f b, float radius) {
    float x=a[0]-b[0], y=a[1]-b[1], z=a[2]-b[2];
    return x*x+y*y+z*z <= radius*radius;
}

static void vr_special_moves_update_power_star(struct MarioState* m) {
    if (sVrPowerStarTick == gGlobalTimer) return;
    sVrPowerStarTick = gGlobalTimer;
    if (sVrPowerStarPickupLevel != gCurrLevelNum || sVrPowerStarPickupArea != gCurrAreaIndex)
        sVrPowerStarPickup = NULL; // Do not touch recycled level objects.
    if (!vr_is_active() || !vr_special_moves_online_allowed() || m->health <= 0x100 ||
        vr_hand_interaction_action_is_death(m->action)) {
        vr_special_moves_reset_power_star();
        vr_special_moves_delete_object(&sVrPowerStarPickup);
        return;
    }
    if (sVrPowerStarTimer && !vr_special_moves_power_star_active()) vr_special_moves_reset_power_star();
    if (sCurrPlayMode == PLAY_MODE_PAUSED) return;
    if (sVrPowerStarTimer && --sVrPowerStarTimer == 0) vr_special_moves_reset_power_star();
    struct Object* p = sVrPowerStarPickup;
    if (p && !(p->activeFlags & ACTIVE_FLAG_ACTIVE)) sVrPowerStarPickup = p = NULL;
    if (p) {
        if (++sVrPowerStarPickupAge > 30*60) { vr_special_moves_delete_object(&sVrPowerStarPickup); }
        else {
            // Native wall/floor solver prevents bouncing through map geometry.
            struct Object* saved = gCurrentObject;
            gCurrentObject = p;
            const f32 previousY = p->oPosY;
            cur_obj_update_floor_and_walls();
            cur_obj_move_standard(-78);
            if (p->oMoveFlags & OBJ_MOVE_HIT_WALL) p->oMoveAngleYaw += 0x8000;
            // Water is not a collision floor. Only catch a descending crossing
            // from above; never teleport a submerged pickup up through a roof.
            const f32 water = find_water_level(p->oPosX, p->oPosZ);
            if (water > gLevelValues.floorLowerLimit &&
                water > p->oFloorHeight && previousY >= water &&
                p->oPosY <= water && p->oVelY <= 0.0f) {
                p->oPosY = water;
                p->oVelY = 22.0f;
                p->oMoveFlags &= ~(OBJ_MOVE_MASK_IN_WATER | OBJ_MOVE_MASK_ON_GROUND);
            }
            if (p->oMoveFlags & OBJ_MOVE_MASK_ON_GROUND) {
                // Includes lava/quicksand collision floors: this pickup does
                // not use Mario's sinking/burning surface actions.
                p->oVelY = 22.0f;
                p->oMoveFlags &= ~OBJ_MOVE_MASK_ON_GROUND;
            }
            gCurrentObject = saved;
            p->oFaceAngleYaw += 0x600;
            obj_update_gfx_pos_and_angle(p);
            if ((gGlobalTimer & 3) == 0) vr_power_star_sparkle(p, p->oPosX, p->oPosY, p->oPosZ);
            Vec3f point = {p->oPosX,p->oPosY,p->oPosZ};
            Vec3f body = {m->pos[0],m->pos[1]+60,m->pos[2]}, head;
            bool collect = vr_power_star_near(point, body, 65);
            if (!collect && vr_get_stabilized_headset_world_position(head, false))
                collect = vr_power_star_near(point, head, 38);
            for (u32 hand=0; !collect && hand<VR_CONTROLLER_COUNT; ++hand) {
                struct VrControllerState state;
                Vec3f fist;
                if (vr_get_controller_state(hand,&state) &&
                    vr_get_controller_world_fist_reach_target_from_state(hand,&state,fist,NULL))
                    collect = vr_power_star_near(point,fist,38);
            }
            if (sVrPowerStarPickupAge > VR_FIRE_FLOWER_PICKUP_GRACE_FRAMES && collect &&
                vr_special_moves_grant_power_star()) vr_special_moves_delete_object(&sVrPowerStarPickup);
        }
    }
    vr_power_star_music(m);
    // One torso sparkle every two simulation ticks: independent of headset Hz.
    if (vr_special_moves_power_star_active() && !(gGlobalTimer & 1)) {
        const float phase = (float)gGlobalTimer * 0.6f;
        vr_power_star_sparkle(m->marioObj,
            m->marioObj->header.gfx.pos[0] + sinf(phase)*25,
            m->marioObj->header.gfx.pos[1] + 75 + cosf(phase)*20,
            m->marioObj->header.gfx.pos[2] + cosf(phase)*25);
    }
}

bool vr_special_moves_power_star_attack(struct MarioState* m, struct Object* target) {
    if (!m || m->playerIndex != 0 || !vr_special_moves_power_star_active() || !target ||
        target == m->heldObj || !(target->activeFlags & ACTIVE_FLAG_ACTIVE) ||
        target->oIntangibleTimer != 0 || (target->oInteractStatus & INT_STATUS_INTERACTED)) return false;
    bool chomp = obj_has_behavior(target,bhvChainChomp);
    bool bobomb = obj_has_behavior(target,bhvBobomb);
    if (!chomp && !bobomb && !(target->oInteractType &
        (INTERACT_BOUNCE_TOP | INTERACT_BOUNCE_TOP2 | INTERACT_HIT_FROM_BELOW | INTERACT_BULLY |
         INTERACT_DAMAGE | INTERACT_KOOPA))) return false;
    m->interactObj = target;
    attack_object(m,target,INT_FAST_ATTACK_OR_SHELL);
    if (bobomb) target->oInteractStatus |= INT_STATUS_TOUCHED_BOB_OMB;
    if (chomp) target->oAction = VR_POWER_STAR_CHOMP_EXPLODE;
    play_sound(SOUND_ACTION_HIT_2, target->header.gfx.cameraToObject);
    network_send_object(target);
    return true;
}
