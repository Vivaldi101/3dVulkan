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
   f32 v0[3], v1[3], vi[3];
   bool intersects;

   // cmd params from the system
   assert(implies(argc > 0, argv[argc-1]));

   HW_window_open(platform, "App window", 0, 0, 800, 600);

   G_frustum_create(&frustum, 800, 600, 90.0f);

   v0[0] = -3.0f; v0[1] = 0.0f; v0[2] = -1.0f;
   v1[0] = 2.0f; v1[1] = 0.0f; v1[2] = -1.0f;

   intersects = G_plane_intersect_segment(&frustum.l, v0, v1, vi);

   HW_event_loop_start(platform, App_frame_draw, App_input_handle);
   HW_window_close(platform);

   return 0;
}
