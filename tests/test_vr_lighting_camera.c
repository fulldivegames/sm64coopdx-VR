#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "engine/math_util.h"

// math_util.c also contains terrain alignment; this test must never call it.
struct Surface;
f32 find_floor(f32 x, f32 y, f32 z, struct Surface **floor) {
    (void)x; (void)y; (void)z; (void)floor;
    assert(!"Unexpected terrain query in camera matrix test");
    return 0;
}

// Exercise the renderer's actual matrix routines across camera rotations and
// interpolation fractions. Static geometry must recover the same world basis.
int main(void) {
    Mat4 tick, frame, inverse, staleInverse, model, view, recovered, stale;
    Vec3f pos = {0, 100, 0}, target;
    mtxf_identity(model);
    model[3][0] = 125; model[3][1] = 50; model[3][2] = -200;
    float staleError = 0;
    for (int yaw = 0; yaw < 360; yaw += 15) {
        float angle = yaw * 0.01745329252f;
        target[0] = sinf(angle) * 100;
        target[1] = 75;
        target[2] = cosf(angle) * 100;
        mtxf_lookat(tick, pos, target, 0);
        mtxf_inverse(staleInverse, tick);
        for (int subframe = 0; subframe <= 4; ++subframe) {
            float renderAngle = angle - (1 - subframe / 4.0f) * 0.08f;
            target[0] = sinf(renderAngle) * 100;
            target[2] = cosf(renderAngle) * 100;
            mtxf_lookat(frame, pos, target, 0);
            mtxf_mul(view, model, frame);
            mtxf_inverse(inverse, frame);
            mtxf_mul(recovered, view, inverse);
            mtxf_mul(stale, view, staleInverse);
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) {
                    assert(fabsf(recovered[r][c] - model[r][c]) < 0.00001f);
                    staleError = fmaxf(staleError, fabsf(stale[r][c] - model[r][c]));
                }
            }
        }
    }
    assert(staleError > 0.05f); // The old tick inverse exposes the reported drift.
    puts("Lighting: stable world basis through camera turns and interpolated frames; stale tick inverse reproduces drift.");
    return 0;
}
