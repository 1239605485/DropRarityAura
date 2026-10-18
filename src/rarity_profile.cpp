#include "rarity_profile.hpp"
RarityProfile rarity_profile(int rarity){
 // Select decorations only. Base gradient strengths and shapes stay identical
 // to the approved prototype, including its 100-unit short beam at rare tiers.
 if(rarity<=0)return {0,1,0,0,0,106,0,.57f};
 if(rarity<=2)return {1,2,0,6,0,106,0,.57f};
 if(rarity<=5)return {2,2,0,12,1,106,100,.57f};
 if(rarity<=8)return {3,2,1,20,3,106,100,.57f};
 return {4,3,2,28,5,106,100,.57f};
}
