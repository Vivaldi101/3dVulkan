#if !defined(_VULKAN_H)
#define _VULKAN_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR
#elif
// other plats for vulkan
#endif

#include <vulkan/vulkan.h>
#include "arena.h"

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

enum
{
   QUEUE_GRAPHICS_INDEX = 0,
   QUEUE_COMPUTE_INDEX,
   QUEUE_PRESENT_INDEX,
   QUEUE_TRANSFER_INDEX,

   QUEUE_INDEX_COUNT,
};

cache_align typedef struct vulkan_swapchain
{
	VkSurfaceFormatKHR image_format;
   VkPresentModeKHR present_mode;
   VkSwapchainKHR handle;

   u32 max_frames_count;
   u32 image_count;

   VkImage* images;
   VkImageView* views;

   vulkan_swapchain_support support;
} vulkan_swapchain;

cache_align typedef struct vulkan_device
{
   VkPhysicalDevice physical_device;
   VkDevice logical_device;

	u32 queue_indexes[QUEUE_INDEX_COUNT];
   u32 queue_family_count;
   VkQueue graphics_queue;
   VkQueue compute_queue;
   VkQueue present_queue;
   VkQueue transfer_queue;

   VkPhysicalDeviceProperties properties;
   VkPhysicalDeviceFeatures features;
   VkPhysicalDeviceMemoryProperties memory;

   VkFormat depth_format;

   b32 use_single_family_queue;
} vulkan_device;

cache_align typedef struct vulkan_context
{
   vulkan_device device;
   vulkan_swapchain swapchain;

   VkInstance instance;
   VkSurfaceKHR surface;

   u32 framebuffer_width;
   u32 framebuffer_height;

   u32 image_index;
   u32 current_frame;

   bool do_recreate_swapchain;
} vulkan_context;

#endif
