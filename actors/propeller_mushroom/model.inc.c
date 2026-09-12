ALIGNED8 const Texture vr_propeller_mushroom_texture[] = {
#include "actors/propeller_mushroom/propeller.rgba16.inc.c"
};
static const Vtx vr_propeller_mushroom_vtx[] = {
    {{{-32, 0, 0}, 0, {0, 992}, {255,255,255,255}}},
    {{{ 32, 0, 0}, 0, {992, 992}, {255,255,255,255}}},
    {{{ 32,64, 0}, 0, {992, 0}, {255,255,255,255}}},
    {{{-32,64, 0}, 0, {0, 0}, {255,255,255,255}}},
};
const Gfx vr_propeller_mushroom_dl[] = {
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_DECALRGBA),
    gsSPClearGeometryMode(G_LIGHTING | G_CULL_BACK | G_CULL_FRONT),
    gsSPTexture(0xFFFF,0xFFFF,0,G_TX_RENDERTILE,G_ON),
    gsDPLoadTextureBlock(vr_propeller_mushroom_texture,G_IM_FMT_RGBA,G_IM_SIZ_16b,
        32,32,0,G_TX_CLAMP,G_TX_CLAMP,5,5,G_TX_NOLOD,G_TX_NOLOD),
    gsSPVertex(vr_propeller_mushroom_vtx,4,0),
    gsSP2Triangles(0,1,2,0,0,2,3,0),
    gsSPTexture(0xFFFF,0xFFFF,0,G_TX_RENDERTILE,G_OFF),
    gsSPSetGeometryMode(G_LIGHTING | G_CULL_BACK),
    gsDPSetCombineMode(G_CC_SHADE,G_CC_SHADE),
    gsSPEndDisplayList(),
};
