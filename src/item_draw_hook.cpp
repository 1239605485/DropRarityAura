#include "item_draw_hook.hpp"
#include "rarity_palette.hpp"
#include "beam_renderer.hpp"
#include "diagnostics.hpp"
#include "tefkernel/patchlib/field.h"
#include "tefkernel/patchlib/method.h"
#include <cstring>
#include <cstdint>

namespace {
struct Vec2 { float x, y; };
patch_handle_t g_inner{}, g_rare{}, g_position{}, g_width{}, g_height{}, g_stack{}, g_type{}, g_active_method{};
using GetActive = bool (*)(void*, const void*);
GetActive g_active = nullptr;
patch_hook_id_t g_hook = PATCH_HOOK_INVALID_ID;
unsigned g_frames[1024]{};
bool g_first_callback = false;
struct Rect{int x,y,w,h;};
using Basics=void(*)(void*,void*,int,void**,Rect*,Rect*,const void*);
Basics basics{};patch_handle_t basicsMethod{};


template<class T> bool read_field(patch_handle_t field, patch_handle_t object, T& out) {
    auto ptr = patchlib_field_get_pointer(field, object);
    if (!ptr) return false;
    std::memcpy(&out, ptr, sizeof(T));
    return true;
}
void draw_item_layers(patch_handle_t instance, void** args, bool front) {
    if (!g_first_callback) { diagnostics_write("Main.DrawItem callback reached"); g_first_callback = true; }
    if (!args || !args[0] || !args[1]) return;
    const auto world_item = *static_cast<patch_handle_t*>(args[0]);
    const int slot = *static_cast<int*>(args[1]);
    if (!world_item || slot < 0 || slot >= 1024) return;
    patch_handle_t item = nullptr;
    if (!g_active || !g_active(world_item,g_active_method) ||
        !read_field(g_inner, world_item, item) || !item) { g_frames[slot]=0; return; }
    int stack = 0, type = 0;
    if (!read_field(g_stack,item,stack) || !read_field(g_type,item,type) || stack<=0 || type<=0) {
        g_frames[slot]=0; return;
    }
    int rarity = 0, width = 16, height = 16;
    Vec2 position{};
    if (!read_field(g_rare,item,rarity) || !read_field(g_position,world_item,position) ||
        !read_field(g_width,world_item,width) || !read_field(g_height,world_item,height)) return;
    RarityColor color{};
    if (!rarity_color(rarity,color)) return;
    void* texture{};Rect frame{},glow{};
    if(basics)basics(instance,item,slot,&texture,&frame,&glow,basicsMethod);
    // World-item sprites are bottom-aligned; their visual centre differs from
    // the physics hitbox centre for tall weapons. Query the actual frame.
    const float ground=position.y+height;
    const float visualY=ground-(frame.h>0?frame.h:height)*.5f+2.f;
    draw_rarity_beam(position.x+width*.5f,visualY,rarity,color,++g_frames[slot],ground,front);
}
bool draw_item_prefix(patch_handle_t instance,void** args,const patch_method_signature_t*,void*){draw_item_layers(instance,args,false);return false; /* PatchLib: false runs the original DrawItem. */}
void draw_item_postfix(patch_handle_t instance,void** args,void*,const patch_method_signature_t*){draw_item_layers(instance,args,true);}
}
bool install_item_draw_hook() {
    // APIs are populated by KernelLoader's plugin registration after loading this library.
    if (!patchlib_type_get_type || !patchlib_type_get_field || !patchlib_field_get_pointer ||
        !patchlib_type_get_method_by_param_count || !patchlib_type_get_method_by_param_types ||
        !patchlib_method_get_pointer || !patchlib_install_prepost_hook) {
        diagnostics_write("Required PatchLib API injection missing"); return false;
    }
    const auto main = patchlib_type_get_type("Terraria","Main");
    const auto world = patchlib_type_get_type("Terraria","WorldItem");
    const auto item = patchlib_type_get_type("Terraria","Item");
    const auto entity = patchlib_type_get_type("Terraria","Entity");
    if (!main || !world || !item || !entity) { diagnostics_write("Item type lookup failed"); return false; }
    g_inner = patchlib_type_get_field(world,"inner");
    g_rare = patchlib_type_get_field(item,"rare");
    g_stack = patchlib_type_get_field(item,"stack");
    g_type = patchlib_type_get_field(item,"type");
    g_active_method = patchlib_type_get_method_by_param_count(world,"get_active",0);
    g_active = g_active_method ? reinterpret_cast<GetActive>(patchlib_method_get_pointer(g_active_method)) : nullptr;
    g_position = patchlib_type_get_field(entity,"position");
    g_width = patchlib_type_get_field(entity,"width");
    g_height = patchlib_type_get_field(entity,"height");
    basicsMethod=patchlib_type_get_method_by_param_count(main,"DrawItem_GetBasics",5);
    basics=basicsMethod?reinterpret_cast<Basics>(patchlib_method_get_pointer(basicsMethod)):nullptr;
    const auto draw = patchlib_type_get_method_by_param_count(main,"DrawItem",2);
    if (!g_inner || !g_rare || !g_stack || !g_type || !g_active || !g_position || !g_width || !g_height || !draw) {
        diagnostics_write("Item field or DrawItem lookup failed"); return false;
    }
    if (!initialize_beam_renderer()) return false;
    g_hook = patchlib_install_prepost_hook(draw,draw_item_prefix,draw_item_postfix);
    if (g_hook == PATCH_HOOK_INVALID_ID) { diagnostics_write("DrawItem hook installation failed"); return false; }
    std::memset(g_frames,0,sizeof(g_frames));
    g_first_callback = false;
    diagnostics_write("DrawItem hook installed; direct ARM64 SpriteBatch renderer enabled");
    return true;
}
void remove_item_draw_hook() {
    if (g_hook != PATCH_HOOK_INVALID_ID && patchlib_uninstall_hook) patchlib_uninstall_hook(g_hook);
    g_hook = PATCH_HOOK_INVALID_ID;
    reset_beam_renderer();
}
