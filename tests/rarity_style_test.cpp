#include "loot_style.hpp"
#include "rarity_profile.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
struct Check{bool front;int count=0,beams=0,sparks=0;bool approvedGround=false,approvedRing=false,approvedBeam=false,approvedStar=false;};
void emit(void* data,EffectTexture tex,float x,float y,float w,float h,RarityColor,float power){
 auto& c=*static_cast<Check*>(data);assert(std::isfinite(x)&&std::isfinite(y));assert(w>0&&h>0&&power>0&&power<1);
 if(!c.front){
  c.approvedGround|=tex==Bloom&&w==106&&h==22&&power>.49f;
  c.approvedRing|=tex==Ring&&w==93&&h==17&&power>.64f;
  c.approvedBeam|=tex==Beam&&w==50&&h==100&&power>.84f;
  c.approvedStar|=tex==Star&&w==190&&h==34&&power>.58f;
 }
 ++c.count;c.beams+=tex==Beam;c.sparks+=tex==Spark;
 if(c.front){assert(tex==Ring||tex==Spark);if(tex==Spark)assert(std::abs(x-100)>=18||std::abs(y-160)>=20);}
}
int main(){int ranks[]={0,1,3,6,9};int previous=0;
 for(int rank:ranks){int total=0;
  for(int frame=0;frame<960;++frame){Check back{false},front{true};float t=frame/24.f;
   draw_loot_style(&back,emit,100,160,202,rank,{185,105,255},t,false);
   draw_loot_style(&front,emit,100,160,202,rank,{185,105,255},t,true);
   if(rank<3)assert(back.beams==0);else assert(back.beams>0&&back.approvedBeam&&back.approvedStar);
   assert(back.approvedGround&&back.approvedRing);
   total+=back.count+front.count;
  }
  assert(total>previous);previous=total;
 }
 assert(rarity_profile(0).particles==0&&rarity_profile(9).particles==28);
 puts("PASS: five increasingly complex rarity tiers; low tiers have no beams; front particles keep item centre clear over 40 seconds");
}
