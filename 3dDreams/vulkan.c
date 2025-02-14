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

static bool vulkan_create_renderer(hw_arena* arena, vulkan_context* context, const hw_window* window)
{
   pre(context);
   pre(window);

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
   if(is_stub(context_arena))
		return false;

   hw_arena frame_arena = {0};
   defer_frame(&hw->main_arena, frame_arena, result = 
      vulkan_create_renderer(&frame_arena, arena_base(&context_arena, vulkan_context), &hw->renderer.window));

   hw->renderer.backends[vulkan_renderer_index] = arena_base(&context_arena, vulkan_context);
   hw->renderer.frame_present = vulkan_present;
   hw->renderer.renderer_index = vulkan_renderer_index;

   post(hw->renderer.backends[vulkan_renderer_index]);
   post(hw->renderer.frame_present);
   post(hw->renderer.renderer_index == vulkan_renderer_index);

   //if(!result)
		//hw_error(&frame_arena, "Vulkan not created successfully");

   return result;
}

// TODO: Remove
#if 0
static queue_family_indices vulkan_find_queue_families(hw_arena* arena, VkPhysicalDevice device, VkSurfaceKHR surface)
{
   queue_family_indices indices = {};

   uint32_t queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, 0);

   VkQueueFamilyProperties* queueFamilies = vulkan_allocate(arena, queueFamilyCount * sizeof(VkQueueFamilyProperties));
   vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

   for(u32 i = 0; i < queueFamilyCount; ++i)
   {
      VkQueueFamilyProperties queueFamily = queueFamilies[i];
      if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
         indices.graphics_family = i;

      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

      if(presentSupport)
      {
         indices.present_family = i;
         indices.valid = true;
      }
      if(indices.valid)
         break;
      i++;
   }

   // TODO: Currently just assume that graphics and present queues belong to same family
   hw_assert(indices.graphics_family == indices.present_family);

   return indices;
}

static bool vulkan_is_device_compatible(hw_arena* arena, VkPhysicalDevice device, VkSurfaceKHR surface)
{
   const queue_family_indices result = vulkan_find_queue_families(arena, device, surface);
   return result.valid && vulkan_are_extensions_supported(device);
}

static void vulkan_create_sync_objects(vulkan_renderer* renderer)
{
   pre(renderer);
   VkSemaphoreCreateInfo semaphoreInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
   VkFenceCreateInfo fenceInfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT};

   if(!VK_VALID(vkCreateSemaphore(renderer->logical_device, &semaphoreInfo, 0, &renderer->semaphore_image_available)))
      hw_assert(0);
   if(!VK_VALID(vkCreateSemaphore(renderer->logical_device, &semaphoreInfo, 0, &renderer->semaphore_render_finished)))
      hw_assert(0);
   if(!VK_VALID(vkCreateFence(renderer->logical_device, &fenceInfo, 0, &renderer->fence_frame)))
      hw_assert(0);
}

