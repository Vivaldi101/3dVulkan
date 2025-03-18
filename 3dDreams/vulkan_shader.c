#include "common.h"
#include "vulkan.h"

// This must match what is in the shader_build.bat file
#define BUILTIN_SHADER_NAME "Builtin.ObjectShader"

// TODO: Format file names
// TODO: ATM expects shaders located in current working dir
static file_result vulkan_shader_read_file(vulkan_context* context, const char* shader_name, VkShaderStageFlagBits stage)
{
   return win32_file_read(context->storage, shader_name);
}

static file_result vulkan_shader_directory()
{
   file_result result = {};
   char shader_dir_buffer[MAX_PATH];
   GetCurrentDirectory(MAX_PATH, shader_dir_buffer);

   result.data = (byte*)shader_dir_buffer;
   result.file_size = strlen(shader_dir_buffer);

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

static bool vulkan_shader_create(vulkan_context* context)
{
   VkShaderStageFlagBits shader_type_bits[OBJECT_SHADER_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

   file_result shader_dir = vulkan_shader_directory();

   for(u32 i = 0; i < OBJECT_SHADER_COUNT; ++i)
   {
      file_result shader_file = vulkan_shader_read_file(context, BUILTIN_SHADER_NAME, shader_type_bits[i]);
      if(shader_file.file_size == 0)
         return false;

      context->shader.stages[i].module_create_info.codeSize = shader_file.file_size;
      context->shader.stages[i].module_create_info.pCode = (u32*)shader_file.data;

      if(!VK_VALID(vkCreateShaderModule(context->device.logical_device, 
                           &context->shader.stages[i].module_create_info, 
                           context->allocator,
                           &context->shader.stages[i].handle)))
         return false;

      context->shader.stages[i].pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      context->shader.stages[i].pipeline_create_info.stage = shader_type_bits[i];
      context->shader.stages[i].pipeline_create_info.module = context->shader.stages[i].handle;
      context->shader.stages[i].pipeline_create_info.pName = "main";
   }

   return true;
}

static void vulkan_shader_use(vulkan_context* context)
{
}