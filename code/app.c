#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202000L
#   error "This code requires C23 or later"
#endif

#define _CRT_SECURE_NO_WARNINGS 1

#include "hw.h"
#include "app.h"
#include "math.h"
#include "arena.h"

#include "vk.c"

static void app_frame(arena scratch, app_state* state)
{
   (void)scratch;
   (void)state;

   if(state->fullscreen)
      ; // toggle here
}

static void app_fps_camera_update(app_state* state)
{
   f32 speed = 0.1f;
   if(state->input.keys['W'] & (KEY_STATE_REPEATING | KEY_STATE_DOWN))
   {
      vec3 eye = state->camera.eye;
      vec3 dir = state->camera.dir;
      vec3_normalize(dir);
      dir = vec3_scale(&dir, speed);
      state->camera.eye = vec3_add(&eye, &dir);
   }
   if(state->input.keys['A'] & (KEY_STATE_REPEATING | KEY_STATE_DOWN))
   {
      vec3 eye = state->camera.eye;
      vec3 dir = state->camera.dir;
      vec3 right = {0};
      vec3 up = {0, 1, 0};

      vec3_normalize(dir);
      vec3_cross(dir, up, right);
      vec3_normalize(right);
      vec3 left = vec3_scale(&right, -speed);

      state->camera.eye = vec3_add(&eye, &left);
   }
   if(state->input.keys['D'] & (KEY_STATE_REPEATING | KEY_STATE_DOWN))
   {
      vec3 eye = state->camera.eye;
      vec3 dir = state->camera.dir;
      vec3 right = {0};
      vec3 up = {0, 1, 0};

      vec3_normalize(dir);
      vec3_cross(dir, up, right);
      vec3_normalize(right);
      right = vec3_scale(&right, speed);

      state->camera.eye = vec3_add(&eye, &right);
   }
   if(state->input.keys['S'] & (KEY_STATE_REPEATING | KEY_STATE_DOWN))
   {
      vec3 eye = state->camera.eye;
      vec3 dir = state->camera.dir;
      vec3_normalize(dir);
      dir = vec3_scale(&dir, -speed);
      state->camera.eye = vec3_add(&eye, &dir);
   }

   // full turn across view plane extents (in azimuth)
   f32 rotation_speed_x = (2.f * PI) / state->camera.viewplane_width;
   f32 rotation_speed_y = (2.f * PI) / state->camera.viewplane_height;

   // delta in pixels
   f32 delta_x = (f32)state->input.mouse_pos[0] - (f32)state->input.mouse_prev_pos[0];
   f32 delta_y = (f32)state->input.mouse_pos[1] - (f32)state->input.mouse_prev_pos[1];

   // rotating input affects target azimuth and altitude
   if(state->input.mouse_buttons & MOUSE_BUTTON_STATE_LEFT)
   {
      state->camera.target_azimuth += rotation_speed_x * delta_x;
      state->camera.target_altitude -= rotation_speed_y * delta_y;

      const f32 max_altitude = PI / 2.0f - 0.01f;
      if(state->camera.target_altitude > max_altitude) state->camera.target_altitude = max_altitude;
      if(state->camera.target_altitude < -max_altitude) state->camera.target_altitude = -max_altitude;
   }

   f32 azimuth = state->camera.target_azimuth;
   f32 altitude = state->camera.target_altitude;

   f32 x = cosf(altitude) * cosf(azimuth);
   f32 z = cosf(altitude) * sinf(azimuth);
   f32 y = sinf(altitude);

   vec3 dir = {x, y, z};

   state->camera.dir = dir;

   state->input.mouse_prev_pos[0] = state->input.mouse_pos[0];
   state->input.mouse_prev_pos[1] = state->input.mouse_pos[1];
}

