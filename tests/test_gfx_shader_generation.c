#define SDL_MAIN_HANDLED
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../src/pc/gfx/gfx_cc.h"
typedef uint8_t u8;
#define SHADER_FLAG_MAX 8
static int sAllowCCPrint;
static unsigned tested, failures;
static size_t maxVs, maxFs;
static unsigned maxAttributes;
static void validate(struct ColorCombiner*, const char*, const char*, size_t, size_t);
static void gfx_pc_precomp_shader(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
static bool gfx_opengl_check_compatibility(void) { return glGetString(GL_VERSION) != NULL; }
static void sys_fatal(const char *fmt, ...) {
    va_list args; va_start(args,fmt); vfprintf(stderr,fmt,args); va_end(args); abort();
}
#include "generated.h"

static GLuint compile(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[4096] = {0};
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        if (failures < 8) fprintf(stderr, "%s shader: %s\n", type == GL_VERTEX_SHADER ? "Vertex" : "Fragment", log);
        ++failures;
    }
    return shader;
}
static void validate(struct ColorCombiner *cc, const char *vs, const char *fs, size_t vl, size_t fl) {
    struct CCFeatures features={0}; gfx_cc_get_features(cc,&features);
    unsigned attributes=1+features.used_textures[0]+features.used_textures[1]+
        cc->cm.use_fog+cc->cm.light_map+features.num_inputs;
    assert(attributes<=OPENGL_MAX_SHADER_ATTRIBUTES);
    if(attributes>maxAttributes) maxAttributes=attributes;
    if (vl > maxVs) maxVs = vl;
    if (fl > maxFs) maxFs = fl;
    ++tested;
    if (vl >= VS_CAPACITY || fl >= FS_CAPACITY) {
        if (failures < 8) fprintf(stderr, "Source overflow: vertex=%zu/%d fragment=%zu/%d\n", vl, VS_CAPACITY, fl, FS_CAPACITY);
        ++failures;
    }
    unsigned before = failures;
    GLuint v = compile(GL_VERTEX_SHADER, vs), f = compile(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    GLint linked = 0; glGetProgramiv(p, GL_LINK_STATUS, &linked);
    if (!linked && failures == before) {
        char log[4096] = {0}; glGetProgramInfoLog(p, sizeof(log), NULL, log);
        if (failures < 8) fprintf(stderr, "Link: %s\n", log);
        ++failures;
    }
    if (failures != before && failures < 8) fprintf(stderr, "Case %u: flags=%x rgb=%08x alpha=%08x\n", tested, cc->cm.flags, cc->cm.rgb1, cc->cm.alpha1);
    glDeleteProgram(p); glDeleteShader(v); glDeleteShader(f);
}
static void gfx_pc_precomp_shader(uint32_t r, uint32_t a, uint32_t r2, uint32_t a2, uint32_t flags) {
    for (unsigned extra = 0; extra < 4; ++extra) {
        struct ColorCombiner cc = {0};
        cc.cm.rgb1=r; cc.cm.alpha1=a; cc.cm.rgb2=r2; cc.cm.alpha2=a2;
        cc.cm.flags=flags; cc.cm.world_geometry=extra&1; cc.cm.light_map=(extra>>1)&1;
        gfx_generate_cc(&cc); generate(&cc);
    }
}
int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
#ifdef USE_GLES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
#endif
    SDL_Window *w=SDL_CreateWindow("Shader regression validation",0,0,16,16,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    assert(w); SDL_GLContext context=SDL_GL_CreateContext(w);
    assert(context && glewInit()==GLEW_OK);
    printf("GPU: %s; GL: %s; GLSL: %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION),glGetString(GL_SHADING_LANGUAGE_VERSION));
    assert(gfx_sdl_check_opengl_compatibility());
    assert(SDL_GL_GetCurrentContext()==context && SDL_GL_GetCurrentWindow()==w);
    gfx_cc_precomp();
    validate_post_effects();
    // Texture 1 without texture 0, alpha noise, full two-cycle formulas, all
    // runtime flag combinations. These are legal inputs to the shared generator.
    const uint8_t patterns[][16] = {
        {0,0,0,CC_TEXEL1,0,0,0,CC_TEXEL1A},
        {CC_TEXEL0,CC_TEXEL1,CC_SHADE,CC_ENV,CC_PRIM,CC_ENVA,CC_LOD,CC_SHADEA,
         CC_COMBINED,CC_TEXEL0A,CC_PRIMA,CC_TEXEL1A,CC_COMBINEDA,CC_PRIMA,CC_LOD,CC_ENVA},
        {0,0,0,CC_TEXEL0,0,0,0,CC_NOISE}
    };
    for(unsigned pattern=0;pattern<sizeof(patterns)/sizeof(patterns[0]);++pattern) {
        for(unsigned flags=0;flags<128;++flags) {
            struct ColorCombiner cc={0}; memcpy(cc.cm.all_values,patterns[pattern],16);
            cc.cm.flags=flags; gfx_generate_cc(&cc); generate(&cc);
        }
    }
    // Deterministic legal N64 selector combinations, including two-cycle
    // mod materials not represented by the startup cache.
    const uint8_t rgb[]={CC_0,CC_TEXEL0,CC_TEXEL1,CC_PRIM,CC_SHADE,CC_ENV,CC_1};
    const uint8_t alpha[]={CC_0,CC_TEXEL0A,CC_TEXEL1A,CC_PRIMA,CC_SHADEA,CC_ENVA,CC_1};
    uint32_t seed=0x534d3634;
    for(unsigned n=0;n<2048;n++) {
        struct ColorCombiner cc={0};
        for(unsigned k=0;k<16;k++) {
            seed^=seed<<13; seed^=seed>>17; seed^=seed<<5;
            cc.cm.all_values[k]=(k%8<4 ? rgb : alpha)[seed%7];
        }
        cc.cm.flags=n%128;
        if(n&1) cc.cm.all_values[8]=CC_COMBINED;
        if(n&2) cc.cm.all_values[12]=CC_COMBINEDA;
        gfx_generate_cc(&cc); generate(&cc);
    }
    printf("%u programs tested; %u failures; max source bytes: VS %zu/%d, FS %zu/%d\n", tested,failures,maxVs,VS_CAPACITY,maxFs,FS_CAPACITY);
    assert(maxAttributes>7); // Reproduce the old array-capacity overrun.
    printf("Attribute capacity: observed %u, allocated %u (old capacity was 7)\n",maxAttributes,(unsigned)OPENGL_MAX_SHADER_ATTRIBUTES);
    SDL_GL_DeleteContext(context); SDL_DestroyWindow(w); SDL_Quit();
    return failures ? 1 : 0;
}