static void vulkan_create_renderer(hw_arena* arena, vulkan_renderer* renderer, HWND windowHandle)
{
   u32 extensionCount = 0;
   if(!VK_VALID(vkEnumerateInstanceExtensionProperties(0, &extensionCount, 0)))
      hw_assert(0);

   inv(extensionCount > 0);
   VkExtensionProperties* extensions = vulkan_allocate(arena, extensionCount * sizeof(VkExtensionProperties));
   inv(extensions);

   if(!VK_VALID(vkEnumerateInstanceExtensionProperties(0, &extensionCount, extensions)))
      hw_assert(0);

   const char** extensionNames = vulkan_allocate(arena, extensionCount * sizeof(const char*));
   inv(extensionNames);
   for(size_t i = 0; i < extensionCount; ++i)
      extensionNames[i] = extensions[i].extensionName;

   const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};

   VkApplicationInfo appInfo = {0};
   appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName = "VulkanApp";
   appInfo.applicationVersion = VK_API_VERSION_1_0;
   appInfo.engineVersion = VK_API_VERSION_1_0;
   appInfo.pEngineName = "No Engine";
   appInfo.apiVersion = VK_API_VERSION_1_0;

   VkInstanceCreateInfo instanceInfo = {0};
   instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   instanceInfo.pApplicationInfo = &appInfo;
   instanceInfo.enabledLayerCount = array_count(validationLayers);
   instanceInfo.ppEnabledLayerNames = validationLayers;
   instanceInfo.enabledExtensionCount = extensionCount;
   instanceInfo.ppEnabledExtensionNames = extensionNames;

   if(!VK_VALID(vkCreateInstance(&instanceInfo, 0, &renderer->instance)))
      hw_assert(0);

   VkDebugUtilsMessengerEXT debugMessenger;
   VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
   debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   debugCreateInfo.pfnUserCallback = vulkan_debug_callback;

   if(!VK_VALID(vulkan_create_debugutils_messenger_ext(renderer->instance, &debugCreateInfo, 0, &debugMessenger)))
      hw_assert(0);

   bool isWin32Surface = false;
   for(size_t i = 0; i < extensionCount; ++i)
      if(strcmp(extensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
         isWin32Surface = true;
         break;
      }

   if(!isWin32Surface)
      hw_assert(0);

   PFN_vkCreateWin32SurfaceKHR vkWin32SurfaceFunction = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(renderer->instance, "vkCreateWin32SurfaceKHR");

   if(!vkWin32SurfaceFunction)
      hw_assert(0);

   VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = {0};
   win32SurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
   win32SurfaceInfo.hinstance = GetModuleHandleA(0);
   win32SurfaceInfo.hwnd = windowHandle;

   vkWin32SurfaceFunction(renderer->instance, &win32SurfaceInfo, 0, &renderer->surface);

   u32 deviceCount = 0;
   if(!VK_VALID(vkEnumeratePhysicalDevices(renderer->instance, &deviceCount, 0)))
      hw_assert(0);
   hw_assert(deviceCount > 0);   // Just use the first device for now

   VkPhysicalDevice* devices = vulkan_allocate(arena, deviceCount * sizeof(VkPhysicalDevice));
   if(!devices)
      hw_assert(0);

   if(!VK_VALID(vkEnumeratePhysicalDevices(renderer->instance, &deviceCount, devices)))
      hw_assert(0);

   for(u32 i = 0; i < deviceCount; ++i)
      if(vulkan_is_device_compatible(arena, devices[i], renderer->surface))
      {
         renderer->physical_device = devices[i];
         break;
      }

   hw_assert(renderer->physical_device != VK_NULL_HANDLE);

   VkDeviceQueueCreateInfo queueInfo = {0};
   const queue_family_indices queueFamilies = vulkan_find_queue_families(arena, renderer->physical_device, renderer->surface);
   hw_assert(queueFamilies.valid);
   queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
   queueInfo.queueFamilyIndex = queueFamilies.graphics_family;
   queueInfo.queueCount = 1;
   const float queuePriorities[] = {1.0f};
   queueInfo.pQueuePriorities = queuePriorities;

   extensionCount = 0;
   if(!VK_VALID(vkEnumerateDeviceExtensionProperties(renderer->physical_device, 0, &extensionCount, 0)))
      hw_assert(0);

   inv(extensionCount > 0);
   extensions = vulkan_allocate(arena, extensionCount * sizeof(VkExtensionProperties));
   inv(extensions);

   if(!VK_VALID(vkEnumerateDeviceExtensionProperties(renderer->physical_device, 0, &extensionCount, extensions)))
      hw_assert(0);

   extensionNames = vulkan_allocate(arena, extensionCount * sizeof(const char*));
   inv(extensionNames);
   for(size_t i = 0; i < extensionCount; ++i)
      extensionNames[i] = extensions[i].extensionName;

   {
      VkDeviceCreateInfo info =
      {
       .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
       .pQueueCreateInfos = &queueInfo,
       .queueCreateInfoCount = 1,
       .enabledExtensionCount = extensionCount,
       .ppEnabledExtensionNames = extensionNames,
       .enabledLayerCount = 0,
       .ppEnabledLayerNames = 0,
      };

      VkPhysicalDeviceFeatures physicalFeatures = {0};
      physicalFeatures.shaderClipDistance = VK_TRUE;
      info.pEnabledFeatures = &physicalFeatures;

      if(!VK_VALID(vkCreateDevice(renderer->physical_device, &info, 0, &renderer->logical_device)))
         hw_assert(0);
   }

   VkSurfaceCapabilitiesKHR surfaceCaps = {0};
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->physical_device, renderer->surface, &surfaceCaps);

   VkExtent2D surfaceExtent = surfaceCaps.currentExtent;
   renderer->surface_width = surfaceExtent.width;
   renderer->surface_height = surfaceExtent.height;

   {
      VkSwapchainCreateInfoKHR info =
      {
       .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
       .surface = renderer->surface,
       .minImageCount = VULKAN_FRAME_BUFFER_COUNT,
       .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
       .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
       .imageExtent = surfaceExtent,
       .imageArrayLayers = 1,
       .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
       .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
       .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
       .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
       .presentMode = VK_PRESENT_MODE_FIFO_KHR,
       .clipped = true,
       .oldSwapchain = 0,
          .queueFamilyIndexCount = 1,
          .pQueueFamilyIndices = &queueFamilies.graphics_family,
      };

      // TODO: test if swap chain is already created and re-create it
      if(!VK_VALID(vkCreateSwapchainKHR(renderer->logical_device, &info, 0, &renderer->swap_chain)))
         hw_assert(0);


      renderer->format = info.imageFormat;
   }

   u32 swap_chain_count = 0;
   if(!VK_VALID(vkGetSwapchainImagesKHR(renderer->logical_device, renderer->swap_chain, &swap_chain_count, 0)))
      hw_assert(0);

   if(swap_chain_count != VULKAN_FRAME_BUFFER_COUNT)
      hw_assert(0);

   renderer->swap_chain_count = swap_chain_count;

   VkImage* swapChainImages = vulkan_allocate(arena, swap_chain_count * sizeof(VkImage));
   if(!swapChainImages)
      hw_assert(0);

   if(!VK_VALID(vkGetSwapchainImagesKHR(renderer->logical_device, renderer->swap_chain, &swap_chain_count, swapChainImages)))
      hw_assert(0);

   for(u32 i = 0; i < swap_chain_count; ++i)
      renderer->images[i] = swapChainImages[i];

   VkImageView* image_views = vulkan_allocate(arena, swap_chain_count * sizeof(VkImageView));
   if(!image_views)
      hw_assert(0);

   renderer->swap_chain_count = swap_chain_count;
   for(u32 i = 0; i < swap_chain_count; ++i) {
      VkImageViewCreateInfo info =
      {
       .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = VK_FORMAT_B8G8R8A8_SRGB,
          .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
          .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
          .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
          .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
          .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .subresourceRange.baseMipLevel = 0,
          .subresourceRange.levelCount = 1,
          .subresourceRange.baseArrayLayer = 0,
          .subresourceRange.layerCount = 1,
          .image = swapChainImages[i]
      };

      if(!VK_VALID(vkCreateImageView(renderer->logical_device, &info, 0, &image_views[i])))
         hw_assert(0);
      renderer->image_views[i] = image_views[i];
   }

   u32 queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queueFamilyCount, 0);
   VkQueueFamilyProperties* queueProperties = vulkan_allocate(arena, queueFamilyCount * sizeof(VkQueueFamilyProperties));
   if(!queueProperties)
      hw_assert(0);
   hw_assert(queueFamilyCount > 0);
   vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queueFamilyCount, queueProperties);

   vkGetDeviceQueue(renderer->logical_device, queueFamilies.present_family, 0, &renderer->queue);

   // TODO: Use the above similar to these
   // TODO: Wrap these
   VkCommandPool commandPool = 0;
   {
      VkCommandPoolCreateInfo info =
      {
          .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
          .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
          .pNext = 0,
          .queueFamilyIndex = queueFamilies.graphics_family,
      };
      if(!VK_VALID(vkCreateCommandPool(renderer->logical_device, &info, 0, &commandPool)))
         hw_assert(0);
   }

   VkCommandBuffer draw_cmd_buffer = 0;
   {
      VkCommandBufferAllocateInfo info =
      {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .commandPool = commandPool,
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = 1,
      };
      if(!VK_VALID(vkAllocateCommandBuffers(renderer->logical_device, &info, &draw_cmd_buffer)))
         hw_assert(0);
      renderer->draw_cmd_buffer = draw_cmd_buffer;
   }

   VkRenderPass render_pass = 0;

   VkAttachmentDescription pass[1] = {0};
   pass[0].format = VK_FORMAT_B8G8R8A8_SRGB;
   pass[0].samples = VK_SAMPLE_COUNT_1_BIT;
   pass[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   pass[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   pass[0].stencilLoadOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   pass[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   pass[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   pass[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

   VkAttachmentReference ref = {0};
   ref.attachment = 0;
   ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   VkSubpassDescription subpass = {0};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &ref;

   {
      VkRenderPassCreateInfo info =
      {
       .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
          .attachmentCount = 1,
          .pAttachments = pass,
          .subpassCount = 1,
          .pSubpasses = &subpass,
      };
      if(!VK_VALID(vkCreateRenderPass(renderer->logical_device, &info, 0, &render_pass)))
         hw_assert(0);
      renderer->render_pass = render_pass;
   }

   VkImageView frameBufferAttachments = 0;
   VkFramebuffer* frame_buffers = vulkan_allocate(arena, swap_chain_count * sizeof(VkFramebuffer));
   if(!frame_buffers)
      hw_assert(0);
   {
      VkFramebufferCreateInfo info =
      {
       .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .attachmentCount = 1,
          .pAttachments = &frameBufferAttachments,
          .width = renderer->surface_width,
          .height = renderer->surface_height,
          .layers = 1,
          .renderPass = render_pass,
      };
      for(u32 i = 0; i < swap_chain_count; ++i) {
         frameBufferAttachments = image_views[i];
         if(!VK_VALID(vkCreateFramebuffer(renderer->logical_device, &info, 0, &frame_buffers[i])))
            hw_assert(0);
         renderer->frame_buffers[i] = frame_buffers[i];
      }
   }

   vulkan_create_sync_objects(renderer);

   hw_assert(renderer->instance);
   hw_assert(renderer->surface);
   hw_assert(renderer->physical_device);
   hw_assert(renderer->logical_device);
   hw_assert(renderer->swap_chain);
   hw_assert(renderer->swap_chain_count == VULKAN_FRAME_BUFFER_COUNT);
   hw_assert(renderer->render_pass);
   hw_assert(renderer->frame_buffers);
   hw_assert(renderer->queue);
   hw_assert(renderer->draw_cmd_buffer);
   hw_assert(renderer->format);
   hw_assert(renderer->images);

   hw_assert(renderer->fence_frame);
   hw_assert(renderer->semaphore_image_available);
   hw_assert(renderer->semaphore_render_finished);
}

// TODO: Use this
static void vulkan_record_command_buffer(vulkan_renderer* renderer, u32 imageIndex)
{
   pre(renderer);
}

static void vulkan_render(vulkan_renderer* renderer)
{
   pre(renderer);
   if(!VK_VALID(vkWaitForFences(renderer->logical_device, 1, &renderer->fence_frame, VK_TRUE, UINT64_MAX)))
      post(0);

   if(!VK_VALID(vkResetFences(renderer->logical_device, 1, &renderer->fence_frame)))
      post(0);

   u32 nextImageIndex = 0;
   if(!VK_VALID(vkAcquireNextImageKHR(renderer->logical_device, renderer->swap_chain, UINT64_MAX, renderer->semaphore_image_available, VK_NULL_HANDLE, &nextImageIndex)))
      ;

   if(!VK_VALID(vkResetCommandBuffer(renderer->draw_cmd_buffer, 0)))
      post(0);

   {
      VkCommandBufferBeginInfo info =
      {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          //.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
          .flags = 0, .pInheritanceInfo = 0, .pNext = 0,
      };
      // start this cmd buffer
      if(!VK_VALID(vkBeginCommandBuffer(renderer->draw_cmd_buffer, &info)))
         post(0);
   }
   {
      // temporary float that oscilates between 0 and 1
      // to gradually change the color on the screen
      static float aa = 0.0f;
      // slowly increment
      aa += 0.011f;
      // when value reaches 1.0 reset to 0
      if(aa >= 1.0) aa = 0;
      // activate render pass:
      // clear color (r,g,b,a)
      VkClearValue clearValue[] =
      {
          { 0.0f, aa, aa, 1.0f },	// color
       { 1.0, 0.0 }				// depth stencil
      };

      // define render pass structure
      VkRenderPassBeginInfo info =
      {
       .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
          .renderPass = renderer->render_pass,
          .framebuffer = renderer->frame_buffers[nextImageIndex],
          .renderArea.offset = {0,0},
          .renderArea.extent = {renderer->surface_width, renderer->surface_height},
      };
      VkOffset2D a = {0, 0};
      VkExtent2D b = {renderer->surface_width, renderer->surface_height};
      VkRect2D c = {a,b};
      info.renderArea = c;
      info.clearValueCount = 2;
      info.pClearValues = clearValue;

      vkCmdBeginRenderPass(renderer->draw_cmd_buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
      {
         // draw cmds
      }
      vkCmdEndRenderPass(renderer->draw_cmd_buffer);
   }

   // end this cmd buffer
   if(!VK_VALID(vkEndCommandBuffer(renderer->draw_cmd_buffer)))
      post(0);

   {
      const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
      VkSubmitInfo info =
      {
       .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &renderer->semaphore_image_available,
          .pWaitDstStageMask = waitStages,
          .commandBufferCount = 1,
          .pCommandBuffers = &renderer->draw_cmd_buffer,
          .signalSemaphoreCount = 1,
          .pSignalSemaphores = &renderer->semaphore_render_finished,
      };

      if(!VK_VALID(vkQueueSubmit(renderer->queue, 1, &info, renderer->fence_frame)))
         post(0);
   }
   // present the image on the screen (flip the swap-chain image)
   {
      VkPresentInfoKHR info =
      {
          .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
          .pNext = NULL,
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &renderer->semaphore_render_finished,
          .swapchainCount = 1,
          .pSwapchains = &renderer->swap_chain,
          .pImageIndices = &nextImageIndex,
          .pResults = NULL,
      };
      if(!VK_VALID(vkQueuePresentKHR(renderer->queue, &info)))
         ;
   }
}

static void vulkan_reset(vulkan_renderer* renderer)
{
   pre(renderer);
   pre(renderer->instance);
   // TODO: Clear all teh resources like vulkan image views framebuffers, fences and semaphores etc.
   vkDestroyInstance(renderer->instance, 0);
}

void vulkan_present(vulkan_renderer* renderer)
{
   pre(renderer);
   if(!VK_VALID(vkWaitForFences(renderer->logical_device, 1, &renderer->fence_frame, VK_TRUE, UINT64_MAX)))
      post(0);

   if(!VK_VALID(vkResetFences(renderer->logical_device, 1, &renderer->fence_frame)))
      post(0);

   u32 nextImageIndex = 0;
   if(!VK_VALID(vkAcquireNextImageKHR(renderer->logical_device, renderer->swap_chain, UINT64_MAX, renderer->semaphore_image_available, VK_NULL_HANDLE, &nextImageIndex)))
      ;

   if(!VK_VALID(vkResetCommandBuffer(renderer->draw_cmd_buffer, 0)))
      post(0);

   {
      VkCommandBufferBeginInfo info =
      {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          //.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
          .flags = 0, .pInheritanceInfo = 0, .pNext = 0,
      };
      // start this cmd buffer
      if(!VK_VALID(vkBeginCommandBuffer(renderer->draw_cmd_buffer, &info)))
         post(0);
   }
   {
      // temporary float that oscilates between 0 and 1
      // to gradually change the color on the screen
      static float aa = 0.0f;
      // slowly increment
      aa += 0.001f;
      // when value reaches 1.0 reset to 0
      if(aa >= 1.0) aa = 0;
      // activate render pass:
      // clear color (r,g,b,a)
      VkClearValue clearValue[] =
      {
          { 1.0f, aa, aa, 1.0f },	// color
          { 1.0, 0.0 }				// depth stencil
      };

      // define render pass structure
      VkRenderPassBeginInfo info =
      {
       .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
          .renderPass = renderer->render_pass,
          .framebuffer = renderer->frame_buffers[nextImageIndex],
          .renderArea.offset = {0,0},
          .renderArea.extent = {renderer->surface_width, renderer->surface_height},
      };
      VkOffset2D a = {0, 0};
      VkExtent2D b = {renderer->surface_width, renderer->surface_height};
      VkRect2D c = {a,b};
      info.renderArea = c;
      info.clearValueCount = 2;
      info.pClearValues = clearValue;

      vkCmdBeginRenderPass(renderer->draw_cmd_buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
      {
         // draw cmds
      }
      vkCmdEndRenderPass(renderer->draw_cmd_buffer);
   }

   // end this cmd buffer
   if(!VK_VALID(vkEndCommandBuffer(renderer->draw_cmd_buffer)))
      post(0);

   {
      const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
      VkSubmitInfo info =
      {
       .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &renderer->semaphore_image_available,
          .pWaitDstStageMask = waitStages,
          .commandBufferCount = 1,
          .pCommandBuffers = &renderer->draw_cmd_buffer,
          .signalSemaphoreCount = 1,
          .pSignalSemaphores = &renderer->semaphore_render_finished,
      };

      if(!VK_VALID(vkQueueSubmit(renderer->queue, 1, &info, renderer->fence_frame)))
         post(0);
   }
   // present the image on the screen (flip the swap-chain image)
   {
      VkPresentInfoKHR info =
      {
          .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
          .pNext = NULL,
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &renderer->semaphore_render_finished,
          .swapchainCount = 1,
          .pSwapchains = &renderer->swap_chain,
          .pImageIndices = &nextImageIndex,
          .pResults = NULL,
      };
      if(!VK_VALID(vkQueuePresentKHR(renderer->queue, &info)))
         ;
   }
}
#endif
