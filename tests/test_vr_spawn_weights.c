#include <assert.h>
#include <stdio.h>
#include "../src/game/vr_spawn_weights.h"
static void check(const unsigned* weights, unsigned count) {
    unsigned total=0, hits[7]={0};
    for(unsigned i=0;i<count;i++) total+=weights[i];
    unsigned limit=65536U-65536U%total;
    for(unsigned random=0;random<limit;random++) {
        unsigned selected=vr_spawn_weight_pick(weights,count,random%total);
        assert(selected<count && weights[selected]>0);
        hits[selected]++;
    }
    for(unsigned i=0;i<count;i++) assert(hits[i]==weights[i]*(limit/total));
}
int main(void) {
    assert(vr_spawn_weight(0)==1 && vr_spawn_weight(101)==100);
    assert(vr_spawn_weight(~0U)==100 && vr_spawn_weight(5)==5);
    for(unsigned weight=1;weight<=100;weight++) {
        assert(vr_spawn_effective_weight(weight,false)==2*vr_spawn_effective_weight(weight,true));
        assert(vr_spawn_effective_weight(weight,true)>0);
    }
    // Every combination of disabled pickups: zero-weight entries never win.
    for(unsigned mask=0;mask<64;mask++) {
        unsigned weights[7]={vr_spawn_effective_weight(50,false),0,0,0,0,0,0};
        for(unsigned i=1;i<7;i++) if(mask & (1U<<(i-1))) weights[i]=vr_spawn_effective_weight(50,i==4 || i==6);
        check(weights,7);
    }
    unsigned defaults[7]={100,100,100,100,50,100,50}; check(defaults,7);
    unsigned rare[7]={100,100,100,100,1,100,1}; check(rare,7);
    unsigned maximum[7]={100,200,200,200,100,200,100}; check(maximum,7);
    // Exhausted pool fallback removes only the failed pickup, never original.
    for(unsigned i=1;i<7;i++) {maximum[i]=0;check(maximum,7);}
    puts("PASS: exact half-weight category at every slider value, all 64 enable combinations, rare/max weights, unavailable pools, bounds");
}
