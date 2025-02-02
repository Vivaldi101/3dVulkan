#include "hw_arena.h"
#include "common.h"
#include "vulkan.h"
#include "malloc.h"
#include <vulkan/vulkan.h>

#pragma comment(lib,	"vulkan-1.lib")

// TODO: this is dumb - we need to query the swapchain count from vulkan
#define VULKAN_IMAGE_COUNT 3		
// TODO: rename these to snake
typedef struct vulkan_renderer
{
   VkInstance instance;
   VkSurfaceKHR surface;
   VkPhysicalDevice physicalDevice;
   VkDevice logicalDevice;
   VkSwapchainKHR swapChain;
   VkCommandBuffer drawCmdBuffer;
   VkRenderPass renderPass;
   VkFramebuffer frameBuffers[VULKAN_IMAGE_COUNT];
   VkImage images[VULKAN_IMAGE_COUNT];
   VkImageView imageViews[VULKAN_IMAGE_COUNT];
   VkQueue queue;
   VkFormat format;
   VkSemaphore semaphoreImageAvailable;
   VkSemaphore semaphoreRenderFinished;
   VkFence fenceFrame;
   u32    surfaceWidth;
   u32    surfaceHeight;
   u32    swapChainCount;
} vulkan_renderer;

typedef struct queue_family_indices
{
   u32 graphics_family;
   u32 present_family;
   bool valid;
} queue_family_indices;

// Function to dynamically load vkCreateDebugUtilsMessengerEXT
static VkResult VulkanCreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
   PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
   if (!func)
      return VK_ERROR_EXTENSION_NOT_PRESENT;
   return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

static void DebugMessage(const char* format, ...)
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

   DebugMessage("Validation layer: %s\n", data->pMessage);

   return VK_FALSE;
}

static bool VulkanAreExtensionsSupported(VkPhysicalDevice device)
{
   return true;
}

// TODO: This allocates from the subarena
static void* Allocate(size_t size)
{
   return _malloca(size);
}

static queue_family_indices VulkanFindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
   queue_family_indices indices = {};

   uint32_t queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, 0);

   VkQueueFamilyProperties* queueFamilies = Allocate(queueFamilyCount * sizeof(VkQueueFamilyProperties));
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

static bool VulkanIsDeviceCompatible(VkPhysicalDevice device, VkSurfaceKHR surface)
{
   const QueueFamilyIndices result = VulkanFindQueueFamilies(device, surface);
   return result.isValid && VulkanAreExtensionsSupported(device);
}

static void VulkanCreateSyncObjects(VulkanContext* context)
{
   Pre(context);
   VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
   VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };

   if (!VK_VALID(vkCreateSemaphore(context->logicalDevice, &semaphoreInfo, 0, &context->semaphoreImageAvailable)))
      Post(0);
   if (!VK_VALID(vkCreateSemaphore(context->logicalDevice, &semaphoreInfo, 0, &context->semaphoreRenderFinished)))
      Post(0);
   if (!VK_VALID(vkCreateFence(context->logicalDevice, &fenceInfo, 0, &context->fenceFrame)))
      Post(0);
}

