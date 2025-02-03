#include "hw.h"
#include "hw_arena.h"
#include "common.h"
#include "app.h"

#define MAX_ARGV 32

cache_align typedef struct hw_window
{
   HWND handle;
} hw_window;

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

#include "d3d12.c"
#include "vulkan.c"

// TODO: Add all the extra garbage for handling window events
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch (uMsg)
   {
   case WM_CLOSE:
   PostQuitMessage(0);
   return 0;
   }

   return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void hw_window_open(hw* hw, const char* title, int x, int y, int width, int height)
{
   RECT winrect;
   WNDCLASS wc;
   DWORD dwExStyle, dwStyle;

   wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
   wc.lpfnWndProc = WndProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = GetModuleHandle(NULL);
   wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = NULL;
   wc.lpszMenuName = NULL;
   wc.lpszClassName = title;
   if (!RegisterClass(&wc))
   {
      hw_error(hw, "(Hardware) Failed to Win32 register class.");
		return;
   }

   dwStyle = WS_OVERLAPPEDWINDOW;
   dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

   winrect.left = 0;
   winrect.right = width;
   winrect.top = 0;
   winrect.bottom = height;
   AdjustWindowRectEx(&winrect, dwStyle, false, dwExStyle);

   hw->renderer.window.handle = CreateWindowEx(dwExStyle,
      wc.lpszClassName, title, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle,
      0, 0, winrect.right - winrect.left, winrect.bottom - winrect.top, NULL, NULL, wc.hInstance, NULL);

   if (!hw->renderer.window.handle)
   {
      hw_error(hw, "(Hardware) Failed to create Win32 window.");
		return;
   }

   ShowWindow(hw->renderer.window.handle, SW_SHOW);
   SetForegroundWindow(hw->renderer.window.handle);
   SetFocus(hw->renderer.window.handle);
   UpdateWindow(hw->renderer.window.handle);
}

void hw_window_close(hw* hw)
{
	pre(hw->renderer.window.handle);
   PostMessage(hw->renderer.window.handle, WM_QUIT, 0, 0L);
}

void hw_event_loop_end(hw* hw)
{
   if (hw->renderer.window.handle)
      hw_window_close(hw);
}

static DWORD hw_get_milliseconds()
{
   static DWORD sys_time_base = 0;
   if (sys_time_base == 0) sys_time_base = timeGetTime();
   return timeGetTime() - sys_time_base;
}

#define MAX_UPS (60)
#define MSEC_PER_SIM (16)

static f32 global_game_time_residual;
static int global_game_frame;

static void hw_sleep(DWORD ms)
{
   Sleep(ms);
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
   timeBeginPeriod(1);
   hw_get_milliseconds();

   for (;;)
   {
      MSG msg;
      app_input input;
      hw_buffer frame_arena;
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
         if (msg.message == WM_QUIT)
            break;
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }

      app_input_function(&input);
      defer(frame_arena = hw_sub_arena_create(&hw->memory), 
         hw_sub_arena_clear(&frame_arena))
         app_frame_function(&frame_arena);
      hw_frame_sync();
      hw_frame_render(hw);
   }

   timeEndPeriod(1);
}

static void hw_error(hw* hw, const char* s)
{
   const usize buffer_size = strlen(s) + 1; // string len + 1 for null
   char* buffer = hw_arena_push_string(&hw->memory, buffer_size);

   memcpy(buffer, s, buffer_size);
   MessageBox(NULL, buffer, "Engine", MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL);

   hw_arena_pop_string(&hw->memory, buffer_size);
   hw_event_loop_end(hw);
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

// TODO: Used by the software renderer
static void hw_blit(hw* hw)
{
   PAINTSTRUCT ps;

   BeginPaint(hw->renderer.window.handle, &ps);                    /* store into a bitmap */
   SetMapMode(ps.hdc, MM_TEXT);                             /* blit a bitmap */
   //SetBitmapBits(hw_bmp,hw_image_size*hw_pixel_size,(void*)G_c_buffer);
   //BitBlt(ps.hdc,0,0,hw_screen_x_size,hw_screen_y_size,hw_mem,0,0,SRCCOPY);
   EndPaint(hw->renderer.window.handle, &ps);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
   hw hw = {0};
   const usize virtual_memory_amount = 10ull * 1024 * 1024;	// 10 megs
   MEMORYSTATUSEX memory_status;
   int argc;
   char** argv;

   memory_status.dwLength = sizeof(memory_status);
   if (!GlobalMemoryStatusEx(&memory_status))
      return 0;

   hw.memory = hw_arena_create(virtual_memory_amount);
   if (!hw.memory.base || (hw.memory.max_size != virtual_memory_amount))
      return 0;

   argv = hw_arena_push_count(&hw.memory, MAX_ARGV, const char*);
   argv[0] = GetCommandLine();				 // put program name as the first one
   argc = cmd_parse(lpszCmdLine, argv);

   if (argc == 0)
      hw_error(&hw, "(Hardware) Invalid number of command line options given.\n");

   app_start(argc, argv, &hw);    // pass the options to the application

   return 0;
}
