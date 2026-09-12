#include <assert.h>
#include <stdio.h>
#include "../src/game/vr_swim_stroke.h"
struct Fixture { struct VrSwimStroke stroke; struct VrSwimTracking tracking; };
static float sample(struct Fixture* f,float x,float y,float z) {
    struct VrSwimSample h={.valid=true,.position={x,y,z},.forward={0,0,-1}};
    h.valid=vr_swim_tracking_delta(&f->tracking,true,h.position,h.movement);
    for(int i=0;i<3;i++) h.worldMovement[i]=h.movement[i];
    return vr_swim_stroke_update(&f->stroke,&h);
}
int main(void) {
    struct Fixture f={0};
    // Realistic shoulder offsets and repeated recovery/power phases. There is
    // no dependency on optional instantaneous runtime velocity samples.
    for(int cycle=0;cycle<20;cycle++) {
        for(int i=0;i<=10;i++) assert(sample(&f,.3f,-.25f,-.1f-i*.04f)==0);
        float total=0;
        for(int i=1;i<=10;i++) total+=sample(&f,.3f,-.25f,-.5f+i*.04f);
        assert(total>.25f && f.stroke.direction[2]==-1 && f.stroke.direction[1]==0);
    }
    float totals[3]={0}; const int counts[3]={5,20,100};
    for(int rate=0;rate<3;rate++) {
        f=(struct Fixture){0}; sample(&f,0,0,-.6f);
        for(int i=1;i<=counts[rate];i++) totals[rate]+=sample(&f,0,0,-.6f+.4f*i/counts[rate]);
        assert(totals[rate]>.3f);
    }
    assert(fabsf(totals[0]-totals[1])<.001f && fabsf(totals[1]-totals[2])<.001f);
    const float axes[][3]={{0,1,0},{0,0,-1},{0,.4f,-.916515f},{0,-.4f,-.916515f}};
    for(int a=0;a<4;a++) {
        f=(struct Fixture){0}; const float* d=axes[a];
        assert(sample(&f,d[0]*.5f,d[1]*.5f,d[2]*.5f)==0);
        float total=0;
        for(int i=1;i<=5;i++) {
            float r=.5f-i*.08f;
            total+=sample(&f,d[0]*r,d[1]*r,d[2]*r);
        }
        assert(total>.25f);
        for(int i=0;i<3;i++) assert(fabsf(f.stroke.direction[i]-d[i])<.0001f);
    }
    f=(struct Fixture){0};
    assert(sample(&f,0,.4f,-.3f)==0);
    assert(sample(&f,0,.3f,-.3f)>0);
    assert(sample(&f,0,.22f,-.25f)>0);
    assert(sample(&f,0,.2f,-.15f)>0);
    assert(f.stroke.direction[2]<-.97f && f.stroke.direction[1]<.20f);
    f=(struct Fixture){0}; sample(&f,.4f,0,-.4f); sample(&f,.38f,0,-.38f);
    float curved=0;
    for(int i=1;i<=6;i++) curved+=sample(&f,.38f-.015f*i,0,-.38f+.025f*i);
    assert(curved>0);
    struct VrSwimStroke s={0};
    struct VrSwimSample h={.valid=true,.position={0,0,-.5f},.forward={0,0,-1}};
    assert(vr_swim_stroke_update(&s,&h)==0);
    h.position[2]=-.4f; h.movement[2]=.1f; h.worldMovement[0]=-.1f;
    assert(vr_swim_stroke_update(&s,&h)>0 && s.direction[0]==1);
    // Head motion alone changes relative position but supplies no raw travel.
    h.position[2]=-.3f; h.movement[2]=0; h.worldMovement[0]=0;
    assert(vr_swim_stroke_update(&s,&h)==0);
    h.movement[2]=4; h.worldMovement[2]=4;
    assert(vr_swim_stroke_update(&s,&h)==0 && !s.valid);
    h.valid=false; assert(vr_swim_stroke_update(&s,&h)==0 && !s.valid);
    h.valid=true; h.worldMovement[0]=NAN;
    assert(vr_swim_stroke_update(&s,&h)==0 && !s.valid);
    float delta[3]={1,1,1}, bad[3]={NAN,0,0};
    assert(!vr_swim_tracking_delta(&f.tracking,true,bad,delta));
    assert(delta[0]==0 && delta[1]==0 && delta[2]==0 && !f.tracking.valid);
    struct Fixture left={0},right={0}; sample(&left,0,0,-.5f); sample(&right,0,0,-.5f);
    assert(sample(&left,0,0,-.4f)>0 && sample(&right,0,0,-.4f)>0);
    assert(sample(&left,0,0,-.5f)==0 && sample(&right,0,0,-.3f)>0);
    // Hand already beside/behind the torso: the old radial gate rejected this.
    f=(struct Fixture){0}; sample(&f,.5f,-.5f,.05f);
    float rearPull=0;
    for(int i=1;i<=8;i++) rearPull+=sample(&f,.5f,-.5f,.05f+i*.02f);
    assert(rearPull>.1f);
    // Short recovery re-arms the next pull instead of keeping a spent stroke.
    assert(sample(&f,.5f,-.5f,.16f)==0);
    float repeated=0;
    for(int i=1;i<=8;i++) repeated+=sample(&f,.5f,-.5f,.16f+i*.02f);
    assert(repeated>.1f);
    puts("PASS: pose-derived swimming, slow/repeated/curved/3D strokes, sampling invariance, recovery, both hands, tracking safety");
}
