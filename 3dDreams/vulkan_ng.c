#include "arena.h"
#include "common.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

// TODO: pass these as function pointers from the platform
#include "win32_file_io.c"

#pragma comment(lib,	"vulkan-1.lib")

enum { OBJECT_SHADER_COUNT = 2 };

// TODO: Why release wont build

#define VK_VALID(v) ((v) == VK_SUCCESS)
#define VK_TEST(v) if(!VK_VALID((v))) return false
#define VK_TEST_HANDLE(v) if(!VK_VALID((v))) return VK_NULL_HANDLE

#ifdef _DEBUG
#define VK_ASSERT(v) \
        do { \
          VkResult r = (v); \
          assert(VK_VALID(r)); \
        } while(0)
#else
#define VK_ASSERT(v) (v)
#endif

align_struct
{
   arena* storage;
   VkInstance instance;
   VkPhysicalDevice pdev;
   VkDevice ldev;
   VkSurfaceKHR surface;
   VkSwapchainKHR swapchain;
   VkAllocationCallbacks* allocator;
   VkSemaphore image_ready_semaphore;
   VkSemaphore image_done_semaphore;
   VkQueue graphics_queue;
   VkCommandPool command_pool;
   VkCommandBuffer command_buffer;

   VkImage swapchain_images[16]; // should be big enough
   u32 swapchain_image_count;
} vulkan_context;

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
   if(dev_count > 0)
      return devs[0];
   return 0;
}

static u32 vulkan_ldevice_select_index(vulkan_context* context)
{
   // placeholder
   return 0;
}

static VkDevice vulkan_ldevice_create(vulkan_context* context, u32 queue_family_index)
{
   f32 queue_prio = 1.0f;

   VkDeviceQueueCreateInfo queue_info = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
   queue_info.queueFamilyIndex = queue_family_index; // TODO: query the right queue family
   queue_info.queueCount = 1;
   queue_info.pQueuePriorities = &queue_prio;

   VkDeviceCreateInfo ldev_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
   const char* dev_ext_names[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
   ldev_info.queueCreateInfoCount = 1;
   ldev_info.pQueueCreateInfos = &queue_info;
   ldev_info.enabledExtensionCount = array_count(dev_ext_names);
   ldev_info.ppEnabledExtensionNames = dev_ext_names;

   VkDevice dev;
   VK_TEST_HANDLE(vkCreateDevice(context->pdev, &ldev_info, context->allocator, &dev));

   return dev;
}

static VkQueue vulkan_graphics_queue_create(vulkan_context* context, u32 queue_family_index)
{
   VkQueue graphics_queue = 0;

   // TODO: Get the queue index
   vkGetDeviceQueue(context->ldev, queue_family_index, 0, &graphics_queue);

   return graphics_queue;
}

static VkSemaphore vulkan_semaphore_create(vulkan_context* context)
{
   VkSemaphore sema = 0;

   VkSemaphoreCreateInfo sema_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
   VK_TEST_HANDLE(vkCreateSemaphore(context->ldev, &sema_info, context->allocator, &sema));

   return sema;
}

static VkSwapchainKHR vulkan_swapchain_create(vulkan_context* context, u32 queue_family_index, u32 w, u32 h)
{
   VkSwapchainKHR swapchain = 0;

   VkSurfaceCapabilitiesKHR surface_caps;
   VK_TEST_HANDLE(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->pdev, context->surface, &surface_caps));

#if 0
      // surface does not allow to choose the size
   if(surface_caps.currentExtent.width != UINT32_MAX)
      swapchain_extent = surface_caps.currentExtent;
   else
   {
      // surface allows to choose the size
      VkExtent2D min = surface_caps.minImageExtent;
      VkExtent2D max = surface_caps.maxImageExtent;
      swapchain_extent.width = clamp(swapchain_extent.width, min.width, max.width);
      swapchain_extent.height = clamp(swapchain_extent.height, min.height, max.height);
   }
#endif

   u32 image_count = surface_caps.minImageCount + 1;

   // atleast double buffering
   if(image_count < 2)
      return 0;

   context->swapchain_image_count = image_count;

   VkSwapchainCreateInfoKHR swapchain_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
   swapchain_info.surface = context->surface;
   swapchain_info.minImageCount = image_count; // triple buffering
   swapchain_info.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
   swapchain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
   swapchain_info.imageExtent.width = w;
   swapchain_info.imageExtent.height = h;
   swapchain_info.imageArrayLayers = 1;
   swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   swapchain_info.queueFamilyIndexCount = 1;
   swapchain_info.pQueueFamilyIndices = &queue_family_index;
   swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
   swapchain_info.preTransform = surface_caps.currentTransform;
   swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   swapchain_info.clipped = VK_TRUE;
   swapchain_info.oldSwapchain = context->swapchain;

   VK_TEST_HANDLE(vkCreateSwapchainKHR(context->ldev, &swapchain_info, context->allocator, &swapchain));

   return swapchain;
}

