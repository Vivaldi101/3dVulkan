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
	u32 graphics_index, present_index, compute_index, transfer_index;
} vulkan_queue_family;

static bool vulkan_device_meets_requirements(arena* perm,
															vulkan_context* context,
                                             const vulkan_physical_device_requirements* requirements,
                                             const VkPhysicalDeviceProperties* properties,
															vulkan_queue_family* queue_family);

static bool vulkan_device_select_physical(arena* perm, vulkan_context* context)
{
   u32 device_count = 0;
   if(!VK_VALID(vkEnumeratePhysicalDevices(context->instance, &device_count, 0)))
      return false;

   VkPhysicalDevice* devices = new(perm, VkPhysicalDevice, device_count);
   if(arena_end(perm, devices) || !VK_VALID(vkEnumeratePhysicalDevices(context->instance, &device_count, devices)))
      return false;

	// get a suitable physical device 
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

      context->device.physical_device = devices[i];

      vulkan_queue_family queue_family = {0};

      if(!vulkan_device_meets_requirements(perm, context, &reqs, &properties, &queue_family))
			continue;	// no device yet met the requirements

      context->device.queue_indexes[QUEUE_GRAPHICS_INDEX] = queue_family.graphics_index;
      context->device.queue_indexes[QUEUE_COMPUTE_INDEX] = queue_family.compute_index;
      context->device.queue_indexes[QUEUE_PRESENT_INDEX] = queue_family.present_index;
      context->device.queue_indexes[QUEUE_TRANSFER_INDEX] = queue_family.transfer_index;

      context->device.properties = properties;
      context->device.features = features;
      context->device.memory = memory;

      return true;
   }

   return false;
}

static bool vulkan_device_swapchain_support(arena* perm, vulkan_context* context, vulkan_swapchain_support* swapchain)
{
	if(!VK_VALID(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->device.physical_device, context->surface, &swapchain->surface_capabilities)))
		return false;

	if(!VK_VALID(vkGetPhysicalDeviceSurfaceFormatsKHR(context->device.physical_device, context->surface, &swapchain->surface_format_count, 0)))
		return false;

   if(swapchain->surface_format_count > 0 && !swapchain->surface_formats)
      swapchain->surface_formats = new(perm, VkSurfaceFormatKHR, swapchain->surface_format_count);

	if(arena_end(perm, swapchain->surface_formats) || 
      !VK_VALID(vkGetPhysicalDeviceSurfaceFormatsKHR(context->device.physical_device, 
      context->surface, &swapchain->surface_format_count, swapchain->surface_formats)))
		return false;

	if(!VK_VALID(vkGetPhysicalDeviceSurfacePresentModesKHR(context->device.physical_device, context->surface, &swapchain->present_mode_count, 0)))
		return false;

	if(swapchain->present_mode_count > 0 && !swapchain->present_modes)
      swapchain->present_modes = new(perm, VkPresentModeKHR, swapchain->present_mode_count);

	if(arena_end(perm, swapchain->present_modes) || 
      !VK_VALID(vkGetPhysicalDeviceSurfacePresentModesKHR(context->device.physical_device, 
      context->surface, &swapchain->present_mode_count, swapchain->present_modes)))
		return false;
   
   return true;
}

static bool vulkan_device_meets_requirements(arena* perm,
															vulkan_context* context,
                                             const vulkan_physical_device_requirements* requirements,
                                             const VkPhysicalDeviceProperties* properties,
															vulkan_queue_family* queue_family)
{
   queue_family->compute_index = (u32)-1;
   queue_family->graphics_index = (u32)-1;
   queue_family->present_index = (u32)-1;
   queue_family->transfer_index = (u32)-1;

   if(!implies(requirements->is_discrete_gpu,
      properties->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
      return false;

   u32 queue_family_count = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(context->device.physical_device, &queue_family_count, 0);
   VkQueueFamilyProperties* queue_families = new(perm, VkQueueFamilyProperties, queue_family_count);

   if(arena_end(perm, queue_families))
      return false;

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
         queue_family->compute_index = i;
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

   // TODO: we need to retain surface formats and present modes
   vulkan_device_swapchain_support(perm, context, &context->swapchain.support);

   return true;
}

static bool vulkan_device_create(arena scratch, arena* perm, vulkan_context* context)
{
	if(!vulkan_device_select_physical(perm, context))
		return false;

   VkDeviceQueueCreateInfo* device_queue_infos = new(&scratch, VkDeviceQueueCreateInfo, QUEUE_INDEX_COUNT);
   VkDeviceQueueCreateInfo* queue = device_queue_infos;
   const VkDeviceQueueCreateInfo* end = arena_end_count(queue, QUEUE_INDEX_COUNT);

   while(queue != end)
   {
      queue->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue->queueFamilyIndex = context->device.queue_indexes[queue - device_queue_infos];
      queue->queueCount = 1;
      queue->flags = 0;
      queue->pNext = 0;
      f32 priorities[1] = {1.0f};
      queue->pQueuePriorities = priorities;

      queue++;
   }

   VkPhysicalDeviceFeatures physical_device_features = {};
   physical_device_features.samplerAnisotropy = VK_TRUE;

   const char* device_extension_name = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
   VkDeviceCreateInfo device_create_info =
   {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pQueueCreateInfos = device_queue_infos,
    .queueCreateInfoCount = QUEUE_INDEX_COUNT,
    .enabledExtensionCount = 1,
    .ppEnabledExtensionNames = &device_extension_name,
    .pEnabledFeatures = &physical_device_features,
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = 0,
   };

   if(!VK_VALID(vkCreateDevice(context->device.physical_device, &device_create_info, 0, &context->device.logical_device)))
      return false;

   vkGetDeviceQueue(context->device.logical_device, context->device.queue_indexes[QUEUE_GRAPHICS_INDEX], 0, &context->device.graphics_queue);
   vkGetDeviceQueue(context->device.logical_device, context->device.queue_indexes[QUEUE_COMPUTE_INDEX], 0, &context->device.compute_queue);
   vkGetDeviceQueue(context->device.logical_device, context->device.queue_indexes[QUEUE_PRESENT_INDEX], 0, &context->device.present_queue);
   vkGetDeviceQueue(context->device.logical_device, context->device.queue_indexes[QUEUE_TRANSFER_INDEX], 0, &context->device.transfer_queue);

	return true;
}

static bool vulkan_device_depth_format(vulkan_context* context)
{
   const VkFormat depth_formats[] =
   {
      VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D24_UNORM_S8_UINT
   };
   u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

   for(size i = 0; i < array_count(depth_formats); ++i)
   {
      VkFormatProperties props = {};
      vkGetPhysicalDeviceFormatProperties(context->device.physical_device, depth_formats[i], &props);

      if((props.linearTilingFeatures & flags) == flags)
      {
         context->device.depth_format = depth_formats[i];
         return true;
      }
      if((props.optimalTilingFeatures & flags) == flags)
      {
         context->device.depth_format = depth_formats[i];
         return true;
      }
   }

   return false;
}

static void vulkan_device_destroy(vulkan_context* context)
{
	// TODO: platform memset
	memset(context, 0, sizeof(*context));

	memset(context->device.queue_indexes, -1, sizeof(context->device.queue_indexes));
}
