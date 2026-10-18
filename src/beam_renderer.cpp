#include "beam_renderer.hpp"
#include "diagnostics.hpp"
#include "tefkernel/patchlib/field.h"
#include "tefkernel/patchlib/method.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace {
struct Vec2 { float x, y; };
struct Rect { int x, y, width, height; };
// Mobile XNA Color is declared in Microsoft.Xna.Framework.Graphics, in ABGR byte order.
struct Color { uint8_t a, b, g, r; };
static_assert(sizeof(Vec2) == 8 && sizeof(Rect) == 16 && sizeof(Color) == 4);
using DrawRect = void (*)(void*, void*, Rect, Color, const void*);
using GetScreenPosition = Vec2 (*)(const void*);
patch_handle_t g_batch_field{}, g_pixel_field{}, g_pixel_value_field{}, g_draw_method{}, g_screen_method{};
DrawRect g_draw{};
GetScreenPosition g_screen{};
bool g_texture_logged{}, g_first_draw_logged{};

patch_handle_t read_object(patch_handle_t field, patch_handle_t instance = nullptr) {
    auto address = patchlib_field_get_pointer(field, instance);
    patch_handle_t value = nullptr;
    if (address) std::memcpy(&value, address, sizeof(value));
    return value;
}
Color tinted(RarityColor rgb, float opacity, float whiten = 0.0f) {
    auto channel = [=](uint8_t v) { return static_cast<uint8_t>((v + (255-v)*whiten) * opacity); };
    return {static_cast<uint8_t>(255*opacity), channel(rgb.b), channel(rgb.g), channel(rgb.r)};
}
void ellipse_ring(void* batch, void* texture, float cx, float cy,
                  int rx, int ry, int stroke, Color color) {
    const int x = static_cast<int>(std::lround(cx));
    const int y = static_cast<int>(std::lround(cy));
    for (int row = -ry; row <= ry; ++row) {
        const float t = static_cast<float>(row) / ry;
        const int outer = static_cast<int>(std::lround(rx * std::sqrt(std::max(0.0f, 1.0f-t*t))));
        const int inner_ry = ry-stroke;
        int inner = -1;
        if (inner_ry > 0 && std::abs(row) < inner_ry) {
            const float u = static_cast<float>(row) / inner_ry;
            inner = static_cast<int>(std::lround((rx-stroke)*std::sqrt(std::max(0.0f,1.0f-u*u))));
        }
        if (inner < 0) {
            g_draw(batch, texture, Rect{x-outer,y+row,outer*2+1,1}, color, g_draw_method);
        } else {
            const int width = std::max(1,outer-inner);
            g_draw(batch, texture, Rect{x-outer,y+row,width,1}, color, g_draw_method);
            g_draw(batch, texture, Rect{x+inner+1,y+row,width,1}, color, g_draw_method);
        }
    }
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
    // Two Draw overloads have three parameters; select exact parameter types.
    g_draw_method = patchlib_type_get_method_by_param_types(sb, "Draw", 3, draw_types);
    if (!g_batch_field || !g_pixel_field || !g_screen_method || !g_draw_method) {
        diagnostics_write("Renderer member lookup failed: batch=%p pixel=%p screen=%p draw=%p",
                          g_batch_field, g_pixel_field, g_screen_method, g_draw_method);
        return false;
    }
    g_draw = reinterpret_cast<DrawRect>(patchlib_method_get_pointer(g_draw_method));
    g_screen = reinterpret_cast<GetScreenPosition>(patchlib_method_get_pointer(g_screen_method));
    diagnostics_write("Native rectangle renderer resolved: draw=%p screen=%p", (void*)g_draw, (void*)g_screen);
    return g_draw && g_screen;
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
    const int height = rarity == 0 ? 36 : 72;
    const float pulse = 0.88f + 0.12f * std::sin(frame * 0.045f);
    // Current world SpriteBatch is already active in Main.DrawItem. Do not Begin/End it.
    // Stacked alpha strips create a continuous beam with a soft outer glow and bright core.
    for (int part = 0; part < 12; ++part) {
        const float t = static_cast<float>(part) / 12.0f;
        const float fade = (1.0f-t)*(1.0f-t)*pulse;
        const int y = static_cast<int>(base) - (part+1)*height/12;
        const int h = height/12 + 1;
        const Rect outer{static_cast<int>(x)-11, y, 22, h};
        const Rect body{static_cast<int>(x)-5, y, 10, h};
        const Rect core{static_cast<int>(x)-1, y, 2, h};
        g_draw(batch, texture, outer, tinted(rgb, 0.16f*fade), g_draw_method);
        g_draw(batch, texture, body, tinted(rgb, 0.50f*fade), g_draw_method);
        g_draw(batch, texture, core, tinted(rgb, 0.90f*fade, 0.55f), g_draw_method);
    }
    // A low base ring anchors the effect; a second ring rises over a four-second
    // cycle at 60 FPS, fading in and out so the loop has no visible reset.
    ellipse_ring(batch, texture, x, base+6.0f, 13, 4, 1, tinted(rgb,0.28f*pulse));
    const float progress = static_cast<float>(frame % 240u) / 240.0f;
    const float ring_y = base+6.0f-progress*height*0.75f;
    const float opacity = std::sin(progress*3.14159265f);
    const int radius = 12+static_cast<int>(progress*4.0f);
    ellipse_ring(batch, texture, x, ring_y, radius+2, 5, 2, tinted(rgb,0.16f*opacity));
    ellipse_ring(batch, texture, x, ring_y, radius, 4, 1, tinted(rgb,0.80f*opacity,0.20f));
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
    g_texture_logged = g_first_draw_logged = false;
}
