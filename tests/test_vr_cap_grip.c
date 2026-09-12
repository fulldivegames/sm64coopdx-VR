#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../src/game/vr_cap_grip.h"

static void near(float a, float b) { assert(fabsf(a - b) < 0.0001f); }
int main(void) {
    const float hat[3][3] = {{0,1,0},{0,0,-1},{-1,0,0}};
    const float brims[2][3] = {{0,0,163},{175,145,0}};
    for (int size = 1; size <= 4; ++size) {
        float scale = 0.1f * size;
        float hand[4][4] = {{0,0,-scale,0},{0,scale,0,0},{scale,0,0,0},{13,21,-17,1}};
        float relative[3][3], result[4][4];
        vr_cap_capture_rotation(relative, hat, hand, scale);
        for (int brim = 0; brim < 2; ++brim) {
            vr_cap_grip_matrix(result, relative, hand, scale, brims[brim]);
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j) near(result[i][j], VR_HELD_CAP_SCALE * hat[i][j]);
            // Rotate the controller after grabbing: the contact remains at
            // the fingers, with constant hat size, for either brim geometry.
            for (int turn = 0; turn < 2; ++turn) {
                vr_cap_grip_matrix(result, relative, hand, scale, brims[brim]);
                for (int k = 0; k < 3; ++k) {
                    float contact = result[3][k];
                    for (int i = 0; i < 3; ++i) contact += brims[brim][i]*result[i][k];
                    near(contact, hand[3][k] + 68*hand[0][k]);
                    float norm = 0;
                    for (int i = 0; i < 3; ++i) norm += result[k][i]*result[k][i];
                    near(norm, 0.015625f);
                }
                for (int i = 0; i < 3; ++i) {
                    float x = hand[i][0]; hand[i][0] = -hand[i][2]; hand[i][2] = x;
                }
            }
            // Restore the original orientation before the next brim.
            for (int i = 0; i < 3; ++i) { hand[i][0] = -hand[i][0]; hand[i][2] = -hand[i][2]; }
        }
    }
    puts("Cap grip: stable orientation, brim contact, and size passed.");
}
