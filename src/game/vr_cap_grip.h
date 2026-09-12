#ifndef VR_CAP_GRIP_H
#define VR_CAP_GRIP_H

// Held cosmetics use tracking-space units (100 per meter), independently of
// glove size and Big Hands. Half the previous size avoids oversized helmets.
#define VR_HELD_CAP_SCALE 0.125f

// Both inputs use row-vector bases. Store rotation only: glove size must not
// enlarge a cosmetic hat. Capture once, never again as the player turns.
static inline void vr_cap_capture_rotation(float relative[3][3],
        const float hat[3][3], const float hand[4][4], float handScale) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            relative[i][j] = 0.0f;
            for (int k = 0; k < 3; ++k)
                relative[i][j] += hat[i][k] * hand[j][k] / handScale;
        }
}

static inline void vr_cap_grip_matrix(float out[4][4],
        const float relative[3][3], const float hand[4][4],
        float handScale, const float brim[3]) {
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) {
            out[i][k] = 0.0f;
            for (int j = 0; j < 3; ++j)
                out[i][k] += VR_HELD_CAP_SCALE * relative[i][j] * hand[j][k] / handScale;
        }
        out[i][3] = 0.0f;
    }
    for (int k = 0; k < 3; ++k) {
        out[3][k] = hand[3][k] + 68.0f * hand[0][k];
        for (int i = 0; i < 3; ++i) out[3][k] -= brim[i] * out[i][k];
    }
    out[3][3] = 1.0f;
}
#endif
