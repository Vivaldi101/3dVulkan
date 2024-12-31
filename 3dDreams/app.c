#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "hw.h"
#include "graphics.h"
#include "fixed_point.h"
#include "common.h"

int main(int argc, const char **argv, hw_platform* platform)                  
{
   gr_frustum frustum;

   assert(platform);
   assert(implies(argc > 0, argv[argc-1]));

   HW_window_open(platform, "App window", 0, 0, 800, 600);

   GR_frustum_create(&frustum, 800, 600, 90.0f);

   //G_clear(0,0,0);

   HW_event_loop_start(0,0,0);
   HW_window_close();

   return 0;
}
