#include "desktop_managed.hpp"
#include "item_draw_hook.hpp"
#include "beam_renderer.hpp"
#include "diagnostics.hpp"
namespace {
struct Vec2{float x,y;};
desktop::Metadata metadata;
patch_handle_t inner{},rare{},position{},width{},height{},stack{},type{},active{},activeMethod{};
patch_hook_id_t hook=PATCH_HOOK_INVALID_ID;
bool first{};
void layers(void** args,bool front){
 if(!args||!args[0]||!args[1])return;
 const auto world=*static_cast<patch_handle_t*>(args[0]);const int slot=*static_cast<int*>(args[1]);
 if(!world||slot<0||slot>=1024)return;
 bool live=false;
 if(activeMethod){if(!desktop::call(activeMethod,world,live)||!live)return;}
 else if(!desktop::read<bool>(active,world))return;
 desktop::Object itemRoot(inner?desktop::read<patch_handle_t>(inner,world):nullptr);
 auto item=inner?itemRoot.value:world;
 if(!item||desktop::read<int>(stack,item)<=0||desktop::read<int>(type,item)<=0)return;
 int rarity=desktop::read<int>(rare,item);RarityColor color{};if(!rarity_color(rarity,color))return;
 auto pos=desktop::read<Vec2>(position,world);int w=desktop::read<int>(width,world),h=desktop::read<int>(height,world);
 const float ground=pos.y+h;
 float visualHeight=desktop_item_visual_height(desktop::read<int>(type,item),float(h));
 draw_rarity_beam(pos.x+w*.5f,ground-visualHeight*.5f+2,rarity,color,ground,front);
 if(!first){diagnostics_write("Desktop DrawItem callback active");first=true;}
}
bool prefix(patch_handle_t,void** args,const patch_method_signature_t*,void*){layers(args,false);return false;}
void postfix(patch_handle_t,void** args,void*,const patch_method_signature_t*){layers(args,true);}
}
bool install_item_draw_hook(){
 if(!patchlib_type_get_type||!patchlib_type_get_field||!patchlib_field_get_value||!patchlib_method_invoke_args||!patchlib_free||!patchlib_install_prepost_hook)return false;
 auto main=metadata.type("Terraria","Main"),world=metadata.type("Terraria","WorldItem"),item=metadata.type("Terraria","Item"),entity=metadata.type("Terraria","Entity");
 inner=metadata.field(world,"inner");auto drawType=inner?world:item;
 rare=metadata.field(item,"rare");stack=metadata.field(item,"stack");type=metadata.field(item,"type");
 position=metadata.field(entity,"position");width=metadata.field(entity,"width");height=metadata.field(entity,"height");
 activeMethod=metadata.method(drawType,"get_active",0);active=activeMethod?nullptr:metadata.field(entity,"active");
 auto draw=metadata.method(main,"DrawItem",2);
 if(!draw||!rare||!stack||!type||!position||!width||!height||(!activeMethod&&!active)||!initialize_beam_renderer()){
  diagnostics_write("Desktop item or renderer lookup failed");remove_item_draw_hook();return false;
 }
 hook=patchlib_install_prepost_hook(draw,prefix,postfix);
 if(hook==PATCH_HOOK_INVALID_ID){remove_item_draw_hook();return false;}
 first=false;diagnostics_write("Desktop managed item hook installed");return true;
}
void remove_item_draw_hook(){
 if(hook!=PATCH_HOOK_INVALID_ID&&patchlib_uninstall_hook)patchlib_uninstall_hook(hook);
 hook=PATCH_HOOK_INVALID_ID;reset_beam_renderer();metadata.clear();
}
