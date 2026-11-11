-- Stand-in for LuaSocket's mime C core (only used by network code, which is unsupported on the PSP for now).
local function unsupported() error("mime is not supported on this platform", 2) end
return { _VERSION = "MIME 1.0.3 (PSP stand-in)", b64 = unsupported, unb64 = unsupported, qp = unsupported, unqp = unsupported,
         qpwrp = unsupported, wrp = unsupported, eol = unsupported, dot = unsupported }
