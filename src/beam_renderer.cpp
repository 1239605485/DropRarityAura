#include "beam_renderer.hpp"
#include "loot_style.hpp"
#include <chrono>
#include "diagnostics.hpp"
#include "tefkernel/patchlib/field.h"
#include "tefkernel/patchlib/method.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace {
struct Vec2 { float x, y; };
using Rect = EffectRect;
using Color = EffectColor;
static_assert(sizeof(Vec2) == 8 && sizeof(Rect) == 16 && sizeof(Color) == 4);
using DrawRect = void (*)(void*, void*, Rect, Color, const void*);
using GetScreenPosition = Vec2 (*)(const void*);
using GetDimension = int (*)(const void*);
patch_handle_t g_batch_field{}, g_pixel_field{}, g_pixel_value_field{}, g_draw_method{}, g_screen_method{};
DrawRect g_draw{};
GetScreenPosition g_screen{};
patch_handle_t g_screen_w_method{},g_screen_h_method{};
GetDimension g_screen_w{},g_screen_h{};
bool g_texture_logged{}, g_first_draw_logged{};

patch_handle_t read_object(patch_handle_t field, patch_handle_t instance = nullptr) {
    auto address = patchlib_field_get_pointer(field, instance);
    patch_handle_t value = nullptr;
    if (address) std::memcpy(&value, address, sizeof(value));
    return value;
}
struct DrawContext { void* batch; void* texture; };
void emit_effect(void* context,EffectRect rect,EffectColor color) {
    const auto& c=*static_cast<DrawContext*>(context);
    g_draw(c.batch,c.texture,rect,color,g_draw_method);
}

}

bool initialize_beam_renderer() {
    reset_beam_renderer();
    const auto main = patchlib_type_get_type("Terraria", "Main");
    const auto assets = patchlib_type_get_type("Terraria.GameContent", "TextureAssets");
    const auto sb = patchlib_type_get_type("Microsoft.Xna.Framework.Graphics", "SpriteBatch");
    const patch_handle_t draw_types[] = {
        patchlib_type_get_type("Microsoft.Xna.Framework.Graphics", "Texture2D"),
        patchlib_type_get_type("Microsoft.Xna.Framework", "Rectangle"),
        patchlib_type_get_type("Microsoft.Xna.Framework.Graphics", "Color"),
    };
    if (!main || !assets || !sb || !draw_types[0] || !draw_types[1] || !draw_types[2]) {
        diagnostics_write("Renderer type lookup failed");
        return false;
    }
    g_batch_field = patchlib_type_get_field(main, "spriteBatch");
    g_pixel_field = patchlib_type_get_field(assets, "MagicPixel");
    g_screen_method = patchlib_type_get_method_by_param_count(main, "get_screenPosition", 0);
    g_screen_w_method = patchlib_type_get_method_by_param_count(main,"get_screenWidth",0);
    g_screen_h_method = patchlib_type_get_method_by_param_count(main,"get_screenHeight",0);
    // Two Draw overloads have three parameters; select exact parameter types.
    g_draw_method = patchlib_type_get_method_by_param_types(sb, "Draw", 3, draw_types);
    if (!g_batch_field || !g_pixel_field || !g_screen_method || !g_screen_w_method || !g_screen_h_method || !g_draw_method) {
        diagnostics_write("Renderer member lookup failed: batch=%p pixel=%p screen=%p draw=%p",
                          g_batch_field, g_pixel_field, g_screen_method, g_draw_method);
        return false;
    }
    g_draw = reinterpret_cast<DrawRect>(patchlib_method_get_pointer(g_draw_method));
    g_screen = reinterpret_cast<GetScreenPosition>(patchlib_method_get_pointer(g_screen_method));
    g_screen_w=reinterpret_cast<GetDimension>(patchlib_method_get_pointer(g_screen_w_method));
    g_screen_h=reinterpret_cast<GetDimension>(patchlib_method_get_pointer(g_screen_h_method));
    diagnostics_write("Native rectangle renderer resolved: draw=%p screen=%p", (void*)g_draw, (void*)g_screen);
    return g_draw && g_screen && g_screen_w && g_screen_h;
}

bool draw_rarity_beam(float world_x, float world_y, int rarity, RarityColor rgb, unsigned frame) {
#if defined(__ANDROID__) && defined(__aarch64__)
    auto batch = read_object(g_batch_field);
    auto asset = read_object(g_pixel_field);
    if (!batch || !asset) return false; // Assets are populated after mod initialization.
    if (!g_pixel_value_field) {
        // IL2CPP objects begin with their inflated Il2CppClass pointer.
        patch_handle_t klass = nullptr;
        std::memcpy(&klass, asset, sizeof(klass));
        g_pixel_value_field = patchlib_type_get_field(klass, "Value");
    }
    if (!g_pixel_value_field) return false;
    auto texture = read_object(g_pixel_value_field, asset);
    if (!texture) return false;
    if (!g_texture_logged) { diagnostics_write("MagicPixel texture ready"); g_texture_logged = true; }
    const Vec2 screen = g_screen(g_screen_method);
    const float x = world_x - screen.x;
    const float base = world_y - screen.y;
    if (!std::isfinite(x) || !std::isfinite(base)) return false;
    if(x < -80 || x > g_screen_w(g_screen_w_method)+80 ||
       base < -30 || base > g_screen_h(g_screen_h_method)+90) return false;
    static const auto epoch=std::chrono::steady_clock::now();
    const float seconds=std::chrono::duration<float>(std::chrono::steady_clock::now()-epoch).count();
    DrawContext context{batch,texture};
    draw_loot_style(&context,emit_effect,x,base,rarity,rgb,seconds);
    (void)frame;
    if (!g_first_draw_logged) {
        diagnostics_write("First native beam drawn: rarity=%d world=(%.1f,%.1f) screen=(%.1f,%.1f)", rarity,world_x,world_y,x,base);
        g_first_draw_logged = true;
    }
    return true;
#else
    (void)world_x; (void)world_y; (void)rarity; (void)rgb; (void)frame;
    return false; // Android ARM64 is the supported runtime for this renderer.
#endif
}
void reset_beam_renderer() {
    g_batch_field = g_pixel_field = g_pixel_value_field = g_draw_method = g_screen_method = nullptr;
    g_draw = nullptr; g_screen = nullptr;
    g_screen_w = g_screen_h = nullptr;
    g_screen_w_method = g_screen_h_method = nullptr;
    g_texture_logged = g_first_draw_logged = false;
}
