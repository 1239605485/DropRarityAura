#include "rarity_palette.hpp"

bool rarity_color(const int rarity, RarityColor& out) {
    static constexpr RarityColor colors[] = {
        {0, 0, 0},
        {80, 160, 255},
        {80, 220, 120},
        {255, 170, 55},
        {255, 90, 90},
        {255, 105, 190},
        {185, 105, 255},
        {150, 255, 80},
        {255, 230, 70},
        {70, 235, 230},
        {235, 45, 65},
        {125, 70, 230},
        {255, 125, 35},
        {255, 215, 245},
    };
    if (rarity < 1 || rarity >= static_cast<int>(sizeof(colors) / sizeof(colors[0])))
        return false;
    out = colors[rarity];
    return true;
}
