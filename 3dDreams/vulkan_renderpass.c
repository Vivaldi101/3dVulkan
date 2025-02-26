#include "common.h"
#include "vulkan.h"

//static bool vulkan_renderpass_create(arena* perm, vulkan_context* context, vulkan_viewport* viewport, f32 clear_color[4], f32 depth, f32 stencil)
static bool vulkan_renderpass_create(arena* perm, vulkan_context* context)
{
   // main subpass
   VkSubpassDescription subpass = {};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

   VkAttachmentDescription attachements[2];

   VkAttachmentDescription color = {};
   color.format = context->swapchain.image_format.format;
   color.samples = VK_SAMPLE_COUNT_1_BIT;
   color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

   attachements[0] = color;

   VkAttachmentReference color_reference = {};
   color_reference.attachment = 0;
   color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &color_reference;

   VkAttachmentDescription depth_attachment = {};
   depth_attachment.format = context->device.depth_format;
   depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
   depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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

   if(!VK_VALID(vkCreateRenderPass(context->device.logical_device, &render_info, context->allocator, &context->renderpass.handle)))
      return false;

   return true;
}
