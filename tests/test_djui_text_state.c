#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
typedef float f32;
typedef struct {float x,y;} Mtx;
struct DjuiBaseRect {float x,y;};
struct DjuiBase {struct DjuiBaseRect comp;};
struct Font {float charWidth,charHeight;void(*render_char)(char*);};
struct DjuiText {struct DjuiBase base;float fontScale;struct Font*font;};
static float sTextRenderX,sTextRenderY,sTextRenderLastX,sTextRenderLastY;
static bool fail,clip;
static int draws,commands;
static Mtx matrix;
static Mtx* alloc_display_list(unsigned size){return fail?NULL:&matrix;}
static bool djui_gfx_add_clipping_specific(struct DjuiBase*b,float x,float y,float w,float h){return clip;}
static void guTranslate(Mtx*m,float x,float y,float z){m->x=x;m->y=y;}
#define gSPMatrix(...) (++commands)
static void draw(char*c){draws++;}
#include "../build/test_djui_text_state.inc"
int main(void){struct Font f={1,1,draw};struct DjuiText t={.fontScale=1,.font=&f};
 sTextRenderX=10;sTextRenderY=5;fail=true;djui_text_render_single_char(&t,"A");
 assert(!draws&&!commands&&!sTextRenderLastX&&!sTextRenderLastY);
 fail=false;djui_text_render_single_char(&t,"A");assert(draws==1&&commands==1&&matrix.x==10&&matrix.y==-5);
 clip=true;sTextRenderX=20;djui_text_render_single_char(&t,"B");assert(draws==1&&sTextRenderLastX==10);
 clip=false;sTextRenderX=30;djui_text_render_single_char(&t,"C");assert(draws==2&&matrix.x==20);
 puts("PASS: actual glyph renderer keeps cursor/commands coherent after allocation failure or clipping.");}
