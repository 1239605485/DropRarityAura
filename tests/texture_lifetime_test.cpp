#include "../src/beam_renderer.cpp"
#include <cassert>
#include <cstdio>
int creates=0,pins=0,frees=0,disposals=0,fail_at=3;
void diagnostics_write(const char*,...){}
void* make_texture(int,int,texture_format_t,void*,size_t){++creates;return creates==fail_at?nullptr:reinterpret_cast<void*>(uintptr_t(creates));}
Root pin(void*,const void*){return {uintptr_t(++pins)};}
void unpin(Root*,const void*){++frees;}
void destroy(void*,const void*){++disposals;}
void* no_field(void*,void*){return nullptr;}
int main(int argc,char** argv){
 assert(argc==2);configure_beam_resources(argv[1]);
 terraria_texture2d_create=make_texture;patchlib_field_get_pointer=no_field;
 alloc=pin;freeRoot=unpin;dispose=destroy;
 assert(!upload()&&failed&&!ready);
 assert(pins==2&&frees==2&&disposals==2);
 for(int i=0;i<TextureCount;++i)assert(!textures[i]&&!roots[i].value);
 assert(!upload()&&creates==3); // Failure is not retried on every item/frame.
 reset_beam_renderer();creates=pins=frees=disposals=0;fail_at=-1;
 assert(upload()&&ready&&creates==TextureCount&&pins==TextureCount);
 assert(upload()&&creates==TextureCount); // Upload once, share across all drops.
 reset_beam_renderer();
 assert(frees==TextureCount&&disposals==TextureCount);
 reset_beam_renderer();assert(frees==TextureCount&&disposals==TextureCount);
 puts("PASS: partial texture failures roll back, textures upload once, unload disposes and unroots each texture once");
}
