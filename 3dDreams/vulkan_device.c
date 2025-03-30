#include "vulkan.h"
#include "common.h"

enum { INVALID_QUEUE_INDEX = -1 };

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

static bool vulkan_device_meets_requirements(arena* storage,
															vulkan_context* context,
                                             const vulkan_physical_device_requirements* requirements,
                                             const VkPhysicalDeviceProperties* properties,
															vulkan_queue_family* queue_family);

static bool vulkan_device_select_physical(arena* storage, vulkan_context* context)
{
   u32 device_count = 0;
   if(!vk_valid(vkEnumeratePhysicalDevices(context->instance, &device_count, 0)))
      return false;

   VkPhysicalDevice* devices = new(storage, VkPhysicalDevice, device_count);
   if(arena_end(storage, devices) || !vk_valid(vkEnumeratePhysicalDevices(context->instance, &device_count, devices)))
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

      if(!vulkan_device_meets_requirements(storage, context, &reqs, &properties, &queue_family))
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

static bool vulkan_device_swapchain_support(arena* storage, vulkan_context* context, vulkan_swapchain_info* swapchain)
{
	if(!vk_valid(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->device.physical_device, context->surface, &swapchain->surface_capabilities)))
		return false;

	if(!vk_valid(vkGetPhysicalDeviceSurfaceFormatsKHR(context->device.physical_device, context->surface, &swapchain->surface_format_count, 0)))
		return false;

   if(swapchain->surface_format_count > 0 && !swapchain->surface_formats)
      swapchain->surface_formats = new(storage, VkSurfaceFormatKHR, swapchain->surface_format_count);

	if(arena_end(storage, swapchain->surface_formats) || 
      !vk_valid(vkGetPhysicalDeviceSurfaceFormatsKHR(context->device.physical_device, 
      context->surface, &swapchain->surface_format_count, swapchain->surface_formats)))
		return false;

	if(!vk_valid(vkGetPhysicalDeviceSurfacePresentModesKHR(context->device.physical_device, context->surface, &swapchain->present_mode_count, 0)))
		return false;

	if(swapchain->present_mode_count > 0 && !swapchain->present_modes)
      swapchain->present_modes = new(storage, VkPresentModeKHR, swapchain->present_mode_count);

	if(arena_end(storage, swapchain->present_modes) || 
      !vk_valid(vkGetPhysicalDeviceSurfacePresentModesKHR(context->device.physical_device, 
      context->surface, &swapchain->present_mode_count, swapchain->present_modes)))
		return false;
   
   return true;
}

static u32 vulkan_find_unique_family_count(u32 g, u32 c, u32 p, u32 t)
{
   u32 result = 1;

   g %= (u32)INVALID_QUEUE_INDEX;
   c %= (u32)INVALID_QUEUE_INDEX;
   p %= (u32)INVALID_QUEUE_INDEX;
   t %= (u32)INVALID_QUEUE_INDEX;

   if(g != c && g != p && g != t) result++;
   if(c != p && c != t) result++;
   if(p != t) result++;

   return result;
}

