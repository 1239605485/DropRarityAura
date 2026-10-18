#pragma once
#include "rarity_palette.hpp"
bool initialize_beam_renderer();
bool draw_rarity_beam(float world_x, float world_y, int rarity, RarityColor color, unsigned frame);
void reset_beam_renderer();
