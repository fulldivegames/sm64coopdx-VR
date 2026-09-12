static void vr_special_moves_update_propeller_pickup(
    struct MarioState* mario
) {
    struct Object* pickup = sVrPropellerPickupObject;
    if (pickup == NULL || mario == NULL || mario->marioObj == NULL) {
        return;
    }
    if ((pickup->activeFlags & ACTIVE_FLAG_ACTIVE) == 0) {
        sVrPropellerPickupObject = NULL;
        sVrPropellerPickupAge = 0;
        return;
    }
    if (sVrPropellerPickupAge < 0xFFFFU) {
        sVrPropellerPickupAge++;
    }
    if (sVrPropellerPickupLanded) {
        struct Surface* support = NULL;
        const f32 supportHeight = find_floor(
            pickup->oPosX, pickup->oPosY + 80.0f,
            pickup->oPosZ, &support
        );
        const f32 targetHeight = supportHeight + VR_PROPELLER_PICKUP_GROUND_OFFSET;
        if (support == NULL ||
            fabsf(pickup->oPosY - targetHeight) > 2.0f) {
            sVrPropellerPickupLanded = false;
            sVrPropellerPickupVelocityY = 0.0f;
        } else {
            pickup->oPosY = targetHeight;
        }
    }
    if (!sVrPropellerPickupLanded) {
        pickup->oPosY += sVrPropellerPickupVelocityY;
        sVrPropellerPickupVelocityY = fmaxf(
            sVrPropellerPickupVelocityY - VR_PROPELLER_PICKUP_GRAVITY,
            -VR_PROPELLER_PICKUP_FALL_SPEED
        );
        struct Surface* floor = NULL;
        const f32 floorHeight = find_floor(
            pickup->oPosX, pickup->oPosY + 80.0f,
            pickup->oPosZ, &floor
        );
        if (sVrPropellerPickupVelocityY <= 0.0f && floor != NULL &&
            pickup->oPosY <= floorHeight + VR_PROPELLER_PICKUP_GROUND_OFFSET) {
            pickup->oPosY = floorHeight + VR_PROPELLER_PICKUP_GROUND_OFFSET;
            sVrPropellerPickupVelocityY = 0.0f;
            sVrPropellerPickupLanded = true;
        }
    }
    pickup->oFaceAngleYaw += 0x300;
    obj_update_gfx_pos_and_angle(pickup);
    if (sVrPropellerPickupAge < VR_FIRE_FLOWER_PICKUP_GRACE_FRAMES) {
        return;
    }

    const f32 px = pickup->oPosX;
    const f32 py = pickup->oPosY - VR_PROPELLER_PICKUP_GROUND_OFFSET;
    const f32 pz = pickup->oPosZ;
    const f32 bodyDx = px - mario->marioObj->oPosX;
    const f32 bodyDy = py - (mario->marioObj->oPosY + 45.0f);
    const f32 bodyDz = pz - mario->marioObj->oPosZ;
    bool collected = bodyDx * bodyDx + bodyDy * bodyDy + bodyDz * bodyDz <=
        VR_FIRE_FLOWER_PICKUP_RADIUS * VR_FIRE_FLOWER_PICKUP_RADIUS;
    u32 collectingHand = VR_CONTROLLER_COUNT;
    Vec3f headsetPosition;
    if (!collected && vr_get_stabilized_headset_world_position(
            headsetPosition, false
        )) {
        const f32 dx = px - headsetPosition[0];
        const f32 dy = py - headsetPosition[1];
        const f32 dz = pz - headsetPosition[2];
        collected = dx * dx + dy * dy + dz * dz <=
            VR_FIRE_FLOWER_HEAD_PICKUP_RADIUS *
                VR_FIRE_FLOWER_HEAD_PICKUP_RADIUS;
    }
    for (u32 hand = 0;
         !collected && hand < VR_CONTROLLER_COUNT;
         hand++) {
        struct VrControllerState state = { 0 };
        Vec3f handPosition;
        Vec3f handVelocity;
        if (!vr_get_controller_state(hand, &state) ||
            !vr_get_controller_world_fist_reach_target_from_state(
                hand, &state, handPosition, handVelocity
            )) {
            continue;
        }
        const f32 dx = px - handPosition[0];
        const f32 dy = py - handPosition[1];
        const f32 dz = pz - handPosition[2];
        if (dx * dx + dy * dy + dz * dz <=
            VR_FIRE_FLOWER_HAND_PICKUP_RADIUS *
                VR_FIRE_FLOWER_HAND_PICKUP_RADIUS) {
            collected = true;
            collectingHand = hand;
        }
    }
    if (collected && vr_special_moves_grant_propeller()) {
        if (collectingHand < VR_CONTROLLER_COUNT) {
            vr_apply_haptic(collectingHand, 0.45f, 0.10f, -1.0f);
        } else {
            vr_apply_haptic(VR_CONTROLLER_LEFT, 0.30f, 0.08f, -1.0f);
            vr_apply_haptic(VR_CONTROLLER_RIGHT, 0.30f, 0.08f, -1.0f);
        }
        vr_special_moves_delete_object(&sVrPropellerPickupObject);
        sVrPropellerPickupAge = 0;
    }
}
