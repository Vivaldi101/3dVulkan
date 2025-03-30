#include "vulkan.h"
#include "common.h"

static bool vk_window_surface_create(vk_context* context, const hw_window* window, const char** extension_names, usize extension_count)
{
   bool isWin32Surface = false;

   for(usize i = 0; i < extension_count; ++i)
      if(strcmp(extension_names[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
         isWin32Surface = true;
         break;
      }

   if(!isWin32Surface)
      return false;

   PFN_vkCreateWin32SurfaceKHR vkWin32SurfaceFunction = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(context->instance, "vkCreateWin32SurfaceKHR");

   if(!vkWin32SurfaceFunction)
      return false;

   VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = {0};
   win32SurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
   win32SurfaceInfo.hinstance = GetModuleHandleA(0);
   win32SurfaceInfo.hwnd = window->handle;

   vkWin32SurfaceFunction(context->instance, &win32SurfaceInfo, 0, &context->surface);

   return true;
}
