#include "vulkan.h"
#include "common.h"

#include "arena.h"

static b32 vulkan_framebuffer_create(vulkan_context* context)
{
   for(u32 i = 0; i < context->swapchain.image_count; ++i)
   {
      VkImageView attachments[] = {context->swapchain.views[i], context->swapchain.depth_attachment.view};
      vulkan_framebuffer* framebuffer = &context->swapchain.framebuffers[i];

      framebuffer->attachments = attachments;
      framebuffer->attachment_count = array_count(attachments);

      VkFramebufferCreateInfo framebuffer_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
      framebuffer_info.width = context->swapchain.support.surface_capabilities.currentExtent.width;
      framebuffer_info.height = context->swapchain.support.surface_capabilities.currentExtent.height;
      framebuffer_info.layers = 1;
      framebuffer_info.attachmentCount = framebuffer->attachment_count;
      framebuffer_info.pAttachments = framebuffer->attachments;
      framebuffer_info.renderPass = context->main_renderpass.handle;

      if(!VK_VALID(vkCreateFramebuffer(context->device.logical_device, &framebuffer_info, context->allocator, &framebuffer->handle)))
         return false;
   }

   return true;
}
