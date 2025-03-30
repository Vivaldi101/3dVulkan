#include "arena.h"
#include "common.h"
#include "vulkan.h"

// TODO: pass these as function pointers from the platform
#include "win32_file_io.c"

// unity build
#include "vulkan_common.c"
#include "vulkan_device.c"
#include "vulkan_surface.c"
#include "vulkan_image.c"
#include "vulkan_renderpass.c"
#include "vulkan_framebuffer.c"
#include "vulkan_command_buffer.c"
#include "vulkan_swapchain.c"
#include "vulkan_fence.c"
#include "vulkan_pipeline.c"
#include "vulkan_buffer.c"
#include "vulkan_shader.c"


// Function to dynamically load vkCreateDebugUtilsMessengerEXT
static VkResult vulkan_create_debugutils_messenger_ext(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
   PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
   if(!func)
      return VK_ERROR_EXTENSION_NOT_PRESENT;
   return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* pUserData) {

   debug_message("Validation layer: %s\n", data->pMessage);

   return VK_FALSE;
}

static bool vulkan_command_buffers_create(vulkan_context* context)
{
   pre(context->swapchain.image_count <= VULKAN_MAX_FRAME_BUFFER_COUNT);

   VkCommandBuffer buffers[VULKAN_MAX_FRAME_BUFFER_COUNT];
   if(!vulkan_command_buffer_allocate_primary(context, buffers, context->device.graphics_command_pool, context->swapchain.image_count))
      return false;

   memcpy(context->graphics_command_buffers, buffers, context->swapchain.image_count*sizeof(VkCommandBuffer));

   for(u32 i = 0; i < context->swapchain.image_count; ++i)
      context->command_buffer_state[i] = COMMAND_BUFFER_READY;

   return true;
}

static void vulkan_resize(vulkan_context* context, u32 width, u32 height)
{
   context->framebuffer_width = width;
   context->framebuffer_height = height;
   context->framebuffer_size_generation++;
}

static bool vulkan_buffers_create(vulkan_context* context)
{
   VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
   u64 vertex_buffer_size = MB(64);

   context->vertex_buffer.memory_flags = memory_flags;
   context->vertex_buffer.total_size = vertex_buffer_size;
   context->vertex_buffer.usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT; 
   context->vertex_buffer.bind_on_create = true;

   if(!vulkan_buffer_create(context, &context->vertex_buffer))
      return false;

   u64 index_buffer_size = MB(64);
   context->index_buffer.memory_flags = memory_flags;
   context->index_buffer.total_size = index_buffer_size;
   context->index_buffer.usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT; 
   context->index_buffer.bind_on_create = true;

   if(!vulkan_buffer_create(context, &context->index_buffer))
      return false;

   return true;
}

// TODO: Atm just uses offset of zero
static bool vulkan_upload_data_to_buffer(vulkan_context* context, vulkan_buffer* buffer, u64 data_size, const void* data)
{
   vulkan_buffer staging_buffer = {};

   staging_buffer.usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
   staging_buffer.memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
   staging_buffer.total_size = data_size;
   staging_buffer.bind_on_create = true;
   if(!vulkan_buffer_create(context, &staging_buffer))
      return false;

   vulkan_buffer_load(context, &staging_buffer, 0, data_size, data);

   VkBufferCopy copy_region = {};
   copy_region.srcOffset = 0;
   copy_region.dstOffset = 0;
   copy_region.size = data_size;

   if(!vulkan_buffer_copy(context, buffer->handle, staging_buffer.handle, copy_region))
      return false;

   return true;
}

