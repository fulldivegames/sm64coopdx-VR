#ifndef DJUI_NOTICE_WINDOW_H
#define DJUI_NOTICE_WINDOW_H
#include <stdbool.h>
#include <stdint.h>

struct DjuiNoticeWindow {
    uint32_t start, lastReport;
    bool valid;
};
#define DJUI_NOTICE_TICKS (30u * 5u)
static inline void djui_notice_report(struct DjuiNoticeWindow *window, uint32_t now) {
    // Repeated reports can update the message, but cannot extend this burst.
    // A new burst may appear after five seconds without a report.
    if (!window->valid || (uint32_t)(now - window->lastReport) >= DJUI_NOTICE_TICKS) {
        window->start = now;
        window->valid = true;
    }
    window->lastReport = now;
}
static inline bool djui_notice_visible(const struct DjuiNoticeWindow *window, uint32_t now) {
    return window->valid && (uint32_t)(now - window->start) < DJUI_NOTICE_TICKS;
}
#endif
