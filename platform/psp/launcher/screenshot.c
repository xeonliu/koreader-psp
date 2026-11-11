/* Development aid (compiled in with -DKO_PSP_SCREENSHOTS): a thread that periodically dumps the PSP display frame
 * buffer to host0:/shot_<n>.bmp, so automated emulator runs can produce screenshots of the running KOReader. */
#include <pspkernel.h>
#include <pspdisplay.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int first_ms = 20000, interval_ms = 10000, count = 6;

static void dump_framebuffer(const char *path) {
    void *top = NULL; int bw = 0, fmt = 0;
    sceDisplayGetFrameBuf(&top, &bw, &fmt, PSP_DISPLAY_SETBUF_IMMEDIATE);
    if (!top) { printf("screenshot: no frame buffer\n"); return; }
    FILE *f = fopen(path, "wb"); if (!f) { printf("screenshot: cannot open %s\n", path); return; }
    const int W = 480, H = 272, rowsz = W * 3;
    uint32_t filesz = 54 + rowsz * H;
    uint8_t hdr[54] = { 'B', 'M' };
    hdr[2] = filesz; hdr[3] = filesz >> 8; hdr[4] = filesz >> 16; hdr[10] = 54; hdr[14] = 40;
    hdr[18] = W & 255; hdr[19] = W >> 8; hdr[22] = H & 255; hdr[23] = H >> 8; hdr[26] = 1; hdr[28] = 24;
    hdr[34] = (rowsz * H); hdr[35] = (rowsz * H) >> 8; hdr[36] = (rowsz * H) >> 16;
    fwrite(hdr, 1, 54, f);
    for (int y = H - 1; y >= 0; y--) {
        uint8_t row[480 * 3];
        for (int x = 0; x < W; x++) {
            uint8_t r, g, b;
            if (fmt == PSP_DISPLAY_PIXEL_FORMAT_8888) { uint32_t p = ((uint32_t *)top)[y * bw + x]; r = p; g = p >> 8; b = p >> 16; }
            else {
                uint16_t p = ((uint16_t *)top)[y * bw + x];
                if (fmt == PSP_DISPLAY_PIXEL_FORMAT_565) { r = (p & 31) << 3; g = ((p >> 5) & 63) << 2; b = ((p >> 11) & 31) << 3; }
                else if (fmt == PSP_DISPLAY_PIXEL_FORMAT_5551) { r = (p & 31) << 3; g = ((p >> 5) & 31) << 3; b = ((p >> 10) & 31) << 3; }
                else { r = (p & 15) << 4; g = ((p >> 4) & 15) << 4; b = ((p >> 8) & 15) << 4; }
            }
            row[x * 3] = b; row[x * 3 + 1] = g; row[x * 3 + 2] = r;
        }
        fwrite(row, 1, rowsz, f);
    }
    fclose(f);
    printf("screenshot: wrote %s\n", path);
    fflush(stdout);
}

static int shot_thread(SceSize args, void *argp) {
    (void)args; (void)argp;
    sceKernelDelayThread(first_ms * 1000);
    for (int i = 1; i <= count; i++) {
        char path[64];
        snprintf(path, sizeof path, "host0:/shot_%d.bmp", i);
        dump_framebuffer(path);
        sceKernelDelayThread(interval_ms * 1000);
    }
    printf("screenshot: done\n");
    fflush(stdout);
    sceKernelExitGame();
    return 0;
}

void ko_start_screenshots(int first, int interval, int n) {
    first_ms = first; interval_ms = interval; count = n;
    int th = sceKernelCreateThread("ko_shots", shot_thread, 0x30, 0x4000, 0, NULL);
    if (th >= 0) sceKernelStartThread(th, 0, NULL);
}
