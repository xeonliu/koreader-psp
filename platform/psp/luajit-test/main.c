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
