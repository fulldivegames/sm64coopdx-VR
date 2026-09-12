#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "engine/math_util.h"
#include "pc/vr/vr_tracking_math.h"
// Include the real Quest bridge, not a second implementation of recentering.
#include "../platform/android/app/src/main/cpp/quest_vr_bridge.c"
unsigned int configVrCameraMode = VR_CAMERA_MODE_THIRD_PERSON;
struct Surface;
f32 find_floor(f32 x,f32 y,f32 z,struct Surface **floor) {
    (void)x;(void)y;(void)z;(void)floor; assert(!"unexpected floor query"); return 0;
}
static void check_pose(float yaw,float pitch) {
    configVrCameraMode = VR_CAMERA_MODE_THIRD_PERSON;
    float sy=sinf(yaw/2),cy=cosf(yaw/2),sp=sinf(pitch/2),cp=cosf(pitch/2);
    float q[4]={cy*sp,sy*cp,-sy*sp,cy*cp}; // yaw * pitch
    float reference[4]={0,0,0,1}; assert(vr_tracking_yaw_reference(q,reference));
    assert(fabsf(reference[1]*reference[1]+reference[3]*reference[3]-1)<.00001f);
    float positions[2][3]={{1,1.7f,2},{1,1.7f,2}}, rotations[2][4];
    memcpy(rotations[0],q,sizeof(q)); memcpy(rotations[1],q,sizeof(q));
    const float fov[2][4]={{-1,1,-1,1},{-1,1,-1,1}};
    const uint32_t size[2]={1024,1024};
    for(int repeat=0;repeat<3;repeat++) {
        if(repeat==1) vr_openxr_request_horizontal_recenter();
        else vr_openxr_request_recenter();
        quest_vr_bridge_update_views(positions,rotations,fov,size,size);
        float head[4]; assert(vr_openxr_get_head_rotation(head));
        // Neutral yaw is exactly zero; physical pitch is intentionally kept.
        assert(fabsf(head[1])<.00001f && fabsf(head[2])<.00001f);
        assert(fabsf(fabsf(head[0])-fabsf(sp))<.00001f);
        float forward[3]={0,0,-1}; rotate_vector(head,forward,forward);
        assert(fabsf(forward[0])<.00001f && forward[2]<0);
        struct VrControllerState hand={.gripPoseValid=true,.gripLinearVelocityValid=true};
        memcpy(hand.gripRotation,q,sizeof(q));
        hand.gripPosition[0]=positions[0][0]-.4f*sinf(yaw);
        hand.gripPosition[1]=positions[0][1];
        hand.gripPosition[2]=positions[0][2]-.4f*cosf(yaw);
        hand.gripLinearVelocity[0]=-sinf(yaw); hand.gripLinearVelocity[2]=-cosf(yaw);
        quest_vr_bridge_update_controller(0,&hand,true);
        assert(vr_openxr_get_controller_state(0,&hand));
        assert(fabsf(hand.gripPosition[0])<.00001f && fabsf(hand.gripPosition[2]+.4f)<.00001f);
        assert(fabsf(hand.gripLinearVelocity[0])<.00001f && fabsf(hand.gripLinearVelocity[2]+1)<.00001f);
    }
    // Switching back must reproduce the unmodified first-person reference,
    // including its calibrated height and controller coordinate transform.
    configVrCameraMode = VR_CAMERA_MODE_FIRST_PERSON;
    float heightBefore; assert(vr_openxr_get_calibrated_head_height(&heightBefore));
    positions[0][1] = positions[1][1] = 1.1f;
    vr_openxr_request_horizontal_recenter();
    quest_vr_bridge_update_views(positions,rotations,fov,size,size);
    float legacyAngle = atan2f(2*(q[3]*q[1]+q[0]*q[2]),1-2*(q[0]*q[0]+q[1]*q[1]));
    float legacyInverse[4]={0,-sinf(legacyAngle*.5f),0,cosf(legacyAngle*.5f)};
    for(int i=0;i<4;i++) assert(sReferenceRotation[i]==legacyInverse[i]);
    float heightAfter; assert(vr_openxr_get_calibrated_head_height(&heightAfter));
    assert(heightAfter==heightBefore);
    struct VrControllerState original={.gripPoseValid=true,.gripLinearVelocityValid=true},actual;
    memcpy(original.gripRotation,q,sizeof(q));
    original.gripPosition[0]=1.2f; original.gripPosition[1]=1.4f; original.gripPosition[2]=1.6f;
    original.gripLinearVelocity[0]=.2f; original.gripLinearVelocity[1]=.3f; original.gripLinearVelocity[2]=-.4f;
    quest_vr_bridge_update_controller(0,&original,true);
    assert(vr_openxr_get_controller_state(0,&actual));
    float expected[3]; rotate_vector(legacyInverse,original.gripLinearVelocity,expected);
    for(int i=0;i<3;i++) assert(actual.gripLinearVelocity[i]==expected[i]);
    // A runtime changes LOCAL coordinates at changeTime, not at event receipt.
    configVrCameraMode=VR_CAMERA_MODE_THIRD_PERSON;
    vr_openxr_request_horizontal_recenter();
    quest_vr_bridge_update_views(positions,rotations,fov,size,size);
    uint32_t generation=sTrackingOriginGeneration;
    quest_vr_bridge_reference_space_change(1000);
    quest_vr_bridge_prepare_tracking(999);
    assert(sReferenceValid && sTrackingOriginGeneration==generation);
    // Typical runtime recenter yields identity yaw regardless of old heading.
    rotations[0][0]=rotations[0][1]=rotations[0][2]=0; rotations[0][3]=1;
    memcpy(rotations[1],rotations[0],sizeof(q));
    quest_vr_bridge_prepare_tracking(1000);
    quest_vr_bridge_update_views(positions,rotations,fov,size,size);
    assert(sTrackingOriginGeneration==generation+1);
    float centered[4]; assert(vr_openxr_get_head_rotation(centered));
    assert(centered[0]==0 && centered[1]==0 && centered[2]==0 && centered[3]==1);
    quest_vr_bridge_prepare_tracking(1001);
    assert(sTrackingOriginGeneration==generation+1);
    configVrCameraMode=VR_CAMERA_MODE_FIRST_PERSON;
    vr_openxr_request_horizontal_recenter();
    quest_vr_bridge_update_views(positions,rotations,fov,size,size);
    float preserved[4]; memcpy(preserved,sReferenceRotation,sizeof(preserved));
    generation=sTrackingOriginGeneration;
    quest_vr_bridge_reference_space_change(2000);
    quest_vr_bridge_prepare_tracking(2000);
    quest_vr_bridge_update_views(positions,rotations,fov,size,size);
    assert(sTrackingOriginGeneration==generation);
    assert(memcmp(preserved,sReferenceRotation,sizeof(preserved))==0);
}
int main(void) {
    // Force the same game-exported atan2f symbol used by the real executable.
    float (*volatile gameAngle)(float,float)=atan2f;
    float broken=gameAngle(0,1);
    assert(fabsf(broken-1.5707963f)<.0001f);
    printf("Reproduced old neutral-heading error: %.1f degrees\n",broken*57.2957795f);
    for(int yaw=-180;yaw<=180;yaw+=5)
        for(int pitch=-60;pitch<=60;pitch+=15)
            check_pose(yaw*.01745329252f,pitch*.01745329252f);
    float out[4]={0,0,0,1},bad[4]={NAN,0,0,1};
    assert(!vr_tracking_yaw_reference(bad,out) && out[3]==1);
    puts("PASS: 1971 entry/app recenter poses; 657 timed runtime-origin changes center third person and preserve first-person behavior; height and hand alignment retained");
}
