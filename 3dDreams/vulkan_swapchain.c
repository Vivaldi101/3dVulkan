#include "vulkan.h"
#include "common.h"
#include "hw_arena.h"

static bool swapchain_create(vulkan_swapchain* swapchain, u32 w, u32 h)
{
   VkExtent2D swapchain_extent = {w, h};
   swapchain->max_frames_count = 2; // triple buffering

   bool found = false;
   for(u32 i = 0; i < swapchain->support.surface_format_count; ++i)
   {
      VkSurfaceFormatKHR format = swapchain->support.surface_formats[i];

      if(format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
         swapchain->image_format = format;
         found = true;
         break;
      }
   }

   return found;
}

static void swapchain_destroy(vulkan_swapchain* swapchain)
{
}

static bool vulkan_swapchain_create(vulkan_context* context)
{
	bool result = swapchain_create(&context->swapchain, context->framebuffer_width, context->framebuffer_height);

	return result;
}

static void vulkan_swapchain_recreate(vulkan_context* context)
{
	swapchain_create(&context->swapchain, context->framebuffer_width, context->framebuffer_height);
	swapchain_destroy(&context->swapchain);
}

static bool vulkan_swapchain_next_image_index(vulkan_context* context, u32 *image_index, u64 timeout, VkSemaphore image_available_semaphore, VkFence fence)
{
   const VkResult result = vkAcquireNextImageKHR(context->device.logical_device, context->swapchain.handle, timeout, image_available_semaphore, fence, image_index);

   if(result == VK_ERROR_OUT_OF_DATE_KHR)
   {
      vulkan_swapchain_recreate(context);
      return false;
   }
   else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
      return false;

   post(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);

   return true;
}

static void vulkan_swapchain_destroy(vulkan_context* context)
{
}

static void vulkan_swapchain_present(vulkan_context* context, u32 present_image_index, VkSemaphore render_complete_semaphore)
{
   VkPresentInfoKHR info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
   info.pWaitSemaphores = &render_complete_semaphore;
   info.waitSemaphoreCount = 1;
   info.pSwapchains = &context->swapchain.handle;
   info.swapchainCount = 1;

   VkResult result = vkQueuePresentKHR(context->device.present_queue, &info);
   // TODO: semcomp these
   if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
   {
      vulkan_swapchain_recreate(context);
   }
   else if(result != VK_SUCCESS)
      ;  // TODO: error messages for vulkan

   post(result == VK_SUCCESS);
}
