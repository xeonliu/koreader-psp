print("version:", jit and jit.version or "no jit table", jit and jit.arch, jit and jit.os)
print("jit status:", jit and jit.status and tostring(jit.status()))
print("1+1 =", 1 + 1, " 0.1+0.2 =", 0.1 + 0.2, " 2^0.5 =", 2 ^ 0.5, " 1/3 =", 1 / 3)
print("string:", ("hello"):upper(), #string.rep("ab", 1000), string.format("%5.2f %d %s", 3.14159, 42, "x"))
print("bit:", require("bit").band(0xff00, 0x0ff0), require("bit").bxor(5, 3))
local t = {} for i = 1, 100000 do t[i] = i * 2 end
local s = 0 for i = 1, #t do s = s + t[i] end
print("sum:", s)
local t0 = os.clock()
local x = 0 for i = 1, 2000000 do x = x + i % 7 end
print("loop 2M iters:", x, string.format("%.2fs", os.clock() - t0))
local okffi, ffi = pcall(require, "ffi")
print("ffi:", okffi)
if okffi then
    ffi.cdef[[ typedef struct { int a; double b; char c[8]; } S; int abs(int); size_t strlen(const char*); ]]
    local v = ffi.new("S", { a = 7, b = 2.5 })
    v.c = "psp"
    print("ffi struct:", v.a, v.b, ffi.string(v.c), ffi.sizeof("S"))
    print("ffi call abs(-5):", ffi.C.abs(-5), " strlen:", tonumber(ffi.C.strlen("hello")))
    local buf = ffi.new("uint8_t[16]") buf[3] = 200
    print("ffi array:", buf[3], ffi.cast("uint32_t*", buf)[0])
end
print("collectgarbage KB:", collectgarbage("count"))