static void app_orbit_camera_update(app_state* state)
{
   f64 decay = -log(0.0025);
   f64 smoothing_factor = 1.0f - exp(-decay * (f32)state->frame_delta_in_seconds);

   // half turn across view plane extents (in azimuth)
   f32 rotation_speed_x = (2.f*PI) / state->camera.viewplane_width;
   f32 rotation_speed_y = (2.f*PI) / state->camera.viewplane_height;

   // delta in pixels
   f32 delta_x = (f32)state->input.mouse_pos[0] - (f32)state->input.mouse_prev_pos[0];
   f32 delta_y = (f32)state->input.mouse_pos[1] - (f32)state->input.mouse_prev_pos[1];

   f32 zoom_speed = 1.f;

   if(state->input.mouse_wheel_state & MOUSE_WHEEL_STATE_UP)
   {
      // closer radius
      state->camera.target_radius -= zoom_speed;
      // prevent flipping
      state->camera.target_radius = max(state->camera.target_radius, 0.0001f);
      state->input.mouse_wheel_state = 0;
   }
   else if(state->input.mouse_wheel_state & MOUSE_WHEEL_STATE_DOWN)
   {
      // further radius
      state->camera.target_radius += zoom_speed;
      state->input.mouse_wheel_state = 0;
   }

   // rotating input affects target azimuth and altitude
   if(state->input.mouse_buttons & MOUSE_BUTTON_STATE_LEFT)
   {
      state->camera.target_azimuth += rotation_speed_x * delta_x;
      state->camera.target_altitude += rotation_speed_y * delta_y;

      const f32 max_altitude = PI / 2.0f - 0.01f;
      if(state->camera.target_altitude > max_altitude) state->camera.target_altitude = max_altitude;
      if(state->camera.target_altitude < -max_altitude) state->camera.target_altitude = -max_altitude;

      // stop when rotating
      state->camera.target_radius = state->camera.smoothed_radius;
      state->input.mouse_wheel_state = 0;
   }

   // smooth damping
   state->camera.smoothed_azimuth += (state->camera.target_azimuth - state->camera.smoothed_azimuth) * (f32)smoothing_factor;
   state->camera.smoothed_altitude += (state->camera.target_altitude - state->camera.smoothed_altitude) * (f32)smoothing_factor;
   state->camera.smoothed_radius += (state->camera.target_radius - state->camera.smoothed_radius) * (f32)smoothing_factor;

   // use smoothed values for position
   f32 azimuth = state->camera.smoothed_azimuth;
   f32 altitude = state->camera.smoothed_altitude;
   f32 radius = state->camera.smoothed_radius;

   vec3 origin = state->camera.origin;

   f32 x = radius * cosf(altitude) * cosf(azimuth) + origin.x;
   f32 z = radius * cosf(altitude) * sinf(azimuth) + origin.z;
   f32 y = radius * sinf(altitude) + origin.y;

   vec3 eye = {x, y, z};

   state->camera.eye = eye;

   vec3 orbit_dir = vec3_sub(&eye, &origin);
   vec3_normalize(orbit_dir);

   if(state->input.mouse_buttons & MOUSE_BUTTON_STATE_RIGHT)
   {
      vec3 xz = {0};
      vec3 up = {0, 1, 0};

      vec3_cross(orbit_dir, up, xz);
      vec3_normalize(xz);

      xz = vec3_scale(&xz, delta_x);
      up = vec3_scale(&up, delta_y);

      xz = vec3_scale(&xz, (f32)(smoothing_factor * smoothing_factor));
      up = vec3_scale(&up, (f32)(smoothing_factor * smoothing_factor));

      state->camera.origin = vec3_sub(&xz, &state->camera.origin);
      state->camera.origin = vec3_add(&up, &state->camera.origin);
   }

   state->camera.dir = orbit_dir;

   // update previous mouse position and store latest radius
   state->input.mouse_prev_pos[0] = state->input.mouse_pos[0];
   state->input.mouse_prev_pos[1] = state->input.mouse_pos[1];
   state->camera.smoothed_radius = radius;
}

void app_camera_set(app_camera* camera, vec3 origin, f32 radius, f32 altitude, f32 azimuth)
{
   f32 x = radius * cosf(altitude) * cosf(azimuth) + origin.x;
   f32 z = radius * cosf(altitude) * sinf(azimuth) + origin.z;
   f32 y = radius * sinf(altitude) + origin.y;
   vec3 eye = {x, y, z};

   camera->origin = origin;
   camera->eye = eye;
   camera->dir = vec3_sub(&eye, &origin);
   camera->smoothed_radius = radius;
   camera->target_radius = radius;

   camera->smoothed_azimuth = azimuth;
   camera->target_azimuth = azimuth;
   camera->smoothed_altitude = altitude;
   camera->target_altitude = altitude;
}

void app_camera_eye_set(app_camera* camera, vec3 origin, vec3 eye)
{
   camera->origin = origin;
   camera->eye = eye;
   camera->dir = vec3_sub(&eye, &origin);
}

static void app_input_handle(app_state* state)
{
   // TODO: hanlde exit also here - currently it is in win32 layer
   if(state->input.keys['F'] & KEY_STATE_RELEASED)
      state->fullscreen = !state->fullscreen;
   if(state->input.keys['P'] & KEY_STATE_RELEASED)
      state->draw_axis = !state->draw_axis;
   if(state->input.keys['M'] & KEY_STATE_RELEASED)
      state->render_rtx = !state->render_rtx;
   if(state->input.keys['N'] & KEY_STATE_RELEASED)
      state->draw_normals = !state->draw_normals;
   if(state->input.keys['X'] & KEY_STATE_RELEASED)
      state->do_postprocess = !state->do_postprocess;
   if(state->input.keys['R'] & KEY_STATE_RELEASED)
   {
      f32 altitude = PI / 8.f;
      f32 azimuth = PI * 2.f;
      vec3 origin = {0, 0, 0};
      app_camera_set(&state->camera, origin, 4.0f, altitude, azimuth);
      state->input.key_state = 0;
   }
   if(state->input.keys['C'] & KEY_STATE_RELEASED)
   {
      // no sticky key
      state->input.key_state = 0;
      state->camera.update_orbit = !state->camera.update_orbit;

      state->camera.smoothed_altitude = 0;
      state->camera.smoothed_azimuth = 0;
      state->camera.smoothed_radius = 0;
   }

   if(state->camera.update_orbit)
      app_orbit_camera_update(state);
   else
      app_fps_camera_update(state);
}

void app_start(hw* hw, s8 asset_file)
{
   int w = 800, h = 600;
	int x = 100, y = 100;
   const char* win_name = "";

   if(!hw_window_open(hw, win_name, x, y, w, h))
   {
      printf("Could not open window: %s\n", win_name);
      return;
   }

   hw->state.asset.file_name = asset_file;

   if(!vk_initialize(hw))
   {
      printf("Could not initialize all the required subsystems for Vulkan backend\n");
      return;
   }

   printf("Vulkan backend initialized succesfully!\n");

   hw_event_loop_start(hw, app_frame, app_input_handle);
   vk_uninitialize(hw);

   printf("Vulkan backend uninitialized succesfully!\n");

   hw_window_close(hw);
}
