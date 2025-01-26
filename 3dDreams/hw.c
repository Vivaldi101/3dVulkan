#if 0
#include "../hardware/hardware.h"           /* macros */
#include "../graphics/graphics.h"           /* G_c_buffer */
#include "../clipper/clipper.h"             /* C_init_clipping */
#include "../trans/trans.h"                 /* T_init_math */
#endif

#include "hw.h"
#include "hw_arena.h"
#include "common.h"
#include "app.h"

cache_align typedef struct hw_window
{
   HWND handle;
} hw_window;

typedef enum hw_command_list_type { } hw_command_list_type;

cache_align typedef struct hw_renderer
{
   hw_command_list_type command_list_type;
   void(*present)(struct hw_renderer* renderer);

} hw_renderer;

cache_align typedef struct hw
{
   hw_buffer memory;
   hw_window window;
   hw_renderer renderer;
   u32 image_pixel_size;
   bool finished;
} hw;

#include "d3d12.c"

#include <stdio.h>                          /* vsprintf */

#define MAX_ARGV 32

void app_start(int argc, const char** argv, hw* hw);

#if 0

/**********************************************************\
* Packing a pixel into a bitmap.                         *
\**********************************************************/

void hw_pixel(char* buf_address, int red, int green, int blue)
{                                           /* adjust and clamp */
   if ((red >>= (8 - hw_red_size)) > hw_red_mask) red = hw_red_mask;
   if ((green >>= (8 - hw_green_size)) > hw_green_mask) green = hw_green_mask;
   if ((blue >>= (8 - hw_blue_size)) > hw_blue_mask) blue = hw_blue_mask;

   switch (hw_pixel_size)                      /* depending on screen depth */
   {                                          /* pack and store */
   case 2:                                   /* 16bpp */
   (*(short*)buf_address) = (red << hw_red_shift) | (green << hw_green_shift) |
      (blue << hw_blue_shift);
   break;
   case 3:                                   /* 24bpp */
   *buf_address = blue; *(buf_address + 1) = green; *(buf_address + 2) = red;
   break;
   case 4:                                   /* 32bpp */
   (*(int*)buf_address) = (red << hw_red_shift) | (green << hw_green_shift) |
      (blue << hw_blue_shift);
   break;
   }
}

/**********************************************************\
* Rendering the bitmap into the window.                  *
\**********************************************************/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
* Windows main callback function. It is being called    *
* as a result of executing our event loop.              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
* Windows Main function. It'll prepare a window and     *
* call our main.                                        *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#endif

#if 0
// Re-enable this
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
   switch (message)
   {
   case WM_PAINT:      if (hw_frame_function != NULL) hw_idle_function();
      break;
   case WM_ERASEBKGND: return(1L);           /* don't erase background */
   case WM_DESTROY:    PostQuitMessage(0);
      break;
   case WM_KEYDOWN:    if (hw_handler_function != NULL) hw_handler_function(wParam);
      if (hw_frame_function != NULL)
      {
         hw_frame_function();
         InvalidateRect(hw_wnd, &hw_rect, TRUE);
         UpdateWindow(hw_wnd);
      }

      break;
   default:            return(DefWindowProc(hWnd, message, wParam, lParam));
   }
#endif
   return 0;
}
#else

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch (uMsg)
   {
   case WM_CLOSE:
   PostQuitMessage(0);
   return 0;
   }

   return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
#endif

void hw_window_open(hw* hw, const char* title, int x, int y, int width, int height)
{
   RECT winrect;
   WNDCLASS wc;
   DWORD dwExStyle, dwStyle;

#if 0
   // These belong elsewhere
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

   hw->window.handle = CreateWindowEx(dwExStyle,
      wc.lpszClassName, title, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle,
      0, 0, winrect.right - winrect.left, winrect.bottom - winrect.top, NULL, NULL, wc.hInstance, NULL);

   if (!hw->window.handle)
      hw_error(hw, "(Hardware) Failed to create Win32 window.");

   ShowWindow(hw->window.handle, SW_SHOW);
   SetForegroundWindow(hw->window.handle);
   SetFocus(hw->window.handle);
   UpdateWindow(hw->window.handle);
}

void hw_window_close(hw* hw)
{
   PostMessage(hw->window.handle, WM_QUIT, 0, 0L);
}

void hw_event_loop_end(hw* hw)
{
   hw_window_close(hw);
}

void hw_event_loop_start(hw* hw, void (*app_frame_function)(hw_buffer* frame_arena), void (*app_input_function)(struct app_input* input))
{
   // first window paint
   InvalidateRect(hw->window.handle, NULL, TRUE);
   UpdateWindow(hw->window.handle);

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

static int hw_cmd_parse(char* cmd, const char** argv)
{
   int argc;
   char* arg_start, * arg_end;

   argc = 0;
   argv[argc++] = GetCommandLine();    // put program name as the first one
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

   BeginPaint(hw->window.handle, &ps);                    /* store into a bitmap */
   SetMapMode(ps.hdc, MM_TEXT);                             /* blit a bitmap */
   //SetBitmapBits(hw_bmp,hw_image_size*hw_pixel_size,(void*)G_c_buffer);
   //BitBlt(ps.hdc,0,0,hw_screen_x_size,hw_screen_y_size,hw_mem,0,0,SRCCOPY);
   EndPaint(hw->window.handle, &ps);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
   hw hw = { 0 };
   MEMORYSTATUSEX memory_status;
   int argc;
   const char** argv;
   usize virtual_memory_amount = 10ull * 1024 * 1024;

   memory_status.dwLength = sizeof(memory_status);
   if (!GlobalMemoryStatusEx(&memory_status))
      return 0;

   hw.memory = hw_arena_create(virtual_memory_amount);
   if (!hw.memory.base || (hw.memory.max_size != virtual_memory_amount))
      return 0;

   argv = hw_arena_push_count(&hw.memory, MAX_ARGV, const char*);
   argc = hw_cmd_parse(lpszCmdLine, argv);

   if (argc == 0)
      hw_error(&hw, "(Hardware) Invalid number of command line options given.\n");

   app_start(argc, argv, &hw);    // pass the options to the application

   return 0;
}