static bool vulkan_device_meets_requirements(arena* storage,
															vulkan_context* context,
                                             const vulkan_physical_device_requirements* requirements,
                                             const VkPhysicalDeviceProperties* properties,
															vulkan_queue_family* queue_family)
{
   queue_family->compute_index = (u32)INVALID_QUEUE_INDEX;
   queue_family->graphics_index = (u32)INVALID_QUEUE_INDEX;
   queue_family->present_index = (u32)INVALID_QUEUE_INDEX;
   queue_family->transfer_index = (u32)INVALID_QUEUE_INDEX;

   if(!implies(requirements->is_discrete_gpu,
      properties->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
      return false;

   u32 queue_family_count = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(context->device.physical_device, &queue_family_count, 0);
   VkQueueFamilyProperties* queue_families = new(storage, VkQueueFamilyProperties, queue_family_count);

   if(arena_end(storage, queue_families))
      return false;

   vkGetPhysicalDeviceQueueFamilyProperties(context->device.physical_device, &queue_family_count, queue_families);

   if(!context->device.use_single_family_queue)
   {
      // iterate all available families and set unique family indexes in the set priority order:
      // graphics, present, compute, transfer
      u32 min_transfer_score = queue_family_count;
      for(u32 i = 0; i < queue_family_count; ++i)
      {
         u32 transfer_score = 0;

         if(queue_families[i].queueCount == 0)
            continue;

         if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
         {
            queue_family->graphics_index = i;
            ++transfer_score;
            continue;
         }

         VkBool32 supports_present = false;
         if(!vk_valid(vkGetPhysicalDeviceSurfaceSupportKHR(context->device.physical_device, i, context->surface, &supports_present)))
            return false;

         if(supports_present)
         {
            queue_family->present_index = i;
            ++transfer_score;
            continue;
         }

         if(queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
         {
            queue_family->compute_index = i;
            ++transfer_score;
            continue;
         }

         if(queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
         {
            if(transfer_score <= min_transfer_score)
            {
               min_transfer_score = transfer_score;
               queue_family->transfer_index = i;
            }
         }
      }
   }
   else
   {
      // enable to use just the first family
      u32 all_queue_bits = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
      for(u32 i = 0; i < queue_family_count; ++i)
      {
         if((queue_families[i].queueFlags & all_queue_bits) == all_queue_bits)
         {
            queue_family->compute_index = i;
            queue_family->graphics_index = i;
            queue_family->present_index = i;
            queue_family->transfer_index = i;

            context->device.queue_family_count = vulkan_find_unique_family_count(queue_family->graphics_index, queue_family->compute_index,
                                                                                 queue_family->present_index, queue_family->transfer_index);
            vulkan_device_swapchain_support(storage, context, &context->swapchain.info);

            return true;
         }
      }

      return false;
   }

   context->device.queue_family_count = vulkan_find_unique_family_count(queue_family->graphics_index, queue_family->compute_index, 
                                                                        queue_family->present_index, queue_family->transfer_index);
   vulkan_device_swapchain_support(storage, context, &context->swapchain.info);

   return true;
}

static bool vulkan_device_create(arena scratch, vulkan_context* context)
{
   pre((array_count(context->device.queue_indexes) >= context->device.queue_family_count));

	if(!vulkan_device_select_physical(context->storage, context))
		return false;

   VkDeviceQueueCreateInfo* device_queue_infos = new(&scratch, VkDeviceQueueCreateInfo, context->device.queue_family_count);
   VkDeviceQueueCreateInfo* queue = device_queue_infos;
   const VkDeviceQueueCreateInfo* end = scratch_end_count(queue, scratch, context->device.queue_family_count);


   u32 family_index = 0;
   while(queue != end)
   {
      if(context->device.queue_indexes[family_index] == INVALID_QUEUE_INDEX)
      {
         family_index++;   // skip to the next family
         continue;
      }

      queue->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

      queue->queueFamilyIndex = context->device.queue_indexes[family_index];
      queue->queueCount = 1;

      queue->flags = 0;
      queue->pNext = 0;

      const f32 queue_priority[] = {1.0f};
      queue->pQueuePriorities = queue_priority;

      queue++; 
      family_index++;
   }

   VkPhysicalDeviceFeatures physical_device_features = {};
   physical_device_features.samplerAnisotropy = VK_TRUE;

   const char* device_extension_name = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
   VkDeviceCreateInfo device_create_info =
   {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pQueueCreateInfos = device_queue_infos,
    .queueCreateInfoCount = context->device.queue_family_count,
    .enabledExtensionCount = 1,
    .ppEnabledExtensionNames = &device_extension_name,
    .pEnabledFeatures = &physical_device_features,
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = 0,
   };

   if(!vk_valid(vkCreateDevice(context->device.physical_device, &device_create_info, 0, &context->device.logical_device)))
      return false;

   if(context->device.queue_indexes[QUEUE_GRAPHICS_INDEX] != INVALID_QUEUE_INDEX)
      vkGetDeviceQueue(context->device.logical_device, context->device.queue_indexes[QUEUE_GRAPHICS_INDEX], 0, &context->device.graphics_queue);
   if(context->device.queue_indexes[QUEUE_PRESENT_INDEX] != INVALID_QUEUE_INDEX)
      vkGetDeviceQueue(context->device.logical_device, context->device.queue_indexes[QUEUE_PRESENT_INDEX], 0, &context->device.present_queue);
   if(context->device.queue_indexes[QUEUE_COMPUTE_INDEX] != INVALID_QUEUE_INDEX)
      vkGetDeviceQueue(context->device.logical_device, context->device.queue_indexes[QUEUE_COMPUTE_INDEX], 0, &context->device.compute_queue);
   if(context->device.queue_indexes[QUEUE_TRANSFER_INDEX] != INVALID_QUEUE_INDEX)
      vkGetDeviceQueue(context->device.logical_device, context->device.queue_indexes[QUEUE_TRANSFER_INDEX], 0, &context->device.transfer_queue);

   VkCommandPoolCreateInfo pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
   pool_create_info.queueFamilyIndex = context->device.queue_indexes[QUEUE_GRAPHICS_INDEX];
   pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

   if(!vk_valid(vkCreateCommandPool(context->device.logical_device, &pool_create_info, context->allocator, &context->device.graphics_command_pool)))
      return false;

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

      // find depth stencil attachment format
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

	memset(context->device.queue_indexes, INVALID_QUEUE_INDEX, sizeof(context->device.queue_indexes));
}
