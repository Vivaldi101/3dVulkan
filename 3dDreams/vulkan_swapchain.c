#include "vulkan.h"
#include "common.h"

static bool vk_swapchain_surface_create(arena* storage, vk_context* context)
{
   VkExtent2D swapchain_extent = {context->framebuffer_width, context->framebuffer_height};
   vk_swapchain* swapchain = &context->swapchain;

   for(u32 i = 0; i < swapchain->info.surface_format_count; ++i)
   {
      VkSurfaceFormatKHR format = swapchain->info.surface_formats[i];

      if(format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
         swapchain->image_format = format;
         break;
      }
   }

   // must be defined image format
   if(swapchain->image_format.format == VK_FORMAT_UNDEFINED)
      return false;

   // always present
   VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

   for(u32 i = 0; i < swapchain->info.present_mode_count; ++i)
   {
      VkPresentModeKHR mode = swapchain->info.present_modes[i];
      // use if present
      if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
      {
         present_mode = mode;
         break;
      }
   }

   // TODO: requery swapchain support if devices etc. change here

   VkSurfaceCapabilitiesKHR surface_caps;
   if(!vk_valid(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->device.physical_device, context->surface, &surface_caps)))
      return false;

   if(surface_caps.currentExtent.width != UINT32_MAX)
      // fixed size
      swapchain_extent = surface_caps.currentExtent;
   else
   {
      // surface allows to choose the size
      VkExtent2D min = surface_caps.minImageExtent;
      VkExtent2D max = surface_caps.maxImageExtent;
      swapchain_extent.width = clamp(swapchain_extent.width, min.width, max.width);
      swapchain_extent.height = clamp(swapchain_extent.height, min.height, max.height);
   }

   swapchain->info.surface_capabilities = surface_caps;

   u32 image_count = swapchain->info.surface_capabilities.minImageCount + 1;

   if(image_count > VULKAN_MAX_FRAME_BUFFER_COUNT)
      return false;

   // atleast double buffering
   if(image_count < 2)
      return false;

   // one image on display
   swapchain->max_frames_in_flight_count = image_count - 1;

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

   swapchain_info.preTransform = swapchain->info.surface_capabilities.currentTransform;
   swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   swapchain_info.clipped = VK_TRUE;
   swapchain_info.presentMode = swapchain->present_mode;
   swapchain_info.oldSwapchain = swapchain->handle;

   if(!vk_valid(vkCreateSwapchainKHR(context->device.logical_device, &swapchain_info, 0, &swapchain->handle)))
      return false;

   context->current_frame_index = 0;
   context->current_image_index = 0;

   if(!vk_valid(vkGetSwapchainImagesKHR(context->device.logical_device, swapchain->handle, &swapchain->image_count, 0)))
      return false;
   if(!swapchain->images)
      swapchain->images = new(storage, VkImage, swapchain->image_count);
   if(!swapchain->views)
      swapchain->views = new(storage, VkImageView, swapchain->image_count);

   if(arena_end(storage, swapchain->images) || 
      arena_end(storage, swapchain->views) ||
      !vk_valid(vkGetSwapchainImagesKHR(
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

      if(!vk_valid(vkCreateImageView(context->device.logical_device, &view_info, 0, swapchain->views + i)))
         return false;
   }

   if(!vk_device_depth_format(context))
      return false;

   return true;
}

static bool vk_swapchain_create(vk_context* context)
{
	if(!vk_swapchain_surface_create(context->storage, context))
      return false;

   context->framebuffer_width = context->swapchain.info.surface_capabilities.currentExtent.width;
   context->framebuffer_height = context->swapchain.info.surface_capabilities.currentExtent.height;

   // depth attachment
   vk_image_info image_info = {};
   image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
   image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
   image_info.format = context->device.depth_format;
   image_info.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
   image_info.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
   image_info.is_view = true;
   context->swapchain.depth_attachment = vk_image_create(context->storage, context, &image_info, context->framebuffer_width, context->framebuffer_height);

   if(!context->swapchain.depth_attachment.handle)
      return false;

	return true;
}

static bool vk_swapchain_recreate(vk_context* context)
{
   if(context->do_recreate_swapchain)
      return false;

   if(context->framebuffer_width == 0 || context->framebuffer_height == 0)
      return false;

   vkDeviceWaitIdle(context->device.logical_device);

   array_clear(context->images_in_flight);

   if(!vk_device_depth_format(context))
      return false;

   if(!vk_swapchain_create(context))
      return false;

   context->framebuffer_size_prev_generation = context->framebuffer_size_generation;

   vk_command_buffer_free(context, context->graphics_command_buffers, context->device.graphics_command_pool, context->swapchain.image_count);
   if(!vk_framebuffer_create(context))
      return false;

   context->do_recreate_swapchain = false;
   if(!vk_command_buffer_allocate_primary(context, context->graphics_command_buffers, 
      context->device.graphics_command_pool, context->swapchain.image_count))
      return false;

   return true;
}

static bool vk_swapchain_next_image_index(arena* storage, vk_context* context, u64 timeout, VkSemaphore image_available_semaphore, VkFence fence)
{
   const VkResult result = vkAcquireNextImageKHR(context->device.logical_device, context->swapchain.handle, timeout, image_available_semaphore, fence, &context->current_image_index);

   if(result == VK_ERROR_OUT_OF_DATE_KHR)
   {
      vk_swapchain_recreate(context);
      return false;
   }
   else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
      return false;

   post(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);

   return true;
}

static void vk_swapchain_destroy(vk_context* context)
{
   vkDestroyImage(context->device.logical_device, context->swapchain.depth_attachment.handle, context->allocator);

   for(u32 i = 0; i < context->swapchain.image_count; ++i)
      vkDestroyImageView(context->device.logical_device, context->swapchain.views[i], context->allocator);

   vkDestroySwapchainKHR(context->device.logical_device, context->swapchain.handle, context->allocator);
}

static bool vk_swapchain_present(vk_context* context, u32 present_image_index, VkSemaphore* queue_complete_semaphore)
{
   VkPresentInfoKHR info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
   info.pWaitSemaphores = queue_complete_semaphore;
   info.waitSemaphoreCount = 1;
   info.pSwapchains = &context->swapchain.handle;
   info.swapchainCount = 1;
   info.pImageIndices = &present_image_index;

   VkResult result = vkQueuePresentKHR(context->device.present_queue, &info);
   if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
      vk_swapchain_recreate(context);
   else if(result != VK_SUCCESS)
      return false;

   context->current_frame_index = (context->current_frame_index + 1) % context->swapchain.max_frames_in_flight_count;
   return true;
}
