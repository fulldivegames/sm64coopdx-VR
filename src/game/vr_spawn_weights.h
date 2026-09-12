#ifndef VR_SPAWN_WEIGHTS_H
#define VR_SPAWN_WEIGHTS_H
#include <stddef.h>
#include <stdbool.h>
static inline unsigned vr_spawn_weight(unsigned weight) {
    return weight < 1 ? 1 : (weight > 100 ? 100 : weight);
}
// Strong/30-second-category powers get half the relative tickets. Double the
// ordinary tickets instead of dividing rare ones, preserving exact halves even
// at slider value 1. Category is independent of optional timer extensions.
static inline unsigned vr_spawn_effective_weight(unsigned weight, bool strongPower) {
    return vr_spawn_weight(weight) * (strongPower ? 1U : 2U);
}
// Ticket must be uniformly distributed over [0, total weight).
static inline unsigned vr_spawn_weight_pick(const unsigned* weights, unsigned count, unsigned ticket) {
    for (unsigned i = 0; i < count; ++i) {
        if (ticket < weights[i]) return i;
        ticket -= weights[i];
    }
    return 0;
}
#endif
