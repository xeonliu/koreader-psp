/* Minimal PSP smoke test for the KOReader port:
 * SDL3 video (480x272) + SDL3 gamepad input + an embedded Lua 5.4 interpreter.
 * It is NOT KOReader: it only checks the toolchain, output, input and EBOOT packaging.
 */
#include <SDL3/SDL.h>
#ifdef SMOKE_AUTOEXIT_MS
#include <pspdisplay.h>
#include <stdio.h>
#include <stdint.h>
/* Debug aid for automated screenshots: dump the PSP display frame buffer as a 24-bit BMP. */
static void dump_framebuffer(const char *path) {
    void *top = NULL; int bw = 0, fmt = 0;
    sceDisplayGetFrameBuf(&top, &bw, &fmt, PSP_DISPLAY_SETBUF_IMMEDIATE);
    if (!top) return;
    FILE *f = fopen(path, "wb"); if (!f) return;
    const int W = 480, H = 272, rowsz = W * 3;
    uint32_t filesz = 54 + rowsz * H;
    uint8_t hdr[54] = { 'B','M' }; hdr[2]=filesz; hdr[3]=filesz>>8; hdr[4]=filesz>>16; hdr[10]=54; hdr[14]=40;
    hdr[18]=W&255; hdr[19]=W>>8; hdr[22]=H&255; hdr[23]=H>>8; hdr[26]=1; hdr[28]=24; hdr[34]=rowsz*H; hdr[35]=(rowsz*H)>>8; hdr[36]=(rowsz*H)>>16;
    fwrite(hdr, 1, 54, f);
    for (int y = H - 1; y >= 0; y--) {
        uint8_t row[480 * 3];
        for (int x = 0; x < W; x++) {
            uint8_t r, g, b;
            if (fmt == PSP_DISPLAY_PIXEL_FORMAT_8888) { uint32_t p = ((uint32_t *)top)[y * bw + x]; r = p; g = p >> 8; b = p >> 16; }
            else { uint16_t p = ((uint16_t *)top)[y * bw + x];
                   if (fmt == PSP_DISPLAY_PIXEL_FORMAT_565) { r = (p & 31) << 3; g = ((p >> 5) & 63) << 2; b = ((p >> 11) & 31) << 3; }
                   else if (fmt == PSP_DISPLAY_PIXEL_FORMAT_5551) { r = (p & 31) << 3; g = ((p >> 5) & 31) << 3; b = ((p >> 10) & 31) << 3; }
                   else { r = (p & 15) << 4; g = ((p >> 4) & 15) << 4; b = ((p >> 8) & 15) << 4; } }
            row[x*3] = b; row[x*3+1] = g; row[x*3+2] = r;
        }
        fwrite(row, 1, rowsz, f);
    }
    fclose(f);
}
#endif
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
#ifdef SMOKE_AUTOEXIT_MS
    /* For automated screenshots: tell the script to quit by itself after this many ms. */
    lua_pushinteger(L, SMOKE_AUTOEXIT_MS); lua_setglobal(L, "AUTOEXIT_MS");
#endif
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
#ifdef SMOKE_AUTOEXIT_MS
    dump_framebuffer("host0:/smoketest_screen.bmp");
#endif
    lua_close(L);
    SDL_Quit();
    return 0;
}
