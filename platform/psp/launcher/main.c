/* KOReader (v2014.11) launcher for the PSP: boots the old Lua frontend on a static LuaJIT with SDL 1.2.
 *  - LuaJIT's ffi.C / ffi.load resolve against `lj_static_symtab` (generated at link time by utils/gen-psp-symtab.py).
 *  - `require("libs/libkoreader-xxx")` and friends are served from the statically linked luaopen_* functions
 *    (Lua's hyphen convention: the module name up to the first '-' is dropped).
 */
#include <SDL.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pspkernel.h>

struct psp_luaopen { const char *name; int (*fn)(void *); };
extern const struct psp_luaopen psp_luaopen_table[];

static int psp_static_loader(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    const char *dash = strchr(name, '-');
    char sym[160] = "luaopen_";
    strncat(sym, dash ? dash + 1 : name, sizeof sym - 16);
    for (char *p = sym; *p; p++) if (*p == '.' || *p == '/') *p = '_';
    for (const struct psp_luaopen *e = psp_luaopen_table; e->name; e++) {
        if (strcmp(e->name, sym) == 0) { lua_pushcfunction(L, (lua_CFunction)e->fn); return 1; }
    }
    lua_pushfstring(L, "\n\tno static module '%s' (%s)", name, sym);
    return 1;
}

static int psp_loadlib(lua_State *L) {
    const char *sym = luaL_checkstring(L, 2);
    for (const struct psp_luaopen *e = psp_luaopen_table; e->name; e++)
        if (strcmp(e->name, sym) == 0) { lua_pushcfunction(L, (lua_CFunction)e->fn); return 1; }
    lua_pushnil(L); lua_pushfstring(L, "static module entry point not found: %s", sym); lua_pushliteral(L, "init");
    return 3;
}

/* stdout/stderr -> the PSP console handle (PPSSPP forwards it to its log, PSPLink shows it on a real device).
 * SDL 1.2's startup code would otherwise send it to the debug screen. Linked with -Wl,--wrap=write. */
extern int __real_write(int fd, const void *buf, unsigned n);
int __wrap_write(int fd, const void *buf, unsigned n) {
    if (fd == 1 || fd == 2) { sceIoWrite(sceKernelStdout(), buf, n); return (int)n; }
    return __real_write(fd, buf, n);
}

#ifdef KO_PSP_SCREENSHOTS
void ko_start_screenshots(int first, int interval, int n);
#endif

int main(int argc, char *argv[]) {
    char dir[256] = "";
    if (argc > 0 && argv[0]) {
        strncpy(dir, argv[0], sizeof dir - 1);
        char *slash = strrchr(dir, '/');
        if (slash) { slash[1] = '\0'; chdir(dir); }
    }
    printf("KOReader 2014.11 PSP launcher: cwd from '%s'\n", dir);
    setenv("HOME", dir[0] ? dir : ".", 0);
    setenv("TZ", "UTC", 0);
    setenv("EMULATE_READER_W", "480", 0);   /* the PSP screen */
    setenv("EMULATE_READER_H", "272", 0);
#ifdef KO_PSP_SCREENSHOTS
    ko_start_screenshots(KO_PSP_SCREENSHOTS_FIRST_MS, KO_PSP_SCREENSHOTS_INTERVAL_MS, KO_PSP_SCREENSHOTS_COUNT);
#endif
    lua_State *L = luaL_newstate();
    if (!L) { printf("KOREADER-PSP FATAL: luaL_newstate failed\n"); return 1; }
    luaL_openlibs(L);

    lua_getglobal(L, "package");
    lua_pushcfunction(L, psp_loadlib); lua_setfield(L, -2, "loadlib");
    lua_getfield(L, -1, "loaders");                 /* insert our loader right after package.preload */
    int n = (int)lua_objlen(L, -1);
    for (int i = n; i >= 2; i--) { lua_rawgeti(L, -1, i); lua_rawseti(L, -2, i + 1); }
    lua_pushcfunction(L, psp_static_loader); lua_rawseti(L, -2, 2);
    lua_pop(L, 2);

    lua_newtable(L);
    lua_pushstring(L, "reader.lua"); lua_rawseti(L, -2, 0);
    int k = 1;
    for (int i = 1; i < argc; i++) { lua_pushstring(L, argv[i]); lua_rawseti(L, -2, k++); }
#ifdef KO_PSP_ARG1
    lua_pushstring(L, KO_PSP_ARG1); lua_rawseti(L, -2, k++);
#endif
#ifdef KO_PSP_OPEN
    lua_pushstring(L, KO_PSP_OPEN); lua_rawseti(L, -2, k++);
#else
    lua_pushstring(L, "."); lua_rawseti(L, -2, k++);      /* a directory: open the file manager */
#endif
    lua_setglobal(L, "arg");

    lua_getglobal(L, "debug"); lua_getfield(L, -1, "traceback"); lua_remove(L, -2);
    int tb = lua_gettop(L);
#ifdef KO_PSP_SCRIPT
    if (luaL_loadfile(L, KO_PSP_SCRIPT) != 0) {
#else
    if (luaL_loadfile(L, "reader.lua") != 0) {
#endif
        printf("KOREADER-PSP FATAL: %s\n", lua_tostring(L, -1)); fflush(stdout); sceKernelDelayThread(3000000); return 1;
    }
    if (lua_pcall(L, 0, 0, tb) != 0) { printf("KOREADER-PSP FATAL: %s\n", lua_tostring(L, -1)); fflush(stdout); sceKernelDelayThread(3000000); return 2; }
    lua_close(L);
    return 0;
}
