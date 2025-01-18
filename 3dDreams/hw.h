// TODO: Rename HW to OS

#if !defined(_HW_H)
#define _HW_H

#if !defined(_WIN32)
#error "Cannot include the file on a non-Win32 platforms"
#endif

#include "common.h"
#include <windows.h>                        /* MS-Windows API */

typedef struct hw hw;
typedef struct hw_buffer hw_buffer;
typedef struct hw_window hw_window;

typedef enum { HW_INPUT_TYPE_KEY, HW_INPUT_TYPE_MOUSE, HW_INPUT_TYPE_TOUCH } hw_input_type;
cache_align typedef struct app_input
{
   hw_input_type input_type;
   union { i32 pos[2]; u64 key; };
} app_input;

void hw_window_open(hw* hw, const char *title, int x, int y, int width, int height);
void hw_window_close(hw* hw);

void hw_draw_pixel(byte address, int r, int g, int b);
void hw_draw_swap(void);

void hw_event_loop_start(hw* hw, void (*app_frame_function)(hw_buffer* frame_arena), void (*app_input_function)(app_input* input));
void hw_event_loop_end(hw* hw);
static void hw_error(hw* hw, const char *s);

#endif
