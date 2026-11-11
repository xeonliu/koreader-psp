/* { library name as used by ffi.load()/require() (no "lib" prefix, no ".so" suffix), module, "dep1,dep2" or 0 }
 * module 0: the library is linked into the EBOOT. Anything not listed here does not exist (ffi.load fails). */
    { "SDL2", 0, 0 }, { "blitbuffer", 0, 0 }, { "freetype", 0, 0 }, { "lodepng", 0, 0 }, { "turbojpeg", 0, 0 },
    { "gif", 0, 0 }, { "z", 0, 0 }, { "c", 0, 0 }, { "m", 0, 0 }, { "pthread", 0, 0 },
    { "koreader-cre", "cre", 0 },              /* CREngine: EPUB, FB2, HTML, TXT, ... */
    { "mupdf", "mupdf", 0 }, { "wrap-mupdf", "mupdf", 0 },   /* MuPDF: PDF, XPS, CBZ, HTML widget */
    { "kotest", "kotest", 0 },                 /* tiny self-test module */
