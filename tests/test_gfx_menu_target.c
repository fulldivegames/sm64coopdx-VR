#define SDL_MAIN_HANDLED
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static int flushes;
static void gfx_gl_queue_flush(void) { ++flushes; }
#include "../src/pc/gfx/gfx_menu_target.h"

int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_Window *window = SDL_CreateWindow("Menu target validation", 0, 0, 128, 128,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(window);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    assert(context && glewInit() == GLEW_OK);
    while (glGetError() != GL_NO_ERROR) {}
    GLuint sourceBuffer, sourceTexture;
    glGenBuffers(1, &sourceBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, sourceBuffer);
    const float input[16] = { 0 };
    glBufferData(GL_ARRAY_BUFFER, sizeof(input), input, GL_STATIC_DRAW);
    for (int i = 0; i < 2; ++i) {
        glVertexAttribPointer(i, 2, GL_FLOAT, GL_FALSE, 16, (void *)(uintptr_t)(i*8));
        glEnableVertexAttribArray(i);
    }
    glGenTextures(1, &sourceTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTexture);
    glViewport(0, 0, 128, 128);
    glClearColor(0, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_SCISSOR_TEST);
    glScissor(7, 9, 80, 90);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gfx_opengl_begin_menu_target();
    assert(sMenuTarget.active);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.25f, 0, 0, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    const float projection[16] = {
        1.0f/320,0,0,0, 0,1.0f/240,0,0, 0,0,0,0, -0.5f,-0.5f,0,1
    };
    // A mod may set a preview viewport while drawing its panel. It must not
    // shrink/offset the final theater quad or leak changes to cached state.
    glViewport(0, 0, 16, 16);
    gfx_opengl_end_menu_target(projection);
    assert(!sMenuTarget.active && flushes == 2);
    GLint value, box[4];
    glGetIntegerv(GL_VIEWPORT, box); assert(box[2] == 16 && box[3] == 16);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &value); assert(value == 0);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &value); assert(value == (GLint)sourceBuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &value); assert(value == (GLint)sourceTexture);
    glGetIntegerv(GL_CURRENT_PROGRAM, &value); assert(value == 0);
    glGetIntegerv(GL_BLEND_SRC_RGB, &value); assert(value == GL_SRC_ALPHA);
    glGetIntegerv(GL_SCISSOR_BOX, box);
    assert(box[0] == 7 && box[1] == 9 && box[2] == 80 && box[3] == 90);
    assert(glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_CULL_FACE) && glIsEnabled(GL_SCISSOR_TEST));
    unsigned char pixel[4];
    glReadPixels(64, 64, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    assert(pixel[0] >= 62 && pixel[0] <= 66 && pixel[1] == 0 && pixel[2] >= 125 && pixel[2] <= 130);
    glReadPixels(4, 4, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    assert(pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 255);
    assert(glGetError() == GL_NO_ERROR);
    // A captured 3D preview must retain depth ordering, with its 2D HUD
    // composited in front. This exercises the same target as the menu layers.
    glViewport(0,0,128,128);
    gfx_opengl_begin_menu_target();
    GLint depthAttachment;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &depthAttachment);
    assert(depthAttachment == GL_RENDERBUFFER);
    glDisable(GL_SCISSOR_TEST); glDisable(GL_CULL_FACE); glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE); glDepthFunc(GL_LESS);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    for(int layer=0;layer<2;++layer) {
        glColor4f(layer?1:0,layer?0:1,0,1);
        float z=layer?0.5f:-0.5f;
        glBegin(GL_QUADS);
        glVertex3f(-1,-1,z);glVertex3f(1,-1,z);
        glVertex3f(1,1,z);glVertex3f(-1,1,z);
        glEnd();
    }
    glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    assert(pixel[0]==0 && pixel[1]==255);
    glDisable(GL_DEPTH_TEST);
    glColor4f(0,0,1,1);
    glBegin(GL_QUADS);
    glVertex2f(-.25f,-.25f);glVertex2f(.25f,-.25f);
    glVertex2f(.25f,.25f);glVertex2f(-.25f,.25f);
    glEnd();
    glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    assert(pixel[0]==0 && pixel[1]==0 && pixel[2]==255);
    gfx_opengl_end_menu_target(projection);
    assert(glGetError()==GL_NO_ERROR);
    gfx_opengl_destroy_menu_target();
    glDeleteTextures(1, &sourceTexture);
    glDeleteBuffers(1, &sourceBuffer);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    puts("Menu target: preview depth, HUD layer ordering, alpha, viewport and GL state restoration passed.");
    return 0;
}