static bool vulkan_create_renderer(arena scratch, vulkan_context* context, const hw_window* window)
{
   u32 ext_count = 0;
   if(!vk_valid(vkEnumerateInstanceExtensionProperties(0, &ext_count, 0)))
      return false;

   VkExtensionProperties* ext = new(&scratch, VkExtensionProperties, ext_count);
   if(scratch_end(scratch, ext) || !vk_valid(vkEnumerateInstanceExtensionProperties(0, &ext_count, ext)))
      return false;

   VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
   app_info.pApplicationName = "VulkanApp";
   app_info.applicationVersion = VK_API_VERSION_1_0;
   app_info.engineVersion = VK_API_VERSION_1_0;
   app_info.pEngineName = "3dDreams";
   app_info.apiVersion = VK_API_VERSION_1_0;

   VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
   instance_info.pApplicationInfo = &app_info;

#ifdef _DEBUG
   {
      const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
      instance_info.enabledLayerCount = array_count(validationLayers);
      instance_info.ppEnabledLayerNames = validationLayers;
   }
#endif

   const char** ext_names = new(&scratch, const char*, ext_count);
   for(size_t i = 0; i < ext_count; ++i)
      ext_names[i] = ext[i].extensionName;

   instance_info.enabledExtensionCount = ext_count;
   instance_info.ppEnabledExtensionNames = ext_names;

   if(scratch_end(scratch, ext_names) || !vk_valid(vkCreateInstance(&instance_info, 0, &context->instance)))
      return false;

#ifdef _DEBUG 
   {
      VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
      debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
      debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      debugCreateInfo.pfnUserCallback = vulkan_debug_callback;

      VkDebugUtilsMessengerEXT messenger;

      if(!vk_valid(vulkan_create_debugutils_messenger_ext(context->instance, &debugCreateInfo, 0, &messenger)))
         return false;
   }
#endif

   if(!vulkan_window_surface_create(context, window, ext_names, ext_count))
      return false;

   if(!vulkan_device_create(scratch, context))
      return false;

   if(!vulkan_swapchain_create(context))
      return false;

   if(!vulkan_renderpass_create(context))
      return false;

   if(!vulkan_command_buffers_create(context))
      return false;

   if(!vulkan_framebuffer_create(context))
      return false;

   if(!vulkan_fence_create(context))
      return false;

   if(!vulkan_shader_create(scratch, context))
      return false;

   if(!vulkan_buffers_create(context))
      return false;

   scratch_clear(scratch);

   // TODO: test drawing code
   {
      vertex3 verts[4] = {};

      const f32 s = 1.0f*0.5f;

      verts[0].vertex.x = -0.5f*s;
      verts[0].vertex.y = -0.5f*s;

      verts[1].vertex.x = 0.5f*s;
      verts[1].vertex.y = -0.5f*s;

      verts[2].vertex.x = 0.5f*s;
      verts[2].vertex.y = 0.5f*s;

      verts[3].vertex.x = -0.5f*s;
      verts[3].vertex.y = 0.5f*s;

      u32 indexes[6] = {0,1,2, 2,3,0};

      if(!vulkan_upload_data_to_buffer(context, &context->vertex_buffer, sizeof(verts), verts))
         return false;

      if(!vulkan_upload_data_to_buffer(context, &context->index_buffer, sizeof(indexes), indexes))
         return false;
   }

   return true;
}

// TODO: This
static bool vulkan_result(VkResult result)
{
   return result == VK_SUCCESS;
}

static bool vulkan_frame_begin(vulkan_context* context)
{
   if(context->do_recreate_swapchain)
      if(!vulkan_result(vkDeviceWaitIdle(context->device.logical_device)))
         return false;

   if(context->framebuffer_size_generation != context->framebuffer_size_prev_generation)
   {
      if(!vulkan_result(vkDeviceWaitIdle(context->device.logical_device)))
         return false;  // TODO: Diagnostics
      if(!vulkan_swapchain_recreate(context))
         return false;  // TODO: Diagnostics
      return false;
   }

   if(!vulkan_fence_wait(context, &context->in_flight_fences[context->current_frame_index], UINT64_MAX))
      return false;

   if(!vulkan_swapchain_next_image_index(context->storage, context, UINT64_MAX, context->image_available_semaphores[context->current_frame_index], 0))
      return false;

   VkCommandBuffer cmd_buffer = context->graphics_command_buffers[context->current_image_index];
   vulkan_command_buffer_begin(cmd_buffer, false, false, false);

   VkViewport viewport = {};
   viewport.x = 0.0f;
   viewport.y = (f32)context->framebuffer_height-1.0f;
   viewport.width = (f32)context->framebuffer_width;
   viewport.height = -(f32)context->framebuffer_height;
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;

   VkRect2D scissor = {};
   scissor.offset.x = scissor.offset.y = 0;
   scissor.extent.width = context->framebuffer_width;
   scissor.extent.height = context->framebuffer_height;
   context->main_renderpass.viewport.w = (i32)viewport.width;
   context->main_renderpass.viewport.h = -(i32)viewport.height;

   // TODO: test clear screen
   context->main_renderpass.r = 0.0f;
   context->main_renderpass.g = 0.0f;
   context->main_renderpass.b = 0.3333f;
   context->main_renderpass.a = 1.0f;

   vulkan_renderpass_begin(&context->main_renderpass, cmd_buffer, &context->swapchain.framebuffers[context->current_image_index]);
   vulkan_shader_pipeline_bind(context);

   context->command_buffer_state[context->current_image_index] = COMMAND_BUFFER_BEGIN_RECORDING;

   vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
   vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

   return true;
}

