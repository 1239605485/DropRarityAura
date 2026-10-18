#include "rarity_profile.hpp"
RarityProfile rarity_profile(int rarity){
 // Terraria's rarity numbers are progression ranks, not the reference's colours.
 // Keep original rarity colours while independently increasing visual complexity.
 if(rarity<=0)return {0,1,0,0,0,34,0,.12f};
 if(rarity<=2)return {1,1,0,4,0,46,0,.20f};
 if(rarity<=5)return {2,2,0,9,1,62,48,.28f};
 if(rarity<=8)return {3,2,1,16,2,80,70,.37f};
 return {4,3,2,24,4,96,90,.46f};
}
