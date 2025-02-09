#include "vulkan.h"
#include "common.h"

typedef struct vulkan_physical_device_requirements
{ 
	bool is_graphics, is_present, is_compute, is_transfer;
   bool is_anisotropy, is_discrete_gpu;
} vulkan_physical_device_requirements;

static bool vulkan_device_select_physical(hw_buffer* vulkan_arena, vulkan_context* context)
{
   u32 device_count = 0;
   if(!VK_VALID(vkEnumeratePhysicalDevices(context->instance, &device_count, 0)))
      return false;

   VkPhysicalDevice* devices = allocate(vulkan_arena, device_count * sizeof(VkPhysicalDevice));
   if(!devices)
      return false;

   if(!VK_VALID(vkEnumeratePhysicalDevices(context->instance, &device_count, devices)))
      return false;

   for(u32 i = 0; i < device_count; ++i)
   {
      VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(devices[i], &properties);

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(devices[i], &features);

      VkPhysicalDeviceMemoryProperties memory;
		vkGetPhysicalDeviceMemoryProperties(devices[i], &memory);
   }

   return true;
}

static bool vulkan_device_create(hw_buffer* vulkan_arena, vulkan_context* context)
{
	if(!vulkan_device_select_physical(vulkan_arena, context))
		return false;

	return true;
}

static void vulkan_device_destroy(vulkan_context* context)
{
}
