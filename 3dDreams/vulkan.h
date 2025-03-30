#if !defined(_VULKAN_H)
#define _VULKAN_H

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif
// other plats for vulkan
#endif

#include "common.h"
#include "shaders.h"
#include <vulkan/vulkan.h>
#include "arena.h"

#pragma comment(lib,	"vulkan-1.lib")

enum { VULKAN_MAX_FRAME_BUFFER_COUNT = 3, OBJECT_SHADER_COUNT = 2 };

#define vk_valid(v) ((v) == VK_SUCCESS)

bool vulkan_initialize(hw* hw);
bool vulkan_deinitialize(hw* hw);

typedef enum vulkan_renderpass_state
{
   RENDERPASS_EMPTY = 0,
   RENDERPASS_READY,
   RENDERPASS_BEGIN_RECORDING,
   RENDERPASS_END_RECORDING,
   RENDERPASS_SUBMITTED,
} vulkan_renderpass_state;

typedef enum vulkan_command_buffer_state
{
   COMMAND_BUFFER_EMPTY = 0,
   COMMAND_BUFFER_READY,
   COMMAND_BUFFER_RENDERPASS,
   COMMAND_BUFFER_BEGIN_RECORDING,
   COMMAND_BUFFER_END_RECORDING,
   COMMAND_BUFFER_SUBMITTED,
   COMMAND_BUFFER_NOT_ALLOCATED,
} vulkan_command_buffer_state;

align_struct vulkan_buffer
{
   u64 total_size;
   u64 offset;
   VkBuffer handle;
   VkBufferUsageFlags usage_flags;
   VkDeviceMemory memory;
   bool bind_on_create;
   i32 memory_index;
   VkMemoryPropertyFlags memory_flags;
} vulkan_buffer;

align_struct vulkan_fence
{
   VkFence handle;
   bool is_signaled;
} vulkan_fence;

align_struct vulkan_viewport
{
   i32 x,y,w,h;
} vulkan_viewport;

align_struct vulkan_renderpass
{
   VkRenderPass handle;
   vulkan_viewport viewport;
   union
   {
      f32 clear_color[4];
      struct { f32 r,g,b,a; };
   };
   f32 depth;
   u32 stencil;

   vulkan_renderpass_state state;
} vulkan_renderpass;

align_struct vulkan_framebuffer
{
   VkFramebuffer handle;
   u32 attachment_count;
   VkImageView* attachments;
   vulkan_renderpass* renderpass;
} vulkan_framebuffer;

align_struct vulkan_image_info
{
   VkImageType type;
   VkFormat format;
   VkImageTiling tiling;
   VkImageUsageFlags usage;
   VkMemoryPropertyFlags memory_flags;
   VkImageAspectFlags aspect_flags;
   bool is_view;
} vulkan_image_info;

align_struct vulkan_image
{
   VkImage handle;
   VkDeviceMemory memory;
   VkImageView view;
   u32 width;
   u32 height;
} vulkan_image;

align_struct vulkan_swapchain_info
{
   VkSurfaceCapabilitiesKHR surface_capabilities;

   u32 surface_format_count;
   VkSurfaceFormatKHR* surface_formats;

   u32 present_mode_count;
	VkPresentModeKHR* present_modes;
} vulkan_swapchain_info;

enum
{
   QUEUE_GRAPHICS_INDEX = 0,
   QUEUE_COMPUTE_INDEX,
   QUEUE_PRESENT_INDEX,
   QUEUE_TRANSFER_INDEX,

   QUEUE_INDEX_COUNT,
};

align_struct vulkan_swapchain
{
   vulkan_framebuffer framebuffers[VULKAN_MAX_FRAME_BUFFER_COUNT];
	VkSurfaceFormatKHR image_format;
   VkPresentModeKHR present_mode;
   VkSwapchainKHR handle;

   u32 max_frames_in_flight_count;
   u32 image_count;

   VkImage* images;
   VkImageView* views;

   vulkan_image depth_attachment;
   u32 attachment_count;

   vulkan_swapchain_info info;
} vulkan_swapchain;

align_struct vulkan_device
{
   VkPhysicalDevice physical_device;
   VkDevice logical_device;

	u32 queue_indexes[QUEUE_INDEX_COUNT];
   u32 queue_family_count;
   VkQueue graphics_queue;
   VkQueue compute_queue;
   VkQueue present_queue;
   VkQueue transfer_queue;

   VkCommandPool graphics_command_pool;

   VkPhysicalDeviceProperties properties;
   VkPhysicalDeviceFeatures features;
   VkPhysicalDeviceMemoryProperties memory;

   VkFormat depth_format;

   bool use_single_family_queue;
} vulkan_device;

align_struct vulkan_shader_stage
{
   VkShaderModuleCreateInfo module_create_info;
   VkPipelineShaderStageCreateInfo pipeline_create_info;
   VkShaderModule handle;
} vulkan_shader_stage;

align_struct vulkan_pipeline
{
	VkPipeline handle;
	VkPipelineLayout layout;
} vulkan_pipeline;

align_struct vulkan_object_shader
{
   vulkan_shader_stage stages[OBJECT_SHADER_COUNT];

   // TODO: compress descriptors
   VkDescriptorPool global_descriptor_pools[VULKAN_MAX_FRAME_BUFFER_COUNT];
   VkDescriptorSet global_descriptor_set[VULKAN_MAX_FRAME_BUFFER_COUNT];
   VkDescriptorSetLayout global_descriptor_set_layout;

   global_uniform_object global_ubo;
   vulkan_buffer global_uniform_buffer;
} vulkan_object_shader;

align_struct vulkan_context
{
   arena* storage;

   vulkan_buffer vertex_buffer;
   vulkan_buffer index_buffer;

   vulkan_pipeline pipeline;
   vulkan_object_shader shader;
   vulkan_device device;
   vulkan_swapchain swapchain;
   vulkan_renderpass main_renderpass;
   vulkan_command_buffer_state command_buffer_state[VULKAN_MAX_FRAME_BUFFER_COUNT];

   VkCommandBuffer graphics_command_buffers[VULKAN_MAX_FRAME_BUFFER_COUNT];

   VkSemaphore image_available_semaphores[VULKAN_MAX_FRAME_BUFFER_COUNT];
   VkSemaphore queue_complete_semaphores[VULKAN_MAX_FRAME_BUFFER_COUNT];

   vulkan_fence in_flight_fences[VULKAN_MAX_FRAME_BUFFER_COUNT];
   vulkan_fence* images_in_flight[VULKAN_MAX_FRAME_BUFFER_COUNT];

   u32 in_flight_fence_count; 

   VkInstance instance;
   VkSurfaceKHR surface;
   VkAllocationCallbacks* allocator;

   u32 framebuffer_width;
   u32 framebuffer_height;
   u64 framebuffer_size_generation;
   u64 framebuffer_size_prev_generation;

   u32 current_frame_index;
   u32 current_image_index;

   bool do_recreate_swapchain;
} vulkan_context;

#endif
