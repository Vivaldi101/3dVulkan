#if 0
#include "../hardware/hardware.h"           /* macros */
#include "../graphics/graphics.h"           /* G_c_buffer */
#include "../clipper/clipper.h"             /* C_init_clipping */
#include "../trans/trans.h"                 /* T_init_math */
#endif

#include "hw.h"
#include "hw_memory.h"
#include "common.h"

#include <stdio.h>                          /* vsprintf */

#define MAX_ARGV 32

cache_align typedef struct { HWND handle; } hw_window;

cache_align typedef struct hw_platform
{
   hw_memory_buffer memory;
   u32 image_pixel_size;
   hw_window window;
   bool finished;
} hw_platform;

//int main(int argc, const char **argv, hw_platform* platform);
void App_start(int argc, const char **argv, hw_platform* platform);

#if 0


int HW_red_size;                            /* how many bits is there */
int HW_green_size;                          /* occupied by each color */
int HW_blue_size;
int HW_red_mask;                            /* which bits are occupied */
int HW_green_mask;                          /* by color */
int HW_blue_mask;
int HW_red_shift;                           /* how colors are packed into */
int HW_green_shift;                         /* the bitmap */
int HW_blue_shift;

HDC HW_mem;                                 /* memory device context */
HBITMAP HW_bmp;                             /* bitmap header */
RECT HW_rect;                               /* "client" area dimensions */
HWND HW_wnd;                                /* window */
HANDLE HW_instance;   
int HW_cmd_show;                            /* not sure why, but needed */

/**********************************************************\
* Packing a pixel into a bitmap.                         *
\**********************************************************/

void HW_pixel(char* buf_address,int red,int green,int blue)
{                                           /* adjust and clamp */
   if((red>>=(8-HW_red_size))>HW_red_mask) red=HW_red_mask;
   if((green>>=(8-HW_green_size))>HW_green_mask) green=HW_green_mask; 
   if((blue>>=(8-HW_blue_size))>HW_blue_mask) blue=HW_blue_mask; 

   switch(HW_pixel_size)                      /* depending on screen depth */ 
   {                                          /* pack and store */
   case 2:                                   /* 16bpp */
      (*(short*)buf_address)=(red<<HW_red_shift)|(green<<HW_green_shift)|
         (blue<<HW_blue_shift); 
      break;
   case 3:                                   /* 24bpp */
      *buf_address=blue; *(buf_address+1)=green; *(buf_address+2)=red;
      break;
   case 4:                                   /* 32bpp */
      (*(int*)buf_address)=(red<<HW_red_shift)|(green<<HW_green_shift)|
         (blue<<HW_blue_shift);
      break;
   }
}

/**********************************************************\
* Rendering the bitmap into the window.                  *
\**********************************************************/

void HW_blit(void)
{
   PAINTSTRUCT ps;

   BeginPaint(HW_wnd,&ps);                    /* store into a bitmap */
   SetMapMode(ps.hdc,MM_TEXT);                /* blit a bitmap */
   SetBitmapBits(HW_bmp,HW_image_size*HW_pixel_size,(void*)G_c_buffer);
   BitBlt(ps.hdc,0,0,HW_screen_x_size,HW_screen_y_size,HW_mem,0,0,SRCCOPY);
   EndPaint(HW_wnd,&ps);
}

/**********************************************************\
* Deallocating some stuff.                               *
\**********************************************************/

/**********************************************************\
* Running the event loop.                                *
\**********************************************************/


/**********************************************************\
* Quiting with a message.                                *
\**********************************************************/

/**********************************************************\
* Quitting the event loop.                               *
\**********************************************************/

void HW_close_event_loop(void)
{
   PostMessage(HW_wnd,WM_CLOSE,0,0L);         /* telling ourselves to quit */
}

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
   switch(message)
   {
   case WM_PAINT:      if(HW_frame_function!=NULL) HW_idle_function();
      break;
   case WM_ERASEBKGND: return(1L);           /* don't erase background */
   case WM_DESTROY:    PostQuitMessage(0);
      break;
   case WM_KEYDOWN:    if(HW_handler_function!=NULL) HW_handler_function(wParam);
      if(HW_frame_function!=NULL) 
      {
         HW_frame_function();
         InvalidateRect(HW_wnd,&HW_rect,TRUE);
         UpdateWindow(HW_wnd);
      }

      break;
   default:            return(DefWindowProc(hWnd,message,wParam,lParam));
   }
#endif
   return 0;
}
#else

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch(uMsg) 
   {
   case WM_CLOSE:
      PostQuitMessage(0);
      return 0;
   }

   return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
#endif

