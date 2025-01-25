#include "hw.h"
#include "app.h"
#include "graphics.h"
#include "fixed_point.h"
#include "common.h"
#include "hw_arena.h"
#include "d3d12.h"

typedef struct app_some_type
{
   int arr[100];
   char* name;
   bool isvalid;
} app_some_type;

// do frame drawing
// TODO: Pass frame data
static void app_frame_draw(hw_buffer* frame_arena)
{
   int i;
   app_some_type* type = hw_arena_push_struct(frame_arena, app_some_type);
   type->isvalid = true;
   type->name = "foo";
   for(i = 0; i < array_count(type->arr); ++i)
      type->arr[i] = 42;
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

void app_start(int argc, const char **argv, hw* hw)
{
   g_frustum frustum = {0};

   // cmd params from the system
   assert(implies(argc > 0, argv[argc-1]));

   d3d_initialize(hw);

   hw_window_open(hw, "App window", 0, 0, 800, 600);

   g_frustum_create(&frustum, 800, 600, 90.0f);

   hw_event_loop_start(hw, app_frame_draw, app_input_handle);
   hw_window_close(hw);
}
