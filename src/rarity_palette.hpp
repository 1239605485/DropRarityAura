#pragma once
#include <cstdint>

struct RarityColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

// Vanilla Terraria Item.rare 0..13; negative special rarities are omitted.
bool rarity_color(int rarity, RarityColor& out);