static VulkanContext VulkanInitContext(HWND windowHandle)
{
   VulkanContext result = {};
   u32 extensionCount = 0;
   if (!VK_VALID(vkEnumerateInstanceExtensionProperties(0, &extensionCount, 0)))
      Post(0);

   Invariant(extensionCount > 0);
   VkExtensionProperties* extensions = Allocate(extensionCount * sizeof(VkExtensionProperties));
   Invariant(extensions);

   if (!VK_VALID(vkEnumerateInstanceExtensionProperties(0, &extensionCount, extensions)))
      Post(0);

   const char** extensionNames = Allocate(extensionCount * sizeof(const char*));
   Invariant(extensionNames);
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
   instanceInfo.enabledLayerCount = ArrayCount(validationLayers);
   instanceInfo.ppEnabledLayerNames = validationLayers;
   instanceInfo.enabledExtensionCount = extensionCount;
   instanceInfo.ppEnabledExtensionNames = extensionNames;

   if (!VK_VALID(vkCreateInstance(&instanceInfo, 0, &result.instance)))
      Post(0);

   VkDebugUtilsMessengerEXT debugMessenger;
   VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { 0 };
   debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   debugCreateInfo.pfnUserCallback = VulkanDebugCallback;

   if (!VK_VALID(VulkanCreateDebugUtilsMessengerEXT(result.instance, &debugCreateInfo, 0, &debugMessenger)))
      Post(0);

   bool isWin32Surface = false;
   for (size_t i = 0; i < extensionCount; ++i)
      if (strcmp(extensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
         isWin32Surface = true;
         break;
      }

   if (!isWin32Surface)
      Post(0);

   PFN_vkCreateWin32SurfaceKHR vkWin32SurfaceFunction = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(result.instance, "vkCreateWin32SurfaceKHR");

   if (!vkWin32SurfaceFunction)
      Post(0);

   VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = { 0 };
   win32SurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
   win32SurfaceInfo.hinstance = GetModuleHandleA(0);
   win32SurfaceInfo.hwnd = windowHandle;

   vkWin32SurfaceFunction(result.instance, &win32SurfaceInfo, 0, &result.surface);

   u32 deviceCount = 0;
   if (!VK_VALID(vkEnumeratePhysicalDevices(result.instance, &deviceCount, 0)))
      Post(0);
   Post(deviceCount > 0);   // Just use the first device for now

   VkPhysicalDevice* devices = Allocate(deviceCount * sizeof(VkPhysicalDevice));
   if (!devices)
      Post(0);

   if (!VK_VALID(vkEnumeratePhysicalDevices(result.instance, &deviceCount, devices)))
      Post(0);

   for (u32 i = 0; i < deviceCount; ++i)
      if (VulkanIsDeviceCompatible(devices[i], result.surface))
      {
         result.physicalDevice = devices[i];
         break;
      }

   Post(result.physicalDevice != VK_NULL_HANDLE);

   VkDeviceQueueCreateInfo queueInfo = { 0 };
   const QueueFamilyIndices queueFamilies = VulkanFindQueueFamilies(result.physicalDevice, result.surface);
   Post(queueFamilies.isValid);
   queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
   queueInfo.queueFamilyIndex = queueFamilies.graphicsFamily;
   queueInfo.queueCount = 1;
   const float queuePriorities[] = { 1.0f };
   queueInfo.pQueuePriorities = queuePriorities;

   extensionCount = 0;
   if (!VK_VALID(vkEnumerateDeviceExtensionProperties(result.physicalDevice, 0, &extensionCount, 0)))
      Post(0);

   Invariant(extensionCount > 0);
   extensions = Allocate(extensionCount * sizeof(VkExtensionProperties));
   Invariant(extensions);

   if (!VK_VALID(vkEnumerateDeviceExtensionProperties(result.physicalDevice, 0, &extensionCount, extensions)))
      Post(0);

   extensionNames = Allocate(extensionCount * sizeof(const char*));
   Invariant(extensionNames);
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

      if (!VK_VALID(vkCreateDevice(result.physicalDevice, &info, 0, &result.logicalDevice)))
         Post(0);
   }

   VkSurfaceCapabilitiesKHR surfaceCaps = { 0 };
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(result.physicalDevice, result.surface, &surfaceCaps);

   VkExtent2D surfaceExtent = surfaceCaps.currentExtent;
   result.surfaceWidth = surfaceExtent.width;
   result.surfaceHeight = surfaceExtent.height;

   {
      VkSwapchainCreateInfoKHR info =
      {
       .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
       .surface = result.surface,
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
          .pQueueFamilyIndices = &queueFamilies.graphicsFamily,
      };
      if (!VK_VALID(vkCreateSwapchainKHR(result.logicalDevice, &info, 0, &result.swapChain)))
         Post(0);

      result.format = info.imageFormat;
   }

   u32 swapChainCount = 0;
   if (!VK_VALID(vkGetSwapchainImagesKHR(result.logicalDevice, result.swapChain, &swapChainCount, 0)))
      Post(0);

   if (swapChainCount != VULKAN_IMAGE_COUNT)
      Post(0);

   result.swapChainCount = swapChainCount;

   VkImage* swapChainImages = Allocate(swapChainCount * sizeof(VkImage));
   if (!swapChainImages)
      Post(0);

   if (!VK_VALID(vkGetSwapchainImagesKHR(result.logicalDevice, result.swapChain, &swapChainCount, swapChainImages)))
      Post(0);

   for (u32 i = 0; i < swapChainCount; ++i)
      result.images[i] = swapChainImages[i];

   VkImageView* imageViews = Allocate(swapChainCount * sizeof(VkImageView));
   if (!imageViews)
      Post(0);

   result.swapChainCount = swapChainCount;
   for (u32 i = 0; i < swapChainCount; ++i) {
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

      if (!VK_VALID(vkCreateImageView(result.logicalDevice, &info, 0, &imageViews[i])))
         Post(0);
      result.imageViews[i] = imageViews[i];
   }

   u32 queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(result.physicalDevice, &queueFamilyCount, 0);
   VkQueueFamilyProperties* queueProperties = Allocate(queueFamilyCount * sizeof(VkQueueFamilyProperties));
   if (!queueProperties)
      Post(0);
   Post(queueFamilyCount > 0);
   vkGetPhysicalDeviceQueueFamilyProperties(result.physicalDevice, &queueFamilyCount, queueProperties);

   vkGetDeviceQueue(result.logicalDevice, queueFamilies.presentFamily, 0, &result.queue);

   // TODO: Use the above similar to these
   // TODO: Wrap these
   VkCommandPool commandPool = 0;
   {
      VkCommandPoolCreateInfo info =
      {
          .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
          .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
          .pNext = 0,
          .queueFamilyIndex = queueFamilies.graphicsFamily,
      };
      if (!VK_VALID(vkCreateCommandPool(result.logicalDevice, &info, 0, &commandPool)))
         Post(0);
   }

   VkCommandBuffer drawCmdBuffer = 0;
   {
      VkCommandBufferAllocateInfo info =
      {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .commandPool = commandPool,
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = 1,
      };
      if (!VK_VALID(vkAllocateCommandBuffers(result.logicalDevice, &info, &drawCmdBuffer)))
         Post(0);
      result.drawCmdBuffer = drawCmdBuffer;
   }

   VkRenderPass renderPass = 0;

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
      if (!VK_VALID(vkCreateRenderPass(result.logicalDevice, &info, 0, &renderPass)))
         Post(0);
      result.renderPass = renderPass;
   }

   VkImageView frameBufferAttachments = 0;
   VkFramebuffer* frameBuffers = Allocate(swapChainCount * sizeof(VkFramebuffer));
   if (!frameBuffers)
      Post(0);
   {
      VkFramebufferCreateInfo info =
      {
       .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .attachmentCount = 1,
          .pAttachments = &frameBufferAttachments,
          .width = result.surfaceWidth,
          .height = result.surfaceHeight,
          .layers = 1,
          .renderPass = renderPass,
      };
      for (u32 i = 0; i < swapChainCount; ++i) {
         frameBufferAttachments = imageViews[i];
         if (!VK_VALID(vkCreateFramebuffer(result.logicalDevice, &info, 0, &frameBuffers[i])))
            Post(0);
         result.frameBuffers[i] = frameBuffers[i];
      }
   }

   VulkanCreateSyncObjects(&result);

   // post conditions for the context
   Post(result.instance);
   Post(result.surface);
   Post(result.physicalDevice);
   Post(result.logicalDevice);
   Post(result.swapChain);
   Post(result.swapChainCount == VULKAN_IMAGE_COUNT);
   Post(result.renderPass);
   Post(result.frameBuffers);
   Post(result.queue);
   Post(result.drawCmdBuffer);
   Post(result.format);
   Post(result.images);

   Post(result.fenceFrame);
   Post(result.semaphoreImageAvailable);
   Post(result.semaphoreRenderFinished);

   return result;
}

