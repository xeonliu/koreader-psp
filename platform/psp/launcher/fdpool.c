/* The PSP firmware only allows a handful of files to be open at the same time (about 16; PPSSPP does not enforce it).
 * KOReader keeps far more than that open: every UI and document font, the book, and whatever the garbage collector has
 * not finalized yet. So files opened read-only get a virtual handle here, and only the KO_FD_POOL most recently used
 * ones are really open: the others are closed behind the scenes and reopened (and repositioned) on their next use.
 * Linked with -Wl,--wrap=sceIoOpen,... so it sits under newlib and everything else.
 *
 * Development aid: -DKO_PSP_MAX_OPEN=n makes the "firmware" refuse more than n open files, to test this in PPSSPP. */
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SceUID __real_sceIoOpen(const char *file, int flags, SceMode mode);
int __real_sceIoClose(SceUID fd);
int __real_sceIoRead(SceUID fd, void *data, SceSize size);
int __real_sceIoWrite(SceUID fd, const void *data, SceSize size);
SceOff __real_sceIoLseek(SceUID fd, SceOff offset, int whence);

#ifndef KO_FD_POOL
#define KO_FD_POOL 6          /* read-only files kept really open */
#endif
#define KO_FD_VBASE 0x10000   /* virtual handles are KO_FD_VBASE + slot */
#define KO_FD_SLOTS 512
#define KO_ERR_MFILE ((int)0x80010018)

struct vfile { char *path; SceUID real; SceOff pos; unsigned stamp; int flags; SceMode mode; };
static struct vfile slots[KO_FD_SLOTS];
static unsigned clock_;
static int n_real;            /* really open pooled files */
static SceUID lock_ = -1;

#ifdef KO_PSP_MAX_OPEN
static int sim_open, sim_max;
static SceUID fw_open(const char *file, int flags, SceMode mode) {
    if (sim_open >= KO_PSP_MAX_OPEN) return KO_ERR_MFILE;
    SceUID fd = __real_sceIoOpen(file, flags, mode);
    if (fd >= 0 && ++sim_open > sim_max) sim_max = sim_open;
    return fd;
}
static int fw_close(SceUID fd) { int r = __real_sceIoClose(fd); if (r >= 0 && sim_open > 0) sim_open--; return r; }
int ko_fdpool_max_open(void) { return sim_max; }
#else
#define fw_open __real_sceIoOpen
#define fw_close __real_sceIoClose
#endif

void ko_fdpool_init(void) { if (lock_ < 0) lock_ = sceKernelCreateSema("ko_fdpool", 0, 1, 1, NULL); }
static void lock(void) { if (lock_ >= 0) sceKernelWaitSema(lock_, 1, NULL); }
static void unlock(void) { if (lock_ >= 0) sceKernelSignalSema(lock_, 1); }

/* Really close the least recently used pooled file (not `keep`). Returns 0 if there was none. */
static int evict(struct vfile *keep) {
    struct vfile *old = NULL;
    for (int i = 0; i < KO_FD_SLOTS; i++) {
        struct vfile *v = &slots[i];
        if (v->path && v->real >= 0 && v != keep && (!old || v->stamp < old->stamp)) old = v;
    }
    if (!old) return 0;
    fw_close(old->real); old->real = -1; n_real--;
    return 1;
}

/* Open through the firmware, making room when it is (or we expect it to be) out of handles. */
static SceUID open_real(const char *file, int flags, SceMode mode, struct vfile *keep) {
    for (;;) {
        SceUID fd = fw_open(file, flags, mode);
        if (fd >= 0 || fd != KO_ERR_MFILE) return fd;
        if (!evict(keep)) return fd;
    }
}

static SceUID ensure(struct vfile *v) {
    v->stamp = ++clock_;
    if (v->real >= 0) return v->real;
    while (n_real >= KO_FD_POOL && evict(v)) ;
    SceUID fd = open_real(v->path, v->flags, v->mode, v);
    if (fd < 0) return fd;
    v->real = fd; n_real++;
    if (v->pos) __real_sceIoLseek(fd, v->pos, PSP_SEEK_SET);
    return fd;
}

static struct vfile *lookup(SceUID fd) {
    if (fd < KO_FD_VBASE || fd >= KO_FD_VBASE + KO_FD_SLOTS) return NULL;
    struct vfile *v = &slots[fd - KO_FD_VBASE];
    return v->path ? v : NULL;
}

SceUID __wrap_sceIoOpen(const char *file, int flags, SceMode mode) {
    lock();
    SceUID ret;
    int readonly = (flags & (PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC | PSP_O_APPEND)) == 0 && (flags & PSP_O_RDONLY);
    struct vfile *v = NULL;
    if (readonly && file) {
        for (int i = 0; i < KO_FD_SLOTS && !v; i++) if (!slots[i].path) v = &slots[i];
    }
    if (!v) { ret = open_real(file, flags, mode, NULL); unlock(); return ret; }   /* written files are not pooled */
    v->path = strdup(file);
    if (!v->path) { unlock(); return open_real(file, flags, mode, NULL); }
    v->real = -1; v->pos = 0; v->flags = flags; v->mode = mode;
    ret = ensure(v);
    if (ret < 0) { free(v->path); v->path = NULL; } else ret = KO_FD_VBASE + (int)(v - slots);
    unlock();
    return ret;
}

int __wrap_sceIoClose(SceUID fd) {
    lock();
    struct vfile *v = lookup(fd);
    int r = 0;
    if (!v) r = fw_close(fd);
    else {
        if (v->real >= 0) { fw_close(v->real); n_real--; }
        free(v->path); v->path = NULL;
    }
    unlock();
    return r;
}

int __wrap_sceIoRead(SceUID fd, void *data, SceSize size) {
    lock();
    struct vfile *v = lookup(fd);
    int r;
    if (!v) { unlock(); return __real_sceIoRead(fd, data, size); }   /* may block (stdin): not under the lock */
    SceUID real = ensure(v);
    r = real < 0 ? (int)real : __real_sceIoRead(real, data, size);
    if (r > 0) v->pos += r;
    unlock();
    return r;
}

int __wrap_sceIoWrite(SceUID fd, const void *data, SceSize size) {
    if (fd >= KO_FD_VBASE && fd < KO_FD_VBASE + KO_FD_SLOTS) return (int)0x80010009;   /* EBADF: pooled files are read-only */
    return __real_sceIoWrite(fd, data, size);
}

SceOff __wrap_sceIoLseek(SceUID fd, SceOff offset, int whence) {
    lock();
    struct vfile *v = lookup(fd);
    SceOff r;
    if (!v) r = __real_sceIoLseek(fd, offset, whence);
    else if (v->real < 0 && whence == PSP_SEEK_SET && offset >= 0) r = v->pos = offset;   /* applied when it is reopened */
    else {
        SceUID real = ensure(v);
        r = real < 0 ? (SceOff)real : __real_sceIoLseek(real, offset, whence);
        if (r >= 0) v->pos = r;
    }
    unlock();
    return r;
}
