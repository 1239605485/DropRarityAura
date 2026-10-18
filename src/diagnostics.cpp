#include "diagnostics.hpp"
#include "version.hpp"
#include "mod_logger.h"
#include <cstdio>
#include <cstdarg>

namespace { FILE* g_log = nullptr; }
void diagnostics_open(const char* private_dir) {
    diagnostics_close();
    if (!private_dir) return;
    // TEFManager exports this directory, unlike the Android-only logcat sink.
    char path[2048];
    std::snprintf(path, sizeof(path), "%s/../../../../logs/tefkernel/droprarityaura_runtime.log", private_dir);
    g_log = std::fopen(path, "w");
    if (!g_log) {
        std::snprintf(path, sizeof(path), "%s/droprarityaura_runtime.log", private_dir);
        g_log = std::fopen(path, "w");
    }
    diagnostics_write("DropRarityAura %s initialized; private_dir=%s", kModVersion, private_dir);
}
void diagnostics_write(const char* format, ...) {
    char message[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    if (g_log) { std::fprintf(g_log, "%s\n", message); std::fflush(g_log); }
    if (mod_logger_write) mod_logger_write(MOD_LOG_LEVEL_INFO, "DropRarityAura", "%s", message);
}
void diagnostics_close() {
    if (g_log) { std::fclose(g_log); g_log = nullptr; }
}
