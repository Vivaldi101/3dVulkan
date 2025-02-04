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

cache_align typedef struct hw
{
   hw_renderer renderer;
   hw_buffer memory;				// TODO: we need concept of permanent storage here since sub arenas are temp
   bool finished;
} hw;

// Do all renderer includes here?
#include "d3d12.c"
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

// Move to win32.c
static void hw_sleep(DWORD ms)
{
   Sleep(ms);
}
	
// Move to win32.c
static DWORD hw_get_milliseconds()
{
   static DWORD sys_time_base = 0;
   if (sys_time_base == 0) sys_time_base = timeGetTime();
   return timeGetTime() - sys_time_base;
}

static void hw_frame_sync()
{
	int num_frames_to_run = 0;
   for (;;) 
   {
      const int current_frame_time = hw_get_milliseconds();
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

      hw_sleep(0);
   }
}

static void hw_frame_render(hw* hw)
{
   void** renderers = hw->renderer.backends;
   const u32 renderer_index = hw->renderer.renderer_index;

   pre(hw->renderer.frame_present);
   pre(renderer_index < (u32)renderer_count);
   pre(renderers[renderer_index]);

   hw->renderer.frame_present(renderers[renderer_index]);
}

void hw_event_loop_start(hw* hw, void (*app_frame_function)(hw_buffer* frame_arena), void (*app_input_function)(struct app_input* input))
{
   // first window paint
   InvalidateRect(hw->renderer.window.handle, NULL, TRUE);
   UpdateWindow(hw->renderer.window.handle);

   // init timers
   // TDOO: mvoe to win32.c
   timeBeginPeriod(1);
   hw_get_milliseconds();

   for (;;)
   {
      MSG msg;
      app_input input;
      hw_buffer frame_arena;

		// TDOO: mvoe to win32.c
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
         if (msg.message == WM_QUIT)
            break;
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }

      app_input_function(&input);
      defer(frame_arena = sub_arena_create(&hw->memory), sub_arena_clear(&frame_arena))
         app_frame_function(&frame_arena);
      hw_frame_sync();
      hw_frame_render(hw);
   }

   // TDOO: mvoe to win32.c
   timeEndPeriod(1);
}

static int cmd_parse(char* cmd, char** argv)
{
   int argc;
   char* arg_start;
   char* arg_end;

   pre(strlen(argv[0]) > 0);
   for (usize i = strlen(argv[0]) - 1; i >= 0; --i)
   {
      if (argv[0][i] == '\"')
      {
         argv[0][i + 1] = 0;
         break;
      }
   }

   // offset by the program name and just parse the arguments
   argc = 1;
   arg_start = cmd;

   while ((arg_end = strchr(arg_start, ' ')))
   {
      if (argc >= MAX_ARGV)                   // exceeds our max number of arguments
         return 0;
      if (arg_end != arg_start)
         argv[argc++] = arg_start;
      *arg_end = 0;
      arg_start = arg_end + 1;
   }

   if (strlen(arg_start) > 0)
      argv[argc++] = arg_start;

   return argc;
}

// Move to win32.c
static void hw_error(hw* hw, const char* s)
{
   const usize buffer_size = strlen(s) + 1; // string len + 1 for null
   char* buffer = arena_push_string(&hw->memory, buffer_size);

   memcpy(buffer, s, buffer_size);
   MessageBox(NULL, buffer, "Engine", MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL);

   arena_pop_string(&hw->memory, buffer_size);
   hw_event_loop_end(hw);
}

