/* POSIX functions that newlib-PSP lacks but that KOReader's dependencies reference. Most are unsupported on the
 * PSP (no processes, users, symlinks or signals): they fail with ENOSYS. */
#include <errno.h>
#include <malloc.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define NOSYS_INT(name, args) int name args { errno = ENOSYS; return -1; }

NOSYS_INT(fchdir, (int fd))
NOSYS_INT(dup2, (int a, int b))
NOSYS_INT(lchmod, (const char *p, mode_t m))
NOSYS_INT(futimens, (int fd, const struct timespec t[2]))
NOSYS_INT(utimensat, (int fd, const char *p, const struct timespec t[2], int f))
NOSYS_INT(utimes, (const char *p, const struct timeval t[2]))
NOSYS_INT(lutimes, (const char *p, const struct timeval t[2]))
NOSYS_INT(futimesat, (int fd, const char *p, const struct timeval t[2]))
NOSYS_INT(mknod, (const char *p, mode_t m, dev_t d))
NOSYS_INT(mkfifo, (const char *p, mode_t m))
NOSYS_INT(lchown, (const char *p, uid_t u, gid_t g))
NOSYS_INT(fdatasync, (int fd))
NOSYS_INT(ftruncate, (int fd, off_t l))
NOSYS_INT(fstatvfs, (int fd, void *buf))
pid_t waitpid(pid_t p, int *s, int o) { (void)p; (void)s; (void)o; errno = ECHILD; return -1; }
pid_t vfork(void) { errno = ENOSYS; return -1; }
gid_t getgid(void) { return 0; }
gid_t getegid(void) { return 0; }
mode_t umask(mode_t m) { return m; }
void closefrom(int fd) { (void)fd; }
int sigaction(int sig, const struct sigaction *act, struct sigaction *old) { (void)sig; (void)act; (void)old; return 0; }

long sysconf(int name) {
    switch (name) {
        case _SC_PAGESIZE: return 4096;
        case _SC_NPROCESSORS_ONLN: case _SC_NPROCESSORS_CONF: return 1;
        case _SC_CLK_TCK: return 100;
        case _SC_OPEN_MAX: return 64;
        default: errno = EINVAL; return -1;
    }
}
long fpathconf(int fd, int name) { (void)fd; (void)name; errno = ENOSYS; return -1; }

int posix_memalign(void **out, size_t align, size_t size) {
    void *p = memalign(align, size);
    if (!p) return ENOMEM;
    *out = p;
    return 0;
}

int _get_timezone(long *tz) { if (tz) *tz = 0; return 0; }

/* timegm(): the inverse of gmtime(), without relying on TZ. */
time_t timegm(struct tm *tm) {
    static const int cum[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    long y = tm->tm_year + 1900L, m = tm->tm_mon;
    y += m / 12; m %= 12; if (m < 0) { m += 12; y--; }
    long days = (y - 1970) * 365 + ((y - 1969) / 4) - ((y - 1901) / 100) + ((y - 1601) / 400) + cum[m] + (tm->tm_mday - 1);
    if (m > 1 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) days++;
    return (time_t)(((days * 24 + tm->tm_hour) * 60 + tm->tm_min) * 60 + tm->tm_sec);
}

/* ---- Functions KOReader's Lua code actually calls through ffi.C: real (if minimal) implementations. ---- */
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

int access(const char *path, int mode) {
    struct stat st;
    (void)mode;
    return stat(path, &st) == 0 ? 0 : -1;
}

/* POSIX dirname()/basename(): modify (or return into) the given buffer. */
char *dirname(char *path) {
    static char dot[] = ".";
    if (!path || !*path) return dot;
    size_t len = strlen(path);
    while (len > 1 && path[len - 1] == '/') path[--len] = '\0';      /* strip trailing slashes */
    char *slash = strrchr(path, '/');
    if (!slash) return dot;
    while (slash > path && slash[-1] == '/') slash--;                 /* collapse */
    if (slash == path) { path[1] = '\0'; return path; }               /* "/x" -> "/" */
    *slash = '\0';
    /* Device-style roots such as "ms0:" keep a trailing slash: "ms0:/x" -> "ms0:/". */
    size_t n = strlen(path);
    if (n && path[n - 1] == ':') { path[n] = '/'; path[n + 1] = '\0'; }
    return path;
}

char *basename(char *path) {
    static char dot[] = ".";
    if (!path || !*path) return dot;
    size_t len = strlen(path);
    while (len > 1 && path[len - 1] == '/') path[--len] = '\0';
    char *slash = strrchr(path, '/');
    if (len == 1 && path[0] == '/') return path;
    return slash ? slash + 1 : path;
}

char *realpath(const char *path, char *resolved) {
    if (!path) { errno = EINVAL; return NULL; }
    if (!resolved) resolved = malloc(1024);
    if (!resolved) return NULL;
    strncpy(resolved, path, 1023); resolved[1023] = '\0';
    return resolved;
}

/* clock_gettime(): derive from gettimeofday (REALTIME) / clock() (MONOTONIC and the rest). */
int clock_gettime(clockid_t id, struct timespec *ts) {
    if (!ts) { errno = EINVAL; return -1; }
    if (id == CLOCK_REALTIME) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        ts->tv_sec = tv.tv_sec; ts->tv_nsec = tv.tv_usec * 1000;
    } else {
        unsigned long long us = (unsigned long long)clock() * 1000000ULL / CLOCKS_PER_SEC;
        ts->tv_sec = (time_t)(us / 1000000ULL); ts->tv_nsec = (long)(us % 1000000ULL) * 1000;
    }
    return 0;
}
int clock_getres(clockid_t id, struct timespec *ts) { (void)id; if (ts) { ts->tv_sec = 0; ts->tv_nsec = 1000; } return 0; }
int settimeofday(const struct timeval *tv, const struct timezone *tz) { (void)tv; (void)tz; errno = EPERM; return -1; }

