#ifndef GFX_STREAM_BUFFER_H
#define GFX_STREAM_BUFFER_H

#include <stddef.h>
#include <stdbool.h>

struct GfxStreamBuffer {
    size_t capacity;
    size_t used;
};

// Reserve disjoint byte ranges, aligned to the current shader's vertex stride.
// The caller must orphan storage when requested before uploading this range.
static inline size_t gfx_stream_buffer_reserve(
    struct GfxStreamBuffer *stream, size_t bytes, size_t stride, bool *orphan
) {
    const size_t minimumCapacity = 4u * 1024u * 1024u;
    const size_t remainder = stream->used % stride;
    const size_t padding = remainder ? stride - remainder : 0;
    *orphan = stream->capacity == 0 || bytes > stream->capacity ||
        stream->used > stream->capacity ||
        padding > stream->capacity - stream->used ||
        bytes > stream->capacity - stream->used - padding;
    if (*orphan) {
        stream->capacity = bytes > minimumCapacity ? bytes : minimumCapacity;
        stream->used = 0;
    }
    const size_t offset = *orphan ? 0 : stream->used + padding;
    stream->used = offset + bytes;
    return offset;
}

#endif
