#include "beam_renderer.hpp"
#include "loot_style.hpp"
#include "diagnostics.hpp"
#include "tefkernel/patchlib/field.h"
#include "tefkernel/patchlib/method.h"
#include "tefkernel/terraria/texture2d.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
namespace {
struct Vec2{float x,y;};
struct Root{uintptr_t value;};
using Draw=void(*)(void*,void*,EffectRect,EffectColor,const void*);
using GetScreen=Vec2(*)(const void*);
using Alloc=Root(*)(void*,const void*);
using Free=void(*)(Root*,const void*);
using SetFilter=void(*)(void*,int,const void*);
patch_handle_t batchField{},drawMethod{},screenMethod{},allocMethod{},freeMethod{},filterMethod{},unityField{};
Draw draw{};GetScreen screen{};Alloc alloc{};Free freeRoot{};SetFilter filter{};
void* textures[TextureCount]{};Root roots[TextureCount]{};
using GetInt=int(*)(const void*);
patch_handle_t widthMethod{},heightMethod{};GetInt screenWidth{},screenHeight{};
std::string directory;bool ready{},failed{},logged{};
void* read(patch_handle_t f,void* o=nullptr){void* v{};auto p=patchlib_field_get_pointer(f,o);if(p)std::memcpy(&v,p,sizeof(v));return v;}
bool upload(){
 if(ready)return true;if(failed)return false;
 const char* names[]={"bloom","ring","star","spark","beam","tail"};
 for(int i=0;i<TextureCount;++i){
  int w=i>=Beam?96:192,h=192;std::ifstream f(directory+"/effects/"+names[i]+".rgba",std::ios::binary);
  std::vector<unsigned char> bytes(w*h*4);f.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
  if(!f||!terraria_texture2d_create){failed=true;diagnostics_write("Effect resource unavailable: %s",names[i]);return false;}
  textures[i]=terraria_texture2d_create(w,h,TEXTURE_FORMAT_RGBA32,bytes.data(),bytes.size());
  if(!textures[i]){failed=true;diagnostics_write("Effect texture upload failed: %s",names[i]);return false;}
  roots[i]=alloc(textures[i],allocMethod);
  if(!roots[i].value){failed=true;return false;}
  auto unity=read(unityField,textures[i]);if(unity)filter(unity,1,filterMethod);
 }
 ready=true;diagnostics_write("Six approved gradient textures uploaded and GC rooted");return true;
}
struct Context{void* batch;};
void emit(void* c,EffectTexture tex,float x,float y,float w,float h,RarityColor rgb,float power,float angle){
 if(w<=0||h<=0||power<=0)return;
 auto channel=[power](int v){return static_cast<uint8_t>(std::clamp(v*power,0.f,255.f));};
 // Premultiplied texture with zero vertex alpha: RGB adds to the existing batch
 // without darkening the world. No Begin/End or blend-state changes inside DrawItem.
 EffectColor color{0,channel(rgb.b),channel(rgb.g),channel(rgb.r)};
 draw(static_cast<Context*>(c)->batch,textures[tex],{(int)std::lround(x-w*.5f),(int)std::lround(y-h*.5f),std::max(1,(int)std::lround(w)),std::max(1,(int)std::lround(h))},color,drawMethod);
 (void)angle;
}
}
void configure_beam_resources(const char* path){directory=path?path:"";}
bool initialize_beam_renderer(){
 const auto main=patchlib_type_get_type("Terraria","Main");
 const auto sb=patchlib_type_get_type("Microsoft.Xna.Framework.Graphics","SpriteBatch");
 const auto gc=patchlib_type_get_type("System.Runtime.InteropServices","GCHandle");
 const auto ut=patchlib_type_get_type("UnityEngine","Texture");
 const auto xt=patchlib_type_get_type("Microsoft.Xna.Framework.Graphics","Texture2D");
 batchField=patchlib_type_get_field(main,"spriteBatch");
 screenMethod=patchlib_type_get_method_by_param_count(main,"get_screenPosition",0);
 widthMethod=patchlib_type_get_method_by_param_count(main,"get_screenWidth",0);
 heightMethod=patchlib_type_get_method_by_param_count(main,"get_screenHeight",0);
 if(!widthMethod||!heightMethod)return false;
 screenWidth=reinterpret_cast<GetInt>(patchlib_method_get_pointer(widthMethod));
 screenHeight=reinterpret_cast<GetInt>(patchlib_method_get_pointer(heightMethod));
 const patch_handle_t types[]={xt,patchlib_type_get_type("Microsoft.Xna.Framework","Rectangle"),patchlib_type_get_type("Microsoft.Xna.Framework.Graphics","Color")};
 drawMethod=patchlib_type_get_method_by_param_types(sb,"Draw",3,types);
 allocMethod=patchlib_type_get_method_by_param_count(gc,"Alloc",1);
 freeMethod=patchlib_type_get_method_by_param_count(gc,"Free",0);
 filterMethod=patchlib_type_get_method_by_param_count(ut,"set_filterMode",1);
 unityField=patchlib_type_get_field(xt,"_unityTexture");
 if(!batchField||!screenMethod||!drawMethod||!allocMethod||!freeMethod||!filterMethod||!unityField)return false;
 draw=reinterpret_cast<Draw>(patchlib_method_get_pointer(drawMethod));screen=reinterpret_cast<GetScreen>(patchlib_method_get_pointer(screenMethod));
 alloc=reinterpret_cast<Alloc>(patchlib_method_get_pointer(allocMethod));freeRoot=reinterpret_cast<Free>(patchlib_method_get_pointer(freeMethod));filter=reinterpret_cast<SetFilter>(patchlib_method_get_pointer(filterMethod));
 return draw&&screen&&alloc&&freeRoot&&filter;
}
bool draw_rarity_beam(float wx,float wy,int rarity,RarityColor rgb,unsigned frame,float ground,bool front){
 auto batch=read(batchField);if(!batch)return false;
 auto s=screen(screenMethod);float x=wx-s.x,y=wy-s.y;
 if(!std::isfinite(x)||!std::isfinite(y)||x < -110||x > screenWidth(widthMethod)+110||y < -110||y > screenHeight(heightMethod)+150)return false;
 if(!upload())return false;
 static const auto epoch=std::chrono::steady_clock::now();float t=std::chrono::duration<float>(std::chrono::steady_clock::now()-epoch).count();
 Context c{batch};draw_loot_style(&c,emit,x,y,ground-s.y,rarity,rgb,t,front);
 if(!logged){diagnostics_write("Layered renderer active: rarity=%d",rarity);logged=true;}(void)frame;return true;
}
void reset_beam_renderer(){
 for(int i=0;i<TextureCount;++i){if(roots[i].value&&freeRoot)freeRoot(&roots[i],freeMethod);roots[i]={};textures[i]=nullptr;}
 ready=failed=logged=false;
}
