#ifndef VR_PROPELLER_STOMP_H
#define VR_PROPELLER_STOMP_H
#include <stdbool.h>
#include <math.h>

// Feet must approach the top, with their center over the enemy footprint.
// The vertical allowance is one falling tick, not an expanded side hitbox.
static inline bool vr_propeller_top_contact(float dx, float dz, float feetY,
        float velocityY, float enemyY, float height, float radius) {
    return velocityY < 0.0f && height > 0.0f && radius > 0.0f &&
        feetY <= enemyY + height &&
        feetY - velocityY >= enemyY + height &&
        dx * dx + dz * dz <= radius * radius;
}
#endif
