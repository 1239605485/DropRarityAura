// KernelLoader injects function pointers into exported variable slots.
// Define each slot once; an undefined data symbol cannot be injected after memdl_open.
#define TEF_API_IMPL 1
#include "tefkernel/patchlib/type.h"
#include "tefkernel/patchlib/field.h"
#include "tefkernel/patchlib/method.h"

#include "tefkernel/terraria/texture2d.h"
#if !defined(__ANDROID__)
#include "tefkernel/patchlib/struct/array.h"
#endif
