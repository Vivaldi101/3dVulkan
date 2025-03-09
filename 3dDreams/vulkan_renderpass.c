#include "common.h"
#include "vulkan.h"

static VkAttachmentDescription vulkan_default_attachment(vulkan_context* context)
{
   VkAttachmentDescription attachment = {};
   attachment.samples = VK_SAMPLE_COUNT_1_BIT;
   attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

   return attachment;
}

static b32 vulkan_renderpass_create(vulkan_context* context)
{
   VkSubpassDescription subpass = {};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

   VkAttachmentDescription attachements[2];

   // TODO: Compress into default attachement
   VkAttachmentDescription color_attachment = vulkan_default_attachment(context);
   color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   color_attachment.format = context->swapchain.image_format.format;

   attachements[0] = color_attachment;

   VkAttachmentReference color_reference = {};
   color_reference.attachment = 0;
   color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &color_reference;

   VkAttachmentDescription depth_attachment = vulkan_default_attachment(context);
   depth_attachment.format = context->device.depth_format;
   depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

   attachements[1] = depth_attachment;

   VkAttachmentReference depth_reference = {};
   depth_reference.attachment = 1;
   depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

   subpass.pDepthStencilAttachment = &depth_reference;

   VkSubpassDependency subpass_deps = {};
   subpass_deps.srcSubpass = VK_SUBPASS_EXTERNAL;
   subpass_deps.dstSubpass = 0;
   subpass_deps.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   subpass_deps.srcAccessMask = 0;
   subpass_deps.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   subpass_deps.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

   VkRenderPassCreateInfo render_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
   render_info.attachmentCount = array_count(attachements);
   render_info.pAttachments = attachements;
   render_info.subpassCount = 1;
   render_info.pSubpasses = &subpass;
   render_info.dependencyCount = 1;
   render_info.pDependencies = &subpass_deps;

   context->swapchain.attachment_count = array_count(attachements);

   if(!VK_VALID(vkCreateRenderPass(context->device.logical_device, &render_info, context->allocator, &context->main_renderpass.handle)))
      return false;

   return true;
}

static void vulkan_renderpass_begin(vulkan_renderpass* renderpass, VkCommandBuffer command_buffer, vulkan_framebuffer* framebuffer)
{
   VkRenderPassBeginInfo begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
   begin_info.renderPass = renderpass->handle;
   begin_info.framebuffer = framebuffer->handle;
   begin_info.renderArea.offset.x = renderpass->viewport.x;
   begin_info.renderArea.offset.y = renderpass->viewport.y;
   begin_info.renderArea.extent.width = renderpass->viewport.w;
   begin_info.renderArea.extent.height = renderpass->viewport.h;

   VkClearValue clear_values[2] = {};
   for(size i = 0; i < array_count(renderpass->clear_color); ++i)
      clear_values[0].color.float32[i] = renderpass->clear_color[i];

   clear_values[1].depthStencil.depth = renderpass->depth;
   clear_values[1].depthStencil.stencil = renderpass->stencil;

   begin_info.clearValueCount = 2;
   begin_info.pClearValues = clear_values;

   vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

static void vulkan_renderpass_end(vulkan_renderpass* renderpass, VkCommandBuffer command_buffer)
{
   vkCmdEndRenderPass(command_buffer);
}
