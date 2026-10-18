#pragma once
#include "rarity_palette.hpp"
#include <cstdint>
struct EffectRect { int x,y,width,height; };
struct EffectColor { uint8_t a,b,g,r; };
enum EffectTexture { Bloom,Ring,Star,Spark,Beam,BeamTail,TextureCount };
using EffectDraw=void(*)(void*,EffectTexture,float,float,float,float,RarityColor,float);
void draw_loot_style(void*,EffectDraw,float,float,float,int,RarityColor,float,bool);
