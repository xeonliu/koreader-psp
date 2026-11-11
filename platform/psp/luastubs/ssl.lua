-- Stand-in for LuaSec on the PSP (no TLS/network support yet).
local function unsupported() return nil, "TLS is not supported on this platform" end
return { _VERSION = "0.0-psp", newcontext = unsupported, wrap = unsupported }
