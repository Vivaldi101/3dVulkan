#include "app.h"
#include "graphics.h"
#include "d3d12.h"
#include "vulkan.h"
#include "hw_arena.h"

#if 1
typedef struct app_some_type
{
   int arr[100];
   char* name;
   bool isvalid;
} app_some_type;
#endif

// do frame drawing
static void app_frame_draw(hw_arena* frame_arena)
{
#if 0
   int i;
   app_some_type* type = arena_push_struct(frame_arena, app_some_type);
   type->isvalid = true;
   type->name = "foo";
   for(i = 0; i < array_count(type->arr); ++i)
      type->arr[i] = 42;
#endif
}

// do input handling
static void app_input_handle(app_input* input)
{
   u64 key;
   i32 pos[2], i;
   if(input->input_type == HW_INPUT_TYPE_MOUSE)
      if(input->pos[0] > 0 && input->pos[1] > 0)
         for(i = 0; i < 2; ++i)
            pos[i] = input->pos[i];
   if(input->input_type == HW_INPUT_TYPE_KEY)
      key = input->key;
}

void app_start(int argc, const char** argv, hw* hw)
{
   pre(implies(argc > 0, argv[argc - 1]));

   g_frustum frustum = {0};

	int w = 800, h = 600;
   hw_window_open(hw, "App window", 0, 0, w, h);

   if(!vulkan_initialize(hw))
		return;	// TODO: error message for vulkan init

   g_frustum_create(&frustum, (f32)w, (f32)h, 90.0f);

   hw_event_loop_start(hw, app_frame_draw, app_input_handle);
   hw_window_close(hw);
}
