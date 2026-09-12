// Included after geo_append_display_list: accessories are geometry owned by
// Mario, not separately simulated objects. Both matrix endpoints and the late
// first-person root correction therefore match the body at every render frame.
static bool sVrPropellerHeadAttached;

static bool vr_local_body_accessories(void) {
    return vr_is_active() && !sVrCharacterMenuScene &&
        // GRAPH_RENDER_PLAYER is only set for animation-offset characters.
        // Native Mario has a body state and object identity but not that flag.
        gCurMarioBodyState != NULL && gCurGraphNodeHeldObject == NULL &&
        gCurGraphNodeProcessingObject == gMarioStates[0].marioObj;
}

static void vr_append_body_accessory(const Gfx* displayList, Mat4 local, Mat4 previous) {
    if (gMatStackIndex + 1 >= MATRIX_STACK_SIZE) return;
    mtxf_mul(gMatStack[gMatStackIndex + 1], local, gMatStack[gMatStackIndex]);
    mtxf_mul(gMatStackPrev[gMatStackIndex + 1], previous, gMatStackPrev[gMatStackIndex]);
    if (!increment_mat_stack()) return;
    geo_append_display_list((void*)displayList, LAYER_OPAQUE);
    --gMatStackIndex;
}

static void vr_append_propeller_helmet(void) {
    if (!vr_local_body_accessories() ||
        (!vr_special_moves_propeller_active() && !vr_special_moves_hammer_suit_active()) ||
        sVrPropellerHeadAttached) return;
    sVrPropellerHeadAttached = true;
    // Use precisely the same visibility rule as the head, including hidden
    // first-person skeleton traversal. The helmet cannot fill the wearer's view.
    if (vr_hide_local_first_person_mario_part()) return;
    if (sVrPaintingExitHatHand < VR_CONTROLLER_COUNT && sVrHeldHelmet != 0) return;
    if (vr_special_moves_hammer_suit_active()) {
        geo_append_display_list((void*)vr_hammer_helmet_dl, LAYER_OPAQUE);
        return;
    }
    geo_append_display_list((void*)vr_propeller_helmet_dl, LAYER_OPAQUE);
    Mat4 local, previous;
    Vec3f hub = {318.0f, -12.0f, 0.0f};
    // Rotor rotation is cosmetic and uses matching simulation endpoints. Its
    // matrix is interpolated by the ordinary renderer, not a separate object.
    Vec3s angle = {(s16)(gGlobalTimer * 0x1000U), 0, 0};
    Vec3s oldAngle = {(s16)((gGlobalTimer - 1) * 0x1000U), 0, 0};
    mtxf_rotate_xyz_and_translate(local, hub, angle);
    mtxf_rotate_xyz_and_translate(previous, hub, oldAngle);
    vr_append_body_accessory(vr_propeller_rotor_dl, local, previous);
}

static void vr_append_hammer_back_shell(void) {
    // A separate torso attachment survives native near/medium/far LOD switches.
    if (vr_local_body_accessories() && vr_special_moves_propeller_active() &&
        gCurMarioBodyState->currAnimPart == MARIO_ANIM_PART_TORSO &&
        !vr_hide_local_first_person_mario_part()) {
        geo_append_display_list((void*)mario_propeller_zipper_dl, LAYER_OPAQUE);
    }
    if (!vr_local_body_accessories() || !vr_special_moves_hammer_suit_active() ||
        gCurMarioBodyState->currAnimPart != MARIO_ANIM_PART_TORSO ||
        vr_hide_local_first_person_mario_part()) return;
    Mat4 mount;
    mtxf_identity(mount);
    mount[0][0]=0; mount[0][2]=2;
    mount[1][1]=-1.2f;
    mount[2][0]=2; mount[2][2]=0;
    mount[3][0]=15; mount[3][1]=-68;
    vr_append_body_accessory(vr_hammer_shell_dl, mount, mount);
}

static bool vr_is_head_rotation_node(struct GraphNodeRotation* node) {
    struct GraphNode* previous = node->node.prev;
    return vr_local_body_accessories() &&
        gCurMarioBodyState->currAnimPart == MARIO_ANIM_PART_HEAD &&
        previous != NULL && previous->type == GRAPH_NODE_TYPE_GENERATED_LIST &&
        ((struct GraphNodeGenerated*)previous)->fnNode.func == (GraphNodeFunc)geo_mario_head_rotation;
}
