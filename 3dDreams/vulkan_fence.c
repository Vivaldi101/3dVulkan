#include "vulkan.h"
#include "common.h"

static bool vulkan_fence_wait(vulkan_context* context, vulkan_fence* fence, u64 timeout)
{
   if(fence->is_signaled)
      return true;

   switch(vkWaitForFences(context->device.logical_device, 1, &fence->handle, true, timeout))
   {
      default:
         return false;
      case VK_SUCCESS:
         fence->is_signaled = true;
         break;
   }

   return true;
}

static bool vulkan_fence_reset(vulkan_context* context, vulkan_fence* fence)
{
   // already reset
   if(!fence->is_signaled)
      return true;

   if(!VK_VALID(vkResetFences(context->device.logical_device, 1, &fence->handle)))
      return false;

   fence->is_signaled = false;
   return true;
}

static bool vulkan_fence_create(vulkan_context* context)
{
   for(u32 i = 0; i < VULKAN_MAX_FRAME_BUFFER_COUNT; ++i)
   {
      VkSemaphoreCreateInfo sema_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
      if(!VK_VALID(vkCreateSemaphore(context->device.logical_device, &sema_info, context->allocator, &context->image_available_semaphores[i])))
         return false;
      if(!VK_VALID(vkCreateSemaphore(context->device.logical_device, &sema_info, context->allocator, &context->queue_complete_semaphores[i])))
         return false;

      VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

      if(!VK_VALID(vkCreateFence(context->device.logical_device, &fence_info, context->allocator, &context->in_flight_fences[i].handle)))
         return false;

      context->in_flight_fences[i].is_signaled = true;
   }

   return true;
}
