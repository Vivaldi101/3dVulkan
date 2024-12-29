#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "hw.h"
#include "graphics.h"
#include "fixed_point.h"
#include "common.h"

int main(int argc, const char **argv, hw_platform* platform)                  
{
   vec3 u = {-1,0, 0};
   vec3 v = { 0,0,-1};
   vec3 n;

   assert(platform);
   assert(implies(argc > 0, argv[argc-1]));

   HW_window_open(platform, "App window", 0, 0, 800, 600);
   vec3_cross(u,v,n);

   implies(vec3_len2(u) == 1.0f && vec3_len2(v) == 1.0f, vec3_len2(n) == 1.0f);

   // TODO: Remove test code
   assert(vec3_dot(n, u) == 0.0f);
   assert(vec3_dot(n, v) == 0.0f);
   assert(vec3_dot(n, n) == 1.0f);

   assert(vec3_len2(u) == 1.0f);
   assert(vec3_len2(v) == 1.0f);
   assert(vec3_len2(n) == 1.0f);

   //G_clear(0,0,0);

   HW_event_loop_start(0,0,0);
   HW_window_close();

   return 0;
}
