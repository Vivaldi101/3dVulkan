#include "common.h"
#include "vulkan.h"

static bool vulkan_command_buffer_allocate_primary(vulkan_context* context, VkCommandBuffer* buffers, VkCommandPool pool, u32 count)
{
   VkCommandBufferAllocateInfo buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
   buffer_info.commandPool = pool;
   buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   buffer_info.commandBufferCount = count;

   if(!VK_VALID(vkAllocateCommandBuffers(context->device.logical_device, &buffer_info, buffers)))
      return false;

   return true;
}

static bool vulkan_command_buffer_allocate_secondary(vulkan_context* context, VkCommandPool pool, u32 count)
{
   VkCommandBufferAllocateInfo buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
   buffer_info.commandPool = pool;
   buffer_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
   buffer_info.commandBufferCount = count;
}

static bool vulkan_command_buffer_begin(VkCommandBuffer command_buffer, bool single_use, bool renderpass_continue, bool parallel_use)
{
   VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

   if(single_use)
      begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
   if(renderpass_continue)
      begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
   if(parallel_use)
      begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

   if(!VK_VALID(vkBeginCommandBuffer(command_buffer, &begin_info)))
      return false;

   return true;
}

static bool vulkan_command_buffer_end(VkCommandBuffer buffer)
{
   if(!VK_VALID(vkEndCommandBuffer(buffer)))
      return false;

   return true;
}

static void vulkan_command_buffer_allocate_and_begin_single_use(vulkan_context* context, VkCommandBuffer* buffer, VkCommandPool pool)
{
   vulkan_command_buffer_allocate_primary(context, buffer, pool, 1);
   vulkan_command_buffer_begin(*buffer, true, false, false);
}

static void vulkan_command_buffer_free(vulkan_context* context, VkCommandBuffer* buffers, VkCommandPool pool, u32 count)
{
   vkFreeCommandBuffers(context->device.logical_device, pool, count, buffers);
   for(u32 i = 0; i < count; ++i)
      context->command_buffer_state[i] = COMMAND_BUFFER_NOT_ALLOCATED;
}

static bool vulkan_command_buffer_allocate_end_single_use(vulkan_context* context, VkCommandBuffer buffer, VkCommandPool pool, VkQueue queue)
{
   vulkan_command_buffer_end(buffer);

   VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &buffer;

   if(!VK_VALID(vkQueueSubmit(queue, 1, &submit_info, 0)))
      return false;

   if(!VK_VALID(vkQueueWaitIdle(queue)))
      return false;

   vulkan_command_buffer_free(context, &buffer, pool, 1);

   return true;
}
