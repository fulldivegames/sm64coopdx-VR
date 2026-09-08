#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "../src/pc/djui/djui_hud_bounds.h"
static int drawn, rejected;
static int draw(lua_State *L) {
    const char *name = lua_tostring(L, 1);
    float h = strstr(name, "left") || strstr(name, "right") ? 64 : 32;
    float x = lua_tonumber(L,2), y = lua_tonumber(L,3);
    if (djui_hud_rect_outside(x,y,32*lua_tonumber(L,4),h*lua_tonumber(L,5),320,240)) rejected++;
    else drawn++;
    return 0;
}
static void run(lua_State *L, const char *code) {
    if (luaL_dostring(L,code)) { fprintf(stderr,"%s\n",lua_tostring(L,-1)); assert(0); }
}
int main(int argc, char **argv) {
    assert(argc==2);
    lua_State *L=luaL_newstate(); luaL_openlibs(L);
    lua_pushcfunction(L,draw); lua_setglobal(L,"djui_hud_render_texture");
    run(L,"CT_MARIO=0 CT_LUIGI=1 CT_TOAD=2 CT_WALUIGI=3 CT_WARIO=4 "
        "gTextures={} gMarioStates={[0]={health=2176,action=0}} "
        "ACT_GROUP_MASK=448 ACT_GROUP_SUBMERGED=192 "
        "function hook_event(...) end function hook_chat_command(...) end "
        "function get_texture_info(n) return n end function djui_hud_set_color(...) end");
    if(luaL_dofile(L,argv[1])) { fprintf(stderr,"%s\n",lua_tostring(L,-1)); return 1; }
    run(L,"for i=1,10 do render_power_meter(109,11,1,1) end");
    drawn=rejected=0;
    run(L,"render_power_meter(109,11,1,1)");
    assert(drawn==0 && rejected==3);
    drawn=rejected=0;
    run(L,"gMarioStates[0].health=1024 render_power_meter(109,11,1,1)");
    assert(drawn==3 && rejected==0);
    run(L,"gMarioStates[0].health=2176 for i=1,150 do render_power_meter(109,11,1,1) end");
    drawn=rejected=0;
    run(L,"render_power_meter(109,11,1,1)");
    assert(drawn==0 && rejected==3);
    drawn=rejected=0;
    run(L,"gMarioStates[0].action=192 render_power_meter(109,11,1,1)");
    assert(drawn==3 && rejected==0);
    lua_close(L);
    puts("PASS: installed B3313 Lua meter hides, reappears on damage, heals/hides, reappears underwater");
}
