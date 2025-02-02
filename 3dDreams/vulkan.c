#include "hw_arena.h"
#include "common.h"
#include "vulkan.h"
#include <vulkan/vulkan.h>

#pragma comment(lib,	"vulkan-1.lib")

#define VULKAN_IMAGE_COUNT 3		// TODO: This is dumb - we need to query the swapchain count from vulkan
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
