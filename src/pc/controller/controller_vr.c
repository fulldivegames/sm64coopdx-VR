#include <math.h>
#include <stdio.h>

#include "controller_vr.h"
#include "sm64.h"

#include "pc/configfile.h"
#include "pc/vr/vr.h"
#include "game/vr_speedrun.h"
#include "game/level_update.h"
#include "game/ingame_menu.h"
#include "game/vr_hand_interaction.h"
#include "game/rendering_graph_node.h"
#include "pc/utils/misc.h"
#include "vr_jump_gesture.h"
#include "vr_punch_travel.h"
#include "vr_dive_stroke.h"
static struct VrDiveStroke sDiveStroke[2];
#include "pc/fs/fs.h"
// The DJUI menu reads the same N64 pad passed to this backend while an
// interactable panel is active. Keep this menu-only flag separate from the
// gameplay Jump binding so rebinding Jump cannot remove the A action.
extern bool gInteractableOverridePad;
extern bool gDjuiInMainMenu;
static struct VrJumpGesture sJumpGesture;
static struct VrWallJumpGesture sWallJumpGesture;
static struct VrLandingJumpGesture sLandingJumpGesture;
static double sPhysicalJumpChainDeadline;
static bool sPhysicalJumpWasGrounded;
static bool sJumpUsedTriggers;
static int sWallJumpCommand;
static uint32_t sJumpTrackingGeneration;

#define VR_STICK_DEADZONE 0.18f
#define VR_TRIGGER_THRESHOLD 0.55f
#define VR_CAMERA_BUTTON_THRESHOLD 0.55f
#define VR_PHYSICAL_CROUCH_RELEASE_HYSTERESIS 0.08f
#define VR_PUNCH_MIN_RESET_SPEED 0.20f
#define VR_PUNCH_RESET_SPEED_SCALE 0.35f
#define VR_PUNCH_TRAVEL_SPEED_SCALE 0.60f

static bool sVrPunchArmed[VR_CONTROLLER_COUNT] = {
    true,
    true
};
static bool sVrPunchLastPositionValid[VR_CONTROLLER_COUNT] = {
    false,
    false
};
static float
    sVrPunchLastPosition[VR_CONTROLLER_COUNT][3] = { 0 };
static float sVrPunchStartPosition[VR_CONTROLLER_COUNT][3];
static float sVrPunchTravel[VR_CONTROLLER_COUNT] = {
    0.0f,
    0.0f
};
static bool sVrPhysicalCrouchActive = false;
static uint32_t sVrPhysicalCrouchTrackingGeneration = 0;

// VR action bindings are exposed to the DJUI binding screen through the
// controller API raw-key channel. Keep this edge-triggered: a held button
// must not immediately rebind another action when the user opens a row.
static bool sVrPreviousBindingDown[VR_CONTROLLER_BINDING_COUNT] = { false };
static u32 sVrPendingRawKey = VK_INVALID;
static bool sVrSplitDown = false;

static void controller_vr_reset_rawkey_state(void) {
    sVrSplitDown = false;
    for (unsigned int binding = 0;
         binding < VR_CONTROLLER_BINDING_COUNT;
         binding++) {
        sVrPreviousBindingDown[binding] = false;
    }
    sVrPendingRawKey = VK_INVALID;
}
static bool sVrPhysicalCrouchReferenceValid = false;
static float sVrPhysicalCrouchReferenceY = 0.0f;

