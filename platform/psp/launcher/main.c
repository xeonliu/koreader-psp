/* KOReader launcher for the PSP: boots KOReader's own Lua (reader.lua) on a static LuaJIT.
 *
 *  - LuaJIT's ffi.C / ffi.load resolve against `lj_static_symtab` (generated at link time by koreader-base's
 *    utils/gen-psp-symtab.py); there is no dynamic loader on the PSP.
 *  - package.loadlib() is replaced by a lookup in the statically linked `luaopen_*` entry points.
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct psp_luaopen { const char *name; int (*fn)(void *); };
extern const struct psp_luaopen psp_luaopen_table[];

static int psp_loadlib(lua_State *L) {
    const char *sym = luaL_checkstring(L, 2);
    for (const struct psp_luaopen *e = psp_luaopen_table; e->name; e++) {
        if (strcmp(e->name, sym) == 0) {
            lua_pushcfunction(L, (lua_CFunction)e->fn);
            return 1;
        }
    }
    lua_pushnil(L);
    lua_pushfstring(L, "static module entry point not found: %s", sym);
    lua_pushliteral(L, "init");
    return 3;
}

#ifdef KO_PSP_SCREENSHOTS
void ko_start_screenshots(int first, int interval, int n);
#endif

static void show_error(const char *msg) {
    printf("KOREADER-PSP FATAL: %s\n", msg);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    /* Run from the directory holding reader.lua: where the EBOOT lives (host0:/ in PPSSPP headless). */
    char dir[256] = "";
    if (argc > 0 && argv[0]) {
        strncpy(dir, argv[0], sizeof dir - 1);
        char *slash = strrchr(dir, '/');
        if (slash) { slash[1] = '\0'; chdir(dir); }
    }
    printf("KOReader PSP launcher: cwd from '%s'\n", dir);
    /* The PSP has no environment: provide what KOReader expects. */
    setenv("HOME", dir[0] ? dir : ".", 0);
    setenv("TZ", "UTC", 0);

#ifdef KO_PSP_SCREENSHOTS
    ko_start_screenshots(KO_PSP_SCREENSHOTS_FIRST_MS, KO_PSP_SCREENSHOTS_INTERVAL_MS, KO_PSP_SCREENSHOTS_COUNT);
#endif
    lua_State *L = luaL_newstate();
    if (!L) { show_error("luaL_newstate failed"); return 1; }
    luaL_openlibs(L);

    /* package.loadlib -> static luaopen_* table. */
    lua_getglobal(L, "package");
    lua_pushcfunction(L, psp_loadlib);
    lua_setfield(L, -2, "loadlib");
    lua_pop(L, 1);

    /* arg = { [0] = "reader.lua", ... } */
    lua_newtable(L);
    lua_pushstring(L, "reader.lua"); lua_rawseti(L, -2, 0);
    for (int i = 1; i < argc; i++) { lua_pushstring(L, argv[i]); lua_rawseti(L, -2, i); }
#ifdef KO_PSP_ARG1
    lua_pushstring(L, KO_PSP_ARG1); lua_rawseti(L, -2, argc);   /* e.g. "-d" for debug logging */
#define KO_PSP_NEXT (argc + 1)
#else
#define KO_PSP_NEXT argc
#endif
#ifdef KO_PSP_OPEN
    /* Development aid: open this document at start-up (as if given on the command line). */
    lua_pushstring(L, KO_PSP_OPEN); lua_rawseti(L, -2, KO_PSP_NEXT);
#endif
    lua_setglobal(L, "arg");

    /* Traceback on error. */
    lua_getglobal(L, "debug"); lua_getfield(L, -1, "traceback"); lua_remove(L, -2);
    int tb = lua_gettop(L);
    #ifdef KO_PSP_SCRIPT
    if (luaL_loadfile(L, KO_PSP_SCRIPT) != 0) {   /* Development aid: run a test script instead of reader.lua. */
#else
    if (luaL_loadfile(L, "reader.lua") != 0) {
#endif
        show_error(lua_tostring(L, -1));
        return 1;
    }
    if (lua_pcall(L, 0, 0, tb) != 0) {
        show_error(lua_tostring(L, -1));
        return 2;
    }
    lua_close(L);
    return 0;
}
