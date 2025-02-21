#include "hw_arena.h"
#include "common.h"
#include "malloc.h"
#include "vulkan.h"

// unity build
#include "vulkan_device.c"
#include "vulkan_surface.c"
#include "vulkan_swapchain.c"

// TODO: Remove
#if 0
typedef struct vulkan_renderer
{
   VkInstance instance;
   VkSurfaceKHR surface;
   VkPhysicalDevice physical_device;
   VkDevice logical_device;
   VkSwapchainKHR swap_chain;
   VkCommandBuffer draw_cmd_buffer;
   VkRenderPass render_pass;
   VkFramebuffer frame_buffers[VULKAN_FRAME_BUFFER_COUNT];
   VkImage images[VULKAN_FRAME_BUFFER_COUNT];
   VkImageView image_views[VULKAN_FRAME_BUFFER_COUNT];
   VkQueue queue;
   VkFormat format;
   VkSemaphore semaphore_image_available;
   VkSemaphore semaphore_render_finished;
   VkFence fence_frame;
   u32    surface_width;
   u32    surface_height;
   u32    swap_chain_count;
} vulkan_renderer;

typedef struct queue_family_indices
{
   u32 graphics_family;
   u32 present_family;
   bool valid;
} queue_family_indices;
#endif

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

static bool vulkan_are_extensions_supported(VkPhysicalDevice device)
{
   return true;
}

static bool vulkan_create_renderer(hw_frame_arena* arena, vulkan_context* context, const hw_window* window)
{
   u32 extension_count = 0;
   if(!VK_VALID(vkEnumerateInstanceExtensionProperties(0, &extension_count, 0)))
      return false;

   // TODO: helper for vulkan allocs
   hw_arena extensions_arena = arena_push_size(arena, extension_count * sizeof(VkExtensionProperties), VkExtensionProperties);
   VkExtensionProperties* extensions = arena_base(arena, VkExtensionProperties);

   if(!arena_is_set(&extensions_arena, extension_count))
		extension_count = 0;
   else if(!VK_VALID(vkEnumerateInstanceExtensionProperties(0, &extension_count, extensions)))
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

   extensions_arena = arena_push_size(arena, extension_count * sizeof(const char*), const char*);
   if(!arena_is_set(&extensions_arena, extension_count))
		extension_count = 0;
   else
   {
      const char** extension_names = arena_base(&extensions_arena, const char*);
      for(size_t i = 0; i < extension_count; ++i)
         extension_names[i] = extensions[i].extensionName;

      instance_info.enabledExtensionCount = extension_count;
      instance_info.ppEnabledExtensionNames = extension_names;
   }

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

   // TODO: compress extension names and count to info struct
   if(!vulkan_window_surface_create(context, window, arena_base(&extensions_arena, const char*), instance_info.enabledExtensionCount))
      return false;

   if(!vulkan_device_create(arena, context))
      return false;

	if(!vulkan_swapchain_create(context))
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

   hw_arena context_arena = arena_push_struct(&hw->main_arena, vulkan_context);
   if(arena_is_stub(context_arena))
		return false;

   // TODO: we need perm and temp vulkan arenas
   // Currently the context is inside the perm arena rest is using frame arenas
   hw_arena frame_arena = {0};
   defer_frame(&hw->main_arena, frame_arena, result = 
      vulkan_create_renderer(&frame_arena, arena_base(&context_arena, vulkan_context), &hw->renderer.window));

   hw->renderer.backends[vulkan_renderer_index] = arena_base(&context_arena, vulkan_context);
   hw->renderer.frame_present = vulkan_present;
   hw->renderer.renderer_index = vulkan_renderer_index;

   post(hw->renderer.backends[vulkan_renderer_index]);
   post(hw->renderer.frame_present);
   post(hw->renderer.renderer_index == vulkan_renderer_index);

   return result;
}

