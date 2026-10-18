#include "loot_style.hpp"
#include <cmath>
namespace { float fract(float x){return x-std::floor(x);} }
void draw_loot_style(void* c,EffectDraw d,float x,float y,float ground,RarityColor col,float t,bool front){
 const float p=.93f+.07f*std::sin(t*2.2f);
 RarityColor pale{(uint8_t)(col.r*.33f+170),(uint8_t)(col.g*.33f+170),(uint8_t)(col.b*.33f+170)};
 auto add=[&](EffectTexture tex,float px,float py,float w,float h,RarityColor color,float power,float angle=0.f){d(c,tex,px,py,w,h,color,power,angle);};
 if(!front){
  add(Bloom,x,ground,106,22,col,.57f*p);add(Bloom,x,ground,65,10,pale,.39f*p);
  add(Ring,x,ground-1,93,17,col,.75f*p);add(Ring,x,ground-2,67,11,pale,.6f*p);
  float q=fract(t/3.8f);add(Ring,x,ground-2-q*4,62+q*49,12+q*9,col,std::sin(q*3.141593f)*.5f,std::sin(t*.6f)*.035f);
  add(Beam,x,y-50,50,100,col,.98f*p);add(Beam,x,y-48,18,96,pale,.72f*p);
  add(BeamTail,x,y+(ground-y)*.5f,23,ground-y,col,.34f,3.141593f);
  add(Bloom,x,y,100,100,col,.65f*p);add(Bloom,x,y,48,48,pale,.42f*p);
  return;
 }
 add(Bloom,x,y,54,54,col,.21f*p);add(Star,x,y,190,34,col,.68f*p);add(Star,x,y,58,22,pale,.43f*p);
 add(Bloom,x+1,y+2,12,12,{255,240,255},.28f*p);
 float q=fract(t/4.2f);add(Ring,x,ground-8-q*28,57+q*13,10,col,.26f*std::sin(q*3.141593f),(-8+q*16)*.0174533f);
 for(int i=0;i<20;++i){
  float seed=i*7.731f;float off=fract(std::sin(seed+1)*43758.5453f),speed=.14f+.11f*fract(std::sin(seed+2)*23731.13f);
  float phase=fract(std::sin(seed+3)*12345.67f)*6.283185f,amp=8+17*fract(std::sin(seed+4)*5341.17f),size=2.5f+4*fract(std::sin(seed+5)*7891.31f);
  float a=fract(t*speed+off),alpha=std::pow(std::sin(a*3.141593f),1.5f)*(.24f+.5f*size/6.5f);
  add(Spark,x+std::sin(phase+a*5)*amp*(.45f+.55f*a),y+12-a*131,size,size,pale,alpha);
 }
 for(int i=0;i<3;++i){float a=t*(.5f+i*.12f)+i*2.094f;add(Spark,x+std::cos(a)*22,y+std::sin(a)*13,5,5,pale,.62f);}
}
