#include "desktop_managed.hpp"
#include "beam_renderer.hpp"
#include "loot_style.hpp"
#include "diagnostics.hpp"
#include "tefkernel/terraria/texture2d.h"
#include "tefkernel/patchlib/struct/array.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>
namespace {
struct Vec2{float x,y;};
struct Color{uint8_t r,g,b,a;};
desktop::Metadata metadata;
patch_handle_t batchField{},screenField{},widthField{},heightField{},drawMethod{},disposeMethod{},itemAssets{},getType{},valueMethod{},animations{},getFrame{},frameHeight{};
patch_handle_t textures[TextureCount]{};
std::string directory;bool ready{},failed{};
void release(){for(auto& tex:textures){if(tex){if(disposeMethod)patchlib_method_invoke_args(disposeMethod,tex,nullptr,nullptr);patchlib_free(tex);tex=nullptr;}}ready=false;}
bool upload(){
 if(ready)return true;if(failed)return false;
 const char* names[]={"bloom","ring","star","spark","beam","tail"};
 for(int i=0;i<TextureCount;++i){
  int w=i>=Beam?96:192,h=192;size_t stride=w*4;
  std::vector<unsigned char> bytes(stride*h);
  std::ifstream f(directory+"/effects/"+names[i]+".rgba",std::ios::binary);f.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
  if(!f){release();failed=true;diagnostics_write("Desktop effect resource missing: %s",names[i]);return false;}
  // Assets are stored bottom-up for Unity. Desktop XNA expects top-down rows.
  for(int y=0;y<h/2;++y)for(size_t x=0;x<stride;++x)std::swap(bytes[y*stride+x],bytes[(h-1-y)*stride+x]);
  textures[i]=terraria_texture2d_create(w,h,TEXTURE_FORMAT_RGBA32,bytes.data(),bytes.size());
  if(!textures[i]){release();failed=true;diagnostics_write("Desktop texture upload failed: %s",names[i]);return false;}
 }
 ready=true;diagnostics_write("Desktop gradient textures ready");return true;
}
struct Context{patch_handle_t batch;};
void emit(void* context,EffectTexture tex,float x,float y,float w,float h,RarityColor rgb,float power){
 if(w<=0||h<=0||power<=0)return;
 auto channel=[power](int v){return static_cast<uint8_t>(std::clamp(v*power,0.f,255.f));};
 Color color{channel(rgb.r),channel(rgb.g),channel(rgb.b),0};
 EffectRect rect{int(std::lround(x-w*.5f)),int(std::lround(y-h*.5f)),std::max(1,int(std::lround(w))),std::max(1,int(std::lround(h)))};
 void* args[]={&textures[tex],&rect,&color};
 patchlib_method_invoke_args(drawMethod,static_cast<Context*>(context)->batch,nullptr,args);
}
}
void configure_beam_resources(const char* path){directory=path?path:"";}
bool initialize_beam_renderer(){
 auto main=metadata.type("Terraria","Main"),sb=metadata.type("Microsoft.Xna.Framework.Graphics","SpriteBatch"),texture=metadata.type("Microsoft.Xna.Framework.Graphics","Texture2D");
 auto color=metadata.type("Microsoft.Xna.Framework","Color"),rect=metadata.type("Microsoft.Xna.Framework","Rectangle");
 if(!color)color=metadata.type("Microsoft.Xna.Framework.Graphics","Color");
 if(!main||!sb||!texture||!color||!rect||!terraria_texture2d_create)return false;
 batchField=metadata.field(main,"spriteBatch");screenField=metadata.field(main,"screenPosition");widthField=metadata.field(main,"screenWidth");heightField=metadata.field(main,"screenHeight");
 patch_handle_t types[]={texture,rect,color};drawMethod=metadata.keep(patchlib_type_get_method_by_param_types(sb,"Draw",3,types));
 for(auto t=texture;t;){disposeMethod=metadata.method(t,"Dispose",0);if(disposeMethod)break;t=metadata.keep(patchlib_type_get_parent(t));}
 auto assets=metadata.type("Terraria.GameContent","TextureAssets");itemAssets=metadata.field(assets,"Item");
 animations=metadata.field(main,"itemAnimations");
 auto animation=metadata.type("Terraria.DataStructures","DrawAnimation");getFrame=metadata.method(animation,"GetFrame",2);frameHeight=metadata.field(rect,"Height");
 auto object=metadata.type("System","Object");getType=metadata.method(object,"GetType",0);
 return batchField&&drawMethod&&screenField&&widthField&&heightField;
}
float desktop_item_visual_height(int itemType,float fallback){
 if(!itemAssets||!getType||itemType<=0||!patchlib_array_at)return fallback;
 desktop::Object array(desktop::read<patch_handle_t>(itemAssets));if(!array.value)return fallback;
 patch_handle_t asset{};if(!patchlib_array_at(array.value,itemType,&asset)||!asset)return fallback;
 desktop::Object assetRoot(asset);
 if(!valueMethod){patch_handle_t t{};if(!desktop::call(getType,asset,t))return fallback;desktop::Object assetType(t);valueMethod=metadata.method(t,"get_Value",0);}
 patch_handle_t tex{};if(!desktop::call(valueMethod,asset,tex)||!tex)return fallback;
 desktop::Object texture(tex);
 if(animations&&getFrame&&frameHeight){
  desktop::Object list(desktop::read<patch_handle_t>(animations));patch_handle_t anim{};
  if(list.value&&patchlib_array_at(list.value,itemType,&anim)&&anim){
   desktop::Object animation(anim);int counter=-1;void* args[]={&tex,&counter};patch_handle_t frame{};
   // Desktop struct return values are boxed handles, not native buffers.
   if(desktop::call(getFrame,anim,frame,args)&&frame){desktop::Object boxedFrame(frame);int h=desktop::read<int>(frameHeight,frame);if(h>0)return float(h);}
  }
 }
 int h=terraria_texture2d_get_height(tex);
 return h>0?float(h):fallback;
}
bool draw_rarity_beam(float wx,float wy,int rarity,RarityColor color,float ground,bool front){
 desktop::Object batch(desktop::read<patch_handle_t>(batchField));if(!batch.value)return false;
 auto screen=desktop::read<Vec2>(screenField);
 int width=desktop::read<int>(widthField),height=desktop::read<int>(heightField);
 float x=wx-screen.x,y=wy-screen.y;
 if(!std::isfinite(x)||!std::isfinite(y)||x < -110||x > width+110||y < -110||y > height+150||!upload())return false;
 static const auto epoch=std::chrono::steady_clock::now();float t=std::chrono::duration<float>(std::chrono::steady_clock::now()-epoch).count();
 Context c{batch.value};draw_loot_style(&c,emit,x,y,ground-screen.y,rarity,color,t,front);return true;
}
void reset_beam_renderer(){release();metadata.clear();valueMethod=nullptr;ready=failed=false;}
