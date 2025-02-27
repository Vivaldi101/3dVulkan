#include "vulkan.h"
#include "common.h"

static bool vulkan_swapchain_surface_create(arena* perm, vulkan_context* context)
{
   VkExtent2D swapchain_extent = {context->framebuffer_width, context->framebuffer_height};
   vulkan_swapchain* swapchain = &context->swapchain;
   swapchain->max_frames_count = 2; // triple buffering

   for(u32 i = 0; i < swapchain->support.surface_format_count; ++i)
   {
      VkSurfaceFormatKHR format = swapchain->support.surface_formats[i];

      if(format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
         swapchain->image_format = format;
         break;
      }
   }

   // must be defiend image format
   if(swapchain->image_format.format == VK_FORMAT_UNDEFINED)
      return false;

   // always present
   VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

   for(u32 i = 0; i < swapchain->support.present_mode_count; ++i)
   {
      VkPresentModeKHR mode = swapchain->support.present_modes[i];
      // use if present
      if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
      {
         present_mode = mode;
         break;
      }
   }

   // TODO: requery swapchain support

   if(swapchain->support.surface_capabilities.currentExtent.width != UINT32_MAX)
      swapchain_extent = swapchain->support.surface_capabilities.currentExtent;

   VkExtent2D min = swapchain->support.surface_capabilities.minImageExtent;
   VkExtent2D max = swapchain->support.surface_capabilities.maxImageExtent;

   swapchain_extent.width = clamp(swapchain_extent.width, min.width, max.width);
   swapchain_extent.height = clamp(swapchain_extent.height, min.height, max.height);

   u32 image_count = swapchain->support.surface_capabilities.minImageCount + 1;
   // TODO: safe guard against bad image counts

   VkSwapchainCreateInfoKHR swapchain_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
   swapchain_info.surface = context->surface;
   swapchain_info.minImageCount = image_count;
   swapchain_info.imageFormat = swapchain->image_format.format;
   swapchain_info.imageColorSpace = swapchain->image_format.colorSpace;
   swapchain_info.imageExtent = swapchain_extent;
   swapchain_info.imageArrayLayers = 1;
   swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

   swapchain->present_mode = present_mode;

   if(context->device.queue_indexes[QUEUE_GRAPHICS_INDEX] != context->device.queue_indexes[QUEUE_PRESENT_INDEX])
   {
      const u32 queue_family_indexes[] = 
      {
         context->device.queue_indexes[QUEUE_GRAPHICS_INDEX], 
         context->device.queue_indexes[QUEUE_PRESENT_INDEX]
      };
      swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      swapchain_info.queueFamilyIndexCount = array_count(queue_family_indexes);
      swapchain_info.pQueueFamilyIndices = queue_family_indexes;
   }
   else
   {
      swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      swapchain_info.queueFamilyIndexCount = 0;
      swapchain_info.pQueueFamilyIndices = 0;
   }

   inv(implies(swapchain_info.imageSharingMode == VK_SHARING_MODE_CONCURRENT, 
      swapchain_info.queueFamilyIndexCount > 1 && swapchain_info.pQueueFamilyIndices));

   swapchain_info.preTransform = swapchain->support.surface_capabilities.currentTransform;
   swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   swapchain_info.clipped = VK_TRUE;
   swapchain_info.presentMode = swapchain->present_mode;
   swapchain_info.oldSwapchain = 0;

   if(!VK_VALID(vkCreateSwapchainKHR(context->device.logical_device, &swapchain_info, 0, &swapchain->handle)))
      return false;

   context->current_frame = 0;
   context->image_index = 0;

   if(!VK_VALID(vkGetSwapchainImagesKHR(context->device.logical_device, swapchain->handle, &swapchain->image_count, 0)))
      return false;
   if(!swapchain->images)
      swapchain->images = new(perm, VkImage, swapchain->image_count);
   if(!swapchain->views)
      swapchain->views = new(perm, VkImageView, swapchain->image_count);

   if(arena_end(perm, swapchain->images) || 
      arena_end(perm, swapchain->views) ||
      !VK_VALID(vkGetSwapchainImagesKHR(
      context->device.logical_device, swapchain->handle, 
      &swapchain->image_count, swapchain->images)))
      return false;

   for(size i = 0; i < swapchain->image_count; ++i)
   {
      VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
      view_info.image = swapchain->images[i];
      view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_info.format = swapchain->image_format.format;
      view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      view_info.subresourceRange.baseMipLevel = 0;
      view_info.subresourceRange.levelCount = 1;
      view_info.subresourceRange.baseArrayLayer = 0;
      view_info.subresourceRange.layerCount = 1;

      if(!VK_VALID(vkCreateImageView(context->device.logical_device, &view_info, 0, swapchain->views + i)))
         return false;
   }

   if(!vulkan_device_depth_format(context))
      return false;

   return true;
}

static void swapchain_destroy(vulkan_swapchain* swapchain)
{
}

static bool vulkan_swapchain_create(vulkan_context* context)
{
	bool result = vulkan_swapchain_surface_create(context->perm, context);
   context->framebuffer_width = context->swapchain.support.surface_capabilities.currentExtent.width;
   context->framebuffer_height = context->swapchain.support.surface_capabilities.currentExtent.height;

   // depth attachment
   vulkan_image_info image_info = {};
   image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
   image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
   image_info.format = context->device.depth_format;
   image_info.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
   image_info.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
   context->swapchain.depth_attachment = vulkan_image_create(context->perm, context, &image_info, context->framebuffer_width, context->framebuffer_height);

   if(!context->swapchain.depth_attachment.handle)
      return false;

	return result;
}

static void vulkan_swapchain_recreate(arena* perm, vulkan_context* context)
{
	vulkan_swapchain_surface_create(perm, context);
	swapchain_destroy(&context->swapchain);
}

static bool vulkan_swapchain_next_image_index(arena* perm, vulkan_context* context, u32 *image_index, u64 timeout, VkSemaphore image_available_semaphore, VkFence fence)
{
   const VkResult result = vkAcquireNextImageKHR(context->device.logical_device, context->swapchain.handle, timeout, image_available_semaphore, fence, image_index);

   if(result == VK_ERROR_OUT_OF_DATE_KHR)
   {
      vulkan_swapchain_recreate(perm, context);
      return false;
   }
   else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
      return false;

   post(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);

   return true;
}

static void vulkan_swapchain_destroy(vulkan_context* context)
{
   vkDestroyImage(context->device.logical_device, context->swapchain.depth_attachment.handle, context->allocator);

   for(u32 i = 0; i < context->swapchain.image_count; ++i)
      vkDestroyImageView(context->device.logical_device, context->swapchain.views[i], context->allocator);

   vkDestroySwapchainKHR(context->device.logical_device, context->swapchain.handle, context->allocator);
}

static void vulkan_swapchain_present(arena* perm, vulkan_context* context, u32 present_image_index, VkSemaphore render_complete_semaphore)
{
   VkPresentInfoKHR info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
   info.pWaitSemaphores = &render_complete_semaphore;
   info.waitSemaphoreCount = 1;
   info.pSwapchains = &context->swapchain.handle;
   info.swapchainCount = 1;

   VkResult result = vkQueuePresentKHR(context->device.present_queue, &info);
   // TODO: semcomp these
   if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
      vulkan_swapchain_recreate(perm, context);
   else if(result != VK_SUCCESS)
      ;  // TODO: error messages for vulkan

   post(result == VK_SUCCESS);
}
