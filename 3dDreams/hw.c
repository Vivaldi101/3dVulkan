#include "hw.h"
#include "hw_arena.h"
#include "common.h"
#include "app.h"

#include <stdio.h>

#define MAX_ARGV 32

cache_align typedef struct hw_renderer
{
   void* backends[renderer_count];
   void(*frame_present)(void* renderer);
   void(*frame_wait)(void* renderer);
   u32 renderer_index;	// ZII - change this so that zero is actually the software renderer
   hw_window window;
} hw_renderer;

cache_align typedef struct hw_timer
{
   void(*sleep)(u32 ms);
   u32(*time)();
} hw_timer;

cache_align typedef struct hw
{
   hw_renderer renderer;
   hw_arena main_arena;				// TODO: we need concept of permanent storage here since sub arenas are temp
   hw_arena renderer_arena;				// TODO: we need concept of permanent storage here since sub arenas are temp
   hw_timer timer;
   bool(*platform_loop)();
   bool finished;
} hw;

// Do all renderer includes here?
//#include "d3d12.c"
#include "vulkan.c"

void hw_window_open(hw* hw, const char *title, int x, int y, int width, int height)
{
   hw->renderer.window.handle = hw->renderer.window.open(title, x, y, width, height);
   // TODO: error on failure
}

void hw_window_close(hw* hw)
{
	pre(hw->renderer.window.handle);
   hw->renderer.window.close(hw->renderer.window);
}

void hw_event_loop_end(hw* hw)
{
   if (hw->renderer.window.handle)
      hw_window_close(hw);
}

#define MSEC_PER_SIM (16)

static f32 global_game_time_residual;
static int global_game_frame;

static void hw_frame_sync(hw* hw)
{
	int num_frames_to_run = 0;
   for (;;) 
   {
      const int current_frame_time = hw->timer.time();
      static int last_frame_time = 0;
      if (last_frame_time == 0) 
         last_frame_time = current_frame_time;

      int delta_milli_seconds = current_frame_time - last_frame_time;
      last_frame_time = current_frame_time;

      global_game_time_residual += delta_milli_seconds;

      for (;;) 
      {
         // how much to wait before running the next frame
         if (global_game_time_residual < MSEC_PER_SIM)
            break;
         global_game_time_residual -= MSEC_PER_SIM;
         global_game_frame++;
         num_frames_to_run++;
      }
      if (num_frames_to_run > 0)
         break;

      hw->timer.sleep(0);
   }
}

static void hw_frame_render(hw* hw)
{
   void** renderers = hw->renderer.backends;
   const u32 renderer_index = hw->renderer.renderer_index;

   if(!hw->renderer.frame_present)
      return;

   pre(renderer_index < (u32)renderer_count);
   hw->renderer.frame_present(renderers[renderer_index]);
}

void hw_event_loop_start(hw* hw, void (*app_frame_function)(hw_arena* frame_arena), void (*app_input_function)(struct app_input* input))
{
   hw->timer.time();

   for (;;)
   {
      app_input input;
      hw_arena frame_arena;

      if (!hw->platform_loop()) 
         break;

      app_input_function(&input);
      defer_frame(&hw->main_arena, frame_arena, app_frame_function(&frame_arena));

      hw_frame_sync(hw);
      hw_frame_render(hw);
   }
}

static int cmd_get_arg_count(char* cmd)
{
	int count = *cmd ? 1 : 0;
   char* arg_start = cmd;
   char* arg_end;

   while((arg_end = strchr(arg_start, ' ')))
   {
      if(count >= MAX_ARGV)                   // exceeds our max number of arguments
         break;
      arg_start = arg_end + 1;
      count++;
   }

   return count;
}

static char** cmd_parse(hw_arena* arena, char* cmd, int* argc)
{
	*argc = cmd_get_arg_count(cmd);
   hw_arena args_arena = arena_push_pointer_strings(arena, *argc);
   char** argv = arena_base(arena, char*);
   char* arg_start = cmd;

   for(u32 i = 0; i < args_arena.count; ++i)
   {
      char* arg_end = strchr(arg_start, ' ');
		argv[i] = arg_start;

		if(!arg_end)
			break;
		*arg_end = 0;	// cut it
      arg_start = arg_end + 1;
   }

   return argv;
}
