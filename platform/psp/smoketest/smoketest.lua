-- Runs inside the PSP smoke test. Draws a frame, shows input state, moves a box with the stick/d-pad.
local x, y = 220, 120
local frames, t0 = 0, psp.ticks()
local fps = 0
while true do
    local i = psp.input()
    if i.start and i.select then return end -- Start+Select quits
    if AUTOEXIT_MS and psp.ticks() > AUTOEXIT_MS then return end
    local dx = (i.right and 1 or 0) - (i.left and 1 or 0) + i.lx * 2
    local dy = (i.down and 1 or 0) - (i.up and 1 or 0) + i.ly * 2
    x = math.max(0, math.min(480 - 20, x + dx * 3))
    y = math.max(0, math.min(272 - 20, y + dy * 3))

    psp.color(20, 24, 32); psp.clear()
    psp.color(255, 255, 255)
    psp.text(8, 8, "KOReader PSP smoke test (SDL3 + " .. _VERSION .. ")")
    psp.text(8, 24, string.format("fps %d  lua heap %d KB", fps, psp.lua_kb()))
    local pressed = {}
    for _, k in ipairs{ "cross", "circle", "square", "triangle", "l", "r", "start", "select" } do
        if i[k] then pressed[#pressed + 1] = k end
    end
    psp.text(8, 40, "buttons: " .. table.concat(pressed, " "))
    psp.text(8, 252, "D-pad / stick moves the box. Start+Select quits.")
    psp.color(80, 200, 120); psp.rect(x, y, 20, 20)
    psp.present()

    frames = frames + 1
    local now = psp.ticks()
    if now - t0 >= 1000 then fps = frames * 1000 // (now - t0); frames, t0 = 0, now end
end
