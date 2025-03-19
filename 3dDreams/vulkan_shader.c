#include "common.h"
#include "vulkan.h"

// This must match what is in the shader_build.bat file
#define BUILTIN_SHADER_NAME "Builtin.ObjectShader"

// This should be in a scratch arena
static char global_shader_dir_buffer[MAX_PATH];

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
   wsprintf(shader_name, "%sbin\\assets\\shaders\\%s.%s.spv", shader_dir, BUILTIN_SHADER_NAME, type_name, array_count(shader_name));

   return win32_file_read(context->storage, shader_name);
}

static file_result vulkan_shader_directory()
{
   file_result result = {};
   GetCurrentDirectory(MAX_PATH, global_shader_dir_buffer);

   result.data = global_shader_dir_buffer;
   result.file_size = strlen(global_shader_dir_buffer);

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
      file_result shader_file = vulkan_shader_spv_read(context, shader_dir.data, shader_type_bits[i]);
      if(shader_file.file_size == 0)
         return false;

      context->shader.stages[i].module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
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