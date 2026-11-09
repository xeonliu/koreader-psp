-- Runs on LuaJIT (soft-float interpreter, JIT off) on the PSP. Prints PASS/FAIL per check.
local fails = 0
local function check(name, got, want)
    if got == want then print("PASS", name) else fails = fails + 1 print("FAIL", name, "got", tostring(got), "want", tostring(want)) end
end
print("version:", jit.version, jit.arch, jit.os, "jit.status", tostring(jit.status()))
check("1+1", 1 + 1, 2)
check("0.1+0.2", string.format("%.17g", 0.1 + 0.2), "0.30000000000000004")
check("2^0.5", string.format("%.12f", 2 ^ 0.5), "1.414213562373")
check("sin(1)", string.format("%.10f", math.sin(1)), "0.8414709848")
check("string.rep", #string.rep("ab", 1000), 2000)
check("format", string.format("%5.2f %d %s", 3.14159, 42, "x"), " 3.14 42 x")
check("bit.band", require("bit").band(0xff00, 0x0ff0), 0x0f00)
-- int32 overflow goes through the 5-argument lj_meta_arith helper (EABI: 5th arg in a4).
local a = 2147483647
check("maxint+1", a + 1, 2147483648)
local s = 0 for i = 1, 3 do s = s + 2000000000 end
check("loop 3x2e9", s, 6000000000)
check("100000*100000", 100000 * 100000, 10000000000)
local t = {} for i = 1, 100000 do t[i] = i * 2 end
local sum = 0 for i = 1, #t do sum = sum + t[i] end
check("table sum", sum, 10000100000)
local words = {} for i = 1, 200 do words["k" .. i] = i end
check("string keys", words.k200, 200)
local function mk() local n = 0 return function() n = n + 1 return n end end
local c = mk() c() check("closure", c(), 2)
collectgarbage() check("gc", collectgarbage("count") < 4096, true)
check("pcall error", select(2, pcall(error, "boom")), "boom")
check("coroutine", (function() local co = coroutine.wrap(function() coroutine.yield(1) return 2 end) return co() + co() end)(), 3)
local t0 = os.clock() local x = 0 for i = 1, 2000000 do x = x + i % 7 end
print("2M-iteration loop:", string.format("%.2fs", os.clock() - t0))
local okffi, ffi = pcall(require, "ffi")
check("ffi loads", okffi, true)
if okffi then
    ffi.cdef[[ typedef struct { int a; double b; char c[8]; } S; ]]
    local v = ffi.new("S", { a = 7, b = 2.5 }) v.c = "psp"
    check("ffi struct", v.a .. "/" .. v.b .. "/" .. ffi.string(v.c) .. "/" .. ffi.sizeof("S"), "7/2.5/psp/24")
    local buf = ffi.new("uint8_t[16]") buf[3] = 200
    check("ffi array", buf[3], 200)
    ffi.cdef[[ int abs(int); int atoi(const char *); size_t strlen(const char *); ]]
    check("ffi.C.abs", ffi.C.abs(-5), 5)
    check("ffi.C.strlen", tonumber(ffi.C.strlen("hello")), 5)
    check("ffi.load + atoi", ffi.load("c").atoi("1234"), 1234)
end
print(fails == 0 and "ALL CHECKS PASSED" or (fails .. " CHECK(S) FAILED"))
