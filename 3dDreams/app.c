#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "hw.h"
#include "graphics.h"
#include "fixed_point.h"
#include "common.h"

int main(int argc, const char **argv, hw_platform* p)
{
   g_frustum frustum;

   assert(implies(argc > 0, argv[argc-1]));

   HW_window_open(p, "App window", 0, 0, 800, 600);

   G_frustum_create(&frustum, 800, 600, 90.0f);

   HW_event_loop_start(p, 0,0,0);
   HW_window_close();

   return 0;
}
