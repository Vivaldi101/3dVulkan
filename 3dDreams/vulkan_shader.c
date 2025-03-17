#include "common.h"
#include "vulkan.h"

// This must match what is in the shader_build.bat file
#define BUILTIN_SHADER_NAME "Builtin.ObjectShader"

static bool vulkan_shader_create(vulkan_context* context)
{
   char* shader_type_names[OBJECT_SHADER_COUNT] = {"vert", "frag"};
   VkShaderStageFlagBits shader_type_bits[OBJECT_SHADER_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

   for(u32 i = 0; i < OBJECT_SHADER_COUNT; ++i)
   {
   }

   return true;
}

static void vulkan_shader_use(vulkan_context* context)
{
}