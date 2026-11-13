-- Stand-in for LuaSocket's C core on the PSP: time functions work; networking is not supported (yet) and fails cleanly.
local ffi = require("ffi")
require("ffi/posix_h")

local core = { _VERSION = "LuaSocket 3.1.0 (PSP stand-in)", _DEBUG = false }

local ts = ffi.new("struct timespec[1]")
function core.gettime()
    ffi.C.clock_gettime(0, ts) -- CLOCK_REALTIME
    return tonumber(ts[0].tv_sec) + tonumber(ts[0].tv_nsec) / 1e9
end

function core.sleep(seconds)
    if seconds and seconds > 0 then ffi.C.usleep(math.floor(seconds * 1e6)) end
    return 1
end

local function unsupported() return nil, "network is not supported on this platform" end
core.tcp, core.tcp4, core.tcp6 = unsupported, unsupported, unsupported
core.udp, core.udp4, core.udp6 = unsupported, unsupported, unsupported
core.connect, core.bind = unsupported, unsupported
core.select = function() return nil, nil, "timeout" end
core.dns = {
    gethostname = function() return "psp" end,
    toip = function() return nil, "network is not supported on this platform" end,
    getaddrinfo = unsupported,
    tohostname = unsupported,
    getnameinfo = unsupported,
}

-- Same semantics as LuaSocket's except.c: newtry() / protect().
local function wrap(...) return setmetatable({ ... }, { __tostring = function() return "socket exception" end }) end
local exception_mt = {}
function core.newtry(finalizer)
    return function(...)
        local first = ...
        if first then return ... end
        if finalizer then pcall(finalizer) end
        error(setmetatable({ select(2, ...) }, exception_mt), 0)
    end
end
function core.protect(func)
    return function(...)
        local res = { pcall(func, ...) }
        if res[1] then return table.unpack(res, 2) end
        local err = res[2]
        if getmetatable(err) == exception_mt then return nil, err[1] end
        error(err, 0)
    end
end

return core
