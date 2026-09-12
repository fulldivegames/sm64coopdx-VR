#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
struct DjuiText {char *message;};
static bool fail;
static void *test_malloc(size_t n) {return fail ? NULL : malloc(n);}
#define malloc test_malloc
#include "../build/test_djui_text_storage.inc"
#undef malloc
int main(void) {
    struct DjuiText text={0};
    char *longText=malloc(70001); memset(longText,'A',70000); longText[70000]=0;
    djui_text_set_text(&text,longText); assert(strlen(text.message)==70000);
    djui_text_set_text(&text,text.message+69990); assert(strlen(text.message)==10);
    char *old=text.message; fail=true;
    djui_text_set_text(&text,"Replacement"); assert(text.message==old);
    fail=false; djui_text_set_text(&text,""); assert(text.message[0]==0);
    free(text.message); free(longText);
    puts("PASS: 70KB text, aliased substring replacement, allocation failure and empty text.");
}
