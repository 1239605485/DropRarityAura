#include "mod_core.h"
#include "mod_logger.h"
#include "item_draw_hook.hpp"

namespace {
void init_mod(kernel_mod_handle_t*) {
    if (!install_item_draw_hook() && mod_logger_write)
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "DropRarityAura", "Initialization failed");
}

void cleanup_mod(kernel_mod_handle_t*) {
    remove_item_draw_hook();
}

kernel_mod_info_t* get_info() {
    static kernel_mod_info_t info = {"xinliu.droprarityaura", 3, 1, "0.3.0"};
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
