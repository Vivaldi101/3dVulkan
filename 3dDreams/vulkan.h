#if !defined(_VULKAN_H)
#define _VULKAN_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR
#elif
// other plats for vulkan
#endif

#include <vulkan/vulkan.h>

#pragma comment(lib,	"vulkan-1.lib")

enum
{
   VULKAN_FRAME_BUFFER_COUNT = 3
};

#define VK_VALID(v) (v) == VK_SUCCESS

bool vulkan_initialize(hw* hw);

cache_align typedef struct vulkan_swapchain_support
{
   VkSurfaceCapabilitiesKHR surface_capabilities;
   u32 surface_format_count;
   VkSurfaceFormatKHR* surface_formats;
   u32 present_mode_count;
	VkPresentModeKHR* present_modes;
} vulkan_swapchain_support;

cache_align typedef struct vulkan_device
{
   VkPhysicalDevice physical_device;
   VkDevice logical_device;
	vulkan_swapchain_support swapchain_support;
   i32 graphics_queue_index;
   i32 present_queue_index;
   i32 transfer_queue_index;
} vulkan_device;

cache_align typedef struct vulkan_context
{
   vulkan_device device;
   VkInstance instance;
   VkSurfaceKHR surface;
} vulkan_context;

#endif
