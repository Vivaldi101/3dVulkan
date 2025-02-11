#include "vulkan.h"
#include "common.h"

typedef struct vulkan_physical_device_requirements
{ 
	bool is_graphics, is_present, is_compute, is_transfer;	// queue type predicates
   bool is_anisotropy, is_discrete_gpu;
   const char** device_extension_names;
} vulkan_physical_device_requirements;

typedef struct vulkan_queue_family
{
	u32 graphics_index, present_index, computer_index, transfer_index;
} vulkan_queue_family;

static bool vulkan_device_select_physical(hw_arena* arena, const vulkan_context* context)
{
   u32 device_count = 0;
   if(!VK_VALID(vkEnumeratePhysicalDevices(context->instance, &device_count, 0)))
      return false;

   hw_arena devices_arena = arena_push_size(arena, device_count * sizeof(VkPhysicalDevice), VkPhysicalDevice);
   VkPhysicalDevice* devices = arena_get_data(&devices_arena, VkPhysicalDevice);

   if(arena_is_set(&devices_arena, device_count))
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

      //vulkan_queue_family queue_family = {0};
   }

   return true;
}

static bool vulkan_device_swapchain_support(hw_arena* arena, vulkan_context* context, vulkan_swapchain_support* swapchain)
{
	if(!VK_VALID(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->device.physical_device, context->surface, &swapchain->surface_capabilities)))
		return false;

	if(!VK_VALID(vkGetPhysicalDeviceSurfaceFormatsKHR(context->device.physical_device, context->surface, &swapchain->surface_format_count, 0)))
		return false;

	if(swapchain->surface_format_count > 0 && !swapchain->surface_formats)
      //swapchain->surface_formats = vulkan_allocate(arena, swapchain->surface_format_count * sizeof(VkSurfaceFormatKHR));

	if(!VK_VALID(vkGetPhysicalDeviceSurfaceFormatsKHR(context->device.physical_device, context->surface, &swapchain->surface_format_count, swapchain->surface_formats)))
		return false;

	if(!VK_VALID(vkGetPhysicalDeviceSurfacePresentModesKHR(context->device.physical_device, context->surface, &swapchain->present_mode_count, 0)))
		return false;

	if(swapchain->present_mode_count > 0 && !swapchain->present_modes)
      //swapchain->present_modes = vulkan_allocate(arena, swapchain->present_mode_count * sizeof(VkPresentModeKHR));

	if(!VK_VALID(vkGetPhysicalDeviceSurfacePresentModesKHR(context->device.physical_device, context->surface, &swapchain->present_mode_count, swapchain->present_modes)))
		return false;
   
   return true;
}

static bool vulkan_device_meets_requirements(hw_arena* arena,
															vulkan_context* context,
                                             const vulkan_physical_device_requirements* requirements,
                                             const VkPhysicalDeviceProperties* properties,
															vulkan_queue_family* queue_family)
{
	queue_family->computer_index = (u32)-1;
   queue_family->graphics_index = (u32)-1;
   queue_family->present_index = (u32)-1;
   queue_family->transfer_index = (u32)-1;

   if(!implies(requirements->is_discrete_gpu,
      properties->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
      return false;
	
   u32 queue_family_count = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(context->device.physical_device, &queue_family_count, 0);
   hw_arena queue_families_arena = arena_push_size(arena, queue_family_count * sizeof(VkQueueFamilyProperties), VkQueueFamilyProperties);
   VkQueueFamilyProperties* queue_families = arena_get_data(&queue_families_arena, VkQueueFamilyProperties);

   if(arena_is_set(&queue_families_arena, queue_family_count))
      vkGetPhysicalDeviceQueueFamilyProperties(context->device.physical_device, &queue_family_count, queue_families);

   u32 min_transfer_score = 255;
   for(u32 i = 0; i < queue_family_count; ++i)
   {
		u32 transfer_score = 0;

      if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
         queue_family->graphics_index = i;
			++transfer_score;
      }

      if(queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
      {
         queue_family->computer_index = i;
			++transfer_score;
      }

      if(queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
      {
         if(transfer_score <= min_transfer_score)
         {
				min_transfer_score = transfer_score;
            queue_family->transfer_index = i;
         }
      }

      VkBool32 supports_present = false;
      if(!VK_VALID(vkGetPhysicalDeviceSurfaceSupportKHR(context->device.physical_device, i, context->surface, &supports_present)))
         return false;
      if(supports_present)
			queue_family->present_index = i;
   }

   vulkan_device_swapchain_support(arena, context, &context->device.swapchain_support);

   return true;
}

static bool vulkan_device_create(hw_arena* arena, vulkan_context* context)
{
	if(!vulkan_device_select_physical(arena, context))
		return false;

	return true;
}

static void vulkan_device_destroy(vulkan_context* context)
{
}