int fchmod(int fd, mode_t m) { (void)fd; (void)m; return 0; }
pid_t getppid(void) { return 1; }
uid_t getuid(void) { return 0; }
int pause(void) { errno = EINTR; return -1; }
int poll(void *fds, unsigned long n, int timeout) { (void)fds; (void)n; (void)timeout; return 0; }
int ioctl(int fd, unsigned long req, ...) { (void)fd; (void)req; errno = ENOTTY; return -1; }
void *mmap(void *a, size_t l, int p, int f, int fd, off_t o) { (void)a; (void)l; (void)p; (void)f; (void)fd; (void)o; errno = ENOSYS; return (void *)-1; }
int munmap(void *a, size_t l) { (void)a; (void)l; return 0; }

/* Plausible filesystem statistics (the PSP's FAT Memory Stick): used for "free space" displays. */
struct ko_statvfs { unsigned long f_bsize, f_frsize; unsigned long long f_blocks, f_bfree, f_bavail, f_files, f_ffree, f_favail; unsigned long f_fsid, f_flag, f_namemax; };
int statvfs(const char *path, struct ko_statvfs *s) {
    (void)path;
    memset(s, 0, sizeof *s);
    s->f_bsize = s->f_frsize = 4096; s->f_blocks = 1048576; s->f_bfree = s->f_bavail = 524288; s->f_namemax = 255;
    return 0;
}

unsigned long htonl(unsigned long v) { return __builtin_bswap32((unsigned)v); }
unsigned short htons(unsigned short v) { return __builtin_bswap16(v); }
unsigned long ntohl(unsigned long v) { return __builtin_bswap32((unsigned)v); }
unsigned short ntohs(unsigned short v) { return __builtin_bswap16(v); }

/* 64-bit file offsets and resource usage (used by Leptonica & co). newlib-PSP's off_t is 64-bit already. */
#include <sys/resource.h>
int fseeko64(FILE *f, long long off, int whence) { return fseeko(f, (off_t)off, whence); }
long long ftello64(FILE *f) { return (long long)ftello(f); }
int getrusage(int who, struct rusage *ru) {
    (void)who;
    memset(ru, 0, sizeof *ru);
    unsigned long long us = (unsigned long long)clock() * 1000000ULL / CLOCKS_PER_SEC;
    ru->ru_utime.tv_sec = (time_t)(us / 1000000ULL); ru->ru_utime.tv_usec = (long)(us % 1000000ULL);
    return 0;
}

struct ko_utsname { char sysname[65], nodename[65], release[65], version[65], machine[65]; };
int uname(struct ko_utsname *u) {
    memset(u, 0, sizeof *u);
    strcpy(u->sysname, "PSP"); strcpy(u->nodename, "psp"); strcpy(u->release, "6.61"); strcpy(u->version, "1"); strcpy(u->machine, "allegrex");
    return 0;
}

/* newlib's glob() needs these; the PSP has no login sessions, so report nothing (not set-uid either). */
char *getlogin(void) { errno = ENOSYS; return NULL; }
int issetugid(void) { return 0; }
