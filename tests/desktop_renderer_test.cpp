#include "../src/desktop_beam_renderer.cpp"
#include <cassert>
#include <cstdio>
#include <cstring>
int created=0,drawn=0,disposed=0,released=0;
void diagnostics_write(const char*,...){}
void free_handle(void*){++released;}
void get_field(void* f,void*,void* out){
 switch(uintptr_t(f)){
 case 1:{void* batch=reinterpret_cast<void*>(100);std::memcpy(out,&batch,sizeof(batch));break;}
 case 2:{Vec2 pos{10,20};std::memcpy(out,&pos,sizeof(pos));break;}
 case 3:{int w=800;std::memcpy(out,&w,sizeof(w));break;}
 case 4:{int h=600;std::memcpy(out,&h,sizeof(h));break;}
 }
}
void* make_texture(int w,int h,texture_format_t format,void* pixels,size_t size){
 assert(format==TEXTURE_FORMAT_RGBA32&&size==size_t(w*h*4));
 const char* names[]={"bloom","ring","star","spark","beam","tail"};
 std::vector<unsigned char> source(size);std::ifstream f(directory+"/effects/"+names[created]+".rgba",std::ios::binary);f.read(reinterpret_cast<char*>(source.data()),size);
 auto bytes=static_cast<unsigned char*>(pixels);size_t stride=w*4;
 for(int row=0;row<h;++row)assert(!std::memcmp(bytes+row*stride,source.data()+(h-1-row)*stride,stride));
 return reinterpret_cast<void*>(uintptr_t(200+(++created)));
}
bool invoke(void* method,void* instance,void*,void** args){
 if(uintptr_t(method)==5){
  assert(uintptr_t(instance)==100&&args&&args[0]&&args[1]&&args[2]);
  auto color=static_cast<Color*>(args[2]);auto rect=static_cast<EffectRect*>(args[1]);
  assert(color->a==0&&rect->width>0&&rect->height>0);if(drawn==0)assert(color->r>0&&color->b>color->r);++drawn;return true;
 }
 assert(uintptr_t(method)==6);++disposed;return true;
}
int main(int argc,char** argv){
 assert(argc==2);configure_beam_resources(argv[1]);
 batchField=(void*)1;screenField=(void*)2;widthField=(void*)3;heightField=(void*)4;drawMethod=(void*)5;disposeMethod=(void*)6;
 patchlib_field_get_value=get_field;patchlib_method_invoke_args=invoke;patchlib_free=free_handle;terraria_texture2d_create=make_texture;
 assert(draw_rarity_beam(110,180,6,{185,105,255},222,false));
 assert(draw_rarity_beam(110,180,6,{185,105,255},222,true));
 assert(created==TextureCount&&drawn>20&&released==2);
 reset_beam_renderer();assert(disposed==TextureCount&&released==TextureCount+2);
 reset_beam_renderer();assert(disposed==TextureCount);
 puts("PASS: desktop managed drawing, RGBA alpha, texture row conversion, shared upload and handle cleanup");
}
