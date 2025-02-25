#include "arena.h"
#include "common.h"
#include "vulkan.h"

// unity build
#include "vulkan_device.c"
#include "vulkan_surface.c"
#include "vulkan_image.c"
#include "vulkan_swapchain.c"

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

static bool vulkan_create_renderer(arena scratch, arena* perm, vulkan_context* context, const hw_window* window)
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

   if(!VK_VALID(vkCreateInstance(&instance_info, 0, &context->instance)))
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

   if(!vulkan_device_create(scratch, perm, context))
      return false;

   if(!vulkan_swapchain_create(perm, context))
      return false;

   return true;
}

void vulkan_present(vulkan_context* context)
{
	pre(context);
}

bool vulkan_initialize(hw* hw)
{
   bool result = true;
   pre(hw->renderer.window.handle);

   vulkan_context* context = new(&hw->permanent, vulkan_context);
   if(arena_end(&hw->permanent, context))
		return false;

   context->device.use_single_family_queue = true;
   result = vulkan_create_renderer(hw->scratch, &hw->permanent, context, &hw->renderer.window);

   hw->renderer.backends[vulkan_renderer_index] = context;
   hw->renderer.frame_present = vulkan_present;
   hw->renderer.renderer_index = vulkan_renderer_index;

   post(hw->renderer.backends[vulkan_renderer_index]);
   post(hw->renderer.frame_present);
   post(hw->renderer.renderer_index == vulkan_renderer_index);

   return result;
}

