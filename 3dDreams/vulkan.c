#include "hw_arena.h"
#include "common.h"
#include "vulkan.h"

typedef struct vulkan_renderer
{
	void* device;
	void* queue;
	void* swap_chain;
	void* command_allocator;
   usize command_list_type;
} vulkan_renderer;

void vulkan_present(vulkan_renderer* renderer)
{
}

void vulkan_initialize(hw* hw)
{
   pre(hw->renderer.window.handle);

   vulkan_renderer* renderer = hw_arena_push_struct(&hw->memory, vulkan_renderer);

   hw->renderer.renderers[vulkan_renderer_index] = renderer;
   hw->renderer.frame_present = vulkan_present;
   hw->renderer.renderer_index = vulkan_renderer_index;

   post(hw->renderer.renderers[vulkan_renderer_index]);
   post(hw->renderer.frame_present);
   post(hw->renderer.renderer_index == vulkan_renderer_index);
}