static float controller_vr_clampf(
    float value,
    float minimum,
    float maximum
) {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static void controller_vr_convert_stick(
    const float input[2],
    s8* outputX,
    s8* outputY
) {
    const float magnitudeSquared =
        input[0] * input[0] +
        input[1] * input[1];

    *outputX = 0;
    *outputY = 0;
    if (magnitudeSquared <=
        VR_STICK_DEADZONE * VR_STICK_DEADZONE) {
        return;
    }
    const float magnitude = sqrtf(magnitudeSquared);

    const float normalizedMagnitude = controller_vr_clampf(
        (magnitude - VR_STICK_DEADZONE) /
            (1.0f - VR_STICK_DEADZONE),
        0.0f,
        1.0f
    );
    const float scale = normalizedMagnitude * 127.0f / magnitude;
    const float scaledX = controller_vr_clampf(
        input[0] * scale,
        -127.0f,
        127.0f
    );
    const float scaledY = controller_vr_clampf(
        input[1] * scale,
        -127.0f,
        127.0f
    );

    *outputX = (s8)roundf(scaledX);
    *outputY = (s8)roundf(scaledY);
}

static void controller_vr_merge_stick(
    s8* destinationX,
    s8* destinationY,
    const float source[2]
) {
    s8 sourceX = 0;
    s8 sourceY = 0;
    controller_vr_convert_stick(source, &sourceX, &sourceY);

    const s32 destinationMagnitudeSquared =
        (s32)(*destinationX) * (s32)(*destinationX) +
        (s32)(*destinationY) * (s32)(*destinationY);
    const s32 sourceMagnitudeSquared =
        (s32)sourceX * (s32)sourceX +
        (s32)sourceY * (s32)sourceY;

    // Keep whichever physical device has the stronger input. This lets a
    // connected gamepad remain usable without neutral Touch sticks erasing it.
    if (sourceMagnitudeSquared > destinationMagnitudeSquared) {
        *destinationX = sourceX;
        *destinationY = sourceY;
    }
}

static bool controller_vr_binding_down(
    unsigned int binding,
    bool leftAvailable,
    const struct VrControllerState* left,
    bool rightAvailable,
    const struct VrControllerState* right
) {
    const struct VrControllerState* state = NULL;

    switch (binding) {
        case VR_CONTROLLER_BINDING_LEFT_PRIMARY:
        case VR_CONTROLLER_BINDING_LEFT_SECONDARY:
        case VR_CONTROLLER_BINDING_LEFT_TRIGGER:
        case VR_CONTROLLER_BINDING_LEFT_GRIP:
        case VR_CONTROLLER_BINDING_LEFT_STICK_CLICK:
        case VR_CONTROLLER_BINDING_LEFT_MENU:
            if (leftAvailable) {
                state = left;
            }
            break;
        case VR_CONTROLLER_BINDING_RIGHT_PRIMARY:
        case VR_CONTROLLER_BINDING_RIGHT_SECONDARY:
        case VR_CONTROLLER_BINDING_RIGHT_TRIGGER:
        case VR_CONTROLLER_BINDING_RIGHT_GRIP:
        case VR_CONTROLLER_BINDING_RIGHT_STICK_CLICK:
        case VR_CONTROLLER_BINDING_RIGHT_MENU:
            if (rightAvailable) {
                state = right;
            }
            break;
        default:
            return false;
    }

    if (state == NULL) {
        return false;
    }

    switch (binding) {
        case VR_CONTROLLER_BINDING_LEFT_PRIMARY:
        case VR_CONTROLLER_BINDING_RIGHT_PRIMARY:
            return state->primaryButton;
        case VR_CONTROLLER_BINDING_LEFT_SECONDARY:
        case VR_CONTROLLER_BINDING_RIGHT_SECONDARY:
            return state->secondaryButton;
        case VR_CONTROLLER_BINDING_LEFT_TRIGGER:
        case VR_CONTROLLER_BINDING_RIGHT_TRIGGER:
            return state->trigger >= VR_TRIGGER_THRESHOLD;
        case VR_CONTROLLER_BINDING_LEFT_GRIP:
        case VR_CONTROLLER_BINDING_RIGHT_GRIP:
            return state->squeeze >= VR_TRIGGER_THRESHOLD;
        case VR_CONTROLLER_BINDING_LEFT_STICK_CLICK:
        case VR_CONTROLLER_BINDING_RIGHT_STICK_CLICK:
            return state->thumbstickButton;
        case VR_CONTROLLER_BINDING_LEFT_MENU:
        case VR_CONTROLLER_BINDING_RIGHT_MENU:
            return state->menuButton;
        default:
            return false;
    }
}

static const float* controller_vr_select_stick(
    unsigned int stick,
    bool leftAvailable,
    const struct VrControllerState* left,
    bool rightAvailable,
    const struct VrControllerState* right
) {
    if (stick == VR_CONTROLLER_STICK_LEFT && leftAvailable) {
        return left->thumbstick;
    }
    if (stick == VR_CONTROLLER_STICK_RIGHT && rightAvailable) {
        return right->thumbstick;
    }
    return NULL;
}

#include "vr_crouch_height.h"

static bool controller_vr_update_physical_crouch(void) {
    if (!vr_is_active() ||
        configVrCameraMode != VR_CAMERA_MODE_FIRST_PERSON ||
        !configVrPhysicalCrouching) {
        sVrPhysicalCrouchActive = false;
        sVrPhysicalCrouchReferenceValid = false;
        return false;
    }

    const uint32_t trackingGeneration =
        vr_get_tracking_origin_generation();
    if (trackingGeneration !=
        sVrPhysicalCrouchTrackingGeneration) {
        sVrPhysicalCrouchTrackingGeneration = trackingGeneration;
        sVrPhysicalCrouchActive = false;
        sVrPhysicalCrouchReferenceValid = false;
        return false;
    }

    float translation[3] = { 0 };
    float rotation[4] = { 0, 0, 0, 1 };
    float calibratedHeight = 0.0f;
    if (!vr_get_head_translation(translation) ||
        !vr_get_head_rotation(rotation) ||
        !vr_get_calibrated_head_height(&calibratedHeight)) {
        sVrPhysicalCrouchActive = false;
        sVrPhysicalCrouchReferenceValid = false;
        return false;
    }

    const float crouchHeight = translation[1] + vr_crouch_nod_correction(rotation);
    const float crouchDescent = calibratedHeight *
        (float)MIN(50U, MAX(10U, configVrPhysicalCrouchDepth)) / 100.0f;
    if (!sVrPhysicalCrouchReferenceValid) {
        sVrPhysicalCrouchReferenceY = crouchHeight;
        sVrPhysicalCrouchReferenceValid = true;
        sVrPhysicalCrouchActive = false;
        return false;
    }
    const float releaseDescent = fmaxf(
        0.0f,
        crouchDescent -
            VR_PHYSICAL_CROUCH_RELEASE_HYSTERESIS
    );
    const float downwardTravel =
        -(crouchHeight - sVrPhysicalCrouchReferenceY);

    if (sVrPhysicalCrouchActive) {
        if (downwardTravel <= releaseDescent) {
            sVrPhysicalCrouchActive = false;
        }
    } else if (downwardTravel >= crouchDescent) {
        sVrPhysicalCrouchActive = true;
    }

    return sVrPhysicalCrouchActive;
}

static bool controller_vr_update_physical_punch(
    uint32_t hand,
    const struct VrControllerState* state
) {
    if (hand >= VR_CONTROLLER_COUNT ||
        state == NULL ||
        !state->gripPoseValid ||
        !state->gripLinearVelocityValid) {
        if (hand < VR_CONTROLLER_COUNT) {
            sVrPunchArmed[hand] = false;
            sVrPunchLastPositionValid[hand] = false;
            sVrPunchTravel[hand] = 0.0f;
        }
        return false;
    }

    const float gripAmount = state->squeeze;
    const float gripThreshold = controller_vr_clampf(
        (float)configVrPunchGripThreshold,
        10.0f,
        100.0f
    ) / 100.0f;
    if (gripAmount < gripThreshold) {
        sVrPunchArmed[hand] = true;
        sVrPunchLastPositionValid[hand] = false;
        sVrPunchTravel[hand] = 0.0f;
        return false;
    }

    // Most frames have an open hand. Avoid the velocity square root and all
    // gesture-threshold work until the grip has armed a physical punch.
    const float speedSquared =
        state->gripLinearVelocity[0] *
            state->gripLinearVelocity[0] +
        state->gripLinearVelocity[1] *
            state->gripLinearVelocity[1] +
        state->gripLinearVelocity[2] *
            state->gripLinearVelocity[2];

    const float requiredSpeed = controller_vr_clampf(
        (float)configVrPunchSpeed,
        75.0f,
        300.0f
    ) / 100.0f;
    const float requiredDistance = controller_vr_clampf(
        (float)configVrPunchDistance,
        5.0f,
        50.0f
    ) / 100.0f;
    const float resetSpeed = fmaxf(
        VR_PUNCH_MIN_RESET_SPEED,
        requiredSpeed * VR_PUNCH_RESET_SPEED_SCALE
    );
    const float travelSpeed =
        requiredSpeed * VR_PUNCH_TRAVEL_SPEED_SCALE;

    if (!sVrPunchLastPositionValid[hand]) {
        for (uint32_t axis = 0; axis < 3; axis++) {
            sVrPunchLastPosition[hand][axis] =
                state->gripPosition[axis];
        }
        sVrPunchLastPositionValid[hand] = true;
        sVrPunchTravel[hand] = 0.0f;
        return false;
    }

    if (sVrPunchTravel[hand] == 0.0f) {
        for (uint32_t axis = 0; axis < 3; axis++) {
            sVrPunchStartPosition[hand][axis] = sVrPunchLastPosition[hand][axis];
        }
    }
    for (uint32_t axis = 0; axis < 3; axis++) {
        sVrPunchLastPosition[hand][axis] =
            state->gripPosition[axis];
    }

    if (speedSquared <= resetSpeed * resetSpeed) {
        sVrPunchArmed[hand] = true;
        sVrPunchTravel[hand] = 0.0f;
        return false;
    }

    if (speedSquared >= travelSpeed * travelSpeed) {
        sVrPunchTravel[hand] = vr_punch_travel(sVrPunchStartPosition[hand],
            state->gripPosition, configVrPhysicalJumping);
    } else {
        sVrPunchTravel[hand] = 0.0f;
    }

    if (!sVrPunchArmed[hand] ||
        speedSquared < requiredSpeed * requiredSpeed ||
        sVrPunchTravel[hand] < requiredDistance) {
        return false;
    }

    sVrPunchArmed[hand] = false;
#ifdef DEBUG
    const float speed = sqrtf(speedSquared);
    printf(
        "[VR] %s physical punch detected "
        "(%.2f m/s, %.2f m travel).\n",
        hand == VR_CONTROLLER_LEFT ? "Left" : "Right",
        speed,
        sVrPunchTravel[hand]
    );
#endif
    sVrPunchTravel[hand] = 0.0f;
    vr_apply_haptic(hand, 0.25f, 0.04f, -1.0f);
    return true;
}

static void controller_vr_reset_physical_punch(uint32_t hand) {
    if (hand < VR_CONTROLLER_COUNT) {
        sVrPunchArmed[hand] = true;
        sVrPunchLastPositionValid[hand] = false;
        sVrPunchTravel[hand] = 0.0f;
    }
}

static void controller_vr_reset_physical_punches(void) {
    for (uint32_t hand = 0;
         hand < VR_CONTROLLER_COUNT;
         hand++) {
        controller_vr_reset_physical_punch(hand);
    }
}

static void controller_vr_update_rawkey_state(
    bool leftAvailable,
    const struct VrControllerState* left,
    bool rightAvailable,
    const struct VrControllerState* right
) {
    for (unsigned int binding = VR_CONTROLLER_BINDING_LEFT_PRIMARY;
         binding < VR_CONTROLLER_BINDING_COUNT;
         binding++) {
        const bool down = controller_vr_binding_down(
            binding,
            leftAvailable,
            left,
            rightAvailable,
            right
        );
        if (down && !sVrPreviousBindingDown[binding] &&
            sVrPendingRawKey == VK_INVALID) {
            // controller_get_raw_key() adds VK_BASE_VR after this callback;
            // return the logical enum value so the UI maps it to the same
            // choices used by gameplay.
            sVrPendingRawKey = binding;
        }
        sVrPreviousBindingDown[binding] = down;
    }
}

static void controller_vr_init(void) {
    vr_jump_gesture_reset(&sJumpGesture);
    sWallJumpGesture = (struct VrWallJumpGesture){0};
    sLandingJumpGesture = (struct VrLandingJumpGesture){0};
}

static bool controller_vr_physical_jump(bool leftAvailable, const struct VrControllerState* left,
        bool rightAvailable, const struct VrControllerState* right) {
    struct MarioState* m = gMarioState;
    const uint32_t generation = vr_get_tracking_origin_generation();
#ifndef __ANDROID__
    static struct VrJumpMotionSample jumpMotion[2];
#endif
    if (generation != sJumpTrackingGeneration || sJumpUsedTriggers != configVrJumpUseTriggers) {
        sJumpTrackingGeneration = generation;
#ifndef __ANDROID__
        memset(jumpMotion, 0, sizeof(jumpMotion));
#endif
        for(unsigned i=0;i<2;i++)sDiveStroke[i]=(struct VrDiveStroke){0};
        sJumpUsedTriggers = configVrJumpUseTriggers;
        vr_jump_gesture_reset(&sJumpGesture);
        sWallJumpGesture=(struct VrWallJumpGesture){0};
        sLandingJumpGesture=(struct VrLandingJumpGesture){0};
        sPhysicalJumpChainDeadline=0;
        sPhysicalJumpWasGrounded=false;
        sWallJumpCommand=0;
        return false;
    }
    float head[3];
    const bool allowed = configVrPhysicalJumping && !gInteractableOverridePad &&
        !gDjuiInMainMenu && gMenuMode == -1 && m && m->controller && m->marioObj && m->area &&
        m->health > 0x100 && !m->heldObj &&
        !(m->action & (ACT_FLAG_SWIMMING | ACT_FLAG_INTANGIBLE | ACT_FLAG_ON_POLE)) &&
        (m->action & ACT_GROUP_MASK) != ACT_GROUP_CUTSCENE &&
        (m->action & ACT_GROUP_MASK) != ACT_GROUP_SUBMERGED &&
        (m->action & ACT_GROUP_MASK) != ACT_GROUP_AUTOMATIC &&
        !vr_hand_interaction_is_physical_climb_active(m) && vr_get_head_translation(head);
    bool grounded = allowed && m->floor && !(m->action & ACT_FLAG_AIR) &&
        fabsf(m->pos[1] - m->floorHeight) <= 5.0f &&
        ((m->action & ACT_GROUP_MASK) == ACT_GROUP_STATIONARY ||
         (m->action & ACT_GROUP_MASK) == ACT_GROUP_MOVING);
    struct VrJumpSample samples[2] = {0};
    const struct VrControllerState* states[2] = {left, right};
    const bool available[2] = {leftAvailable, rightAvailable};
    const double now=clock_elapsed_f64();
    for (unsigned i = 0; i < 2 && allowed; ++i) {
        const struct VrControllerState* s = states[i];
        samples[i] = (struct VrJumpSample){
            .valid = available[i] && s->gripPoseValid && s->gripLinearVelocityValid &&
                     !vr_is_controller_holding_cap(i),
            .held = (configVrJumpUseTriggers ? s->trigger : s->squeeze) >= VR_TRIGGER_THRESHOLD,
            .y = s->gripPosition[1] - head[1], .upSpeed = s->gripLinearVelocity[1],
            .lateralSpeedSquared = s->gripLinearVelocity[0] * s->gripLinearVelocity[0] +
                                   s->gripLinearVelocity[2] * s->gripLinearVelocity[2]
        };
#ifndef __ANDROID__
        // Streaming runtimes can report a delayed/zero velocity at the stroke
        // peak. Measure the same positions the gesture observes each tick.
        const bool poseValid=available[i] && s->gripPoseValid &&
            !vr_is_controller_holding_cap(i);
        const bool measured=vr_jump_pose_velocity(&jumpMotion[i],
            s->gripPosition,poseValid,now,&samples[i]);
        samples[i].valid=poseValid && measured;
#endif
    }
#ifndef __ANDROID__
    if (!allowed) memset(jumpMotion, 0, sizeof(jumpMotion));
#endif
    if (allowed && !grounded && sPhysicalJumpWasGrounded) {
        // Carry the hand's rearm state across takeoff. Continuing the same
        // upward swing must not become the "another gesture" propeller burst.
        sWallJumpGesture=(struct VrWallJumpGesture){0};
        sLandingJumpGesture=(struct VrLandingJumpGesture){0};
        for (unsigned i=0; i<2; ++i) {
            sWallJumpGesture.stroke.hand[i]=sJumpGesture.hand[i];
            sLandingJumpGesture.stroke.hand[i]=sJumpGesture.hand[i];
        }
    }
    sPhysicalJumpWasGrounded=grounded;
    const bool wallWindow=allowed &&
        ((m->action==ACT_AIR_HIT_WALL && m->actionTimer<2) ||
         (m->wallKickTimer!=0 && m->prevAction==ACT_AIR_HIT_WALL &&
          (m->action==ACT_SOFT_BONK || m->action==ACT_BACKWARD_AIR_KB ||
           m->action==ACT_FORWARD_AIR_KB)));
    sWallJumpCommand=vr_wall_jump_update(&sWallJumpGesture,samples,allowed,
        allowed && (m->action & ACT_FLAG_AIR)!=0,
        wallWindow || (allowed && vr_special_moves_propeller_can_burst(m)),
        allowed && (m->controller->buttonDown & A_BUTTON)!=0,now);
    bool jump=vr_jump_gesture_update(&sJumpGesture,samples,allowed,grounded,now);
    int landing = vr_landing_jump_update(&sLandingJumpGesture, samples, allowed, grounded,
        allowed && (m->action & ACT_FLAG_AIR) && m->vel[1] < 0.0f,
        allowed && (m->controller->buttonDown & A_BUTTON), now);
    sJumpGesture.reservedHands |= sWallJumpGesture.stroke.reservedHands;
    sJumpGesture.reservedHands |= sLandingJumpGesture.stroke.reservedHands;
    if(sWallJumpCommand) {
        sJumpGesture.activeHand=0;
        if (sWallJumpCommand > 0) {
            // A wall kick needs A held during ascent, just like a ground jump.
            // Preserve the triggering fist(s), not merely the initial press.
            vr_jump_gesture_continue_airborne(
                &sJumpGesture, &sWallJumpGesture.stroke, now);
        }
        sLandingJumpGesture=(struct VrLandingJumpGesture){0};
        return sWallJumpCommand>0;
    }
    if (landing) {
        sWallJumpCommand = landing; // Same one-frame release/press edge contract.
        sJumpGesture.activeHand = 0;
        if (landing > 0) {
            for (unsigned i=0; i<2; ++i) sJumpGesture.hand[i]=sLandingJumpGesture.stroke.hand[i];
            sJumpGesture.activeHand=sLandingJumpGesture.hands;
            sJumpGesture.jumpTime=now;
            sJumpGesture.airborne=false;
        }
        jump=landing>0;
    }
    // Only physical gestures get two additional simulation ticks for a late
    // double/triple chain. Native action/speed/steep-floor rules still apply.
    if (!grounded) sPhysicalJumpChainDeadline=0;
    else if (m->doubleJumpTimer > 0) sPhysicalJumpChainDeadline=now + 0.067;
    else if (jump && now < sPhysicalJumpChainDeadline) {
        m->doubleJumpTimer=2;
        sPhysicalJumpChainDeadline=0;
    }
    return jump;
}

static void controller_vr_read(OSContPad* pad) {
    sWallJumpCommand=0;
    if (!vr_is_active() || pad == NULL) {
        vr_jump_gesture_reset(&sJumpGesture);
        sWallJumpGesture = (struct VrWallJumpGesture){0};
        sLandingJumpGesture = (struct VrLandingJumpGesture){0};
        controller_vr_reset_physical_punches();
        sVrPhysicalCrouchActive = false;
        sVrPhysicalCrouchReferenceValid = false;
        controller_vr_reset_rawkey_state();
        return;
    }

    if (controller_vr_update_physical_crouch()) {
        pad->button |= Z_TRIG;
    }

    if (!configVrMotionControllerInput) {
        vr_jump_gesture_reset(&sJumpGesture);
        sWallJumpGesture = (struct VrWallJumpGesture){0};
        sLandingJumpGesture = (struct VrLandingJumpGesture){0};
        controller_vr_reset_physical_punches();
        controller_vr_reset_rawkey_state();
        return;
    }

    struct VrControllerState left = { 0 };
    struct VrControllerState right = { 0 };
    const bool leftAvailable = vr_get_controller_state(
        VR_CONTROLLER_LEFT,
        &left
    );
    const bool rightAvailable = vr_get_controller_state(
        VR_CONTROLLER_RIGHT,
        &right
    );

    const bool splitDown = controller_vr_binding_down(configVrSplitBinding,
        leftAvailable, &left, rightAvailable, &right);
    if (configVrSpeedrunHud && !gInteractableOverridePad && splitDown && !sVrSplitDown) {
        vr_speedrun_initialize(fs_get_write_path("speedrun/setup.json"), configVrSpeedrunSegments);
        vr_speedrun_split(vr_speedrun_now());
    }
    // Update while in menus too, preventing the binding-capture press from
    // starting a run when the menu closes with the button still held.
    sVrSplitDown = splitDown;
    controller_vr_update_rawkey_state(
        leftAvailable,
        &left,
        rightAvailable,
        &right
    );

    const bool physicalJump = controller_vr_physical_jump(leftAvailable, &left, rightAvailable, &right);
    if (physicalJump) pad->button |= A_BUTTON;
    const bool jumpPriority=physicalJump || sJumpGesture.reservedHands!=0;
    vr_set_jump_gesture_priority(jumpPriority);
    float diveHead[3], diveRotation[4];
    bool diveTracking=vr_get_head_translation(diveHead) && vr_get_head_rotation(diveRotation);
    float diveForward[2]={0,-1};
    if(diveTracking) {
        diveForward[0]=-2.0f*(diveRotation[0]*diveRotation[2]+diveRotation[3]*diveRotation[1]);
        diveForward[1]=-(1.0f-2.0f*(diveRotation[0]*diveRotation[0]+diveRotation[1]*diveRotation[1]));
        float length=hypotf(diveForward[0],diveForward[1]);
        if(length<0.2f)diveTracking=false;
        else {diveForward[0]/=length;diveForward[1]/=length;}
    }
    for(unsigned hand=0;hand<2;hand++) {
        const struct VrControllerState* state=hand==0?&left:&right;
        bool available=hand==0?leftAvailable:rightAvailable;
        bool airborne=(gMarioStates[0].action & ACT_FLAG_AIR)!=0;
        bool allowed=diveTracking && available && state->gripPoseValid && state->gripLinearVelocityValid &&
            !jumpPriority && !gInteractableOverridePad && !gDjuiInMainMenu && gMenuMode==-1 &&
            configVrCameraMode==VR_CAMERA_MODE_FIRST_PERSON && configVrPhysicalPunching &&
            (airborne?configVrMotionControlledDive:configVrMotionControlledGroundDive) &&
            state->squeeze >= controller_vr_clampf((float)configVrPunchGripThreshold,10,100)/100.0f;
        float p[3]={0};
        if(!allowed)vr_consume_motion_dive(hand);
        if(allowed)for(int axis=0;axis<3;axis++)p[axis]=state->gripPosition[axis]-diveHead[axis];
        if(vr_dive_stroke_update(&sDiveStroke[hand],p,state->gripLinearVelocity,diveForward,allowed,airborne))
            vr_queue_motion_dive(hand);
    }

    if (!leftAvailable && !rightAvailable) {
        controller_vr_reset_physical_punches();
        return;
    }

    if (!leftAvailable) {
        controller_vr_reset_physical_punch(VR_CONTROLLER_LEFT);
    }
    if (!rightAvailable) {
        controller_vr_reset_physical_punch(VR_CONTROLLER_RIGHT);
    }

    if (configVrCameraMode == VR_CAMERA_MODE_FIRST_PERSON &&
        configVrPhysicalPunching) {
        if (leftAvailable && !jumpPriority) {
            if (controller_vr_update_physical_punch(
                    VR_CONTROLLER_LEFT,
                    &left
                )) {
                vr_queue_physical_punch(VR_CONTROLLER_LEFT, &left);
            }
        } else {
            controller_vr_reset_physical_punch(VR_CONTROLLER_LEFT);
        }
        if (rightAvailable && !jumpPriority) {
            if (controller_vr_update_physical_punch(
                    VR_CONTROLLER_RIGHT,
                    &right
                )) {
                vr_queue_physical_punch(VR_CONTROLLER_RIGHT, &right);
            }
        } else {
            controller_vr_reset_physical_punch(VR_CONTROLLER_RIGHT);
        }
    } else {
        controller_vr_reset_physical_punches();
    }

    const float* movementStick = controller_vr_select_stick(
        configVrMoveStick,
        leftAvailable,
        &left,
        rightAvailable,
        &right
    );
    if (movementStick != NULL) {
        controller_vr_merge_stick(
            &pad->stick_x,
            &pad->stick_y,
            movementStick
        );
    }

    if (controller_vr_binding_down(
            configVrJumpBinding,
            leftAvailable,
            &left,
            rightAvailable,
            &right
        )) {
        pad->button |= A_BUTTON;
    }

    if (gInteractableOverridePad &&
        rightAvailable &&
        right.primaryButton) {
        pad->button |= A_BUTTON;
    }
    if (controller_vr_binding_down(
            configVrAttackBinding,
            leftAvailable,
            &left,
            rightAvailable,
            &right
        )) {
        pad->button |= B_BUTTON;
    }
    if (controller_vr_binding_down(
            configVrCrouchBinding,
            leftAvailable,
            &left,
            rightAvailable,
            &right
        )) {
        pad->button |= Z_TRIG;
    }
    if (controller_vr_binding_down(
            configVrLBinding,
            leftAvailable,
            &left,
            rightAvailable,
            &right
        )) {
        pad->button |= L_TRIG;
    }
    if (controller_vr_binding_down(
            configVrRBinding,
            leftAvailable,
            &left,
            rightAvailable,
            &right
        )) {
        pad->button |= R_TRIG;
    }
    if (controller_vr_binding_down(
            configVrPauseBinding,
            leftAvailable,
            &left,
            rightAvailable,
            &right
        )) {
        pad->button |= START_BUTTON;
    }
    if (controller_vr_binding_down(
            configVrSpecialBinding,
            leftAvailable,
            &left,
            rightAvailable,
            &right
        )) {
        pad->button |= Y_BUTTON;
    }

    const float* cameraStick = controller_vr_select_stick(
        configVrCameraStick,
        leftAvailable,
        &left,
        rightAvailable,
        &right
    );
    if (cameraStick != NULL) {
#ifdef __ANDROID__
        // OpenXR Touch X and the port's camera yaw use opposite signs on
        // Quest. Invert only the selected camera stick, leaving locomotion
        // and user stick remapping unchanged.
        const float questCameraStick[2] = {
            -cameraStick[0],
            cameraStick[1]
        };
        cameraStick = questCameraStick;
#endif
        controller_vr_merge_stick(
            &pad->ext_stick_x,
            &pad->ext_stick_y,
            cameraStick
        );

        if (cameraStick[0] <=
            -VR_CAMERA_BUTTON_THRESHOLD) {
            pad->button |= L_CBUTTONS;
        } else if (cameraStick[0] >=
                   VR_CAMERA_BUTTON_THRESHOLD) {
            pad->button |= R_CBUTTONS;
        }
        if (cameraStick[1] >=
            VR_CAMERA_BUTTON_THRESHOLD) {
            pad->button |= U_CBUTTONS;
        } else if (cameraStick[1] <=
                   -VR_CAMERA_BUTTON_THRESHOLD) {
            pad->button |= D_CBUTTONS;
        }
    }

    // A wall kick needs a new press, including when the previous gesture held A.
    if(sWallJumpCommand<0) pad->button &= ~A_BUTTON;
}

static u32 controller_vr_rawkey(void) {
    const u32 key = sVrPendingRawKey;
    sVrPendingRawKey = VK_INVALID;
    return key;
}

static void controller_vr_rumble_play(
    float strength,
    float durationSeconds
) {
    if (!vr_is_active() || !configVrMotionControllerInput) {
        return;
    }

    vr_apply_haptic(
        VR_CONTROLLER_LEFT,
        strength,
        durationSeconds,
        -1.0f
    );
    vr_apply_haptic(
        VR_CONTROLLER_RIGHT,
        strength,
        durationSeconds,
        -1.0f
    );
}

static void controller_vr_rumble_stop(void) {
    vr_apply_haptic(VR_CONTROLLER_LEFT, 0.0f, 0.0f, -1.0f);
    vr_apply_haptic(VR_CONTROLLER_RIGHT, 0.0f, 0.0f, -1.0f);
}

static void controller_vr_shutdown(void) {
}

struct ControllerAPI controller_vr = {
    VK_BASE_VR,
    controller_vr_init,
    controller_vr_read,
    controller_vr_rawkey,
    controller_vr_rumble_play,
    controller_vr_rumble_stop,
    NULL,
    controller_vr_shutdown
};
