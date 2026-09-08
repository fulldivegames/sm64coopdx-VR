#include <assert.h>
#include <stdio.h>
#include "../src/pc/gfx/gfx_stream_buffer.h"

int main(void) {
    struct GfxStreamBuffer stream = { 0 };
    size_t previousEnd = 0;
    unsigned renewals = 0;
    for (size_t i = 0; i < 100000; i++) {
        // Exercise changing shader layouts, tiny draws, and full batches.
        const size_t stride = (4 + i % 23) * sizeof(float);
        const size_t bytes = stride * 3 * (1 + i % 256);
        bool orphan;
        const size_t offset = gfx_stream_buffer_reserve(&stream, bytes, stride, &orphan);
        assert(offset % stride == 0);
        assert(offset + bytes <= stream.capacity);
        assert(stream.used == offset + bytes);
        if (orphan) { assert(offset == 0); renewals++; }
        else assert(offset >= previousEnd);
        previousEnd = stream.used;
    }
    assert(renewals > 1 && renewals < 10000);
    bool orphan;
    const size_t large = 8u * 1024u * 1024u;
    assert(gfx_stream_buffer_reserve(&stream, large, 16, &orphan) == 0);
    assert(orphan && stream.capacity == large);
    assert(gfx_stream_buffer_reserve(&stream, 48, 16, &orphan) == 0);
    assert(orphan);
    puts("PASS: 100000 mixed-layout allocations, rollover, and oversized draw");
    return 0;
}