VkCommandBuffer vulkan_command_buffer_create(vulkan_context* context)
{
   VkCommandBuffer buffer = 0;
   VkCommandBufferAllocateInfo buffer_allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};

   buffer_allocate_info.commandBufferCount = 1;
   buffer_allocate_info.commandPool = context->command_pool;
   buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

   VK_ASSERT(vkAllocateCommandBuffers(context->ldev, &buffer_allocate_info, &buffer));

   return buffer;
}

VkCommandPool vulkan_command_pool_create(vulkan_context* context, u32 queue_family_index)
{
   VkCommandPool pool = 0;

   VkCommandPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
   pool_info.queueFamilyIndex = queue_family_index;

   VK_TEST_HANDLE(vkCreateCommandPool(context->ldev, &pool_info, context->allocator, &pool));

   return pool;
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
   VK_TEST_HANDLE(vkWin32SurfaceFunction(context->instance, &win32SurfaceInfo, 0, &surface));

   return surface;
}

void vulkan_resize(void* renderer, u32 width, u32 height)
{
   // ...
}

void vulkan_present(vulkan_context* context)
{
   u32 image_index = 0;

   VK_ASSERT(vkAcquireNextImageKHR(context->ldev, context->swapchain, UINT64_MAX, context->image_ready_semaphore, VK_NULL_HANDLE, &image_index));
   VK_ASSERT(vkResetCommandPool(context->ldev, context->command_pool, 0));

   VkCommandBufferBeginInfo buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
   buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   VkCommandBuffer buffer = context->command_buffer;

   VK_ASSERT(vkBeginCommandBuffer(buffer, &buffer_begin_info));

   VkClearColorValue color = {1, 0, 1, 1};

   VkImageSubresourceRange range = {};
   range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   range.layerCount = 1;
   range.levelCount = 1;

   vkCmdClearColorImage(buffer, context->swapchain_images[image_index], VK_IMAGE_LAYOUT_GENERAL, &color, 1, &range);

   VK_ASSERT(vkEndCommandBuffer(buffer));

   VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
   submit_info.waitSemaphoreCount = 1;
   submit_info.pWaitSemaphores = &context->image_ready_semaphore;

   submit_info.pWaitDstStageMask = &(VkPipelineStageFlags) { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &buffer;

   submit_info.signalSemaphoreCount = 1;
   submit_info.pSignalSemaphores = &context->image_done_semaphore;

   VK_ASSERT(vkQueueSubmit(context->graphics_queue, 1, &submit_info, VK_NULL_HANDLE));

   VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
   present_info.swapchainCount = 1;
   present_info.pSwapchains = &context->swapchain;

   present_info.pImageIndices = &image_index;

   present_info.waitSemaphoreCount = 1;
   present_info.pWaitSemaphores = &context->image_done_semaphore;

   VK_ASSERT(vkQueuePresentKHR(context->graphics_queue, &present_info));

   // wait until all queue ops are done
   VK_ASSERT(vkDeviceWaitIdle(context->ldev));
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

   u32 queue_family_index = vulkan_ldevice_select_index(context);
   context->pdev = vulkan_pdevice_select(context);
   context->ldev = vulkan_ldevice_create(context, queue_family_index);
   context->surface = vulkan_window_surface_create(context, &hw->renderer.window, ext_names, ext_count);
   context->swapchain = vulkan_swapchain_create(context, queue_family_index, hw->renderer.window.width, hw->renderer.window.height);
   context->image_ready_semaphore = vulkan_semaphore_create(context);
   context->image_done_semaphore = vulkan_semaphore_create(context);
   context->graphics_queue = vulkan_graphics_queue_create(context, queue_family_index);
   context->command_pool = vulkan_command_pool_create(context, queue_family_index);
   context->command_buffer = vulkan_command_buffer_create(context);

   VK_TEST(vkGetSwapchainImagesKHR(context->ldev, context->swapchain, &context->swapchain_image_count, context->swapchain_images));

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
