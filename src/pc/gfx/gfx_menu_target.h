/* Private OpenGL menu target. Include after gfx_gl_submission.h.
 * Drain BOTH the CPU triangle batch (caller) and ordered GL queue at every
 * target boundary. Direct GL state is restored so submission caches remain valid.
 */
#ifndef GFX_MENU_TARGET_H
#define GFX_MENU_TARGET_H

static struct {
    GLuint framebuffer, texture, depthBuffer, program, buffer;
    GLint width, height, previousFramebuffer;
    GLint viewport[4];
    GLint blendSrcRGB, blendDstRGB, blendSrcAlpha, blendDstAlpha;
    bool active;
} sMenuTarget;

static GLuint menu_target_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char message[1024];
        glGetShaderInfoLog(shader, sizeof(message), NULL, message);
        fprintf(stderr, "Menu target shader: %s\n", message);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static bool menu_target_program(void) {
    if (sMenuTarget.program) return true;
#if defined(USE_GLES) || defined(__ANDROID__)
    const char *version = "#version 100\nprecision highp float;\n";
#else
    const char *version = "#version 120\n";
#endif
    char vertex[1024], fragment[1024];
    snprintf(vertex, sizeof(vertex), "%sattribute vec2 pos; attribute vec2 uv;"
        "uniform mat4 projection; varying vec2 texCoord;"
        "void main(){texCoord=uv;gl_Position=projection*vec4(pos,0.0,1.0);}", version);
    snprintf(fragment, sizeof(fragment), "%suniform sampler2D image; varying vec2 texCoord;"
        "void main(){gl_FragColor=texture2D(image,texCoord);}", version);
    GLuint vs = menu_target_shader(GL_VERTEX_SHADER, vertex);
    GLuint fs = menu_target_shader(GL_FRAGMENT_SHADER, fragment);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glBindAttribLocation(program, 0, "pos");
    glBindAttribLocation(program, 1, "uv");
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) { glDeleteProgram(program); return false; }
    sMenuTarget.program = program;
    glGenBuffers(1, &sMenuTarget.buffer);
    return true;
}

static void gfx_opengl_begin_menu_target(void) {
    gfx_gl_queue_flush();
    if (sMenuTarget.active || !menu_target_program()) return;
    GLint viewport[4], texture;
    glGetIntegerv(GL_VIEWPORT, viewport);
    if (viewport[2] <= 0 || viewport[3] <= 0) return;
    memcpy(sMenuTarget.viewport, viewport, sizeof(viewport));
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &sMenuTarget.previousFramebuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    if (!sMenuTarget.framebuffer) glGenFramebuffers(1, &sMenuTarget.framebuffer);
    if (!sMenuTarget.texture) glGenTextures(1, &sMenuTarget.texture);
    if (!sMenuTarget.depthBuffer) glGenRenderbuffers(1, &sMenuTarget.depthBuffer);
    glBindTexture(GL_TEXTURE_2D, sMenuTarget.texture);
    if (sMenuTarget.width != viewport[2] || sMenuTarget.height != viewport[3]) {
        sMenuTarget.width = viewport[2];
        sMenuTarget.height = viewport[3];
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sMenuTarget.width,
            sMenuTarget.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GLint oldRenderbuffer;
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, sMenuTarget.depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
            sMenuTarget.width, sMenuTarget.height);
        glBindRenderbuffer(GL_RENDERBUFFER, oldRenderbuffer);
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindFramebuffer(GL_FRAMEBUFFER, sMenuTarget.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        sMenuTarget.texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER, sMenuTarget.depthBuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, sMenuTarget.previousFramebuffer);
        return;
    }
    // The preview world needs depth; Lua HUD callbacks explicitly disable it.
    GLfloat clear[4];
    GLfloat clearDepth;
    GLboolean depthMask, colorMask[4];
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clearDepth);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
    GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clear);
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#if defined(USE_GLES) || defined(__ANDROID__)
    glClearDepthf(1.0f);
#else
    glClearDepth(1.0);