void HW_window_open(hw_platform* platform, const char *title, int x, int y, int width, int height)
{
   RECT winrect;
   WNDCLASS wc;
   DWORD dwExStyle, dwStyle;

#if 0
   // These belong elsewhere
   PAINTSTRUCT ps;

   int i,remap;
   COLORREF cr,cr2;
   BYTE r,g,b;
   HW_mem=CreateCompatibleDC(BeginPaint(HW_wnd,&ps));
   if((GetDeviceCaps(ps.hdc,PLANES))!=1)
      HW_error("(Hardware) Direct RGB color expected.");

   HW_pixel_size=GetDeviceCaps(ps.hdc,BITSPIXEL)/8;
   if((HW_pixel_size!=2)&&(HW_pixel_size!=3)&&(HW_pixel_size!=4))
      HW_error("(Hardware) 16bpp, 24bpp or 32bpp expected.");

   G_init_graphics();

   HW_bmp=CreateCompatibleBitmap(ps.hdc,HW_screen_x_size,HW_screen_y_size);
   SelectObject(HW_mem,HW_bmp);

   cr2=0;                                     /* ugly way of doing something */
   HW_red_mask=HW_green_mask=HW_blue_mask=0;  /* trivial. Windows don't */
   for (i=1;i<256;i++)                        /* have any service to report */
   {                                          /* pixel bit layout */
      cr=SetPixel(HW_mem,0,0,RGB(i,i,i));
      if(GetRValue(cr)!=GetRValue(cr2)) HW_red_mask++;
      if(GetGValue(cr)!=GetGValue(cr2)) HW_green_mask++;
      if(GetBValue(cr)!=GetBValue(cr2)) HW_blue_mask++;       
      cr2=cr;                                   /* masks - which bits are masked */
   }                                          /* by every color */
   HW_red_size=HW_green_size=HW_blue_size=0;  
   for(i=0;i<8;i++)                           /* finding how many bits */
   {
      if(HW_red_mask>>i) HW_red_size++;
      if(HW_green_mask>>i) HW_green_size++;
      if(HW_blue_mask>>i) HW_blue_size++;
   }
   HW_red_shift=HW_green_size+HW_blue_size;   /* finding how to pack colors */
   HW_green_shift=HW_blue_size;               /* into a pixel */
   HW_blue_shift=0;

   EndPaint(HW_wnd,&ps);

   HW_rect.left=HW_rect.top=0;
   HW_rect.right=HW_screen_x_size;
   HW_rect.bottom=HW_screen_y_size;

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
   if(!RegisterClass(&wc)) 
      HW_error(platform, "(Hardware) Failed to Win32 register class.");

   dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
   dwStyle = WS_OVERLAPPEDWINDOW;
   AdjustWindowRectEx(&winrect, dwStyle, false, dwExStyle);

   platform->window.handle = CreateWindowEx(dwExStyle, 
      wc.lpszClassName, title, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle, 
      0, 0, winrect.right - winrect.left, winrect.bottom - winrect.top, NULL, NULL, wc.hInstance, NULL);

   if(!platform->window.handle)
      HW_error(platform, "(Hardware) Failed to create Win32 window.");

   ShowWindow(platform->window.handle, SW_SHOW);
   SetForegroundWindow(platform->window.handle);
   SetFocus(platform->window.handle);
   UpdateWindow(platform->window.handle);
}

void HW_window_close(hw_platform* platform)
{
   PostMessage(platform->window.handle,WM_QUIT,0,0L);
}

void HW_event_loop_end(hw_platform* platform)
{
   HW_window_close(platform);
}

static hw_input_type HW_input_type(const MSG* m)
{
   return HW_INPUT_TYPE_KEY;  // placeholder
}

void HW_event_loop_start(hw_platform* platform, void (*frame_function)(hw_memory_buffer* frame_memory), void (*input_function)(app_input* input))
{
   // first window paint
   InvalidateRect(platform->window.handle, NULL, TRUE);
   UpdateWindow(platform->window.handle);

   for(;;)
   {
      MSG msg;
      app_input input;
      hw_memory_buffer frame_memory;
      if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
      {
         if(msg.message == WM_QUIT) 
            break;
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }

      // random test code for input
      input.input_type = HW_input_type(&msg);
      switch(input.input_type)
      {
         case HW_INPUT_TYPE_KEY:
            input.key = 0; 
            break;
         case HW_INPUT_TYPE_MOUSE:
            input.pos[0] = input.pos[1] = 0; 
            break;
         default: break;
      }
      input_function(&input);

      // TODO: join the arenas
      frame_memory.bytes_used = platform->memory.bytes_used;
      // hard cap of 10 megabytes of frame memory
      frame_memory.max_size = 10ull*1024*1024;
      frame_memory.base = platform->memory.base + platform->memory.bytes_used;
      assert(frame_memory.max_size < platform->memory.max_size);
      // TODO: function for clearing the memory arena buffers
      frame_function(&frame_memory);
      frame_memory.bytes_used = 0;
      frame_memory.max_size = 0;
      frame_memory.base = 0;
   }
}

static void HW_error(hw_platform* platform, const char *s)
{
   const usize buffer_size = strlen(s)+1; // string len + 1 for null
   char* buffer = HW_push_string(&platform->memory, buffer_size);

   memcpy(buffer, s, buffer_size);
   MessageBox(NULL,buffer,"Engine",MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);

   HW_pop_string(&platform->memory, buffer_size);
   HW_event_loop_end(platform);
}

static int HW_parse_cmdline_into_arguments(char* cmd, char** argv)
{
   int argc;
   char *arg_start,*arg_end;

   argc = 0;
   argv[argc++] = GetCommandLine();    // put program name as the first one
   arg_start = cmd;

   while (arg_end = strchr(arg_start, ' '))
   {
      if(argc >= MAX_ARGV)                   // exceeds our max number of arguments
         return 0;
      if (arg_end != arg_start) 
         argv[argc++] = arg_start;
      *arg_end = 0;
      arg_start = arg_end+1;
   }

   if(strlen(arg_start) > 0) 
      argv[argc++] = arg_start;

   return argc;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
   hw_platform platform = {0};
   MEMORYSTATUSEX memory_status;
   int argc;
   char **argv;

   memory_status.dwLength = sizeof(memory_status);
   if (!GlobalMemoryStatusEx(&memory_status))
      return 0;

   platform.memory = HW_memory_buffer_create(memory_status.ullAvailPhys);
   if (!platform.memory.base || (platform.memory.max_size != memory_status.ullAvailPhys))
      return 0;

   argv = HW_push_count(&platform.memory, MAX_ARGV, char*);
   argc = HW_parse_cmdline_into_arguments(lpszCmdLine, argv);

   if (argc == 0)
      HW_error(&platform, "(Hardware) Invalid number of command line options given.\n");

   App_start(argc, argv, &platform);    // pass the options to the application

   return 0;
}
