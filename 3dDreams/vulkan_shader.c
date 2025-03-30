#include "common.h"
#include "vulkan.h"

// This must match what is in the shader_build.bat file
#define BUILTIN_SHADER_NAME "Builtin.ObjectShader"

// Reads the spv files
static file_result vulkan_shader_spv_read(vulkan_context* context, const char* shader_dir, VkShaderStageFlagBits type)
{
   char* type_name;
   char shader_name[MAX_PATH];

   switch(type)
   {
      default:
         type_name = "invalid";
      case VK_SHADER_STAGE_VERTEX_BIT:
         type_name = "vert";
         break;
      case VK_SHADER_STAGE_FRAGMENT_BIT:
         type_name = "frag";
         break;
   }

   wsprintf(shader_name, shader_dir, array_count(shader_name));
   wsprintf(shader_name, "%sbin\\assets\\shaders\\%s.%s.spv", shader_dir, BUILTIN_SHADER_NAME, type_name);

   return win32_file_read(context->storage, shader_name);
}

static file_result vulkan_shader_directory(arena* storage)
{
   file_result result = {};

   if(arena_size(storage) < MAX_PATH)
      return (file_result){0};

   char* file_buffer = new(storage, char, MAX_PATH);
   GetCurrentDirectory(MAX_PATH, file_buffer);

   result.data = file_buffer;
   result.file_size = strlen(file_buffer);

   u32 count = 0;
   for(size i = result.file_size-1; i-- >= 0;)
   {
      if(result.data[i] == '\\')
         ++count;
      if(count == 2)
      {
         result.data[i+1] = 0;
         break;
      }
   }

   result.file_size = strlen((const char*)result.data);

   return result;
}

static bool vulkan_shader_create(arena scratch, vulkan_context* context)
{
   VkShaderStageFlagBits shader_type_bits[OBJECT_SHADER_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

   file_result shader_dir = vulkan_shader_directory(&scratch);

   for(u32 i = 0; i < OBJECT_SHADER_COUNT; ++i)
   {
      file_result shader_file = vulkan_shader_spv_read(context, shader_dir.data, shader_type_bits[i]);
      if(shader_file.file_size == 0)
         return false;

      context->shader.stages[i].module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      context->shader.stages[i].module_create_info.codeSize = shader_file.file_size;
      context->shader.stages[i].module_create_info.pCode = (u32*)shader_file.data;

      if(!vk_valid(vkCreateShaderModule(context->device.logical_device, 
                           &context->shader.stages[i].module_create_info, 
                           context->allocator,
                           &context->shader.stages[i].handle)))
         return false;

      context->shader.stages[i].pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      context->shader.stages[i].pipeline_create_info.stage = shader_type_bits[i];
      context->shader.stages[i].pipeline_create_info.module = context->shader.stages[i].handle;
      context->shader.stages[i].pipeline_create_info.pName = "main";
   }

   // Descriptors for uniform object buffers
   VkDescriptorSetLayoutBinding global_ubo_binding;
   global_ubo_binding.binding = 0;
   global_ubo_binding.descriptorCount = 1;
   global_ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   global_ubo_binding.pImmutableSamplers = 0;
   global_ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

   VkDescriptorSetLayoutCreateInfo global_ubo_layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
   global_ubo_layout_info.bindingCount = 1;
   global_ubo_layout_info.pBindings = &global_ubo_binding;

   if(!vk_valid(vkCreateDescriptorSetLayout(context->device.logical_device, &global_ubo_layout_info, context->allocator, &context->shader.global_descriptor_set_layout)))
      return false;

   VkDescriptorPoolSize global_pool_size;
   global_pool_size.type = global_ubo_binding.descriptorType;
   global_pool_size.descriptorCount = 1;

   for(u32 i = 0; i < context->swapchain.image_count; ++i)
   {
      VkDescriptorPoolCreateInfo global_pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
      global_pool_info.poolSizeCount = 1;
      global_pool_info.pPoolSizes = &global_pool_size;
      global_pool_info.maxSets = 1;
      if(!vk_valid(vkCreateDescriptorPool(context->device.logical_device, &global_pool_info, context->allocator, context->shader.global_descriptor_pools + i)))
         return false;
   }

   if(!vulkan_pipeline_create(context))
      return false;

   context->shader.global_uniform_buffer.total_size = sizeof(global_uniform_object);
   context->shader.global_uniform_buffer.usage_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
   context->shader.global_uniform_buffer.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
   context->shader.global_uniform_buffer.bind_on_create = true;

   if(!vulkan_buffer_create(context, &context->shader.global_uniform_buffer))
      return false;

   for(u32 i = 0; i < context->swapchain.image_count; ++i)
   {
      VkDescriptorSetAllocateInfo set_allocate_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
      set_allocate_info.descriptorPool = context->shader.global_descriptor_pools[i];
      set_allocate_info.descriptorSetCount = 1;
      set_allocate_info.pSetLayouts = &context->shader.global_descriptor_set_layout;

      if(!vk_valid(vkAllocateDescriptorSets(context->device.logical_device, &set_allocate_info, context->shader.global_descriptor_set + i)))
         return false;
   }

   return true;
}

static void vulkan_shader_pipeline_bind(vulkan_context* context)
{
   u32 index = context->current_image_index;
   vulkan_pipeline_bind(context->graphics_command_buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline.handle);
}

static bool vulkan_shader_update_state(vulkan_context* context, u32 global_descriptor_set_index)
{
   u32 image_index = context->current_image_index;
   VkCommandBuffer buffer = context->graphics_command_buffers[image_index];
   VkDescriptorSet* desc_set = &context->shader.global_descriptor_set[image_index];

   u32 range = sizeof(global_uniform_object);
   u64 offset = 0;

   if(!vulkan_buffer_load(context, &context->shader.global_uniform_buffer, 0, range, &context->shader.global_ubo))
      return false;

   VkDescriptorBufferInfo buffer_info;
   buffer_info.buffer = context->shader.global_uniform_buffer.handle;
   buffer_info.offset = offset;
   buffer_info.range = range;

   VkWriteDescriptorSet write_desc_set = {};

   write_desc_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   write_desc_set.dstSet = *desc_set;
   write_desc_set.dstBinding = 0;
   write_desc_set.dstArrayElement = 0;
   write_desc_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   write_desc_set.descriptorCount = 1;
   write_desc_set.pBufferInfo = &buffer_info;

   vkUpdateDescriptorSets(context->device.logical_device, 1, &write_desc_set, 0, 0);

   vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline.layout, global_descriptor_set_index, 1, desc_set, 0, 0);

   return true;
}