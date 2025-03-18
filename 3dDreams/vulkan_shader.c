#include "common.h"
#include "vulkan.h"

// This must match what is in the shader_build.bat file
#define BUILTIN_SHADER_NAME "Builtin.ObjectShader"

// TODO: Format file names
// TODO: ATM expects shaders located in current working dir
static file_result vulkan_shader_create_module(vulkan_context* context, const char* shader_name, VkShaderStageFlagBits stage)
{
   const char* shader_type_name;

   switch(stage)
   {
      case VK_SHADER_STAGE_VERTEX_BIT:
         shader_type_name = "vert";
         break;
      case VK_SHADER_STAGE_FRAGMENT_BIT:
         shader_type_name = "frag";
         break;

      default:;
   }

   file_result file = win32_file_read(context->storage, shader_name);

   return file;
}

static bool vulkan_shader_create(vulkan_context* context)
{
   VkShaderStageFlagBits shader_type_bits[OBJECT_SHADER_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

   file_result file = {};
   for(u32 i = 0; i < OBJECT_SHADER_COUNT; ++i)
   {
      file = vulkan_shader_create_module(context, BUILTIN_SHADER_NAME, shader_type_bits[i]);
      if(file.file_size == 0)
         return false;

      context->shader.stages[i].module_create_info.codeSize = file.file_size;
      context->shader.stages[i].module_create_info.pCode = (u32*)file.data;

      if(!VK_VALID(vkCreateShaderModule(context->device.logical_device, 
                           &context->shader.stages[i].module_create_info, 
                           context->allocator,
                           &context->shader.stages[i].handle)))
         return false;
   }

   return true;
}

static void vulkan_shader_use(vulkan_context* context)
{
}