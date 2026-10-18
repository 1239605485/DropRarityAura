#pragma once
#include "rarity_palette.hpp"
#include <cstdint>
struct EffectRect { int x, y, width, height; };
struct EffectColor { uint8_t a, b, g, r; };
using EffectDraw = void (*)(void* context, EffectRect rect, EffectColor color);
// Stateless geometry: picked-up items stop producing every layer immediately.
void draw_loot_style(void* context, EffectDraw draw, float x, float y,
                     int rarity, RarityColor color, float seconds);
