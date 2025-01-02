// TODO: Rename HW to OS

#if !defined(_HW_H)
#define _HW_H

#if !defined(_WIN32)
#error "Cannot include the file on a non-Win32 platforms"
#endif

#include "common.h"
#include <windows.h>                        /* MS-Windows API */

#define main _main                          /* to accomodate windows */

#define HW_KEY_ARROW_LEFT  VK_LEFT
#define HW_KEY_ARROW_RIGHT VK_RIGHT
#define HW_KEY_ARROW_UP    VK_UP
#define HW_KEY_ARROW_DOWN  VK_DOWN
#define HW_KEY_PLUS        VK_ADD
#define HW_KEY_MINUS       VK_SUBTRACT
#define HW_KEY_ENTER       VK_RETURN
#define HW_KEY_SPACE       VK_SPACE
#define HW_KEY_TAB         VK_TAB           /* ids of some keys */

//typedef char* HW_pixel_ptr;                 /* may not fit a char though */
//typedef int HW_fixed;                       /* better be 32 bit machine */

typedef struct hw_platform hw_platform;
typedef struct hw_input hw_input;

void HW_window_open(hw_platform* platform, const char *title, int x, int y, int width, int height);
void HW_window_close(void);

void HW_draw_pixel(byte address, int r, int g, int b);
void HW_draw_swap(void);

void HW_event_loop_start(hw_platform* platform, void (*frame_function)(), void (*input_function)(hw_input* input));
void HW_event_loop_end(void);
void HW_error(char *string, ...);

#endif
