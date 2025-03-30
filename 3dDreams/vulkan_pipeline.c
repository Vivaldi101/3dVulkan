#include "vulkan.h"
#include "common.h"
#include "graphics.h"

bool vulkan_pipeline_create(vulkan_context* context)
{
#define ATTRIBUTE_COUNT 1
   u32 attribute_offset = 0;
   VkVertexInputAttributeDescription attribute_descriptions[ATTRIBUTE_COUNT];
   {
      VkFormat formats[ATTRIBUTE_COUNT] = {VK_FORMAT_R32G32B32_SFLOAT};
      u32 sizes[ATTRIBUTE_COUNT] = {sizeof(vec3)};
      for(u32 i = 0; i < ATTRIBUTE_COUNT; ++i)
      {
         attribute_descriptions[i].binding = 0;
         attribute_descriptions[i].location = i;
         attribute_descriptions[i].format = formats[i];
         attribute_descriptions[i].offset = attribute_offset;

         attribute_offset += sizes[i];  // offset by this one 
      }
   }

   VkPipelineShaderStageCreateInfo stage_infos[OBJECT_SHADER_COUNT];
   for(u32 i = 0; i < OBJECT_SHADER_COUNT; ++i)
      stage_infos[i] = context->shader.stages[i].pipeline_create_info;

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

   viewport_info.viewportCount = 1;
   viewport_info.pViewports = &viewport;
   viewport_info.scissorCount = 1;
   viewport_info.pScissors = &scissor;

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
   depth_stencil_info.depthCompareOp = VK_COMPARE_OP_ALWAYS;
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

   VkPipelineColorBlendStateCreateInfo color_blend_state_info = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
   color_blend_state_info.logicOpEnable = VK_FALSE;
   color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
   color_blend_state_info.attachmentCount = 1;
   color_blend_state_info.pAttachments = &color_state;

   VkDynamicState dynamic_states[] = 
   {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
      VK_DYNAMIC_STATE_LINE_WIDTH
   };

   VkPipelineDynamicStateCreateInfo dynamic_create_info = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
   dynamic_create_info.dynamicStateCount = array_count(dynamic_states);
   dynamic_create_info.pDynamicStates = dynamic_states;

   VkVertexInputBindingDescription vertex_binding = {};
   vertex_binding.binding = 0; // binding index
   vertex_binding.stride = sizeof(vertex3);
   vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // per vertex

   VkPipelineVertexInputStateCreateInfo vertex_info = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
   vertex_info.vertexBindingDescriptionCount = 1;
   vertex_info.pVertexBindingDescriptions = &vertex_binding;
   vertex_info.vertexAttributeDescriptionCount = ATTRIBUTE_COUNT;
   vertex_info.pVertexAttributeDescriptions = attribute_descriptions;

   VkPipelineInputAssemblyStateCreateInfo assembly_info = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
   assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   assembly_info.primitiveRestartEnable = VK_FALSE;

   VkPipelineLayoutCreateInfo pipeline_layout_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
   pipeline_layout_info.setLayoutCount = 1;
   pipeline_layout_info.pSetLayouts = &context->shader.global_descriptor_set_layout;

   if(!vk_valid(vkCreatePipelineLayout(context->device.logical_device, &pipeline_layout_info, context->allocator, &context->pipeline.layout)))
      return false;

   VkGraphicsPipelineCreateInfo pipeline_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
   pipeline_info.stageCount = OBJECT_SHADER_COUNT;
   pipeline_info.pStages = stage_infos;
   pipeline_info.pVertexInputState = &vertex_info;
   pipeline_info.pInputAssemblyState = &assembly_info;

   pipeline_info.pViewportState = &viewport_info;
   pipeline_info.pRasterizationState = &raster_info;
   pipeline_info.pMultisampleState = &multisample_info;
   pipeline_info.pDepthStencilState = &depth_stencil_info;
   pipeline_info.pColorBlendState = &color_blend_state_info;
   pipeline_info.pDynamicState = &dynamic_create_info;
   pipeline_info.pTessellationState = 0;

   pipeline_info.layout = context->pipeline.layout;

   pipeline_info.renderPass = context->main_renderpass.handle;
   pipeline_info.subpass = 0;
   pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
   pipeline_info.basePipelineIndex = -1;

   if(!vk_valid(vkCreateGraphicsPipelines(context->device.logical_device, VK_NULL_HANDLE, 1, &pipeline_info, context->allocator, &context->pipeline.handle)))
      return false;

   return true;
}

void vulkan_pipeline_bind(VkCommandBuffer buffer, VkPipelineBindPoint bind_point, VkPipeline pipeline)
{
   vkCmdBindPipeline(buffer, bind_point, pipeline);
}
