local ffi = require("ffi")
ffi.cdef[[
int kotest_add(int, int); int kotest_count(void); double kotest_hyp(double, double); float kotest_half(float);
char *kotest_greet(const char *); void free(void *);
int sceKernelTotalFreeMemSize(void); int sceKernelMaxFreeMemSize(void);
]]
print("free before module:", ffi.C.sceKernelTotalFreeMemSize())
local t = ffi.load("libs/libkotest.so")
print("add", t.kotest_add(2, 3), "count", t.kotest_count(), t.kotest_count())
print("hyp", t.kotest_hyp(3, 4), "half", t.kotest_half(5))
local s = t.kotest_greet("KOReader"); print(ffi.string(s)); ffi.C.free(s)
print("free after module:", ffi.C.sceKernelTotalFreeMemSize())
print("PRX-TEST-OK")
