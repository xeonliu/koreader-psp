/* Console-only PSP test: runs test.lua on LuaJIT (interpreter only, JIT disabled) and prints the results.
 * Build: make STAGING=/path/to/koreader-base/build/psp/staging  (after `make TARGET=psp` builds luajit there)
 * Output goes to stdout (PPSSPP headless prints it; on a PSP use PSPLink or look at the debug screen).
 */
#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

PSP_MODULE_INFO("LuaJITTest", PSP_MODULE_USER, 0, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);

extern const char test_lua[];

/* Static symbol table for ffi.C / ffi.load (see LuaJIT's lj_clib.c PSP branch). Must be sorted in strcmp() order. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* Float-argument test functions (hard-float single EABI: floats in FPRs). */
float ko_fmix(int a, float b, int c, float d) { return (float)a + b * 2.0f + (float)c * 4.0f + d * 8.0f; }
double ko_dmix(int a, double b) { return (double)a + b; }
struct lj_static_sym { const char *name; void *addr; };
const struct lj_static_sym lj_static_symtab[] = {
    { "abs", (void *)abs },
    { "atoi", (void *)atoi },
    { "ko_dmix", (void *)ko_dmix },
    { "ko_fmix", (void *)ko_fmix },
    { "snprintf", (void *)snprintf },
    { "strlen", (void *)strlen },
    { NULL, NULL }
};

int main(void) {
    printf("LuaJIT PSP test starting\n");
    lua_State *L = luaL_newstate();
    if (!L) { printf("luaL_newstate failed\n"); sceKernelExitGame(); return 1; }
    luaL_openlibs(L);
    int rc = luaL_dostring(L, test_lua);
    if (rc) printf("LUA ERROR: %s\n", lua_tostring(L, -1));
    printf("LuaJIT PSP test finished rc=%d\n", rc);
    lua_close(L);
    sceKernelExitGame();
    return 0;
}
