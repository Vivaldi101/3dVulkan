#if !defined(_WIN32)
#error "Cannot include the file on a non-Win32 platforms"
#endif

#include <Windows.h>

#include "common.h"

#define hw_fail __debugbreak();
#define MAX_ARGV 32

cache_align typedef struct hw_window
{
   HWND (*open)(const char* title, int x, int y, int width, int height);
   void (*close)(struct hw_window window);
   HWND handle;
} hw_window;

#include "hw.c"

// TODO: Add all the extra garbage for handling window events
static LRESULT CALLBACK win32_win_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch (uMsg)
   {
   case WM_CLOSE:
   PostQuitMessage(0);
   return 0;
   }

   return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static HWND win32_window_open(const char* title, int x, int y, int width, int height)
{
   HWND result;

   RECT winrect;
   DWORD dwExStyle, dwStyle;
   WNDCLASS wc;

   wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
   wc.lpfnWndProc = win32_win_proc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = GetModuleHandle(NULL);
   wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = NULL;
   wc.lpszMenuName = NULL;
   wc.lpszClassName = title;

   if (!RegisterClass(&wc))
      return 0;

   dwStyle = WS_OVERLAPPEDWINDOW;
   dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

   winrect.left = 0;
   winrect.right = width;
   winrect.top = 0;
   winrect.bottom = height;

   AdjustWindowRectEx(&winrect, dwStyle, 0, dwExStyle);

   result = CreateWindowEx(dwExStyle,
      wc.lpszClassName, title, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle,
      0, 0, winrect.right - winrect.left, winrect.bottom - winrect.top, NULL, NULL, wc.hInstance, NULL);

   if (!result)
      return 0;

   ShowWindow(result, SW_SHOW);
   SetForegroundWindow(result);
   SetFocus(result);
   UpdateWindow(result);

   return result;
}

static void win32_window_close(hw_window window)
{
   PostMessage(window.handle, WM_QUIT, 0, 0L);
}

// TODO: Used by the software renderer
#if 0
static void hw_blit(hw* hw)
{
   PAINTSTRUCT ps;

   BeginPaint(hw->renderer.window.handle, &ps);                    /* store into a bitmap */
   SetMapMode(ps.hdc, MM_TEXT);                             /* blit a bitmap */
   //SetBitmapBits(hw_bmp,hw_image_size*hw_pixel_size,(void*)G_c_buffer);
   //BitBlt(ps.hdc,0,0,hw_screen_x_size,hw_screen_y_size,hw_mem,0,0,SRCCOPY);
   EndPaint(hw->renderer.window.handle, &ps);
}
#endif

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

   hw.memory = arena_create(virtual_memory_amount);
   if (!hw.memory.base)
      return 0;

   argv = arena_push_count(&hw.memory, MAX_ARGV, const char*);
   argv[0] = GetCommandLine();				 // put program name as the first one
   argc = cmd_parse(lpszCmdLine, argv);

   if (argc == 0)
      hw_error(&hw, "(Hardware) Invalid number of command line options given.\n");

   hw.renderer.window.open = win32_window_open;
   hw.renderer.window.close = win32_window_close;

   app_start(argc, argv, &hw);    // pass the options to the application

   return 0;
}

