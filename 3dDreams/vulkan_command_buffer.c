#include "common.h"
#include "vulkan.h"

static b32 vulkan_command_buffer_allocate_primary(vulkan_context* context, VkCommandPool pool)
{
   VkCommandBufferAllocateInfo buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
   buffer_info.commandPool = pool;
   buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   buffer_info.commandBufferCount = 1;

   if(!VK_VALID(vkAllocateCommandBuffers(context->device.logical_device, &buffer_info, &context->graphics_command_buffers->handle)))
      return false;

   context->graphics_command_buffers->state = COMMAND_BUFFER_READY;
   return true;
}

static b32 vulkan_command_buffer_allocate_secondary(vulkan_context* context, VkCommandPool pool)
{
   VkCommandBufferAllocateInfo buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
   buffer_info.commandPool = pool;
   buffer_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
   buffer_info.commandBufferCount = 1;
}

// TODO: aggreate bools into bit flags
static b32 vulkan_command_buffer_begin(vulkan_command_buffer* command_buffer, b32 single_use, b32 renderpass_continue, b32 parallel_use)
{
   VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

   if(single_use)
      begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
   if(renderpass_continue)
      begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
   if(parallel_use)
      begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

   if(!VK_VALID(vkBeginCommandBuffer(command_buffer->handle, &begin_info)))
      return false;

   command_buffer->state = COMMAND_BUFFER_BEGIN_RECORDING;
   return true;
}

static b32 vulkan_command_buffer_end(vulkan_command_buffer* command_buffer)
{
   assert(command_buffer->state == COMMAND_BUFFER_BEGIN_RECORDING);

   if(!VK_VALID(vkEndCommandBuffer(command_buffer->handle)))
      return false;

   command_buffer->state = COMMAND_BUFFER_END_RECORDING;
   return true;
}

static void vulkan_command_buffer_submit(vulkan_command_buffer* command_buffer)
{
   command_buffer->state = COMMAND_BUFFER_SUBMITTED;
}

static void vulkan_command_buffer_reset(vulkan_command_buffer* command_buffer)
{
   command_buffer->state = COMMAND_BUFFER_READY;
}

static void vulkan_command_buffer_allocate_and_begin_single_use(vulkan_context* context, VkCommandPool pool)
{
   vulkan_command_buffer_allocate_primary(context, pool);
   vulkan_command_buffer_begin(context->graphics_command_buffers, true, false, false);
}

static void vulkan_command_buffer_free(vulkan_context* context, vulkan_command_buffer* buffer, VkCommandPool pool)
{
   vkFreeCommandBuffers(context->device.logical_device, pool, 1, &buffer->handle);
   context->graphics_command_buffers->state = COMMAND_BUFFER_NOT_ALLOCATED;
}

static b32 vulkan_command_buffer_allocate_end_single_use(vulkan_context* context, vulkan_command_buffer* command_buffer, VkCommandPool pool, VkQueue queue)
{
   vulkan_command_buffer_end(command_buffer);

   VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &command_buffer->handle;

   if(!VK_VALID(vkQueueSubmit(queue, 1, &submit_info, 0)))
      return false;

   if(!VK_VALID(vkQueueWaitIdle(queue)))
      return false;

   vulkan_command_buffer_free(context, command_buffer, pool);

   return true;
}
