#include "mod_core.h"
#include "mod_logger.h"
#include "item_draw_hook.hpp"
#include "diagnostics.hpp"
#include "beam_renderer.hpp"

namespace {
void init_mod(kernel_mod_handle_t* handle) {
    diagnostics_open(handle ? handle->private_dir : nullptr);
    configure_beam_resources(handle ? handle->private_dir : nullptr);
    if (mod_logger_write)
        mod_logger_write(MOD_LOG_LEVEL_INFO, "DropRarityAura", "Initializing rarity light pillar mod v0.10.1");
    if (!install_item_draw_hook() && mod_logger_write)
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "DropRarityAura", "Initialization failed");
}

void cleanup_mod(kernel_mod_handle_t*) {
    remove_item_draw_hook();
    diagnostics_close();
}

kernel_mod_info_t* get_info() {
    static kernel_mod_info_t info = {"xinliu.droprarityaura", 11, 1, "0.10.1"};
    return &info;
}
}

#if defined(_WIN32)
#define MOD_EXPORT extern "C" __declspec(dllexport)
#else
#define MOD_EXPORT extern "C" __attribute__((visibility("default")))
#endif

MOD_EXPORT kernel_mod_ops_t* create_kernel_mod() {
    static kernel_mod_ops_t ops = {init_mod, cleanup_mod, get_info};
    return &ops;
}
