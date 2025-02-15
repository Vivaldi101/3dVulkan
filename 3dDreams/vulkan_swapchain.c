#include "vulkan.h"
#include "common.h"
#include "hw_arena.h"

static void swapchain_create(vulkan_swapchain* swapchain, u32 w, u32 h)
{
}

static void swapchain_destroy(vulkan_swapchain* swapchain)
{
}

static bool vulkan_swapchain_create(hw_arena* arena, vulkan_context* context)
{
	swapchain_create(&context->swapchain, context->framebuffer_width, context->framebuffer_height);

	return true;
}

static void vulkan_swapchain_recreate(hw_arena* arena, vulkan_context* context)
{
	swapchain_create(&context->swapchain, context->framebuffer_width, context->framebuffer_height);
	swapchain_destroy(&context->swapchain);
}

static bool vulkan_swapchain_next_image_index(vulkan_context* context, u32 *image_index, u64 timeout, VkSemaphore image_available_semaphore, VkFence fence)
{
   return true;
}

static void vulkan_swapchain_destroy(vulkan_context* context)
{
}

static void vulkan_swapchain_present(vulkan_context* context, u32 present_image_index, VkSemaphore render_complete_semaphore)
{
}
