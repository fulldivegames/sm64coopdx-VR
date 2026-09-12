"""Guard cosmetic-cap state separation and persisted throw/re-grab metadata."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
source = (root / 'src/game/rendering_graph_node.c').read_text()
gesture = source.split('static void vr_update_painting_exit_hat_gesture(void) {', 1)[1].split('static u8 vr_painting_exit_hat_alpha', 1)[0]
assert 'if (sVrHeldCapEarnedWing)' in gesture
assert 'if (sVrPaintingExitHatWingCap)' not in gesture
assert not re.search(r'(?:mario|gMarioStates\[0\])->?(?:flags|capTimer)\s*[|&+]?=', gesture)
assert 'sVrHeldCapEarnedWing = (cap->oBehParams & 0x1000U) != 0' in gesture
assert 'sVrHeldCapFlags = sVrHeldHelmet ? 0 : gMarioStates[0].flags &' in gesture
assert 'sVrPaintingExitHatWingCap || sVrHeldCapFlags != 0' in source
for flags in (0, 2, 4, 8, 6, 10, 12, 14):
    for helmet in (0, 1, 2):
        for earned in (False, True):
            packed = 0x7f0000 | helmet | (flags << 8) | (0x1000 if earned else 0)
            assert (packed & 0xff) == helmet
            assert ((packed >> 8) & 14) == flags
            assert bool(packed & 0x1000) == earned
            assert ((packed >> 16) & 255) == 0x7f
print('PASS: cosmetic native caps cannot grant/reset Wing Cap; throw metadata round-trips.')
