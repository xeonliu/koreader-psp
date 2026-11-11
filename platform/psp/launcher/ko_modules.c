/* Main-program side of KOReader's PSP modules (see koreader-base toolchain/psp/prx/ko_prx.h).
 *
 * LuaJIT calls lj_psp_loadlib() for ffi.load("libs/libfoo.so") and lj_psp_getsym() for symbol lookups. Library names
 * listed in `libmap` live in PRX modules under modules/ and are loaded (with their dependencies) on first use;
 * everything else is linked into the main program and resolved from its static symbol table.
 */
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include "ko_prx.h"
#include "ko_modules.h"

extern const struct ko_sym lj_static_symtab[];   /* generated: sorted, NULL-name terminated */

#define MAX_MODULES 8
struct ko_module { char name[24]; SceUID id; const struct ko_sym *syms; int nsyms; };
static struct ko_module modules[MAX_MODULES];
static int nmodules;
static char module_dir[256] = "modules/";
static char last_error[256];

/* library name (without "lib" prefix and ".so..." suffix) -> module, and the modules that one needs first. */
static const struct { const char *lib, *module, *deps; } libmap[] = {
#include "ko_libmap.inc"
    { 0, 0, 0 }
};

void ko_modules_set_dir(const char *dir) { snprintf(module_dir, sizeof module_dir, "%s", dir); }
const char *ko_modules_last_error(void) { return last_error; }

static void *table_lookup(const struct ko_sym *tab, int n, const char *name) {
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = (lo + hi) >> 1, c = strcmp(name, tab[mid].name);
        if (c == 0) return tab[mid].addr;
        if (c < 0) hi = mid - 1; else lo = mid + 1;
    }
    return NULL;
}

static int main_nsyms = -1;
void *ko_resolve(const char *name) {
    if (main_nsyms < 0) { int n = 0; while (lj_static_symtab[n].name) n++; main_nsyms = n; }
    void *p = table_lookup(lj_static_symtab, main_nsyms, name);
    for (int i = 0; !p && i < nmodules; i++) p = table_lookup(modules[i].syms, modules[i].nsyms, name);
    return p;
}

static struct ko_module *find_module(const char *name) {
    for (int i = 0; i < nmodules; i++) if (strcmp(modules[i].name, name) == 0) return &modules[i];
    return NULL;
}

struct ko_module *ko_module_load(const char *name) {
    struct ko_module *m = find_module(name);
    if (m) return m;
    if (nmodules >= MAX_MODULES) { snprintf(last_error, sizeof last_error, "too many modules"); return NULL; }
    /* Dependencies first. */
    for (int i = 0; libmap[i].lib; i++) {
        if (!libmap[i].module || strcmp(libmap[i].module, name) != 0 || !libmap[i].deps) continue;
        char deps[96]; snprintf(deps, sizeof deps, "%s", libmap[i].deps);
        for (char *d = strtok(deps, ","); d; d = strtok(NULL, ",")) if (!ko_module_load(d)) return NULL;
        break;
    }
    char path[320];
    snprintf(path, sizeof path, "%s%s.prx", module_dir, name);
    printf("[modules] loading %s: %d KB free outside the heap (largest block %d KB)\n", path,
           (int)(sceKernelTotalFreeMemSize() / 1024), (int)(sceKernelMaxFreeMemSize() / 1024));
    SceUID id = sceKernelLoadModule(path, 0, NULL);
    if (id < 0) {
        snprintf(last_error, sizeof last_error, "cannot load %s (0x%08x%s)", path, (unsigned)id,
                 (unsigned)id == 0x800200d9u ? ", not enough memory left for modules" : "");
        return NULL;
    }
    struct ko_module_link link = { KO_PRX_MAGIC, ko_resolve, NULL, 0, NULL };
    struct ko_module_link *linkp = &link;
    int status = -1;
    int rc = sceKernelStartModule(id, sizeof linkp, &linkp, &status, NULL);
    if (rc < 0 || status != 0 || !link.exports) {
        snprintf(last_error, sizeof last_error, "cannot start %s: %s%s (rc 0x%08x, status %d)", path,
                 link.error ? "missing symbol " : "", link.error ? link.error : "", (unsigned)rc, status);
        sceKernelUnloadModule(id);
        return NULL;
    }
    m = &modules[nmodules++];
    snprintf(m->name, sizeof m->name, "%s", name);
    m->id = id; m->syms = link.exports; m->nsyms = link.nexports;
    printf("[modules] loaded %s (%d symbols)\n", path, m->nsyms);
    return m;
}

/* "libs/libkoreader-cre.so" / "libs/libmupdf.so.3" / "koreader-cre" -> index in libmap, or -1. */
static int libmap_find(const char *libname) {
    const char *base = strrchr(libname, '/'); base = base ? base + 1 : libname;
    if (strncmp(base, "lib", 3) == 0) base += 3;
    size_t len = strcspn(base, ".");
    for (int i = 0; libmap[i].lib; i++)
        if (strlen(libmap[i].lib) == len && strncmp(libmap[i].lib, base, len) == 0) return i;
    return -1;
}

struct ko_module *ko_module_for_library(const char *libname) {
    int i = libmap_find(libname);
    if (i < 0 || !libmap[i].module) { last_error[0] = '\0'; return NULL; }
    return ko_module_load(libmap[i].module);
}

void *ko_module_symbol(struct ko_module *m, const char *name) { return table_lookup(m->syms, m->nsyms, name); }

/* --- LuaJIT hooks (lj_clib.c, PSP branch) -------------------------------------------------------------------------- */
#define STATIC_HANDLE ((void *)(long)1)

void *lj_psp_loadlib(const char *name, const char **err) {
    int i = libmap_find(name);
    if (i < 0) { snprintf(last_error, sizeof last_error, "no such library on the PSP"); *err = last_error; return NULL; }
    if (!libmap[i].module) return STATIC_HANDLE;               /* linked into the main program */
    struct ko_module *m = ko_module_load(libmap[i].module);
    if (!m) { *err = last_error; return NULL; }
    return m;
}

void *lj_psp_getsym(void *handle, const char *name) {
    if (handle >= (void *)modules && handle < (void *)(modules + MAX_MODULES))
        return ko_module_symbol((struct ko_module *)handle, name);
    return NULL;   /* ffi.C or a static library: LuaJIT falls back to lj_static_symtab */
}
