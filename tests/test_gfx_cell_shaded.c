#define SDL_MAIN_HANDLED
#define FOR_WINDOWS 1
#define VR_COLOR_FILTER_CELL_SHADED 4
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static unsigned int configVrColorFilter=4;
static void gfx_gl_queue_flush(void) {}
#include "../src/pc/gfx/gfx_menu_target.h"
#include "../src/pc/gfx/gfx_cell_shaded.h"
int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO)==0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
    SDL_Window *w=SDL_CreateWindow("Cell shader validation",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    assert(w);SDL_GLContext context=SDL_GL_CreateContext(w);
    assert(context && glewInit()==GLEW_OK);
    while(glGetError()!=GL_NO_ERROR){}
    GLuint fbo,rb[2];glGenFramebuffers(1,&fbo);glGenRenderbuffers(2,rb);
    glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    for(int samples=1;samples<=4;samples+=3){
        for(int i=0;i<2;i++){
            glBindRenderbuffer(GL_RENDERBUFFER,rb[i]);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER,samples,i?GL_DEPTH_COMPONENT24:GL_RGBA8,128,128);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,i?GL_DEPTH_ATTACHMENT:GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,rb[i]);
        }
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
        glViewport(0,0,128,128);glDisable(GL_SCISSOR_TEST);glDepthMask(GL_TRUE);
        glClearColor(.1f,.65f,.2f,1);glClearDepth(.4);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST);glScissor(64,0,64,128);glClearDepth(.8);glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);glEnable(GL_BLEND);glEnable(GL_CULL_FACE);
        gfx_opengl_cell_shaded();
        assert(glGetError()==GL_NO_ERROR);
        GLint bound;glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&bound);assert(bound==(GLint)fbo);
        assert(glIsEnabled(GL_SCISSOR_TEST)&&glIsEnabled(GL_DEPTH_TEST)&&glIsEnabled(GL_BLEND)&&glIsEnabled(GL_CULL_FACE));
        // Resolve the filtered result into the existing single-sample target.
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,sCell.fbo);
        glBlitFramebuffer(0,0,128,128,0,0,128,128,GL_COLOR_BUFFER_BIT,GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER,sCell.fbo);
        unsigned char pixel[4];glReadPixels(32,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
        assert(pixel[1]>=150 && pixel[1]<=156 && pixel[0]<30);
        glReadPixels(63,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
        assert(pixel[0]<12 && pixel[1]<12 && pixel[2]<12);
        glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        // The same modest silhouette separation must survive at long range,
        // where depth approaches 1.0. The old 1e-5 divisor hid these edges.
        const double nearDepth[] = {.9,.999,.999996};
        const double farDepth[] = {.92,.9992,.999997};
        for (int distance=0;distance<3;++distance) {
            glDisable(GL_SCISSOR_TEST);glDepthMask(GL_TRUE);
            glClearColor(.8f,.8f,.8f,1);glClearDepth(nearDepth[distance]);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            glEnable(GL_SCISSOR_TEST);glScissor(64,0,64,128);
            glClearDepth(farDepth[distance]);glClear(GL_DEPTH_BUFFER_BIT);
            gfx_opengl_cell_shaded();
            glDisable(GL_SCISSOR_TEST);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER,sCell.fbo);
            glBlitFramebuffer(0,0,128,128,0,0,128,128,GL_COLOR_BUFFER_BIT,GL_NEAREST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER,sCell.fbo);
            glReadPixels(63,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
            assert(pixel[0]<12 && pixel[1]<12 && pixel[2]<12);
            glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        }
        // Broad color boundaries at identical depth must retain ink outlines.
        // This specifically catches the previous depth-only regression.
        glDisable(GL_SCISSOR_TEST);glDepthMask(GL_TRUE);
        glClearColor(.8f,.8f,.8f,1);glClearDepth(.6);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST);glScissor(64,0,64,128);
        glClearColor(.25f,.25f,.25f,1);glClear(GL_COLOR_BUFFER_BIT);
        gfx_opengl_cell_shaded();
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,sCell.fbo);
        glBlitFramebuffer(0,0,128,128,0,0,128,128,GL_COLOR_BUFFER_BIT,GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER,sCell.fbo);
        glReadPixels(63,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
        assert(pixel[0]<12 && pixel[1]<12 && pixel[2]<12);
        glReadPixels(32,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
        assert(pixel[0]>=200 && pixel[0]<=206);
        glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        // High-contrast tiled color on a planar surface must not create black
        // outlines in its white texels. Vary depth linearly toward the horizon
        // too: a perspective plane's gradient is not a geometry boundary.
        glDepthMask(GL_TRUE);
        glEnable(GL_SCISSOR_TEST);
        for (int x=0;x<128;++x) {
            glScissor(x,0,1,128);
            float c=(x&1)?1.0f:0.1f;
            glClearColor(c,c,c,1);
            glClearDepth(0.3 + 0.69*x/127.0);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        }
        gfx_opengl_cell_shaded();
        assert(glGetError()==GL_NO_ERROR);
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,sCell.fbo);
        glBlitFramebuffer(0,0,128,128,0,0,128,128,GL_COLOR_BUFFER_BIT,GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER,sCell.fbo);
        for(int x=3;x<125;x+=2) {
            glReadPixels(x,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
            assert(pixel[0]>245 && pixel[1]>245 && pixel[2]>245);
        }
        glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    }
    gfx_opengl_destroy_cell_shaded();
    configVrColorFilter=0;gfx_opengl_cell_shaded();assert(!sCell.program);
    glDeleteFramebuffers(1,&fbo);glDeleteRenderbuffers(2,rb);
    SDL_GL_DeleteContext(context);SDL_DestroyWindow(w);SDL_Quit();
    puts("Cell Shaded: bands, silhouettes, color outlines, tiled-color rejection, planar-depth rejection, MSAA and state restoration passed.");
    return 0;
}
