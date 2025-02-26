#include "vulkan.h"
#include "common.h"

#include "arena.h"

static i32 vulkan_find_memory_index(vulkan_context* context, u32 memory_type_mask, u32 flags)
{
   VkPhysicalDeviceMemoryProperties memory_properties = {};
   vkGetPhysicalDeviceMemoryProperties(context->device.physical_device, &memory_properties);

   for(u32 i = 0; i < memory_properties.memoryTypeCount; ++i)
      if((memory_type_mask & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & flags) == flags)
         return i;
   return -1;
}

static bool vulkan_image_view_create(vulkan_context* context, vulkan_image* image, vulkan_image_info* image_info);

static vulkan_image vulkan_image_create(arena* perm, vulkan_context* context, vulkan_image_info* image_info, u32 w, u32 h)
{
   vulkan_image result = {};

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

   if(!VK_VALID(vkCreateImage(context->device.logical_device, &image_create_info, 0, &result.handle)))
      return (vulkan_image){0};

   VkMemoryRequirements memory_reqs = {};
   vkGetImageMemoryRequirements(context->device.logical_device, result.handle, &memory_reqs);

   i32 memory_index = vulkan_find_memory_index(context, memory_reqs.memoryTypeBits, image_info->memory_flags);
   if(memory_index == -1)
      return (vulkan_image){0};

   VkMemoryAllocateInfo memory_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
   memory_info.allocationSize = memory_reqs.size;
   memory_info.memoryTypeIndex = memory_index;

   if(!VK_VALID(vkAllocateMemory(context->device.logical_device, &memory_info, context->allocator, &result.memory)))
      return (vulkan_image){0};

   if(!VK_VALID(vkBindImageMemory(context->device.logical_device, result.handle, result.memory, 0)))
      return (vulkan_image){0};

   if(image_info->is_view)
      if(!vulkan_image_view_create(context, &result, image_info))
         return (vulkan_image){0};

   result.width = w;
   result.height = h;

   return result;
}

static bool vulkan_image_view_create(vulkan_context* context, vulkan_image* image, vulkan_image_info* image_info)
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

   image->view = 0;

   if(!VK_VALID(vkCreateImageView(context->device.logical_device, &image_view_info, context->allocator, &image->view)))
      return false;

   return true;
}

// TODO: Fillout
static void vulkan_image_destroy(vulkan_image* image)
{
}
