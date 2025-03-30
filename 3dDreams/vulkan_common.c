#include "common.h"
#include "vulkan.h"

static i32 vk_find_memory_index(vk_context* context, u32 memory_type_mask, u32 flags)
{
   VkPhysicalDeviceMemoryProperties memory_properties = {};
   vkGetPhysicalDeviceMemoryProperties(context->device.physical_device, &memory_properties);

   for(u32 i = 0; i < memory_properties.memoryTypeCount; ++i)
      if((memory_type_mask & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & flags) == flags)
         return i;
   return -1;
}
