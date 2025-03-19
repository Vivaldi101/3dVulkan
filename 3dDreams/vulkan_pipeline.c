#include "vulkan.h"
#include "common.h"

bool vulkan_pipeline_create(vulkan_context* context)
{
   VkPipelineViewportStateCreateInfo viewport_info = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};

   // Store viewport and scissor inside the context?
   VkViewport viewport = {};
   viewport.x = 0.0f;
   viewport.y = (f32)context->framebuffer_height-1.0f;
   viewport.width = (f32)context->framebuffer_width;
   viewport.height = -(f32)context->framebuffer_height;
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;

   VkRect2D scissor = {};
   scissor.offset.x = scissor.offset.y = 0;
   scissor.extent.width = context->framebuffer_width;
   scissor.extent.height = context->framebuffer_height;

   VkPipelineRasterizationStateCreateInfo raster_info = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
   raster_info.depthClampEnable = VK_FALSE;
   raster_info.rasterizerDiscardEnable = VK_FALSE;
   raster_info.polygonMode = VK_POLYGON_MODE_FILL;
   raster_info.lineWidth = 1.0f;
   raster_info.cullMode = VK_CULL_MODE_BACK_BIT;
   raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
   raster_info.depthBiasEnable = VK_FALSE;
   raster_info.depthBiasConstantFactor = 0.0f;
   raster_info.depthBiasClamp = 0.0f;
   raster_info.depthBiasSlopeFactor = 0.0f;

   VkPipelineMultisampleStateCreateInfo multisample_info = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
   multisample_info.sampleShadingEnable = VK_FALSE;
   multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   multisample_info.minSampleShading = 1.0f;
   multisample_info.pSampleMask = 0;
   multisample_info.alphaToCoverageEnable = VK_FALSE;
   multisample_info.alphaToOneEnable = VK_FALSE;

   VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
   depth_stencil_info.depthTestEnable = VK_TRUE;
   depth_stencil_info.depthWriteEnable = VK_TRUE;
   depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
   depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
   depth_stencil_info.stencilTestEnable = VK_FALSE;

   VkPipelineColorBlendAttachmentState color_state = {};
   color_state.blendEnable = VK_TRUE;
   color_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
   color_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   color_state.colorBlendOp = VK_BLEND_OP_ADD;
   color_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
   color_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   color_state.alphaBlendOp = VK_BLEND_OP_ADD;

   color_state.colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

   VkPipelineColorBlendStateCreateInfo blend_state_info = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
   blend_state_info.attachmentCount = 1;
   blend_state_info.pAttachments = &color_state;
   blend_state_info.logicOp = VK_LOGIC_OP_COPY;
   blend_state_info.logicOpEnable = VK_FALSE;


   return true;
}
