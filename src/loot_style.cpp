#include "loot_style.hpp"
#include <cmath>
#include <algorithm>

namespace {
constexpr float pi = 3.14159265359f;
struct Canvas {
    void* context;
    EffectDraw draw;
    RarityColor color;
    void rect(float x,float y,int w,int h,float alpha,float white=0) const {
        alpha=std::clamp(alpha,0.0f,1.0f);
        if(alpha<0.004f || w<1 || h<1) return;
        auto c=[=](uint8_t v){ return static_cast<uint8_t>((v+(255-v)*white)*alpha); };
        draw(context,{static_cast<int>(std::lround(x)),static_cast<int>(std::lround(y)),w,h},
             {static_cast<uint8_t>(255*alpha),c(color.b),c(color.g),c(color.r)});
    }
    void ellipse(float x,float y,int rx,int ry,int stroke,float alpha,float white=0) const {
        for(int row=-ry;row<=ry;++row) {
            float u=static_cast<float>(row)/ry;
            int outer=static_cast<int>(std::lround(rx*std::sqrt(std::max(0.0f,1-u*u))));
            int inner=-1;
            if(stroke>0 && ry>stroke && std::abs(row)<ry-stroke) {
                float v=static_cast<float>(row)/(ry-stroke);
                inner=static_cast<int>(std::lround((rx-stroke)*std::sqrt(std::max(0.0f,1-v*v))));
            }
            if(inner<0) rect(x-outer,y+row,outer*2+1,1,alpha,white);
            else {
                int width=std::max(1,outer-inner);
                rect(x-outer,y+row,width,1,alpha,white);
                rect(x+inner+1,y+row,width,1,alpha,white);
            }
        }
    }
    void star(float x,float y,float alpha,int reach=2) const {
        rect(x-2,y-2,5,5,alpha*0.06f);
        rect(x-reach,y,reach*2+1,1,alpha*0.45f,0.35f);
        rect(x,y-reach,1,reach*2+1,alpha*0.45f,0.35f);
        rect(x,y,1,1,alpha,0.85f);
    }
};
float cycle(float x){return x-std::floor(x);}
}
void draw_loot_style(void* context,EffectDraw draw,float x,float y,
                     int rarity,RarityColor color,float seconds) {
    if(!draw || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(seconds))return;
    const int tier=rarity<=0?0:rarity<=2?1:rarity<=5?2:rarity<=8?3:4;
    const Canvas c{context,draw,color};
    const float pulse=0.88f+0.12f*std::sin(seconds*2.1f);
    const float ground=y+9;
    // Soft pool and elliptical rim, kept below the actual item icon.
    c.ellipse(x,ground,20+tier*2,4,0,0.025f*pulse);
    c.ellipse(x,ground,15+tier*2,3,0,0.045f*pulse);
    c.ellipse(x,ground,10+tier,2,0,0.09f*pulse);
    c.ellipse(x,ground,13+tier*2,3,1,0.48f*pulse);
    // Luminous orbital shell with a transparent centre rather than a solid orb.
    const int r=9+tier;
    c.ellipse(x,y,r+6,r+6,5,0.045f*pulse);
    c.ellipse(x,y,r+3,r+3,3,0.10f*pulse);
    c.ellipse(x,y,r+1,r+1,2,0.25f*pulse);
    c.ellipse(x,y,r,r,1,0.88f*pulse,0.40f);
    // Smoothly taper the beam; common drops only have the shell and ground glow.
    const int height=tier==0?0:16+tier*10;
    if(tier>0)for(int i=0;i<18;++i) {
        const float t=static_cast<float>(i)/18;
        const float a=(1-t)*(1-t)*pulse;
        const int h=(i+1)*height/18-i*height/18;
        const float py=std::lround(y)-8-(i+1)*height/18;
        const int spread=std::max(1,static_cast<int>((1-t)*(3+tier)));
        c.rect(x-spread-3,py,spread*2+7,h,a*0.10f);
        c.rect(x-spread,py,spread*2+1,h,a*0.25f);
        c.rect(x,py,1,h,a*0.78f,0.65f);
    }
    // Deterministic orbiting highlights and upward sparkles. No dust persists in-world.
    const int sparks=tier==0?0:2+tier*2;
    for(int i=0;i<sparks;++i) {
        const float p=cycle(seconds*(0.22f+i*0.008f)+i*0.618034f);
        const float phase=i*2.399963f+p*4;
        const float sx=x+std::sin(phase)*(8+p*(8+tier*2));
        const float sy=y+5-p*(height+14);
        c.star(sx,sy,std::sin(p*pi)*(0.35f+tier*0.08f),i%3==0?2:1);
    }
    for(int i=0;i<2+tier;++i) {
        const float a=seconds*(i%2==0?0.8f:-0.6f)+i*2.4f;
        c.star(x+std::cos(a)*(r+1),y+std::sin(a)*(r+1),0.36f,1);
    }
    if(tier>=2) {
        // One expanding ground wave, plus a slow rising halo for higher tiers.
        const float p=cycle(seconds*0.25f);
        c.ellipse(x,ground,15+static_cast<int>(p*13),3+static_cast<int>(p*2),1,
                  std::sin(p*pi)*0.22f);
        if(tier>=3)c.ellipse(x,ground-p*32,15+static_cast<int>(p*5),4,1,std::sin(p*pi)*0.32f);
        // Horizontal flare and bright centre point, matching the chosen reference.
        for(int i=3;i>=0;--i)c.rect(x-(8+i*5),y,17+i*10,1,(0.025f+0.035f*(3-i))*pulse,0.15f);
        c.star(x,y,0.70f*pulse,3);
    }
}
