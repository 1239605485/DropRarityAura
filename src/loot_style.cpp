#include "loot_style.hpp"
#include "rarity_profile.hpp"
#include <cmath>
namespace {float fract(float x){return x-std::floor(x);} constexpr float pi=3.14159265f;}
void draw_loot_style(void* c,EffectDraw d,float x,float y,float ground,int rarity,RarityColor col,float t,bool front){
 const auto style=rarity_profile(rarity);
 const float pulse=.94f+.06f*std::sin(t*2.2f),radius=style.radius;
 RarityColor pale{(uint8_t)(col.r*.4f+153),(uint8_t)(col.g*.4f+153),(uint8_t)(col.b*.4f+153)};
 auto add=[&](EffectTexture tex,float px,float py,float w,float h,RarityColor color,float power){
  if(power>0&&w>0&&h>0)d(c,tex,px,py,w,h,color,power,0);
 };
 if(!front){
  add(Bloom,x,ground,radius,12+style.tier*2,col,style.glowPower*pulse);
  for(int i=0;i<style.groundRings;++i){
   const float w=radius*(1-i*.22f);
   add(Ring,x,ground-1-i*2,w,w*.17f,i%2?pale:col,(.23f+style.tier*.055f-i*.035f)*pulse);
  }
  if(style.tier>=1){
   add(Bloom,x,y,38+style.tier*12,38+style.tier*12,col,style.glowPower*.65f*pulse);
   float q=fract(t/3.8f);add(Ring,x,ground-2-q*3,radius*(.65f+q*.4f),radius*(.11f+q*.06f),col,std::sin(q*pi)*(.12f+style.tier*.035f));
  }
  if(style.beamHeight>0){
   // Both the tapered beam and its bright foot are BEHIND the original item.
   add(Beam,x,y-style.beamHeight*.5f,24+style.tier*5,style.beamHeight,col,(.25f+style.tier*.09f)*pulse);
   if(style.tier>=3)add(Beam,x,y-style.beamHeight*.48f,10,style.beamHeight*.96f,pale,.24f*pulse);
   add(BeamTail,x,y+(ground-y)*.5f,15,ground-y,col,.13f);
   add(Star,x,y,60+style.tier*18,14+style.tier*2,col,(.14f+style.tier*.035f)*pulse);
  }
  if(style.tier==4){
   float a=fract(t/5.f);add(Ring,x,ground-2,80+a*24,14+a*5,col,std::sin(a*pi)*.23f);
   add(Star,x,ground-3,112,12,pale,.16f*pulse);
  }
  return;
 }
 // Never place a bloom, beam or central star over the item sprite.
 // Front decoration stays at the periphery; all particles are stateless.
 for(int i=0;i<style.liftRings;++i){
  float q=fract(t/4.2f+i/(float)style.liftRings);
  add(Ring,x,ground-7-q*(24+style.tier*5),radius*(.6f+q*.2f),8,col,.14f*std::sin(q*pi));
 }
 for(int i=0;i<style.particles;++i){
  float seed=i*7.731f,off=fract(std::sin(seed+1)*43758.5453f);
  float speed=.14f+.11f*fract(std::sin(seed+2)*23731.13f);
  float phase=fract(std::sin(seed+3)*12345.67f)*2*pi;
  float q=fract(t*speed+off),size=2.5f+2.5f*fract(std::sin(seed+5)*7891.31f);
  float px=x+std::sin(phase+q*5)*(14+style.tier*4),py=y+10-q*(style.beamHeight+22);
  // Leave a clear 36x40 window around the original item even at top rarity.
  if(std::abs(px-x)<18&&std::abs(py-y)<20)px=x+(px<x?-1:1)*20;
  float alpha=std::pow(std::sin(q*pi),1.5f)*(.18f+style.tier*.045f);
  add(Spark,px,py,size,size,pale,alpha);
 }
 for(int i=0;i<style.orbitGlints;++i){
  float a=t*(.5f+i*.12f)+i*2*pi/style.orbitGlints;
  float px=x+std::cos(a)*(23+style.tier*2),py=y+std::sin(a)*23;
  if(std::abs(px-x)<18&&std::abs(py-y)<20)continue;
  add(Spark,px,py,4,4,pale,.28f);
 }
}
