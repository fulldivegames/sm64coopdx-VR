/* World-only, per-eye cel pass. Run after opaque/translucent world geometry,
 * before HUD rendering. No CPU readback, and no resources/work when disabled. */
#ifndef GFX_CELL_SHADED_H
#define GFX_CELL_SHADED_H
#if defined(__ANDROID__) || !defined(USE_GLES)
static struct {
    GLuint fbo, color, depth, program, buffer;
    GLint width, height;
} sCell;

static bool cell_program(void) {
    if (sCell.program) return true;
#ifdef __ANDROID__
    const char *version = "#version 100\nprecision highp float;\nprecision highp sampler2D;\n";
#else
    const char *version = "#version 120\n";
#endif
    char vertex[512], fragment[4096];
    snprintf(vertex, sizeof(vertex), "%sattribute vec2 pos; varying vec2 uv;"
        "void main(){uv=pos*0.5+0.5;gl_Position=vec4(pos,0.0,1.0);}", version);
    snprintf(fragment, sizeof(fragment), "%s"
        "uniform sampler2D colorImage, depthImage; uniform vec2 pixel; varying vec2 uv;"
        "float value(vec3 c){return max(c.r,max(c.g,c.b));}"
        "vec3 colorAt(vec2 p){return texture2D(colorImage,clamp(p,pixel*0.5,vec2(1.0)-pixel*0.5)).rgb;}"
        "float depthAt(vec2 p){return texture2D(depthImage,clamp(p,pixel*0.5,vec2(1.0)-pixel*0.5)).r;}"
        "void main(){"
        "vec4 source=texture2D(colorImage,uv);"
        "float v=value(source.rgb);"
        "float steps=v*5.0;"
        "float band=(floor(steps)+smoothstep(0.42,0.58,fract(steps)))/5.0;"
        "vec3 color=source.rgb*(max(band,0.08)/max(v,0.0001));"
        "float z=texture2D(depthImage,uv).r;"
        // Opposing depth differences cancel the gradient of a planar floor.
        // Keep silhouettes even when fine texture detail becomes subpixel.
        "float l=depthAt(uv-vec2(pixel.x,0.0));"
        "float r=depthAt(uv+vec2(pixel.x,0.0));"
        "float b=depthAt(uv-vec2(0.0,pixel.y));"
        "float t=depthAt(uv+vec2(0.0,pixel.y));"
        "float curvature=max(abs(l+r-2.0*z),abs(b+t-2.0*z));"
        "float nearest=min(z,min(min(l,r),min(b,t)));"
        // Respect 24-bit depth precision instead of clamping away distant
        // depth differences. Suppress quantization noise before normalizing.
        "float edge=smoothstep(0.02,0.08,max(curvature-0.00000012,0.0)/max(1.0-nearest,0.00000006));"
        // Opposing samples retain broad color contours (faces, doors, bricks),
        // while a 3-tap average across each side suppresses tiny alternating
        // tiles. A symmetric one-pixel pattern has zero gradient, not black ink.
        "vec2 dx=vec2(pixel.x,0.0),dy=vec2(0.0,pixel.y);"
        "vec3 tl=colorAt(uv-dx+dy),tr=colorAt(uv+dx+dy);"
        "vec3 bl=colorAt(uv-dx-dy),br=colorAt(uv+dx-dy);"
        "vec3 gx=(tr+2.0*colorAt(uv+dx)+br-tl-2.0*colorAt(uv-dx)-bl)*0.25;"
        "vec3 gy=(tl+2.0*colorAt(uv+dy)+tr-bl-2.0*colorAt(uv-dy)-br)*0.25;"
        "float ink=smoothstep(0.08,0.24,max(length(gx),length(gy)));"
        "edge=max(edge,ink);"
        "gl_FragColor=vec4(mix(color,vec3(0.025),edge),source.a);}", version);
    GLuint vs=menu_target_shader(GL_VERTEX_SHADER,vertex);
    GLuint fs=menu_target_shader(GL_FRAGMENT_SHADER,fragment);
    if (!vs || !fs) {
        if(vs) glDeleteShader(vs);
        if(fs) glDeleteShader(fs);
        return false;
    }
    GLuint program=glCreateProgram();
    glAttachShader(program,vs); glAttachShader(program,fs);
    glBindAttribLocation(program,0,"pos"); glLinkProgram(program);
    glDeleteShader(vs); glDeleteShader(fs);
    GLint linked=0; glGetProgramiv(program,GL_LINK_STATUS,&linked);
    if(!linked){glDeleteProgram(program);return false;}
    sCell.program=program;
    glGenBuffers(1,&sCell.buffer);
    glGenFramebuffers(1,&sCell.fbo);
    glGenTextures(1,&sCell.color); glGenTextures(1,&sCell.depth);
    return true;
}