#endif
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(clear[0], clear[1], clear[2], clear[3]);
#if defined(USE_GLES) || defined(__ANDROID__)
    glClearDepthf(clearDepth);
#else
    glClearDepth(clearDepth);
#endif
    glDepthMask(depthMask);
    glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    if (scissor) glEnable(GL_SCISSOR_TEST);
    glGetIntegerv(GL_BLEND_SRC_RGB, &sMenuTarget.blendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &sMenuTarget.blendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &sMenuTarget.blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &sMenuTarget.blendDstAlpha);
    // Accumulate premultiplied color and correct coverage, rather than alpha squared.
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    sMenuTarget.active = true;
}

static void gfx_opengl_end_menu_target(const float *projection) {
    gfx_gl_queue_flush();
    if (!sMenuTarget.active) return;
    sMenuTarget.active = false;
    glBindFramebuffer(GL_FRAMEBUFFER, sMenuTarget.previousFramebuffer);

    GLint program, buffer, activeTexture, texture, scissorBox[4], viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
    GLboolean depth = glIsEnabled(GL_DEPTH_TEST), blend = glIsEnabled(GL_BLEND);
    GLboolean cull = glIsEnabled(GL_CULL_FACE), scissor = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean depthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    struct { GLint enabled, size, type, normalized, stride, buffer; void *pointer; } attrib[2];
    for (GLuint i = 0; i < 2; ++i) {
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attrib[i].enabled);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &attrib[i].size);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &attrib[i].type);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &attrib[i].normalized);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &attrib[i].stride);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &attrib[i].buffer);
        glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &attrib[i].pointer);
    }
    const GLfloat vertices[] = {
        0,0,0,0, 320,0,1,0, 320,240,1,1,
        0,0,0,0, 320,240,1,1, 0,240,0,1
    };
    glDisable(GL_DEPTH_TEST);
    glViewport(sMenuTarget.viewport[0], sMenuTarget.viewport[1],
        sMenuTarget.viewport[2], sMenuTarget.viewport[3]);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(sMenuTarget.program);
    glUniformMatrix4fv(glGetUniformLocation(sMenuTarget.program, "projection"), 1, GL_FALSE, projection);
    glUniform1i(glGetUniformLocation(sMenuTarget.program, "image"), 0);
    glBindTexture(GL_TEXTURE_2D, sMenuTarget.texture);
    glBindBuffer(GL_ARRAY_BUFFER, sMenuTarget.buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void *)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void *)(2*sizeof(float)));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    for (GLuint i = 0; i < 2; ++i) {
        glBindBuffer(GL_ARRAY_BUFFER, attrib[i].buffer);
        glVertexAttribPointer(i, attrib[i].size, attrib[i].type, attrib[i].normalized,
            attrib[i].stride, attrib[i].pointer);
        if (attrib[i].enabled) glEnableVertexAttribArray(i); else glDisableVertexAttribArray(i);
    }
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(activeTexture);
    glUseProgram(program);
    glBlendFuncSeparate(sMenuTarget.blendSrcRGB, sMenuTarget.blendDstRGB,
        sMenuTarget.blendSrcAlpha, sMenuTarget.blendDstAlpha);
    if (depth) glEnable(GL_DEPTH_TEST);
    glDepthMask(depthMask);
    if (!blend) glDisable(GL_BLEND);
    if (cull) glEnable(GL_CULL_FACE);
    glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
    if (scissor) glEnable(GL_SCISSOR_TEST);
}

static void gfx_opengl_destroy_menu_target(void) {
    if (sMenuTarget.depthBuffer) glDeleteRenderbuffers(1, &sMenuTarget.depthBuffer);
    if (sMenuTarget.framebuffer) glDeleteFramebuffers(1, &sMenuTarget.framebuffer);
    if (sMenuTarget.texture) glDeleteTextures(1, &sMenuTarget.texture);
    if (sMenuTarget.program) glDeleteProgram(sMenuTarget.program);
    if (sMenuTarget.buffer) glDeleteBuffers(1, &sMenuTarget.buffer);
    memset(&sMenuTarget, 0, sizeof(sMenuTarget));
}
#endif
