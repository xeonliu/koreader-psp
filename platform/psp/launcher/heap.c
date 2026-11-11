/* The C heap of the launcher: replaces pspsdk's _sbrk() so that the memory left outside the heap (for PRX modules
 * and thread stacks) can depend on how much memory the PSP model has. See main.c. */
#include <pspsysmem.h>
#include <errno.h>
#include <stddef.h>

#ifndef KO_PSP_MODULE_RESERVE_KB
#define KO_PSP_MODULE_RESERVE_KB 2560   /* 24 MB: cre.prx fits; mupdf.prx needs the 52 MB profile */
#endif
#ifndef KO_PSP_MODULE_RESERVE_LARGE_KB
#define KO_PSP_MODULE_RESERVE_LARGE_KB 10240
#endif

extern SceUID __psp_heap_blockid;   /* pspsdk frees this block at exit */
extern int ko_free_at_boot_kb, ko_module_reserve_kb;
static char *heap_bottom, *heap_top, *heap_ptr;

void *_sbrk(ptrdiff_t incr) {
    if (!heap_bottom) {
        SceSize total = sceKernelTotalFreeMemSize(), largest = sceKernelMaxFreeMemSize();
        ko_free_at_boot_kb = (int)(total / 1024);
        ko_module_reserve_kb = ko_free_at_boot_kb > 32 * 1024 ? KO_PSP_MODULE_RESERVE_LARGE_KB : KO_PSP_MODULE_RESERVE_KB;
        SceSize reserve = (SceSize)ko_module_reserve_kb * 1024;
        SceSize size = largest > reserve + 1024 * 1024 ? largest - reserve : largest / 2;
        __psp_heap_blockid = sceKernelAllocPartitionMemory(2 /* user partition */, "block", PSP_SMEM_Low, size, NULL);
        if (__psp_heap_blockid < 0) { errno = ENOMEM; return (void *)-1; }
        heap_bottom = heap_ptr = sceKernelGetBlockHeadAddr(__psp_heap_blockid);
        heap_top = heap_bottom + size;
    }
    char *prev = heap_ptr;
    if (prev + incr > heap_top || prev + incr < heap_bottom) { errno = ENOMEM; return (void *)-1; }
    heap_ptr = prev + incr;
    return prev;
}
