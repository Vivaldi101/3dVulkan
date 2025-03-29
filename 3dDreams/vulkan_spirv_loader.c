#include "arena.h"

// This must match what is in the shader_build.bat file
#define BUILTIN_SHADER_NAME "Builtin.ObjectShader"

static file_result vulkan_shader_spv_read(arena* storage, const char* shader_dir, VkShaderStageFlagBits type)
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

   return win32_file_read(storage, shader_name);
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
