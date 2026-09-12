#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "sm64.h"
#include "game/mario.h"
#include "game/interaction.h"
static bool sVrHeadsetColliderActive;
static struct Object* sVrHeadsetColliderObject;
static float sVrHeadsetColliderSavedRadius, sVrHeadsetColliderSavedHeight, sVrHeadsetColliderSavedDownOffset;
#include "../build/test_vr_headset_contacts.inc"
int main(void) {
    struct Object player={0}, target={0};
    struct MarioState m={0}; m.marioObj=&player;
    player.hitboxRadius=37; player.hitboxHeight=160;
    Vec3f head={100,300,100};
    target.oPosX=100; target.oPosY=280; target.oPosZ=100;
    target.hitboxRadius=20; target.hitboxHeight=40;
    assert(!detect_object_hitbox_overlap(&player,&target));
    vr_hand_interaction_apply_headset_collider(&m,head);
    assert(detect_object_hitbox_overlap(&player,&target));
    assert(target.collidedObjs[0]==&player && player.collidedObjs[0]==&target);
    assert(m.pos[0]==0 && m.pos[1]==0 && m.pos[2]==0);
    player.numCollidedObjs=target.numCollidedObjs=0;
    head[0]=500;
    vr_hand_interaction_apply_headset_collider(&m,head);
    assert(!detect_object_hitbox_overlap(&player,&target));
    assert(sVrHeadsetColliderSavedRadius==37 && sVrHeadsetColliderSavedHeight==160);
    puts("PASS: actual headset collider/contact functions, current head overlap, movement-away and unchanged body physics");
}
