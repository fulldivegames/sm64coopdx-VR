#include <assert.h>
#include <stdio.h>
#include "../src/game/vr_propeller_stomp.h"
int main(void) {
    assert(vr_propeller_top_contact(0,0,78,-10,0,80,50));
    assert(vr_propeller_top_contact(20,0,25,-60,0,80,50));
    assert(!vr_propeller_top_contact(51,0,78,-10,0,80,50)); // side edge
    assert(!vr_propeller_top_contact(0,0,20,-10,0,80,50)); // side height
    assert(!vr_propeller_top_contact(0,0,78,10,0,80,50)); // rising
    assert(!vr_propeller_top_contact(0,0,-1,-60,0,80,50)); // below enemy
    assert(!vr_propeller_top_contact(0,0,78,-10,0,80,0));
    assert(vr_propeller_top_contact(0,0,0,-60,0,60,70)); // crosses full Shy Guy height
    assert(!vr_propeller_top_contact(0,0,-1,-60,0,60,70)); // did not cross top
    puts("PASS: top stomp/glide/drill geometry; no side, rising or underside rebound");
}
