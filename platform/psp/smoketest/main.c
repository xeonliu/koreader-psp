/* Minimal PSP smoke test for the KOReader port:
 * SDL3 video (480x272) + SDL3 gamepad input + an embedded Lua 5.4 interpreter.
 * It is NOT KOReader: it only checks the toolchain, output, input and EBOOT packaging.
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>

/* NOTE: SDL3's PSP backend (SDL_main) already defines PSP_MODULE_INFO and the main thread attributes. */

static SDL_Renderer *R;

static int l_color(lua_State *L) {
    SDL_SetRenderDrawColor(R, luaL_checkinteger(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), 255);
    return 0;
}
static int l_clear(lua_State *L) { (void)L; SDL_RenderClear(R); return 0; }
static int l_rect(lua_State *L) {
    SDL_FRect r = { luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4) };
    SDL_RenderFillRect(R, &r);
    return 0;
}
static int l_text(lua_State *L) {
    SDL_RenderDebugText(R, luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checkstring(L, 3));
    return 0;
}
static int l_present(lua_State *L) { (void)L; SDL_RenderPresent(R); return 0; }

/* Returns a table of pressed gamepad buttons, plus the left stick. */
static SDL_Gamepad *pad;
static int l_input(lua_State *L) {
    static const struct { const char *n; SDL_GamepadButton b; } map[] = {
        { "cross", SDL_GAMEPAD_BUTTON_SOUTH }, { "circle", SDL_GAMEPAD_BUTTON_EAST },
        { "square", SDL_GAMEPAD_BUTTON_WEST }, { "triangle", SDL_GAMEPAD_BUTTON_NORTH },
        { "l", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER }, { "r", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER },
        { "up", SDL_GAMEPAD_BUTTON_DPAD_UP }, { "down", SDL_GAMEPAD_BUTTON_DPAD_DOWN },
        { "left", SDL_GAMEPAD_BUTTON_DPAD_LEFT }, { "right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT },
        { "start", SDL_GAMEPAD_BUTTON_START }, { "select", SDL_GAMEPAD_BUTTON_BACK },
    };
    lua_newtable(L);
    if (!pad) return 1;
    for (size_t i = 0; i < sizeof map / sizeof map[0]; i++) {
        lua_pushboolean(L, SDL_GetGamepadButton(pad, map[i].b));
        lua_setfield(L, -2, map[i].n);
    }
    lua_pushnumber(L, SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX) / 32768.0); lua_setfield(L, -2, "lx");
    lua_pushnumber(L, SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY) / 32768.0); lua_setfield(L, -2, "ly");
    return 1;
}
static int l_ticks(lua_State *L) { lua_pushinteger(L, (lua_Integer)SDL_GetTicks()); return 1; }
static int l_mem(lua_State *L) { lua_pushinteger(L, lua_gc(L, LUA_GCCOUNT, 0)); return 1; }

static const luaL_Reg psp_lib[] = {
    { "color", l_color }, { "clear", l_clear }, { "rect", l_rect }, { "text", l_text },
    { "present", l_present }, { "input", l_input }, { "ticks", l_ticks }, { "lua_kb", l_mem },
    { NULL, NULL }
};

/* The Lua script is embedded so the EBOOT is self-contained. */
extern const char smoketest_lua[];

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) return 1;
    SDL_Window *win = SDL_CreateWindow("KOReader PSP", 480, 272, 0);
    R = SDL_CreateRenderer(win, NULL);
    int n = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&n);
    if (ids && n > 0) pad = SDL_OpenGamepad(ids[0]);
    SDL_free(ids);

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaL_newlib(L, psp_lib);
    lua_setglobal(L, "psp");
    if (luaL_dostring(L, smoketest_lua) != LUA_OK) {
        /* Show the error on screen instead of silently exiting. */
        const char *err = lua_tostring(L, -1);
        for (int i = 0; i < 300; i++) {
            SDL_SetRenderDrawColor(R, 90, 0, 0, 255); SDL_RenderClear(R);
            SDL_SetRenderDrawColor(R, 255, 255, 255, 255);
            SDL_RenderDebugText(R, 4, 4, "Lua error:");
            SDL_RenderDebugText(R, 4, 20, err ? err : "?");
            SDL_RenderPresent(R);
            SDL_Delay(16);
        }
    }
    lua_close(L);
    SDL_Quit();
    return 0;
}
