#pragma once
#include "rarity_palette.hpp"
void configure_beam_resources(const char*);
bool initialize_beam_renderer();
bool draw_rarity_beam(float,float,int,RarityColor,float,bool);
void reset_beam_renderer();
