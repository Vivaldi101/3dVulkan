#if !defined(_HW_H)
#define _HW_H

#if _WIN32
#define hw_message(p) { MessageBoxA(0, #p, "Assertion", MB_OK); __debugbreak(); }
#pragma comment(lib,	"winmm.lib") // timers etc.
#elif
// other plats like linux, osx and ios
#endif

// Every platform should define hw_message
#define hw_assert(p) if(!(p)) hw_message(p)

// TODO: Move this to general renderer.h
enum
{
   vulkan_renderer_index, 
	d3d12_renderer_index, 
   soft_renderer_index,
   renderer_count
};

// hw is the main platform context
typedef struct hw hw;
typedef struct hw_buffer hw_buffer;
typedef enum { HW_INPUT_TYPE_KEY, HW_INPUT_TYPE_MOUSE, HW_INPUT_TYPE_TOUCH } hw_input_type;
void hw_window_open(hw* hw, const char *title, int x, int y, int width, int height);
void hw_window_close(hw* hw);

void hw_event_loop_start(hw* hw, void (*app_frame_function)(hw_buffer* frame_arena), void (*app_input_function)(struct app_input* input));
void hw_event_loop_end(hw* hw);
static void hw_error(hw_buffer* error_arena, const char* s);

#endif
