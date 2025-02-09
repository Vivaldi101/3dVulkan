#include "vulkan.h"
#include "common.h"

typedef struct vulkan_physical_device_requirements
{ 
	bool is_graphics, is_present, is_compute, is_transfer;	// queue type predicates
   bool is_anisotropy, is_discrete_gpu;
   const char** device_extension_names;
} vulkan_physical_device_requirements;

typedef struct vulkan_queue_family_index
{
	u32 graphics, present, computer, transfer;
} vulkan_queue_family_index;

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

	// get a suitable device 
   for(u32 i = 0; i < device_count; ++i)
   {
      VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(devices[i], &properties);

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(devices[i], &features);

      VkPhysicalDeviceMemoryProperties memory;
		vkGetPhysicalDeviceMemoryProperties(devices[i], &memory);

      vulkan_physical_device_requirements reqs = {0};
      reqs.is_graphics = reqs.is_present = reqs.is_transfer = true;
      reqs.is_anisotropy = reqs.is_discrete_gpu = true;

      vulkan_queue_family_index queue_index = {0};
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
