#if !defined(_WIN32)
#error "Cannot include the file on a non-Win32 platforms"
#endif

#include <Windows.h>

#include "common.h"
#include "arena.h"

cache_align typedef struct hw_window
{
   HWND(*open)(const char* title, int x, int y, int width, int height);
   void (*close)(struct hw_window window);
   HWND handle;
} hw_window;

static void debug_message(const char* format, ...)
{
   char temp[1024];
   va_list args;
   va_start(args, format);
   wvsprintfA(temp, format, args);
   va_end(args);
   OutputDebugStringA(temp);
}

static u64 global_perf_counter_frequency;
#include "hw.c"

static void win32_sleep(u32 ms)
{
   Sleep(ms);
}

static u32 win32_time()
{
   static DWORD sys_time_base = 0;
   if(sys_time_base == 0) sys_time_base = timeGetTime();
   return timeGetTime() - sys_time_base;
}

static bool win32_platform_loop()
{
   MSG msg;
   if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
   {
      if(msg.message == WM_QUIT)
         return false;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   return true;
}

// TODO: Add all the extra garbage for handling window events
static LRESULT CALLBACK win32_win_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
   switch(umsg)
   {
      case WM_CLOSE:
         PostQuitMessage(0);
         return 0;
      case WM_PAINT:
      {
         PAINTSTRUCT ps;
         HDC hdc = BeginPaint(hwnd, &ps);
         FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1)); // Default white background
         EndPaint(hwnd, &ps);
      } break;

      case WM_SIZE:
         InvalidateRect(hwnd, NULL, TRUE); // Forces a repaint when resizing
         break;
   }

   return DefWindowProc(hwnd, umsg, wparam, lparam);
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

   if(!RegisterClass(&wc))
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

   if(!result)
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

static void win32_abort(u32 code)
{
   ExitProcess(code);
}

#if 0
static void hw_error(hw_frame_arena* error_arena, const char* s)
{
   const usize buffer_size = strlen(s) + 1; // string len + 1 for null
   char* buffer = arena_push_string(error_arena, buffer_size);

   memcpy(buffer, s, buffer_size);
}
#endif

#define hw_error(m) MessageBox(NULL, (m), "Engine", MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
   const usize virtual_memory_amount = default_arena_size;
   const char** argv = 0;
   int argc = 0;
   hw hw = {0};

   // TODO: might change to OS specific alloc later
   //hw_virtual_memory_init();

   arena base = hw.permanent = arena_new(virtual_memory_amount);
   hw.scratch = arena_new(virtual_memory_amount);
   argv = cmd_parse(&hw.permanent, lpszCmdLine, &argc);

   hw.renderer.window.open = win32_window_open;
   hw.renderer.window.close = win32_window_close;

   hw.timer.sleep = win32_sleep;
   hw.timer.time = win32_time;

   hw.platform_loop = win32_platform_loop;

   timeBeginPeriod(1);
   app_start(argc, argv, &hw);
   timeEndPeriod(1);

   arena_free(&base);
   arena_free(&hw.scratch);

   return 0;
}

