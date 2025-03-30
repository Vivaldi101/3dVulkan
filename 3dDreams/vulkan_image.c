#include "vulkan.h"
#include "common.h"

#include "arena.h"

static bool vk_image_view_create(vk_context* context, vk_image* image, vk_image_info* image_info);

static vk_image vk_image_create(arena* storage, vk_context* context, vk_image_info* image_info, u32 w, u32 h)
{
   vk_image result = {};

   VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
   image_create_info.imageType = VK_IMAGE_TYPE_2D;
   image_create_info.extent.width = w;
   image_create_info.extent.height = h;
   image_create_info.extent.depth = 1;
   image_create_info.mipLevels = 4;
   image_create_info.arrayLayers = 1;
   image_create_info.format = image_info->format;
   image_create_info.tiling = image_info->tiling;
   image_create_info.usage = image_info->usage;
   image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
   image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

   if(!vk_valid(vkCreateImage(context->device.logical_device, &image_create_info, 0, &result.handle)))
      return (vk_image){0};

   VkMemoryRequirements memory_reqs = {};
   vkGetImageMemoryRequirements(context->device.logical_device, result.handle, &memory_reqs);

   i32 memory_index = vk_find_memory_index(context, memory_reqs.memoryTypeBits, image_info->memory_flags);
   if(memory_index == -1)
      return (vk_image){0};

   VkMemoryAllocateInfo memory_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
   memory_info.allocationSize = memory_reqs.size;
   memory_info.memoryTypeIndex = memory_index;

   if(!vk_valid(vkAllocateMemory(context->device.logical_device, &memory_info, context->allocator, &result.memory)))
      return (vk_image){0};

   if(!vk_valid(vkBindImageMemory(context->device.logical_device, result.handle, result.memory, 0)))
      return (vk_image){0};

   if(image_info->is_view)
      if(!vk_image_view_create(context, &result, image_info))
         return (vk_image){0};

   result.width = w;
   result.height = h;

   return result;
}

static bool vk_image_view_create(vk_context* context, vk_image* image, vk_image_info* image_info)
{
   VkImageViewCreateInfo image_view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
   image_view_info.image = image->handle;
   image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
   image_view_info.format = image_info->format;
   image_view_info.subresourceRange.aspectMask = image_info->aspect_flags;

   image_view_info.subresourceRange.baseMipLevel = 0;
   image_view_info.subresourceRange.levelCount = 1;
   image_view_info.subresourceRange.baseArrayLayer = 0;
   image_view_info.subresourceRange.layerCount = 1;

   if(!vk_valid(vkCreateImageView(context->device.logical_device, &image_view_info, context->allocator, &image->view)))
      return false;

   return true;
}