static bool vulkan_frame_update_state(vulkan_context* context, mat4 proj, mat4 view)
{
   vulkan_shader_pipeline_bind(context);

   context->shader.global_ubo.proj = proj;
   context->shader.global_ubo.view = view;

   if(!vulkan_shader_update_state(context, 0))
      return false;

   // TODO: test code
   VkDeviceSize offsets[] = {0};
   const VkCommandBuffer cmd_buffer = context->graphics_command_buffers[context->current_image_index];
   vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &context->vertex_buffer.handle, offsets);

   vkCmdBindIndexBuffer(cmd_buffer, context->index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

   vkCmdDrawIndexed(cmd_buffer, 6, 1, 0, 0, 0);

   return true;
}

static bool vulkan_frame_end(vulkan_context* context)
{
   const VkCommandBuffer cmd_buffer = context->graphics_command_buffers[context->current_image_index];

   vulkan_renderpass_end(&context->main_renderpass, cmd_buffer);

   vulkan_command_buffer_end(cmd_buffer);

   if(context->images_in_flight[context->current_image_index] != VK_NULL_HANDLE)
      // current image index is already used by previous frame so wait on the fence
      if(!vulkan_fence_wait(context, context->images_in_flight[context->current_image_index], UINT64_MAX))
         return false;

   // current image index was not used so mark it as fenced
   context->images_in_flight[context->current_image_index] = &context->in_flight_fences[context->current_frame_index];

   // unsignal the current fence so that it can signaled again
   vulkan_fence_reset(context, &context->in_flight_fences[context->current_frame_index]);

   VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &cmd_buffer;
   submit_info.signalSemaphoreCount = 1;
   submit_info.pSignalSemaphores = &context->queue_complete_semaphores[context->current_frame_index];
   submit_info.waitSemaphoreCount = 1;
   submit_info.pWaitSemaphores = &context->image_available_semaphores[context->current_frame_index];

   VkPipelineStageFlags flags[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
   submit_info.pWaitDstStageMask = flags;

   if(!vk_valid(vkQueueSubmit(context->device.graphics_queue, 1, &submit_info, context->in_flight_fences[context->current_frame_index].handle)))
      return false;

   context->command_buffer_state[context->current_frame_index] = COMMAND_BUFFER_SUBMITTED;

   if(!vulkan_swapchain_present(context, context->current_image_index, &context->queue_complete_semaphores[context->current_frame_index]))
      return false;

   return true;
}

bool vulkan_present(vulkan_context* context)
{
   if(!vulkan_frame_begin(context))
      return false;

   // Test projection
   //mat4 proj = mat4_perspective_fov(90.0f, (f32)context->framebuffer_width/context->framebuffer_height, 0.01f, 1000.f);
   //static f32 z = -1.0f;
   //z -= 0.01f;
   //mat4 view = mat4_translate((vec3){0.0f, 0.0f, z});
   if(!vulkan_frame_update_state(context, mat4_identity(), mat4_identity()))
      return false;

   if(!vulkan_frame_end(context))
      return false;

   return true;
}

bool vulkan_initialize(hw* hw)
{
   bool result = true;
   pre(hw->renderer.window.handle);

   vulkan_context* context = new(&hw->vulkan_storage, vulkan_context);
   if(arena_end(&hw->vulkan_storage, context))
		return false;
   context->storage = &hw->vulkan_storage;

   //context->device.use_single_family_queue = true;
   result = vulkan_create_renderer(hw->vulkan_scratch, context, &hw->renderer.window);

   hw->renderer.backends[vulkan_renderer_index] = context;
   hw->renderer.frame_present = vulkan_present;
   hw->renderer.renderer_index = vulkan_renderer_index;
   hw->renderer.frame_resize = vulkan_resize;

   post(hw->renderer.backends[vulkan_renderer_index]);
   post(hw->renderer.frame_present);
   post(hw->renderer.renderer_index == vulkan_renderer_index);
   post(hw->renderer.frame_resize == vulkan_resize);

   return result;
}

bool vulkan_uninitialize(hw* hw)
{
   vulkan_context* context = hw->renderer.backends[vulkan_renderer_index];

   if(!vk_valid(vkDeviceWaitIdle(context->device.logical_device)))
      return false;

   // TODO...

   return true;
}
