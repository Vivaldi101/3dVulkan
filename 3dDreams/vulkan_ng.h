#if !defined(_VULKAN_NG_H)
#define _VULKAN_NG_H

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif
// other plats for vulkan
#endif

#include "common.h"

bool vulkan_uninitialize(struct hw* hw);
bool vulkan_initialize(struct hw* hw);

#endif
