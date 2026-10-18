#pragma once
void diagnostics_open(const char* private_dir);
void diagnostics_write(const char* format, ...);
void diagnostics_close();
