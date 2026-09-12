#ifndef DJUI_TUTORIAL_PAGES_H
#define DJUI_TUTORIAL_PAGES_H
#include <stddef.h>
#include <string.h>

// Literal tutorial prose is paginated at word boundaries, never truncated.
// Small pages also bound glyph/matrix allocations on standalone Quest.
static inline size_t djui_tutorial_next(const char* message, size_t start) {
    size_t end = start, columns = 0, lines = 1;
    // Conservative 24-column / 9-line layout fits both menu fonts at 32px
    // within the narrowest 444x300 tutorial body, including paragraph gaps.
    while (message[end]) {
        size_t word = end;
        while (message[word] && message[word] != ' ') ++word;
        size_t width = word - end;
        size_t nextLines = lines, nextColumns = columns;
        if (columns && columns + 1 + width > 24) { ++nextLines; nextColumns = 0; }
        nextColumns += (nextColumns ? 1 : 0) + width;
        if (nextColumns > 24) { nextLines += (nextColumns - 1) / 24; nextColumns = (nextColumns - 1) % 24 + 1; }
        if (nextLines > 9 && end > start) return end - 1;
        end = word;
        columns = nextColumns;
        lines = nextLines;
        if (!message[end]) return end;
        if (end > start && message[end - 1] == '.') { lines += 2; columns = 0; }
        ++end;
    }
    return end;
}
static inline void djui_tutorial_format(char* out, size_t capacity, const char* message,
        size_t start, size_t end) {
    size_t used = 0;
    if (capacity < 1) return;
    for (size_t i = start; i < end && used + 1 < capacity; ++i) {
        out[used++] = message[i];
        if (message[i] == '.' && i+1 < end && message[i+1] == ' ' && used+2 < capacity) {
            out[used++] = '\n'; out[used++] = '\n'; ++i;
        }
    }
    out[used] = 0;
}
#endif
