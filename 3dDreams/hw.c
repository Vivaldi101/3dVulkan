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
	// void* could be opaque handles too
	void* device;
	void* queue;
	void* swap_chain;
	void* command_allocator;
   void(*present)(struct hw_renderer* renderer);
   hw_window window;
   usize command_list_type;
} hw_renderer;

cache_align typedef struct hw
{
   hw_renderer renderer;
   hw_buffer memory;
   u32 image_pixel_size;
   bool finished;
} hw;

#include "d3d12.c"

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

   // TODO: Move these into soft.c for software rasterizing
#if 0
   PAINTSTRUCT ps;

   int i, remap;
   COLORREF cr, cr2;
   BYTE r, g, b;
   hw_mem = CreateCompatibleDC(BeginPaint(hw_wnd, &ps));
   if ((GetDeviceCaps(ps.hdc, PLANES)) != 1)
      hw_error("(Hardware) Direct RGB color expected.");

   hw_pixel_size = GetDeviceCaps(ps.hdc, BITSPIXEL) / 8;
   if ((hw_pixel_size != 2) && (hw_pixel_size != 3) && (hw_pixel_size != 4))
      hw_error("(Hardware) 16bpp, 24bpp or 32bpp expected.");

   G_init_graphics();

   hw_bmp = CreateCompatibleBitmap(ps.hdc, hw_screen_x_size, hw_screen_y_size);
   SelectObject(hw_mem, hw_bmp);

   cr2 = 0;                                     /* ugly way of doing something */
   hw_red_mask = hw_green_mask = hw_blue_mask = 0;  /* trivial. Windows don't */
   for (i = 1; i < 256; i++)                        /* have any service to report */
   {                                          /* pixel bit layout */
      cr = SetPixel(hw_mem, 0, 0, RGB(i, i, i));
      if (GetRValue(cr) != GetRValue(cr2)) hw_red_mask++;
      if (GetGValue(cr) != GetGValue(cr2)) hw_green_mask++;
      if (GetBValue(cr) != GetBValue(cr2)) hw_blue_mask++;
      cr2 = cr;                                   /* masks - which bits are masked */
   }                                          /* by every color */
   hw_red_size = hw_green_size = hw_blue_size = 0;
   for (i = 0; i < 8; i++)                           /* finding how many bits */
   {
      if (hw_red_mask >> i) hw_red_size++;
      if (hw_green_mask >> i) hw_green_size++;
      if (hw_blue_mask >> i) hw_blue_size++;
   }
   hw_red_shift = hw_green_size + hw_blue_size;   /* finding how to pack colors */
   hw_green_shift = hw_blue_size;               /* into a pixel */
   hw_blue_shift = 0;

   EndPaint(hw_wnd, &ps);

   hw_rect.left = hw_rect.top = 0;
   hw_rect.right = hw_screen_x_size;
   hw_rect.bottom = hw_screen_y_size;

#endif
   winrect.left = 0;
   winrect.right = width;
   winrect.top = 0;
   winrect.bottom = height;

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
      hw_error(hw, "(Hardware) Failed to Win32 register class.");

   dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
   dwStyle = WS_OVERLAPPEDWINDOW;
   AdjustWindowRectEx(&winrect, dwStyle, false, dwExStyle);

   hw->renderer.window.handle = CreateWindowEx(dwExStyle,
      wc.lpszClassName, title, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle,
      0, 0, winrect.right - winrect.left, winrect.bottom - winrect.top, NULL, NULL, wc.hInstance, NULL);

   if (!hw->renderer.window.handle)
      hw_error(hw, "(Hardware) Failed to create Win32 window.");

   ShowWindow(hw->renderer.window.handle, SW_SHOW);
   SetForegroundWindow(hw->renderer.window.handle);
   SetFocus(hw->renderer.window.handle);
   UpdateWindow(hw->renderer.window.handle);
}

void hw_window_close(hw* hw)
{
   PostMessage(hw->renderer.window.handle, WM_QUIT, 0, 0L);
}

void hw_event_loop_end(hw* hw)
{
   hw_window_close(hw);
}

void hw_event_loop_start(hw* hw, void (*app_frame_function)(hw_buffer* frame_arena), void (*app_input_function)(struct app_input* input))
{
   // first window paint
   InvalidateRect(hw->renderer.window.handle, NULL, TRUE);
   UpdateWindow(hw->renderer.window.handle);

   // TODO: 60 FPS cap
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
      frame_arena = hw_sub_arena_create(&hw->memory);
      app_frame_function(&frame_arena);
      hw_sub_arena_clear(&frame_arena);

      pre(hw->renderer.present);
      hw->renderer.present(&hw->renderer);
   }
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
   char* arg_start, *arg_end;

   pre(strlen(argv[0]) > 0);
   for(usize i = strlen(argv[0])-1; i >= 0; --i)
   {
		if(argv[0][i] == '\"') 
      {
			argv[0][i+1] = 0;
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

void hw_blit(hw* hw)
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
   hw hw = { 0 };
   usize virtual_memory_amount = 10ull * 1024 * 1024;
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