// TODO: Use this
static void VulkanRecordCommandBuffer(VulkanContext* context, u32 imageIndex)
{
   Pre(context);
}

static void VulkanRender(VulkanContext* context)
{
   Pre(context);
   if (!VK_VALID(vkWaitForFences(context->logicalDevice, 1, &context->fenceFrame, VK_TRUE, UINT64_MAX)))
      Post(0);

   if (!VK_VALID(vkResetFences(context->logicalDevice, 1, &context->fenceFrame)))
      Post(0);

   u32 nextImageIndex = 0;
   if (!VK_VALID(vkAcquireNextImageKHR(context->logicalDevice, context->swapChain, UINT64_MAX, context->semaphoreImageAvailable, VK_NULL_HANDLE, &nextImageIndex)))
      ;

   if (!VK_VALID(vkResetCommandBuffer(context->drawCmdBuffer, 0)))
      Post(0);

   {
      VkCommandBufferBeginInfo info =
      {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          //.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
          .flags = 0, .pInheritanceInfo = 0, .pNext = 0,
      };
      // start this cmd buffer
      if (!VK_VALID(vkBeginCommandBuffer(context->drawCmdBuffer, &info)))
         Post(0);
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
          .renderPass = context->renderPass,
          .framebuffer = context->frameBuffers[nextImageIndex],
          .renderArea.offset = {0,0},
          .renderArea.extent = {context->surfaceWidth, context->surfaceHeight},
      };
      VkOffset2D a = { 0, 0 };
      VkExtent2D b = { context->surfaceWidth, context->surfaceHeight };
      VkRect2D c = { a,b };
      info.renderArea = c;
      info.clearValueCount = 2;
      info.pClearValues = clearValue;

      vkCmdBeginRenderPass(context->drawCmdBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
      {
         // draw cmds
      }
      vkCmdEndRenderPass(context->drawCmdBuffer);
   }

   // end this cmd buffer
   if (!VK_VALID(vkEndCommandBuffer(context->drawCmdBuffer)))
      Post(0);

   {
      const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
      VkSubmitInfo info =
      {
       .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &context->semaphoreImageAvailable,
          .pWaitDstStageMask = waitStages,
          .commandBufferCount = 1,
          .pCommandBuffers = &context->drawCmdBuffer,
          .signalSemaphoreCount = 1,
          .pSignalSemaphores = &context->semaphoreRenderFinished,
      };

      if (!VK_VALID(vkQueueSubmit(context->queue, 1, &info, context->fenceFrame)))
         Post(0);
   }
   // present the image on the screen (flip the swap-chain image)
   {
      VkPresentInfoKHR info =
      {
          .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
          .pNext = NULL,
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &context->semaphoreRenderFinished,
          .swapchainCount = 1,
          .pSwapchains = &context->swapChain,
          .pImageIndices = &nextImageIndex,
          .pResults = NULL,
      };
      if (!VK_VALID(vkQueuePresentKHR(context->queue, &info)))
         ;
   }
}

static void VulkanReset(VulkanContext* context)
{
   Pre(context);
   Pre(context->instance);
   // TODO: Clear all teh resources like vulkan image views framebuffers, fences and semaphores etc.
   vkDestroyInstance(context->instance, 0);
}

void vulkan_present(vulkan_renderer* renderer)
{
}

void vulkan_initialize(hw* hw)
{
   pre(hw->renderer.window.handle);

   vulkan_renderer* renderer = hw_arena_push_struct(&hw->memory, vulkan_renderer);

   hw->renderer.backends[vulkan_renderer_index] = renderer;
   hw->renderer.frame_present = vulkan_present;
   hw->renderer.renderer_index = vulkan_renderer_index;

   post(hw->renderer.backends[vulkan_renderer_index]);
   post(hw->renderer.frame_present);
   post(hw->renderer.renderer_index == vulkan_renderer_index);
}
