/* Out-of-memory diagnostics: report the first few failed heap allocations (size, heap usage) on stdout.
 * Linked with -Wl,--wrap=_malloc_r,--wrap=_realloc_r,--wrap=_calloc_r. */
#include <malloc.h>
#include <reent.h>
#include <stdio.h>

void *__real__malloc_r(struct _reent *, size_t);
void *__real__realloc_r(struct _reent *, void *, size_t);
void *__real__calloc_r(struct _reent *, size_t, size_t);

static int reports;
static void report(const char *what, size_t n) {
    if (reports >= 8) return;
    reports++;
    struct mallinfo mi = mallinfo();
    printf("[memory] %s of %u bytes failed: heap %u KB, in use %u KB, free in heap %u KB\n", what, (unsigned)n,
           (unsigned)mi.arena / 1024, (unsigned)mi.uordblks / 1024, (unsigned)mi.fordblks / 1024);
}
void *__wrap__malloc_r(struct _reent *r, size_t n) {
    void *p = __real__malloc_r(r, n);
    if (!p && n) report("malloc", n);
    return p;
}
void *__wrap__realloc_r(struct _reent *r, void *o, size_t n) {
    void *p = __real__realloc_r(r, o, n);
    if (!p && n) report("realloc", n);
    return p;
}
void *__wrap__calloc_r(struct _reent *r, size_t a, size_t b) {
    void *p = __real__calloc_r(r, a, b);
    if (!p && a && b) report("calloc", a * b);
    return p;
}

#ifdef KO_PSP_MEMTRACE
/* Development aid: log every big allocation with its caller (address relative to ko_mem_anchor). */
#include <stdlib.h>
void *__real_malloc(size_t); void *__real_calloc(size_t, size_t); void *__real_realloc(void *, size_t);
void ko_mem_anchor(void) {}
#define BIG (KO_PSP_MEMTRACE * 1024)
#define LOG(what, n) printf("[memtrace] %s %u KB from anchor%+d\n", what, (unsigned)(n) / 1024, (int)((char *)__builtin_return_address(0) - (char *)ko_mem_anchor))
void *__wrap_malloc(size_t n) { if (n >= BIG) LOG("malloc", n); return __real_malloc(n); }
void *__wrap_calloc(size_t a, size_t b) { if (a * b >= BIG) LOG("calloc", a * b); return __real_calloc(a, b); }
void *__wrap_realloc(void *p, size_t n) { if (n >= BIG) LOG("realloc", n); return __real_realloc(p, n); }
#endif
