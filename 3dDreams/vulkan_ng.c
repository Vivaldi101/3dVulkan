#include "arena.h"
#include "common.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

// TODO: pass these as function pointers from the platform
#include "win32_file_io.c"

#pragma comment(lib,	"vulkan-1.lib")

enum { VULKAN_MAX_FRAME_BUFFER_COUNT = 3, OBJECT_SHADER_COUNT = 2 };

#define VK_VALID(v) ((v) == VK_SUCCESS)
#define VK_ASSERT(v) \
        do { \
          VkResult r = (v); \
          assert(VK_VALID(r)); \
        } while(0)

#define VK_TEST(v) if(!VK_VALID((v))) return false
#define VK_TEST_HANDLE(v) if(!VK_VALID((v))) return VK_NULL_HANDLE
#define VK_RETURN_HANDLE(r, v) VK_TEST(r) ? (v) : VK_NULL_HANDLE

align_struct
{
   arena* storage;
   VkInstance instance;
   VkPhysicalDevice pdev;
   VkDevice ldev;
   VkSurfaceKHR surface;
   VkSwapchainKHR swapchain;
   VkAllocationCallbacks* allocator;
   u32 queue_family_index;
} vulkan_context;

void vulkan_resize(void* renderer, u32 width, u32 height)
{
   // ...
}

void vulkan_present(hw* hw)
{
   // ...
}

static VkPhysicalDevice vulkan_pdevice_select(vulkan_context* context)
{
   VkPhysicalDevice devs[16];
   u32 dev_count = array_count(devs);
   VK_TEST_HANDLE(vkEnumeratePhysicalDevices(context->instance, &dev_count, devs));

   for(u32 i = 0; i < dev_count; ++i)
   {
      VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(devs[i], &props);

      if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
         return devs[i];
   }

   return dev_count > 0 ? devs[0] : 0;
}

static u32 vulkan_ldevice_select_index(vulkan_context* context)
{
   // placeholder
   return 0;
}

static VkDevice vulkan_ldevice_select(vulkan_context* context, u32 family_index)
{
   f32 queue_prio = 1.0f;

   VkDeviceQueueCreateInfo queue_info = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
   queue_info.queueFamilyIndex = family_index; // TODO: query the right queue family
   queue_info.queueCount = 1;
   queue_info.pQueuePriorities = &queue_prio;

   VkDeviceCreateInfo ldev_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
   const char* dev_ext_names[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
   ldev_info.queueCreateInfoCount = 1;
   ldev_info.pQueueCreateInfos = &queue_info;
   ldev_info.enabledExtensionCount = array_count(dev_ext_names);
   ldev_info.ppEnabledExtensionNames = dev_ext_names;

   VkDevice dev;
   VkResult r = vkCreateDevice(context->pdev, &ldev_info, context->allocator, &dev);

   return VK_VALID(r) ? dev : 0;
}

static VkSwapchainKHR vulkan_swapchain_create(vulkan_context* context, u32 w, u32 h)
{
   VkSwapchainKHR swapchain;
   VkSwapchainCreateInfoKHR swapchain_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};

   swapchain_info.surface = context->surface;
   swapchain_info.minImageCount = 2;
   swapchain_info.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
   swapchain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
   swapchain_info.imageExtent.width = w;
   swapchain_info.imageExtent.height = h;
   swapchain_info.imageArrayLayers = 1;
   swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   swapchain_info.queueFamilyIndexCount = 1;
   swapchain_info.pQueueFamilyIndices = &context->queue_family_index;
   swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;

   VkResult r = vkCreateSwapchainKHR(context->ldev, &swapchain_info, context->allocator, &swapchain);

   return VK_VALID(r) ? swapchain : 0;
}

static VkSurfaceKHR vulkan_window_surface_create(vulkan_context* context, const hw_window* window, const char** const extension_names, usize extension_count)
{
   bool isWin32Surface = false;

   for(usize i = 0; i < extension_count; ++i)
      if(strcmp(extension_names[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
         isWin32Surface = true;
         break;
      }

   if(!isWin32Surface)
      return 0;

   PFN_vkCreateWin32SurfaceKHR vkWin32SurfaceFunction = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(context->instance, "vkCreateWin32SurfaceKHR");

   if(!vkWin32SurfaceFunction)
      return 0;

   VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = {0};
   win32SurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
   win32SurfaceInfo.hinstance = GetModuleHandleA(0);
   win32SurfaceInfo.hwnd = window->handle;

   VkSurfaceKHR surface;
   VkResult r = vkWin32SurfaceFunction(context->instance, &win32SurfaceInfo, 0, &surface);

   return VK_VALID(r) ? surface : 0;
}

bool vulkan_initialize(hw* hw)
{
   if(!hw->renderer.window.handle)
      return false;

   vulkan_context* context = new(&hw->vulkan_storage, vulkan_context);
   if(arena_end(&hw->vulkan_storage, context))
      return false;

   context->storage = &hw->vulkan_storage;

   VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
   instance_info.pApplicationInfo = &(VkApplicationInfo) { .apiVersion = VK_API_VERSION_1_2 };

   arena scratch = hw->vulkan_scratch;
   // extensions
   u32 ext_count = 0;
   if(!VK_VALID(vkEnumerateInstanceExtensionProperties(0, &ext_count, 0)))
      return false;
   VkExtensionProperties* ext = new(&scratch, VkExtensionProperties, ext_count);
   if(scratch_end(scratch, ext) || !VK_VALID(vkEnumerateInstanceExtensionProperties(0, &ext_count, ext)))
      return false;
   const char** ext_names = new(&scratch, const char*, ext_count);

   for(size_t i = 0; i < ext_count; ++i)
      ext_names[i] = ext[i].extensionName;

   instance_info.enabledExtensionCount = ext_count;
   instance_info.ppEnabledExtensionNames = ext_names;

#ifdef _DEBUG
   {
      const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
      instance_info.enabledLayerCount = array_count(validationLayers);
      instance_info.ppEnabledLayerNames = validationLayers;
   }
#endif

   VK_TEST(vkCreateInstance(&instance_info, 0, &context->instance));

   context->pdev = vulkan_pdevice_select(context);
   context->queue_family_index = vulkan_ldevice_select_index(context);
   context->ldev = vulkan_ldevice_select(context, context->queue_family_index);
   context->surface = vulkan_window_surface_create(context, &hw->renderer.window, ext_names, ext_count);
   context->swapchain = vulkan_swapchain_create(context, hw->renderer.window.width, hw->renderer.window.height);

   // app callbacks
   hw->renderer.backends[vulkan_renderer_index] = context;
   hw->renderer.frame_present = vulkan_present;
   hw->renderer.frame_resize = vulkan_resize;
   hw->renderer.renderer_index = vulkan_renderer_index;

   return true;
}

bool vulkan_uninitialize(hw* hw)
{
   vulkan_context* context = hw->renderer.backends[vulkan_renderer_index];

   vkDestroySwapchainKHR(context->ldev, context->swapchain, context->allocator);
   vkDestroySurfaceKHR(context->instance, context->surface, context->allocator);
   vkDestroyDevice(context->ldev, context->allocator);
   vkDestroyInstance(context->instance, context->allocator);

   return true;
}
