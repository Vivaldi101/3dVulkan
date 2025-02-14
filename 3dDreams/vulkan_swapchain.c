#include "vulkan.h"
#include "common.h"
#include "hw_arena.h"

void create(vulkan_context* context, vulkan_swapchain* swapchain, u32 w, u32 h);
void destroy(vulkan_context* context, vulkan_swapchain* swapchain);

static void vulkan_swapchain_create(vulkan_context* context, u32 w, u32 h)
{
	//create(context, );
}

static void vulkan_swapchain_recreate(vulkan_context* context, u32 w, u32 h)
{
	//create(context, );
	//destroy();
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
