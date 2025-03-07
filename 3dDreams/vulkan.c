#include "arena.h"
#include "common.h"
#include "vulkan.h"

// unity build
#include "vulkan_common.c"
#include "vulkan_device.c"
#include "vulkan_surface.c"
#include "vulkan_image.c"
#include "vulkan_swapchain.c"
#include "vulkan_renderpass.c"
#include "vulkan_command_buffer.c"
#include "vulkan_framebuffer.c"
#include "vulkan_fence.c"

// Function to dynamically load vkCreateDebugUtilsMessengerEXT
static VkResult vulkan_create_debugutils_messenger_ext(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
   PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
   if(!func)
      return VK_ERROR_EXTENSION_NOT_PRESENT;
   return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* pUserData) {

   debug_message("Validation layer: %s\n", data->pMessage);

   return VK_FALSE;
}

static b32 vulkan_command_buffers_create(vulkan_context* context)
{
   b32 result = true;

   pre(context->swapchain.image_count <= VULKAN_MAX_FRAME_BUFFER_COUNT);

   for(u32 i = 0; i < context->swapchain.image_count; ++i)
   {
      if(context->graphics_command_buffers[i].handle)
         vulkan_command_buffer_free(context, context->graphics_command_buffers + i, context->device.graphics_command_pool);
      result &= vulkan_command_buffer_allocate_primary(context, context->device.graphics_command_pool);
   }

   return result;
}

static b32 vulkan_regenerate_framebuffers(vulkan_context* context)
{
   for(u32 i = 0; i < context->swapchain.image_count; ++i)
   {
      VkImageView attachments[] = {context->swapchain.views[i], context->swapchain.depth_attachment.view};
      context->swapchain.framebuffers[i].attachments = attachments;
      context->swapchain.framebuffers[i].attachment_count = array_count(attachments);
      if(!vulkan_framebuffer_create(context, context->swapchain.framebuffers + i))
         return false;
   }

   return true;
}

static b32 vulkan_create_renderer(arena scratch, vulkan_context* context, const hw_window* window)
{
   // TODO: semcomp
   u32 ext_count = 0;
   if(!VK_VALID(vkEnumerateInstanceExtensionProperties(0, &ext_count, 0)))
      return false;

   VkExtensionProperties* ext = new(&scratch, VkExtensionProperties, ext_count);
   if(scratch_end(scratch, ext) || !VK_VALID(vkEnumerateInstanceExtensionProperties(0, &ext_count, ext)))
      return false;

   VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
   app_info.pApplicationName = "VulkanApp";
   app_info.applicationVersion = VK_API_VERSION_1_0;
   app_info.engineVersion = VK_API_VERSION_1_0;
   app_info.pEngineName = "3dDreams";
   app_info.apiVersion = VK_API_VERSION_1_0;

   VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
   instance_info.pApplicationInfo = &app_info;

#ifdef _DEBUG
   {
      const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
      instance_info.enabledLayerCount = array_count(validationLayers);
      instance_info.ppEnabledLayerNames = validationLayers;
   }
#endif

   const char** ext_names = new(&scratch, const char*, ext_count);
   for(size_t i = 0; i < ext_count; ++i)
      ext_names[i] = ext[i].extensionName;

   instance_info.enabledExtensionCount = ext_count;
   instance_info.ppEnabledExtensionNames = ext_names;

   if(scratch_end(scratch, ext_names) || !VK_VALID(vkCreateInstance(&instance_info, 0, &context->instance)))
      return false;

#ifdef _DEBUG 
   {
      VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
      debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
      debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      debugCreateInfo.pfnUserCallback = vulkan_debug_callback;

      VkDebugUtilsMessengerEXT messenger;

      if(!VK_VALID(vulkan_create_debugutils_messenger_ext(context->instance, &debugCreateInfo, 0, &messenger)))
         return false;
   }
#endif

   if(!vulkan_window_surface_create(context, window, ext_names, ext_count))
      return false;

   if(!vulkan_device_create(scratch, context))
      return false;

   if(!vulkan_swapchain_create(context))
      return false;

   if(!vulkan_renderpass_create(context))
      return false;

   if(!vulkan_command_buffers_create(context))
      return false;

   if(!vulkan_regenerate_framebuffers(context))
      return false;

   if(!vulkan_fence_create(context))
      return false;

   return true;
}

void vulkan_present(vulkan_context* context)
{
	pre(context);

   // todo...
}

b32 vulkan_initialize(hw* hw)
{
   b32 result = true;
   pre(hw->renderer.window.handle);

   vulkan_context* context = new(&hw->vulkan_storage, vulkan_context);
   if(arena_end(&hw->vulkan_storage, context))
		return false;
   context->storage = &hw->vulkan_storage;

   //context->device.use_single_family_queue = true;
   result = vulkan_create_renderer(hw->vulkan_scratch, context, &hw->renderer.window);

   hw->renderer.backends[vulkan_renderer_index] = context;
   hw->renderer.frame_present = vulkan_present;
   hw->renderer.renderer_index = vulkan_renderer_index;

   post(hw->renderer.backends[vulkan_renderer_index]);
   post(hw->renderer.frame_present);
   post(hw->renderer.renderer_index == vulkan_renderer_index);

   return result;
}

b32 vulkan_deinitialize(hw* hw)
{
   vulkan_context* context = hw->renderer.backends[vulkan_renderer_index];

   if(!VK_VALID(vkDeviceWaitIdle(context->device.logical_device)))
      return false;

   return true;
}
