#include <assert.h>
#include <stdio.h>
#include "../src/pc/djui/djui_hud_bounds.h"
int main(void) {
    // B3313 b-roll-hud.lua: y=11 + ascendValue, hiding down to -100.
    assert(djui_hud_rect_outside(109, -89, 32, 64, 320, 240));
    assert(djui_hud_rect_outside(141, -89, 32, 64, 320, 240));
    assert(djui_hud_rect_outside(125, -73, 32, 32, 320, 240));
    // Damaged/submerged meter, partial hide animation, lives remain.
    assert(!djui_hud_rect_outside(109, 10, 32, 64, 320, 240));
    assert(!djui_hud_rect_outside(109, -63, 32, 64, 320, 240));
    assert(!djui_hud_rect_outside(28, 14, 16, 16, 320, 240));
    // Character Select hiding, scaled HUD and mirrored textures.
    assert(djui_hud_rect_outside(109, 208-301, 32, 64, 320, 240));
    assert(djui_hud_rect_outside(218, -178, 64, 128, 640, 480));
    assert(!djui_hud_rect_outside(32, 32, -32, -32, 320, 240));
    assert(!djui_hud_rect_outside(-1, -1, 2, 2, 320, 240));
    assert(djui_hud_rect_outside(32, -64, 32, 64, 320, 240));
    puts("PASS: hidden mod meters rejected; visible/partial/mirrored HUD retained");
}
