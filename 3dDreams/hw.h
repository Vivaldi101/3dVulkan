// TODO: Rename HW to OS

#if !defined(_HW_H)
#define _HW_H

#if !defined(_WIN32)
#error "Cannot include the file on a non-Win32 platforms"
#endif

#include "common.h"
#include <windows.h>                        /* MS-Windows API */

typedef struct hw_platform hw_platform;
typedef struct hw_memory_buffer hw_memory_buffer;
typedef struct hw_window hw_window;

typedef enum { HW_INPUT_TYPE_KEY, HW_INPUT_TYPE_MOUSE, HW_INPUT_TYPE_TOUCH } hw_input_type;
cache_align typedef struct app_input
{
   hw_input_type input_type;
   union { i32 pos[2]; u64 key; };
} app_input;

void HW_platform_window_open(hw_platform* platform, const char *title, int x, int y, int width, int height);
void HW_platform_window_close(hw_platform* platform);

void HW_draw_pixel(byte address, int r, int g, int b);
void HW_draw_swap(void);

void HW_platform_event_loop_start(hw_platform* platform, void (*App_frame_function)(hw_memory_buffer* frame_arena), void (*App_input_function)(app_input* input));
void HW_platform_event_loop_end(hw_platform* platform);
static void HW_platform_error(hw_platform* platform, const char *s);

#endif
