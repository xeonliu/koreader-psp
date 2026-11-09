-- Stand-in for ssl.https on the PSP.
return {
    PORT = 443,
    request = function() return nil, "https is not supported on this platform" end,
    tcp = function() return nil, "https is not supported on this platform" end,
}