static void gfx_opengl_cell_shaded(void) {
    if(configVrColorFilter!=VR_COLOR_FILTER_CELL_SHADED) return;
#if FOR_WINDOWS
    if(!GLEW_VERSION_3_0) return; // VR contexts provide framebuffer blits.
#endif
    gfx_gl_queue_flush();
    if(!cell_program()) return;
    GLint viewport[4], readFbo, drawFbo, program, buffer, active, textures[2];
    glGetIntegerv(GL_VIEWPORT,viewport);
    if(viewport[2]<=0 || viewport[3]<=0) return;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&readFbo);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&drawFbo);
    glGetIntegerv(GL_CURRENT_PROGRAM,&program);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE,&active);
    for(int i=0;i<2;i++) {glActiveTexture(GL_TEXTURE0+i);glGetIntegerv(GL_TEXTURE_BINDING_2D,&textures[i]);}
    GLboolean depth=glIsEnabled(GL_DEPTH_TEST), blend=glIsEnabled(GL_BLEND);
    GLboolean cull=glIsEnabled(GL_CULL_FACE), scissor=glIsEnabled(GL_SCISSOR_TEST);
    GLboolean mask, colorMask[4];
    glGetBooleanv(GL_DEPTH_WRITEMASK,&mask); glGetBooleanv(GL_COLOR_WRITEMASK,colorMask);
    GLint enabled,size,type,normalized,stride,attribBuffer; void *pointer;
    glGetVertexAttribiv(0,GL_VERTEX_ATTRIB_ARRAY_ENABLED,&enabled);
    glGetVertexAttribiv(0,GL_VERTEX_ATTRIB_ARRAY_SIZE,&size);
    glGetVertexAttribiv(0,GL_VERTEX_ATTRIB_ARRAY_TYPE,&type);
    glGetVertexAttribiv(0,GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,&normalized);
    glGetVertexAttribiv(0,GL_VERTEX_ATTRIB_ARRAY_STRIDE,&stride);
    glGetVertexAttribiv(0,GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,&attribBuffer);
    glGetVertexAttribPointerv(0,GL_VERTEX_ATTRIB_ARRAY_POINTER,&pointer);
    const bool resize=sCell.width!=viewport[2] || sCell.height!=viewport[3];
    for(int i=0;i<2;i++) {
        glActiveTexture(GL_TEXTURE0+i); glBindTexture(GL_TEXTURE_2D,i?sCell.depth:sCell.color);
        if(resize) {
            glTexImage2D(GL_TEXTURE_2D,0,i?GL_DEPTH_COMPONENT24:GL_RGBA8,viewport[2],viewport[3],0,
                i?GL_DEPTH_COMPONENT:GL_RGBA,i?GL_UNSIGNED_INT:GL_UNSIGNED_BYTE,NULL);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        }
    }
    sCell.width=viewport[2];sCell.height=viewport[3];
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,sCell.fbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,sCell.color,0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,sCell.depth,0);
    if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE) {
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER,drawFbo);
        glBlitFramebuffer(viewport[0],viewport[1],viewport[0]+viewport[2],viewport[1]+viewport[3],
            0,0,viewport[2],viewport[3],GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,GL_NEAREST);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,drawFbo);
        glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
        glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
        glUseProgram(sCell.program);
        glUniform1i(glGetUniformLocation(sCell.program,"colorImage"),0);
        glUniform1i(glGetUniformLocation(sCell.program,"depthImage"),1);
        glUniform2f(glGetUniformLocation(sCell.program,"pixel"),1.5f/viewport[2],1.5f/viewport[3]);
        const GLfloat vertices[]={-1,-1,1,-1,1,1,-1,-1,1,1,-1,1};
        glBindBuffer(GL_ARRAY_BUFFER,sCell.buffer);
        glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
        glDrawArrays(GL_TRIANGLES,0,6);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER,readFbo);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,drawFbo);
    glBindBuffer(GL_ARRAY_BUFFER,attribBuffer);
    glVertexAttribPointer(0,size,type,normalized,stride,pointer);
    if(!enabled)glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,buffer);glUseProgram(program);
    for(int i=0;i<2;i++){glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,textures[i]);}
    glActiveTexture(active);
    if(depth)glEnable(GL_DEPTH_TEST);
    if(blend)glEnable(GL_BLEND);
    if(cull)glEnable(GL_CULL_FACE);
    if(scissor)glEnable(GL_SCISSOR_TEST);
    glDepthMask(mask);glColorMask(colorMask[0],colorMask[1],colorMask[2],colorMask[3]);
}
static void gfx_opengl_destroy_cell_shaded(void) {
    if(sCell.program)glDeleteProgram(sCell.program);
    if(sCell.buffer)glDeleteBuffers(1,&sCell.buffer);
    if(sCell.fbo)glDeleteFramebuffers(1,&sCell.fbo);
    if(sCell.color)glDeleteTextures(1,&sCell.color);
    if(sCell.depth)glDeleteTextures(1,&sCell.depth);
    memset(&sCell,0,sizeof(sCell));
}
#else
static void gfx_opengl_cell_shaded(void) {}
static void gfx_opengl_destroy_cell_shaded(void) {}
#endif
#endif
