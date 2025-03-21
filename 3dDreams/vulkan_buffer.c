#include "vulkan.h"
#include "common.h"

static bool vulkan_buffer_bind(vulkan_context* context, vulkan_buffer* buffer)
{
   return vkBindBufferMemory(context->device.logical_device, buffer->handle, buffer->memory, buffer->offset);
}

static bool vulkan_buffer_create(vulkan_context* context, vulkan_buffer* buffer)
{
   VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
   buffer_info.size = buffer->total_size;        // cannot be zero
   buffer_info.usage = buffer->usage_flags;      // cannot be zero
   buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // single queue only

   if(!VK_VALID(vkCreateBuffer(context->device.logical_device, &buffer_info, context->allocator, &buffer->handle)))
      return false;

   VkMemoryRequirements requirements = {};
   vkGetBufferMemoryRequirements(context->device.logical_device, buffer->handle, &requirements);

   buffer->memory_index = vulkan_find_memory_index(context, requirements.memoryTypeBits, buffer->memory_flags);
   if(buffer->memory_index == -1)
      return false;

   VkMemoryAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
   alloc_info.allocationSize = requirements.size;
   alloc_info.memoryTypeIndex = buffer->memory_index;

   if(!VK_VALID(vkAllocateMemory(context->device.logical_device, &alloc_info, context->allocator, &buffer->memory)))
      return false;

   if(buffer->bind_on_create)
      return VK_VALID(vulkan_buffer_bind(context, buffer));

   return true;
}

static void* vulkan_buffer_lock_memory(vulkan_context* context, vulkan_buffer* buffer, VkMemoryMapFlags flags)
{
   void* memory = 0;
   if(!VK_VALID(vkMapMemory(context->device.logical_device, buffer->memory, buffer->offset, buffer->total_size, flags, &memory)))
      return 0;

   return memory;
}

static void vulkan_buffer_unlock_memory(vulkan_context* context, vulkan_buffer* buffer)
{
   vkUnmapMemory(context->device.logical_device, buffer->memory);
}

static bool vulkan_buffer_copy(vulkan_context* context, VkBuffer dest, VkBuffer source, VkBufferCopy copy_region)
{
   if(!VK_VALID(vkQueueWaitIdle(context->device.graphics_queue)))
      return false;

   VkCommandBuffer temp = 0;
   vulkan_command_buffer_allocate_and_begin_single_use(context, &temp, context->device.graphics_command_pool);

   vkCmdCopyBuffer(temp, source, dest, 1, &copy_region);

   if(!vulkan_command_buffer_allocate_end_single_use(context, temp, context->device.graphics_command_pool, context->device.graphics_queue))
      return false;

   return true;
}

static void vulkan_buffer_load(vulkan_context* context, vulkan_buffer* buffer, VkMemoryMapFlags flags, u64 size, const void* data)
{
   if(size > buffer->total_size)
      return;

   void* memory = vulkan_buffer_lock_memory(context, buffer, flags);
   if(!memory)
      return;

   memcpy(memory, data, size);
   vulkan_buffer_unlock_memory(context, buffer);
}
