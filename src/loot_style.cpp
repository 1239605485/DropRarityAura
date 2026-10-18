#include "loot_style.hpp"
#include "rarity_profile.hpp"
#include <cmath>
#include <array>
namespace {
float fract(float x){return x-std::floor(x);}
constexpr float pi=3.14159265f;
struct SparkSeed {float offset,speed,phase,amplitude,size;};
const std::array<SparkSeed,28>& spark_seeds(){
 // Five seed sine calculations per particle are invariant: compute once,
 // retaining exactly the same float expressions and particle trajectories.
 static const auto seeds=[] {
  std::array<SparkSeed,28> values{};
  for(int i=0;i<28;++i){
   float seed=i*7.731f;
   values[i]={fract(std::sin(seed+1)*43758.5453f),
    .14f+.11f*fract(std::sin(seed+2)*23731.13f),
    fract(std::sin(seed+3)*12345.67f)*2*pi,
    8+17*fract(std::sin(seed+4)*5341.17f),
    2.5f+4*fract(std::sin(seed+5)*7891.31f)};
  }
  return values;
 }();
 return seeds;
}
}
void draw_loot_style(void* c,EffectDraw d,float x,float y,float ground,int rarity,RarityColor col,float t,bool front){
 const auto style=rarity_profile(rarity);
 // Preserve the approved prototype's shapes, dimensions, tint and energy.
 // Rarity enables extra layers; it must not globally attenuate this base style.
 const float p=.93f+.07f*std::sin(t*2.2f);
 RarityColor pale{(uint8_t)(col.r*.33f+170),(uint8_t)(col.g*.33f+170),(uint8_t)(col.b*.33f+170)};
 auto add=[&](EffectTexture tex,float px,float py,float w,float h,RarityColor color,float power){
  if(power>0&&w>0&&h>0)d(c,tex,px,py,w,h,color,power);
 };
 if(!front){
  // Original base passes: these values come directly from the approved demo.
  add(Bloom,x,ground,106,22,col,.57f*p);add(Bloom,x,ground,65,10,pale,.39f*p);
  add(Ring,x,ground-1,93,17,col,.75f*p);
  if(style.groundRings>=2)add(Ring,x,ground-2,67,11,pale,.6f*p);
  if(style.groundRings>=3)add(Ring,x,ground-3,111,21,col,.38f*p);
  if(style.tier>=1){
   float q=fract(t/3.8f);add(Ring,x,ground-2-q*4,62+q*49,12+q*9,col,std::sin(q*pi)*.5f);
  }
  if(style.beamHeight>0){
   add(Beam,x,y-style.beamHeight*.5f,50,style.beamHeight,col,.98f*p);
   if(style.tier>=3)add(Beam,x,y-style.beamHeight*.48f,18,style.beamHeight*.96f,pale,.72f*p);
   add(BeamTail,x,y+(ground-y)*.5f,23,ground-y,col,.34f);
  }
  add(Bloom,x,y,100,100,col,.65f*p);
  if(style.tier>=1)add(Bloom,x,y,48,48,pale,.42f*p);
  if(style.tier>=2){
   // Restore the original star flare, but place it BEFORE the original item.
   add(Star,x,y,190,34,col,.68f*p);add(Star,x,y,58,22,pale,.43f*p);
  }
  return;
 }
 // The original item is drawn between the rear and front passes. Do not add
 // the prototype's central front bloom/white core: it would wash out its pixels.
 for(int i=0;i<style.liftRings;++i){
  float q=fract(t/4.2f+i/(float)style.liftRings);
  add(Ring,x,ground-8-q*28,57+q*13,10,col,.26f*std::sin(q*pi));
 }
 const auto* seeds=style.particles?spark_seeds().data():nullptr;
 for(int i=0;i<style.particles;++i){
  const auto& seed=seeds[i];
  const float size=seed.size;
  float q=fract(t*seed.speed+seed.offset),px=x+std::sin(seed.phase+q*5)*seed.amplitude*(.45f+.55f*q),py=y+12-q*131;
  if(std::abs(px-x)<18&&std::abs(py-y)<20)px=x+(px<x?-1:1)*20;
  float alpha=std::pow(std::sin(q*pi),1.5f)*(.24f+.5f*size/6.5f);
  add(Spark,px,py,size,size,pale,alpha);
 }
 for(int i=0;i<style.orbitGlints;++i){
  float a=t*(.5f+i*.12f)+i*2*pi/style.orbitGlints;
  float px=x+std::cos(a)*22,py=y+std::sin(a)*13;
  if(std::abs(px-x)<18&&std::abs(py-y)<20)px=x+(px<x?-1:1)*20;
  add(Spark,px,py,5,5,pale,.62f);
 }
}
