#pragma once
#include <cstdint>

struct RarityColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

// Vanilla Terraria Item.rare 1..13. Common rarity 0 has no aura.
bool rarity_color(int rarity, RarityColor& out);
