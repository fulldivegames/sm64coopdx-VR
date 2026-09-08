#include <assert.h>
#include <stdio.h>
#include "../src/pc/djui/djui_notice_window.h"
int main(void) {
    struct DjuiNoticeWindow window = {0};
    assert(!djui_notice_visible(&window,0));
    for (uint32_t i=0;i<1000;++i) {
        djui_notice_report(&window,i); // includes alternating messages/mods
        assert(djui_notice_visible(&window,i)==(i<150));
    }
    djui_notice_report(&window,1150);
    assert(djui_notice_visible(&window,1299));
    assert(!djui_notice_visible(&window,1300));
    window.valid=false;
    djui_notice_report(&window,UINT32_MAX-50);
    assert(djui_notice_visible(&window,98));
    assert(!djui_notice_visible(&window,99));
    puts("Script notices: five-second expiry, repeated bursts, quiet restart, timer wrap passed.");
}
