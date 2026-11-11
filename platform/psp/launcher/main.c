/* KOReader (v2018.12) launcher for the PSP: boots the Lua frontend on a static LuaJIT with SDL2.
 *
 *  - ffi.C / ffi.load resolve against `lj_static_symtab` (generated at link time) and, for the document engines,
 *    against PRX modules loaded on demand (ko_modules.c).
 *  - `require("libs/libkoreader-xxx")` and other C modules are served from statically linked luaopen_* functions
 *    or from a module's (Lua's hyphen convention: the name up to the first '-' is dropped).
 */
#include <SDL.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pspmoduleinfo.h>
#include <pspsysmem.h>
#include "ko_modules.h"

/* The C heap takes all free memory except a reserve, which is what the PRX modules (code + data; their malloc()
 * calls land in this program's heap) and thread stacks are loaded into.
 *  - 24 MB models (PSP-1000): room for one document engine at a time (MuPDF is the biggest, ~3.3 MB).
 *  - 52 MB models (PSP-2000 and later, PSP Go): room for all of them.
 * heap.c creates the heap on the first allocation and picks the reserve then. */
#ifndef KO_PSP_MODULE_RESERVE_KB
#define KO_PSP_MODULE_RESERVE_KB 2560
#endif
#ifndef KO_PSP_MODULE_RESERVE_LARGE_KB
#define KO_PSP_MODULE_RESERVE_LARGE_KB 10240
#endif
int ko_free_at_boot_kb, ko_module_reserve_kb;   /* set by _sbrk() in heap.c */
PSP_MAIN_THREAD_STACK_SIZE_KB(512);

struct psp_luaopen { const char *name; int (*fn)(void *); };
extern const struct psp_luaopen psp_luaopen_table[];

static lua_CFunction find_luaopen(const char *modname, const char *sym) {
    for (const struct psp_luaopen *e = psp_luaopen_table; e->name; e++)
        if (strcmp(e->name, sym) == 0) return (lua_CFunction)e->fn;
    struct ko_module *m = ko_module_for_library(modname);
    return m ? (lua_CFunction)ko_module_symbol(m, sym) : NULL;
}

static int psp_static_loader(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    const char *dash = strchr(name, '-');
    char sym[160] = "luaopen_";
    strncat(sym, dash ? dash + 1 : name, sizeof sym - 16);
    for (char *p = sym; *p; p++) if (*p == '.' || *p == '/') *p = '_';
    lua_CFunction fn = find_luaopen(name, sym);
    if (fn) { lua_pushcfunction(L, fn); return 1; }
    const char *err = ko_modules_last_error();
    lua_pushfstring(L, "\n\tno static module or PRX for '%s' (%s)%s%s", name, sym, err[0] ? ": " : "", err);
    return 1;
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
    printf("KOReader 2018.12 PSP launcher: cwd from '%s'; %d KB free at boot, %d KB kept for modules\n", dir,
           ko_free_at_boot_kb, ko_module_reserve_kb);
    setenv("HOME", dir[0] ? dir : ".", 0);
    setenv("TZ", "UTC", 0);
    setenv("EMULATE_READER_W", "480", 0);   /* the PSP screen */
    setenv("EMULATE_READER_H", "272", 0);
    setenv("KO_LOW_MEMORY", "1", 0);             /* small CREngine in-memory caches */
    setenv("KO_LIGHT_IMAGE_DECODERS", "1", 0);   /* UI icons without loading the MuPDF module */
    { char mdir[300]; snprintf(mdir, sizeof mdir, "%smodules/", dir); ko_modules_set_dir(mdir); }
#ifdef KO_PSP_SCREENSHOTS
    ko_start_screenshots(KO_PSP_SCREENSHOTS_FIRST_MS, KO_PSP_SCREENSHOTS_INTERVAL_MS, KO_PSP_SCREENSHOTS_COUNT);
#endif
    lua_State *L = luaL_newstate();
    if (!L) { printf("KOREADER-PSP FATAL: luaL_newstate failed\n"); return 1; }
    luaL_openlibs(L);

    lua_getglobal(L, "package");
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
#endif
    if (k == 1) { lua_pushstring(L, "."); lua_rawseti(L, -2, k++); }   /* no argument: browse the KOReader folder */
    lua_setglobal(L, "arg");

    lua_getglobal(L, "debug"); lua_getfield(L, -1, "traceback"); lua_remove(L, -2);
    int tb = lua_gettop(L);
#ifdef KO_PSP_SCRIPT
    if (luaL_loadfile(L, KO_PSP_SCRIPT) != 0) {
#else
    if (luaL_loadfile(L, "reader.lua") != 0) {
#endif
        printf("KOREADER-PSP FATAL: %s\n", lua_tostring(L, -1)); fflush(stdout); return 1;
    }
    if (lua_pcall(L, 0, 0, tb) != 0) { printf("KOREADER-PSP FATAL: %s\n", lua_tostring(L, -1)); fflush(stdout); return 2; }
    lua_close(L);
    return 0;
}
