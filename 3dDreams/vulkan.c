#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR

#include "hw_arena.h"
#include "common.h"
#include "vulkan.h"
#include "malloc.h"
#include <vulkan/vulkan.h>

#pragma comment(lib,	"vulkan-1.lib")

// TODO: this is dumb - we need to query the swapchain count from vulkan
#define VULKAN_IMAGE_COUNT 3		
#define VK_VALID(v) (v) == VK_SUCCESS
// TODO: rename these to snake

typedef struct vulkan_renderer
{
   VkInstance instance;
   VkSurfaceKHR surface;
   VkPhysicalDevice physical_device;
   VkDevice logical_device;
   VkSwapchainKHR swap_chain;
   VkCommandBuffer draw_cmd_buffer;
   VkRenderPass render_pass;
   VkFramebuffer frame_buffers[VULKAN_IMAGE_COUNT];
   VkImage images[VULKAN_IMAGE_COUNT];
   VkImageView image_views[VULKAN_IMAGE_COUNT];
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

// Function to dynamically load vkCreateDebugUtilsMessengerEXT
static VkResult vulkan_create_debugutils_messenger_ext(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
   PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
   if (!func)
      return VK_ERROR_EXTENSION_NOT_PRESENT;
   return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

// TODO: Move to win32_utils
static void debug_message(const char* format, ...)
{
   char temp[1024];
   va_list args;
   va_start(args, format);
   wvsprintfA(temp, format, args);
   va_end(args);
   OutputDebugStringA(temp);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
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

// TODO: This allocates from the subarena
static void* allocate(size_t size)
{
   return _malloca(size);
}

static queue_family_indices vulkan_find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
{
   queue_family_indices indices = {};

   uint32_t queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, 0);

   VkQueueFamilyProperties* queueFamilies = allocate(queueFamilyCount * sizeof(VkQueueFamilyProperties));
   vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

   for (u32 i = 0; i < queueFamilyCount; ++i)
   {
      VkQueueFamilyProperties queueFamily = queueFamilies[i];
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
         indices.graphics_family = i;

      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

      if (presentSupport)
      {
         indices.present_family = i;
         indices.valid = true;
      }
      if (indices.valid)
         break;
      i++;
   }

   // TODO: Currently just assume that graphics and present queues belong to same family
   post(indices.graphics_family == indices.present_family);

   return indices;
}

static bool vulkan_is_device_compatible(VkPhysicalDevice device, VkSurfaceKHR surface)
{
   const queue_family_indices result = vulkan_find_queue_families(device, surface);
   return result.valid && vulkan_are_extensions_supported(device);
}

static void vulkan_create_sync_objects(vulkan_renderer* renderer)
{
   pre(renderer);
   VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
   VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };

   if (!VK_VALID(vkCreateSemaphore(renderer->logical_device, &semaphoreInfo, 0, &renderer->semaphore_image_available)))
      post(0);
   if (!VK_VALID(vkCreateSemaphore(renderer->logical_device, &semaphoreInfo, 0, &renderer->semaphore_render_finished)))
      post(0);
   if (!VK_VALID(vkCreateFence(renderer->logical_device, &fenceInfo, 0, &renderer->fence_frame)))
      post(0);
}

static void vulkan_create_renderer(vulkan_renderer* renderer, HWND windowHandle)
{
   u32 extensionCount = 0;
   if (!VK_VALID(vkEnumerateInstanceExtensionProperties(0, &extensionCount, 0)))
      post(0);

   inv(extensionCount > 0);
   VkExtensionProperties* extensions = allocate(extensionCount * sizeof(VkExtensionProperties));
   inv(extensions);

   if (!VK_VALID(vkEnumerateInstanceExtensionProperties(0, &extensionCount, extensions)))
      post(0);

   const char** extensionNames = allocate(extensionCount * sizeof(const char*));
   inv(extensionNames);
   for (size_t i = 0; i < extensionCount; ++i)
      extensionNames[i] = extensions[i].extensionName;

   const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };

   VkApplicationInfo appInfo = { 0 };
   appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName = "VulkanApp";
   appInfo.applicationVersion = VK_API_VERSION_1_0;
   appInfo.engineVersion = VK_API_VERSION_1_0;
   appInfo.pEngineName = "No Engine";
   appInfo.apiVersion = VK_API_VERSION_1_0;

   VkInstanceCreateInfo instanceInfo = { 0 };
   instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   instanceInfo.pApplicationInfo = &appInfo;
   instanceInfo.enabledLayerCount = array_count(validationLayers);
   instanceInfo.ppEnabledLayerNames = validationLayers;
   instanceInfo.enabledExtensionCount = extensionCount;
   instanceInfo.ppEnabledExtensionNames = extensionNames;

   if (!VK_VALID(vkCreateInstance(&instanceInfo, 0, &renderer->instance)))
      post(0);

   VkDebugUtilsMessengerEXT debugMessenger;
   VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { 0 };
   debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   debugCreateInfo.pfnUserCallback = VulkanDebugCallback;

   if (!VK_VALID(vulkan_create_debugutils_messenger_ext(renderer->instance, &debugCreateInfo, 0, &debugMessenger)))
      post(0);

   bool isWin32Surface = false;
   for (size_t i = 0; i < extensionCount; ++i)
      if (strcmp(extensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
         isWin32Surface = true;
         break;
      }

   if (!isWin32Surface)
      post(0);

   PFN_vkCreateWin32SurfaceKHR vkWin32SurfaceFunction = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(renderer->instance, "vkCreateWin32SurfaceKHR");

   if (!vkWin32SurfaceFunction)
      post(0);

   VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = { 0 };
   win32SurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
   win32SurfaceInfo.hinstance = GetModuleHandleA(0);
   win32SurfaceInfo.hwnd = windowHandle;

   vkWin32SurfaceFunction(renderer->instance, &win32SurfaceInfo, 0, &renderer->surface);

   u32 deviceCount = 0;
   if (!VK_VALID(vkEnumeratePhysicalDevices(renderer->instance, &deviceCount, 0)))
      post(0);
   post(deviceCount > 0);   // Just use the first device for now

   VkPhysicalDevice* devices = allocate(deviceCount * sizeof(VkPhysicalDevice));
   if (!devices)
      post(0);

   if (!VK_VALID(vkEnumeratePhysicalDevices(renderer->instance, &deviceCount, devices)))
      post(0);

   for (u32 i = 0; i < deviceCount; ++i)
      if (vulkan_is_device_compatible(devices[i], renderer->surface))
      {
         renderer->physical_device = devices[i];
         break;
      }

   post(renderer->physical_device != VK_NULL_HANDLE);

   VkDeviceQueueCreateInfo queueInfo = { 0 };
   const queue_family_indices queueFamilies = vulkan_find_queue_families(renderer->physical_device, renderer->surface);
   post(queueFamilies.valid);
   queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
   queueInfo.queueFamilyIndex = queueFamilies.graphics_family;
   queueInfo.queueCount = 1;
   const float queuePriorities[] = { 1.0f };
   queueInfo.pQueuePriorities = queuePriorities;

   extensionCount = 0;
   if (!VK_VALID(vkEnumerateDeviceExtensionProperties(renderer->physical_device, 0, &extensionCount, 0)))
      post(0);

   inv(extensionCount > 0);
   extensions = allocate(extensionCount * sizeof(VkExtensionProperties));
   inv(extensions);

   if (!VK_VALID(vkEnumerateDeviceExtensionProperties(renderer->physical_device, 0, &extensionCount, extensions)))
      post(0);

   extensionNames = allocate(extensionCount * sizeof(const char*));
   inv(extensionNames);
   for (size_t i = 0; i < extensionCount; ++i)
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

      VkPhysicalDeviceFeatures physicalFeatures = { 0 };
      physicalFeatures.shaderClipDistance = VK_TRUE;
      info.pEnabledFeatures = &physicalFeatures;

      if (!VK_VALID(vkCreateDevice(renderer->physical_device, &info, 0, &renderer->logical_device)))
         post(0);
   }

   VkSurfaceCapabilitiesKHR surfaceCaps = { 0 };
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->physical_device, renderer->surface, &surfaceCaps);

   VkExtent2D surfaceExtent = surfaceCaps.currentExtent;
   renderer->surface_width = surfaceExtent.width;
   renderer->surface_height = surfaceExtent.height;

   {
      VkSwapchainCreateInfoKHR info =
      {
       .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
       .surface = renderer->surface,
       .minImageCount = VULKAN_IMAGE_COUNT,
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
      if (!VK_VALID(vkCreateSwapchainKHR(renderer->logical_device, &info, 0, &renderer->swap_chain)))
         post(0);

      renderer->format = info.imageFormat;
   }

   u32 swap_chain_count = 0;
   if (!VK_VALID(vkGetSwapchainImagesKHR(renderer->logical_device, renderer->swap_chain, &swap_chain_count, 0)))
      post(0);

   if (swap_chain_count != VULKAN_IMAGE_COUNT)
      post(0);

   renderer->swap_chain_count = swap_chain_count;

   VkImage* swapChainImages = allocate(swap_chain_count * sizeof(VkImage));
   if (!swapChainImages)
      post(0);

   if (!VK_VALID(vkGetSwapchainImagesKHR(renderer->logical_device, renderer->swap_chain, &swap_chain_count, swapChainImages)))
      post(0);

   for (u32 i = 0; i < swap_chain_count; ++i)
      renderer->images[i] = swapChainImages[i];

   VkImageView* image_views = allocate(swap_chain_count * sizeof(VkImageView));
   if (!image_views)
      post(0);

   renderer->swap_chain_count = swap_chain_count;
   for (u32 i = 0; i < swap_chain_count; ++i) {
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

      if (!VK_VALID(vkCreateImageView(renderer->logical_device, &info, 0, &image_views[i])))
         post(0);
      renderer->image_views[i] = image_views[i];
   }

   u32 queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queueFamilyCount, 0);
   VkQueueFamilyProperties* queueProperties = allocate(queueFamilyCount * sizeof(VkQueueFamilyProperties));
   if (!queueProperties)
      post(0);
   post(queueFamilyCount > 0);
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
      if (!VK_VALID(vkCreateCommandPool(renderer->logical_device, &info, 0, &commandPool)))
         post(0);
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
      if (!VK_VALID(vkAllocateCommandBuffers(renderer->logical_device, &info, &draw_cmd_buffer)))
         post(0);
      renderer->draw_cmd_buffer = draw_cmd_buffer;
   }

   VkRenderPass render_pass = 0;

   VkAttachmentDescription pass[1] = { 0 };
   pass[0].format = VK_FORMAT_B8G8R8A8_SRGB;
   pass[0].samples = VK_SAMPLE_COUNT_1_BIT;
   pass[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   pass[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   pass[0].stencilLoadOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   pass[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   pass[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   pass[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

   VkAttachmentReference ref = { 0 };
   ref.attachment = 0;
   ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   VkSubpassDescription subpass = { 0 };
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
      if (!VK_VALID(vkCreateRenderPass(renderer->logical_device, &info, 0, &render_pass)))
         post(0);
      renderer->render_pass = render_pass;
   }

   VkImageView frameBufferAttachments = 0;
   VkFramebuffer* frame_buffers = allocate(swap_chain_count * sizeof(VkFramebuffer));
   if (!frame_buffers)
      post(0);
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
      for (u32 i = 0; i < swap_chain_count; ++i) {
         frameBufferAttachments = image_views[i];
         if (!VK_VALID(vkCreateFramebuffer(renderer->logical_device, &info, 0, &frame_buffers[i])))
            post(0);
         renderer->frame_buffers[i] = frame_buffers[i];
      }
   }

   vulkan_create_sync_objects(renderer);

   post(renderer->instance);
   post(renderer->surface);
   post(renderer->physical_device);
   post(renderer->logical_device);
   post(renderer->swap_chain);
   post(renderer->swap_chain_count == VULKAN_IMAGE_COUNT);
   post(renderer->render_pass);
   post(renderer->frame_buffers);
   post(renderer->queue);
   post(renderer->draw_cmd_buffer);
   post(renderer->format);
   post(renderer->images);

   post(renderer->fence_frame);
   post(renderer->semaphore_image_available);
   post(renderer->semaphore_render_finished);
}

// TODO: Use this
static void vulkan_record_command_buffer(vulkan_renderer* renderer, u32 imageIndex)
{
   pre(renderer);
}

static void vulkan_render(vulkan_renderer* renderer)
{
   pre(renderer);
   if (!VK_VALID(vkWaitForFences(renderer->logical_device, 1, &renderer->fence_frame, VK_TRUE, UINT64_MAX)))
      post(0);

   if (!VK_VALID(vkResetFences(renderer->logical_device, 1, &renderer->fence_frame)))
      post(0);

   u32 nextImageIndex = 0;
   if (!VK_VALID(vkAcquireNextImageKHR(renderer->logical_device, renderer->swap_chain, UINT64_MAX, renderer->semaphore_image_available, VK_NULL_HANDLE, &nextImageIndex)))
      ;

   if (!VK_VALID(vkResetCommandBuffer(renderer->draw_cmd_buffer, 0)))
      post(0);

   {
      VkCommandBufferBeginInfo info =
      {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          //.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
          .flags = 0, .pInheritanceInfo = 0, .pNext = 0,
      };
      // start this cmd buffer
      if (!VK_VALID(vkBeginCommandBuffer(renderer->draw_cmd_buffer, &info)))
         post(0);
   }
   {
      // temporary float that oscilates between 0 and 1
      // to gradually change the color on the screen
      static float aa = 0.0f;
      // slowly increment
      aa += 0.011f;
      // when value reaches 1.0 reset to 0
      if (aa >= 1.0) aa = 0;
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
      VkOffset2D a = { 0, 0 };
      VkExtent2D b = { renderer->surface_width, renderer->surface_height };
      VkRect2D c = { a,b };
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
   if (!VK_VALID(vkEndCommandBuffer(renderer->draw_cmd_buffer)))
      post(0);

   {
      const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
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

      if (!VK_VALID(vkQueueSubmit(renderer->queue, 1, &info, renderer->fence_frame)))
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
      if (!VK_VALID(vkQueuePresentKHR(renderer->queue, &info)))
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
   if (!VK_VALID(vkWaitForFences(renderer->logical_device, 1, &renderer->fence_frame, VK_TRUE, UINT64_MAX)))
      post(0);

   if (!VK_VALID(vkResetFences(renderer->logical_device, 1, &renderer->fence_frame)))
      post(0);

   u32 nextImageIndex = 0;
   if (!VK_VALID(vkAcquireNextImageKHR(renderer->logical_device, renderer->swap_chain, UINT64_MAX, renderer->semaphore_image_available, VK_NULL_HANDLE, &nextImageIndex)))
      ;

   if (!VK_VALID(vkResetCommandBuffer(renderer->draw_cmd_buffer, 0)))
      post(0);

   {
      VkCommandBufferBeginInfo info =
      {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          //.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
          .flags = 0, .pInheritanceInfo = 0, .pNext = 0,
      };
      // start this cmd buffer
      if (!VK_VALID(vkBeginCommandBuffer(renderer->draw_cmd_buffer, &info)))
         post(0);
   }
   {
      // temporary float that oscilates between 0 and 1
      // to gradually change the color on the screen
      static float aa = 0.0f;
      // slowly increment
      aa += 0.011f;
      // when value reaches 1.0 reset to 0
      if (aa >= 1.0) aa = 0;
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
      VkOffset2D a = { 0, 0 };
      VkExtent2D b = { renderer->surface_width, renderer->surface_height };
      VkRect2D c = { a,b };
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
   if (!VK_VALID(vkEndCommandBuffer(renderer->draw_cmd_buffer)))
      post(0);

   {
      const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
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

      if (!VK_VALID(vkQueueSubmit(renderer->queue, 1, &info, renderer->fence_frame)))
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
      if (!VK_VALID(vkQueuePresentKHR(renderer->queue, &info)))
         ;
   }
}

void vulkan_initialize(hw* hw)
{
   pre(hw->renderer.window.handle);

   vulkan_renderer* renderer = hw_arena_push_struct(&hw->memory, vulkan_renderer);

   vulkan_create_renderer(renderer, hw->renderer.window.handle);

   hw->renderer.backends[vulkan_renderer_index] = renderer;
   hw->renderer.frame_present = vulkan_present;
   hw->renderer.renderer_index = vulkan_renderer_index;

   post(hw->renderer.backends[vulkan_renderer_index]);
   post(hw->renderer.frame_present);
   post(hw->renderer.renderer_index == vulkan_renderer_index);
}
