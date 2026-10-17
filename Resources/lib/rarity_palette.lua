-- Unique RGB per positive vanilla rarity. Rarity zero stays intentionally hidden.
-- The base particle is WhiteTorch so the chosen RGB remains visible and stable.
local rgb_by_rarity = {
    [1]  = {  80, 160, 255 }, -- blue
    [2]  = {  80, 220, 120 }, -- green
    [3]  = { 255, 170,  55 }, -- orange
    [4]  = { 255,  90,  90 }, -- light red
    [5]  = { 255, 105, 190 }, -- pink
    [6]  = { 185, 105, 255 }, -- purple
    [7]  = { 150, 255,  80 }, -- lime
    [8]  = { 255, 230,  70 }, -- yellow
    [9]  = {  70, 235, 230 }, -- cyan
    [10] = { 235,  45,  65 }, -- red
    [11] = { 125,  70, 230 }, -- deep purple
    [12] = { 255, 125,  35 }, -- expert orange
    [13] = { 255, 215, 245 }, -- master pink-white
}

return function(rarity)
    return rgb_by_rarity[rarity]
end
