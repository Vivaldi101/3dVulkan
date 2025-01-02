#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "hw.h"
#include "graphics.h"
#include "fixed_point.h"
#include "common.h"

static void App_frame_draw()
{
}

static void App_input_handle(app_input* input)
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

int main(int argc, const char **argv, hw_platform* platform)
{
   g_frustum frustum;

   // cmd params from the system
   assert(implies(argc > 0, argv[argc-1]));

   HW_window_open(platform, "App window", 0, 0, 800, 600);

   G_frustum_create(&frustum, 800, 600, 90.0f);

   HW_event_loop_start(platform, App_frame_draw, App_input_handle);
   HW_window_close(platform);

   return 0;
}
