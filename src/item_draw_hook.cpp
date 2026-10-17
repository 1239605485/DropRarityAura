#include "item_draw_hook.hpp"

#include "rarity_palette.hpp"
#include "tefkernel/patchlib/field.h"
#include "tefkernel/patchlib/method.h"
#include "tefkernel/patchlib/property.h"
#include "tefkernel/patchlib/type.h"
#include "mod_logger.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace {
constexpr const char* kTag = "DropRarityAura";
constexpr int kDustWhiteTorch = 63;
constexpr uint32_t kSpawnInterval = 6;
constexpr float kBeamHeight = 128.0f;
constexpr size_t kTrackedSlots = 1024;
constexpr int kBeamSegments = 13;

struct Vector2Value { float x; float y; };
// XNA Color memory order from the supplied PE source: A, B, G, R.
struct ColorValue { uint8_t a; uint8_t b; uint8_t g; uint8_t r; };

patch_handle_t g_world_inner{};
patch_handle_t g_item_rare{};
patch_handle_t g_entity_position{};
patch_handle_t g_entity_width{};
patch_handle_t g_entity_height{};
patch_handle_t g_draw_dust{};
patch_hook_id_t g_draw_hook = PATCH_HOOK_INVALID_ID;
uint32_t g_draw_counts[kTrackedSlots]{};
bool g_seen_rare_item[kTrackedSlots]{};
bool g_dust_call_failed_logged{};

void draw_item_postfix(patch_handle_t, void** args, void*, const patch_method_signature_t*) {
    if (!args || !args[0] || !g_world_inner || !g_item_rare || !g_entity_position ||
        !g_entity_width || !g_entity_height || !g_draw_dust)
        return;

    const auto world_item = *static_cast<patch_handle_t*>(args[0]);
    if (!world_item) return;

    patch_handle_t item = PATCH_NULL;
    patchlib_field_get_value(g_world_inner, world_item, &item);
    if (!item) return;

    int rarity = 0;
    patchlib_field_get_value(g_item_rare, item, &rarity);
    RarityColor rgb{};
    if (!rarity_color(rarity, rgb)) return;

    int slot = 0;
    if (args[1]) slot = *static_cast<int*>(args[1]);
    if (slot < 0 || slot >= static_cast<int>(kTrackedSlots)) return;
    const uint32_t count = ++g_draw_counts[static_cast<size_t>(slot)];
    if (count % kSpawnInterval != 0) return;

    if (!g_seen_rare_item[static_cast<size_t>(slot)]) {
        g_seen_rare_item[static_cast<size_t>(slot)] = true;
        if (mod_logger_write)
            mod_logger_write(MOD_LOG_LEVEL_INFO, kTag,
                             "Rare world item reached draw hook: slot=%d rarity=%d", slot, rarity);
    }

    Vector2Value position{};
    patchlib_field_get_value(g_entity_position, world_item, &position);
    int item_width = 16;
    int item_height = 16;
    patchlib_field_get_value(g_entity_width, world_item, &item_width);
    patchlib_field_get_value(g_entity_height, world_item, &item_height);
    const float center_x = position.x + static_cast<float>(item_width) * 0.5f;
    const float center_y = position.y + static_cast<float>(item_height) * 0.5f;
    int width = 2;
    int height = 2;
    int dust_id = kDustWhiteTorch;
    float speed_x = 0.0f;
    float speed_y = 0.0f;
    int alpha = 0;
    ColorValue tint{255, rgb.b, rgb.g, rgb.r};
    float scale = 1.25f;
    const uint32_t pulse = count / kSpawnInterval;
    for (int i = 0; i < kBeamSegments; ++i) {
        const float segment = static_cast<float>(i) / static_cast<float>(kBeamSegments - 1);
        const float wobble = std::sin(static_cast<float>(pulse) * 0.2f + segment * 6.2831853f) * 2.5f;
        Vector2Value particle_position{
            center_x + wobble,
            center_y + 12.0f - segment * kBeamHeight,
        };
        void* call_args[] = {
            &particle_position, &width, &height, &dust_id,
            &speed_x, &speed_y, &alpha, &tint, &scale,
        };
        int dust_index = -1;
        if (!patchlib_method_invoke_args(g_draw_dust, PATCH_NULL, &dust_index, call_args) &&
            !g_dust_call_failed_logged) {
            g_dust_call_failed_logged = true;
            if (mod_logger_write)
                mod_logger_write(MOD_LOG_LEVEL_ERROR, kTag, "Dust.NewDust invocation failed");
            break;
        }
    }
}
}

bool install_item_draw_hook() {
    const auto main_type = patchlib_type_get_type("Terraria", "Main");
    const auto world_type = patchlib_type_get_type("Terraria", "WorldItem");
    const auto item_type = patchlib_type_get_type("Terraria", "Item");
    const auto entity_type = patchlib_type_get_type("Terraria", "Entity");
    const auto dust_type = patchlib_type_get_type("Terraria", "Dust");
    if (!main_type || !world_type || !item_type || !entity_type || !dust_type) {
        if (mod_logger_write) mod_logger_write(MOD_LOG_LEVEL_ERROR, kTag, "Terraria type lookup failed");
        return false;
    }

    g_world_inner = patchlib_type_get_field(world_type, "inner");
    g_item_rare = patchlib_type_get_field(item_type, "rare");
    g_entity_position = patchlib_type_get_field(entity_type, "position");
    g_entity_width = patchlib_type_get_field(entity_type, "width");
    g_entity_height = patchlib_type_get_field(entity_type, "height");
    g_draw_dust = patchlib_type_get_method_by_param_count(dust_type, "NewDust", 9);
    const auto draw_item = patchlib_type_get_method_by_param_count(main_type, "DrawItem", 2);
    if (!g_world_inner || !g_item_rare || !g_entity_position || !g_entity_width ||
        !g_entity_height || !g_draw_dust || !draw_item) {
        if (mod_logger_write) mod_logger_write(MOD_LOG_LEVEL_ERROR, kTag, "Required field or method lookup failed");
        return false;
    }

    g_draw_hook = patchlib_install_prepost_hook(draw_item, nullptr, draw_item_postfix);
    if (g_draw_hook == PATCH_HOOK_INVALID_ID) {
        if (mod_logger_write) mod_logger_write(MOD_LOG_LEVEL_ERROR, kTag, "Main.DrawItem hook installation failed");
        return false;
    }
    std::memset(g_draw_counts, 0, sizeof(g_draw_counts));
    std::memset(g_seen_rare_item, 0, sizeof(g_seen_rare_item));
    g_dust_call_failed_logged = false;
    if (mod_logger_write) mod_logger_write(MOD_LOG_LEVEL_INFO, kTag, "Drop rarity light pillar hook installed");
    return true;
}

void remove_item_draw_hook() {
    if (g_draw_hook != PATCH_HOOK_INVALID_ID) {
        patchlib_uninstall_hook(g_draw_hook);
        g_draw_hook = PATCH_HOOK_INVALID_ID;
    }
    std::memset(g_draw_counts, 0, sizeof(g_draw_counts));
    std::memset(g_seen_rare_item, 0, sizeof(g_seen_rare_item));
    g_dust_call_failed_logged = false;
    g_world_inner = g_item_rare = g_entity_position = g_entity_width = g_entity_height = g_draw_dust = PATCH_NULL;
}
