#include "arena.h"
#include "common.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "win32_file_io.c" // TODO: pass these as function pointers from the platform
#include "vulkan_spirv_loader.c"

#pragma comment(lib,	"vulkan-1.lib")

enum { MAX_VULKAN_OBJECT_COUNT = 16, OBJECT_SHADER_COUNT = 2 };

// TODO: Why release wont build

#define vk_valid_handle(v) ((v) != VK_NULL_HANDLE)
#define vk_valid_format(v) ((v) != VK_FORMAT_UNDEFINED)

#define vk_valid(v) ((v) == VK_SUCCESS)
#define vk_test_return(v) if(!vk_valid((v))) return false
#define vk_test_return_handle(v) if(!vk_valid((v))) return VK_NULL_HANDLE

#define vk_info(i) VK_STRUCTURE_TYPE_##i##_CREATE_INFO
#define vk_info_allocate(i) VK_STRUCTURE_TYPE_##i##_ALLOCATE_INFO
#define vk_info_khr(i) VK_STRUCTURE_TYPE_##i##_CREATE_INFO_KHR

#define vk_info_begin(i) VK_STRUCTURE_TYPE_##i##_BEGIN_INFO
#define vk_info_end(i) VK_STRUCTURE_TYPE_##i##_END_INFO

#ifdef _DEBUG
#define vk_assert(v) \
        do { \
          VkResult _r = (v); \
          assert(vk_valid(_r)); \
        } while(0)
#else
#define vk_assert(v) (v)
#endif

align_struct swapchain_surface_info
{
   u32 image_width;
   u32 image_height;
   u32 image_count;
   VkFormat format;
   VkSurfaceTransformFlagBitsKHR transform;
   VkSurfaceKHR surface;
} swapchain_surface_info;

align_struct
{
   VkShaderModule vs;
   VkShaderModule fs;
} vulkan_shader_modules;

align_struct
{
   arena* storage;

   VkInstance instance;
   VkPhysicalDevice pdev;
   VkDevice ldev;
   VkSurfaceKHR surface;
   VkSwapchainKHR swapchain;
   VkAllocationCallbacks* allocator;
   VkSemaphore image_ready_semaphore;
   VkSemaphore image_done_semaphore;
   VkQueue graphics_queue;
   VkCommandPool command_pool;
   VkCommandBuffer command_buffer;
   VkRenderPass renderpass;
   VkPipeline pipeline;
   VkFramebuffer framebuffers[MAX_VULKAN_OBJECT_COUNT];
   VkImage swapchain_images[MAX_VULKAN_OBJECT_COUNT];
   VkImageView swapchain_image_views[MAX_VULKAN_OBJECT_COUNT];

   swapchain_surface_info swapchain_info;
} vulkan_context;

static VkFormat vulkan_swapchain_format(VkPhysicalDevice pdev, VkSurfaceKHR surface)
{
   assert(vk_valid_handle(pdev));
   assert(vk_valid_handle(surface));

   VkSurfaceFormatKHR formats[MAX_VULKAN_OBJECT_COUNT] = {};
   u32 format_count = array_count(formats);
   if(!vk_valid(vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surface, &format_count, formats)))
      return VK_FORMAT_UNDEFINED;

   // for now expect rgba unorm
   assert(formats[0].format == VK_FORMAT_R8G8B8A8_UNORM);
   return formats[0].format;
}

static swapchain_surface_info vulkan_window_swapchain_surface_info(VkPhysicalDevice pdev, u32 width, u32 height, VkSurfaceKHR surface)
{
   assert(vk_valid_handle(pdev));
   assert(vk_valid_handle(surface));

   swapchain_surface_info result = {};

   VkSurfaceCapabilitiesKHR surface_caps;
   if(!vk_valid(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surface, &surface_caps)))
      return (swapchain_surface_info){0};

   // triple buffering
   if(surface_caps.minImageCount < 2)
      return (swapchain_surface_info){0};

   if(!implies(surface_caps.maxImageCount != 0, surface_caps.maxImageCount >= surface_caps.minImageCount + 1))
      return (swapchain_surface_info){0};

   u32 image_count = surface_caps.minImageCount + 1;

   result.image_width = width;
   result.image_height = height;
   result.image_count = image_count;
   result.transform = surface_caps.currentTransform;
   result.format = vulkan_swapchain_format(pdev, surface);
   result.surface = surface;

   // wp(imgcount = min + 1, imgcount >= 3)
   // min + 1 >= 3
   // min >= 2

   // wp(imgcount = min + 1, maximgcount == 0 || maximgcount >= imgcount)
   // (maximgcount == 0 || maximgcount >= min + 1)

   // (maximgcount == 0 || maximgcount >= min + 1)
   // implies(maximgcount != 0, maximgcount >= min + 1)

   return result;
}

static VkPhysicalDevice vulkan_pdevice_select(VkInstance instance)
{
   assert(vk_valid_handle(instance));

   VkPhysicalDevice devs[MAX_VULKAN_OBJECT_COUNT] = {};
   u32 dev_count = array_count(devs);
   vk_test_return_handle(vkEnumeratePhysicalDevices(instance, &dev_count, devs));

   for(u32 i = 0; i < dev_count; ++i)
   {
      VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(devs[i], &props);

      if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
         return devs[i];
   }
   if(dev_count > 0)
      return devs[0];
   return 0;
}

static u32 vulkan_ldevice_select_index()
{
   // placeholder
   return 0;
}

static VkDevice vulkan_ldevice_create(VkPhysicalDevice pdev, u32 queue_family_index)
{
   assert(vk_valid_handle(pdev));
   f32 queue_prio = 1.0f;

   VkDeviceQueueCreateInfo queue_info = {vk_info(DEVICE_QUEUE)};
   queue_info.queueFamilyIndex = queue_family_index; // TODO: query the right queue family
   queue_info.queueCount = 1;
   queue_info.pQueuePriorities = &queue_prio;

   VkDeviceCreateInfo ldev_info = {vk_info(DEVICE)};
   const char* dev_ext_names[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
   ldev_info.queueCreateInfoCount = 1;
   ldev_info.pQueueCreateInfos = &queue_info;
   ldev_info.enabledExtensionCount = array_count(dev_ext_names);
   ldev_info.ppEnabledExtensionNames = dev_ext_names;

   VkDevice ldev;
   vk_test_return_handle(vkCreateDevice(pdev, &ldev_info, 0, &ldev));

   return ldev;
}

static VkQueue vulkan_graphics_queue_create(VkDevice ldev, u32 queue_family_index)
{
   assert(vk_valid_handle(ldev));

   VkQueue graphics_queue = 0;

   // TODO: Get the queue index
   vkGetDeviceQueue(ldev, queue_family_index, 0, &graphics_queue);

   return graphics_queue;
}

static VkSemaphore vulkan_semaphore_create(VkDevice ldev)
{
   assert(vk_valid_handle(ldev));

   VkSemaphore sema = 0;

   VkSemaphoreCreateInfo sema_info = {vk_info(SEMAPHORE)};
   vk_test_return_handle(vkCreateSemaphore(ldev, &sema_info, 0, &sema));

   return sema;
}

static VkSwapchainKHR vulkan_swapchain_create(VkDevice ldev, swapchain_surface_info* surface_info, u32 queue_family_index)
{
   assert(vk_valid_handle(ldev));
   assert(vk_valid_handle(surface_info->surface));
   assert(vk_valid_format(surface_info->format));

   VkSwapchainKHR swapchain = 0;

   VkSwapchainCreateInfoKHR swapchain_info = {vk_info_khr(SWAPCHAIN)};
   swapchain_info.surface = surface_info->surface;
   swapchain_info.minImageCount = surface_info->image_count;
   swapchain_info.imageFormat = surface_info->format;
   swapchain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
   swapchain_info.imageExtent.width = surface_info->image_width;
   swapchain_info.imageExtent.height = surface_info->image_height;
   swapchain_info.imageArrayLayers = 1;
   swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
   swapchain_info.queueFamilyIndexCount = 1;
   swapchain_info.pQueueFamilyIndices = &queue_family_index;
   swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
   swapchain_info.preTransform = surface_info->transform;
   swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   swapchain_info.clipped = VK_TRUE;
   swapchain_info.oldSwapchain = swapchain;

   vk_test_return_handle(vkCreateSwapchainKHR(ldev, &swapchain_info, 0, &swapchain));

   return swapchain;
}

static VkCommandBuffer vulkan_command_buffer_create(VkDevice ldev, VkCommandPool pool)
{
   assert(vk_valid_handle(ldev));
   assert(vk_valid_handle(pool));

   VkCommandBuffer buffer = 0;
   VkCommandBufferAllocateInfo buffer_allocate_info = {vk_info_allocate(COMMAND_BUFFER)};

   buffer_allocate_info.commandBufferCount = 1;
   buffer_allocate_info.commandPool = pool;
   buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

   vk_assert(vkAllocateCommandBuffers(ldev, &buffer_allocate_info, &buffer));

   return buffer;
}

static VkCommandPool vulkan_command_pool_create(VkDevice ldev, u32 queue_family_index)
{
   assert(vk_valid_handle(ldev));

   VkCommandPool pool = 0;

   VkCommandPoolCreateInfo pool_info = {vk_info(COMMAND_POOL)};
   pool_info.queueFamilyIndex = queue_family_index;

   vk_test_return_handle(vkCreateCommandPool(ldev, &pool_info, 0, &pool));

   return pool;
}

static VkRenderPass vulkan_renderpass_create(VkDevice ldev, swapchain_surface_info* surface_info)
{
   assert(vk_valid_handle(ldev));
   assert(vk_valid_format(surface_info->format));

   VkRenderPass renderpass = 0;

   u32 attachment_index = 0;
   VkAttachmentReference attachment_ref = {attachment_index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}; 

   VkSubpassDescription subpass = {};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &attachment_ref;

   VkAttachmentDescription attachments[1] = {};

   attachments[attachment_index].format = surface_info->format;
   attachments[attachment_index].samples = VK_SAMPLE_COUNT_1_BIT;
   attachments[attachment_index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   attachments[attachment_index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   attachments[attachment_index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   attachments[attachment_index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   attachments[attachment_index].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   attachments[attachment_index].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   VkRenderPassCreateInfo renderpass_info = {vk_info(RENDER_PASS)}; 
   renderpass_info.subpassCount = 1;
   renderpass_info.pSubpasses = &subpass;
   renderpass_info.attachmentCount = array_count(attachments);
   renderpass_info.pAttachments = attachments;

   vk_test_return_handle(vkCreateRenderPass(ldev, &renderpass_info, 0, &renderpass));

   return renderpass;
}

static VkFramebuffer vulkan_framebuffer_create(VkDevice ldev, VkRenderPass renderpass, swapchain_surface_info* surface_info, VkImageView image_view)
{
   VkFramebuffer framebuffer = 0;

   VkFramebufferCreateInfo framebuffer_info = {vk_info(FRAMEBUFFER)};
   framebuffer_info.renderPass = renderpass;
   framebuffer_info.attachmentCount = 1;
   framebuffer_info.pAttachments = &image_view;
   framebuffer_info.width = surface_info->image_width;
   framebuffer_info.height = surface_info->image_height;
   framebuffer_info.layers = 1;

   vk_test_return_handle(vkCreateFramebuffer(ldev, &framebuffer_info, 0, &framebuffer));

   return framebuffer;
}

static VkImageView vulkan_image_view_create(VkDevice ldev, VkFormat format, VkImage image)
{
   assert(vk_valid_handle(ldev));
   assert(vk_valid_format(format));
   assert(vk_valid_handle(image));

   VkImageView image_view = 0;

   VkImageViewCreateInfo view_info = {vk_info(IMAGE_VIEW)};
   view_info.image = image;
   view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
   view_info.format = format;
   view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   view_info.subresourceRange.layerCount = 1;
   view_info.subresourceRange.levelCount = 1;

   vk_test_return_handle(vkCreateImageView(ldev, &view_info, 0, &image_view));

   return image_view;
}

void vulkan_resize(void* renderer, u32 width, u32 height)
{
   // ...
}

void vulkan_present(vulkan_context* context)
{
   u32 image_index = 0;

   vk_assert(vkAcquireNextImageKHR(context->ldev, context->swapchain, UINT64_MAX, context->image_ready_semaphore, VK_NULL_HANDLE, &image_index));
   vk_assert(vkResetCommandPool(context->ldev, context->command_pool, 0));

   VkCommandBufferBeginInfo buffer_begin_info = {vk_info_begin(COMMAND_BUFFER)};
   buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   VkRenderPassBeginInfo renderpass_info = {vk_info_begin(RENDER_PASS)};
   renderpass_info.renderPass = context->renderpass;
   renderpass_info.framebuffer = context->framebuffers[image_index];
   renderpass_info.renderArea.extent = (VkExtent2D)
   { context->swapchain_info.image_width, context->swapchain_info.image_height };

   VkCommandBuffer buffer = context->command_buffer;
   {
      vk_assert(vkBeginCommandBuffer(buffer, &buffer_begin_info));

      const f32 c = 255.0f, r = 48, g = 10, b = 36;
      VkClearValue clear = {r / c, g / c, b / c, 1.0f};

      renderpass_info.clearValueCount = 1;
      renderpass_info.pClearValues = &clear;

      vkCmdBeginRenderPass(buffer, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);

      VkViewport viewport = {};
      viewport.x = 0.0f;
      viewport.y = (f32)context->swapchain_info.image_height;
      viewport.width = (f32)context->swapchain_info.image_width;
      viewport.height = -(f32)context->swapchain_info.image_height;
      viewport.minDepth = 0;
      viewport.maxDepth = 1;

      vkCmdSetViewport(buffer, 0, 1, &viewport);

      VkRect2D scissor = {};
      scissor.offset.x = 0;
      scissor.offset.y = 0;
      scissor.extent.width = (u32)context->swapchain_info.image_width;
      scissor.extent.height = (u32)context->swapchain_info.image_height;
      vkCmdSetScissor(buffer, 0, 1, &scissor);

      vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline);
      vkCmdDraw(buffer, 3, 1, 0, 0);

      vkCmdEndRenderPass(buffer);

      vk_assert(vkEndCommandBuffer(buffer));
   }

   VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
   submit_info.waitSemaphoreCount = 1;
   submit_info.pWaitSemaphores = &context->image_ready_semaphore;

   submit_info.pWaitDstStageMask = &(VkPipelineStageFlags) { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &buffer;

   submit_info.signalSemaphoreCount = 1;
   submit_info.pSignalSemaphores = &context->image_done_semaphore;

   vk_assert(vkQueueSubmit(context->graphics_queue, 1, &submit_info, VK_NULL_HANDLE));

   VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
   present_info.swapchainCount = 1;
   present_info.pSwapchains = &context->swapchain;

   present_info.pImageIndices = &image_index;

   present_info.waitSemaphoreCount = 1;
   present_info.pWaitSemaphores = &context->image_done_semaphore;

   vk_assert(vkQueuePresentKHR(context->graphics_queue, &present_info));

   // wait until all queue ops are done
   vk_assert(vkDeviceWaitIdle(context->ldev));
}

static vulkan_shader_modules vulkan_shaders_load(VkDevice ldev, arena scratch)
{
   assert(vk_valid_handle(ldev));

   vulkan_shader_modules shader_modules = {};
   VkShaderStageFlagBits shader_type_bits[OBJECT_SHADER_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

   file_result shader_dir = vulkan_shader_directory(&scratch);

   for(u32 i = 0; i < OBJECT_SHADER_COUNT; ++i)
   {
      file_result shader_file = vulkan_shader_spv_read(&scratch, shader_dir.data, shader_type_bits[i]);
      if(shader_file.file_size == 0)
         return (vulkan_shader_modules){};

      VkShaderModuleCreateInfo module_info = {};
      module_info.sType = vk_info(SHADER_MODULE);
      module_info.pCode = (u32*)shader_file.data;
      module_info.codeSize = shader_file.file_size;

      if(!vk_valid(vkCreateShaderModule(ldev, 
                           &module_info, 
                           0,
                           shader_type_bits[i] == VK_SHADER_STAGE_VERTEX_BIT ? &shader_modules.vs : &shader_modules.fs)))
         return (vulkan_shader_modules){};
   }

   return shader_modules;
}

static VkPipelineLayout vulkan_pipeline_layout_create(VkDevice ldev)
{
   VkPipelineLayout layout = 0;

   VkPipelineLayoutCreateInfo info = {vk_info(PIPELINE_LAYOUT)};
   vk_test_return(vkCreatePipelineLayout(ldev, &info, 0, &layout));

   return layout;
}

static VkPipeline vulkan_pipeline_create(VkDevice ldev, VkRenderPass renderpass, VkPipelineCache cache, VkPipelineLayout layout, const vulkan_shader_modules* shaders)
{
   assert(vk_valid_handle(ldev));
   assert(vk_valid_handle(shaders->fs));
   assert(vk_valid_handle(shaders->fs));
   assert(!vk_valid_handle(cache));

   VkPipeline pipeline = 0;

   VkPipelineShaderStageCreateInfo stages[OBJECT_SHADER_COUNT] = {};

   stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
   stages[0].module = shaders->vs;
   stages[0].pName = "main";

   stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
   stages[1].module = shaders->fs;
   stages[1].pName = "main";

   VkGraphicsPipelineCreateInfo pipeline_info = {vk_info(GRAPHICS_PIPELINE)};
   pipeline_info.stageCount = array_count(stages);
   pipeline_info.pStages = stages;

   VkPipelineVertexInputStateCreateInfo vertex_input_info = {vk_info(PIPELINE_VERTEX_INPUT_STATE)};
   pipeline_info.pVertexInputState = &vertex_input_info;

   VkPipelineInputAssemblyStateCreateInfo assembly_info = {vk_info(PIPELINE_INPUT_ASSEMBLY_STATE)};
   assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   pipeline_info.pInputAssemblyState = &assembly_info;

   VkPipelineViewportStateCreateInfo viewport_info = {vk_info(PIPELINE_VIEWPORT_STATE)};
   viewport_info.scissorCount = 1;
   viewport_info.viewportCount = 1;
   pipeline_info.pViewportState = &viewport_info;

   VkPipelineRasterizationStateCreateInfo raster_info = {vk_info(PIPELINE_RASTERIZATION_STATE)};
   raster_info.lineWidth = 1.0f;
   pipeline_info.pRasterizationState = &raster_info;

   VkPipelineMultisampleStateCreateInfo sample_info = {vk_info(PIPELINE_MULTISAMPLE_STATE)};
   sample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   pipeline_info.pMultisampleState = &sample_info;

   VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {vk_info(PIPELINE_DEPTH_STENCIL_STATE)};
   depth_stencil_info.depthBoundsTestEnable = VK_TRUE;
   depth_stencil_info.depthTestEnable = VK_TRUE;
   depth_stencil_info.depthWriteEnable = VK_TRUE;
   depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
   depth_stencil_info.minDepthBounds = 0;
   depth_stencil_info.maxDepthBounds = 0;
   pipeline_info.pDepthStencilState = &depth_stencil_info;

   VkPipelineColorBlendAttachmentState color_blend_attachment = {};
   color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

   VkPipelineColorBlendStateCreateInfo color_blend_info = {vk_info(PIPELINE_COLOR_BLEND_STATE)};
   color_blend_info.attachmentCount = 1;
   color_blend_info.pAttachments = &color_blend_attachment;
   pipeline_info.pColorBlendState = &color_blend_info;

   VkPipelineDynamicStateCreateInfo dynamic_info = {vk_info(PIPELINE_DYNAMIC_STATE)};
   dynamic_info.pDynamicStates = (VkDynamicState[2]){VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
   dynamic_info.dynamicStateCount = 2;
   pipeline_info.pDynamicState = &dynamic_info;

   pipeline_info.renderPass = renderpass;
   pipeline_info.layout = layout;

   vk_test_return(vkCreateGraphicsPipelines(ldev, cache, 1, &pipeline_info, 0, &pipeline));

   return pipeline;
}

bool vulkan_initialize(hw* hw)
{
   if(!hw->renderer.window.handle)
      return false;

   vulkan_context* context = new(&hw->vulkan_storage, vulkan_context);
   if(arena_end(&hw->vulkan_storage, context))
      return false;

   context->storage = &hw->vulkan_storage;

   VkInstanceCreateInfo instance_info = {vk_info(INSTANCE)};
   instance_info.pApplicationInfo = &(VkApplicationInfo) { .apiVersion = VK_API_VERSION_1_2 };

   u32 ext_count = 0;
   if(!vk_valid(vkEnumerateInstanceExtensionProperties(0, &ext_count, 0)))
      return false;

   arena scratch = hw->vulkan_scratch;
   VkExtensionProperties* ext = new(&scratch, VkExtensionProperties, ext_count);
   if(!implies(!scratch_end(scratch, ext), vk_valid(vkEnumerateInstanceExtensionProperties(0, &ext_count, ext))))
      return false;
   const char** ext_names = new(&scratch, const char*, ext_count);

   for(size_t i = 0; i < ext_count; ++i)
      ext_names[i] = ext[i].extensionName;

   instance_info.enabledExtensionCount = ext_count;
   instance_info.ppEnabledExtensionNames = ext_names;

#ifdef _DEBUG
   {
      const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
      instance_info.enabledLayerCount = array_count(validationLayers);
      instance_info.ppEnabledLayerNames = validationLayers;
   }
#endif

   VkInstance instance = 0;
   vk_test_return(vkCreateInstance(&instance_info, 0, &instance));

   u32 queue_family_index = vulkan_ldevice_select_index();

   context->pdev = vulkan_pdevice_select(instance);
   context->ldev = vulkan_ldevice_create(context->pdev, queue_family_index);
   context->surface = hw->renderer.window_surface_create(instance, hw->renderer.window.handle);
   context->swapchain_info = vulkan_window_swapchain_surface_info(context->pdev, hw->renderer.window.width, hw->renderer.window.height, context->surface);
   context->swapchain = vulkan_swapchain_create(context->ldev, &context->swapchain_info, queue_family_index);
   context->image_ready_semaphore = vulkan_semaphore_create(context->ldev);
   context->image_done_semaphore = vulkan_semaphore_create(context->ldev);
   context->graphics_queue = vulkan_graphics_queue_create(context->ldev, queue_family_index);
   context->command_pool = vulkan_command_pool_create(context->ldev, queue_family_index);
   context->command_buffer = vulkan_command_buffer_create(context->ldev, context->command_pool);
   context->renderpass = vulkan_renderpass_create(context->ldev, &context->swapchain_info);

   vk_test_return(vkGetSwapchainImagesKHR(context->ldev, context->swapchain, &context->swapchain_info.image_count, context->swapchain_images));

   for(u32 i = 0; i < context->swapchain_info.image_count; ++i)
   {
      context->swapchain_image_views[i] = vulkan_image_view_create(context->ldev, context->swapchain_info.format, context->swapchain_images[i]);
      context->framebuffers[i] = vulkan_framebuffer_create(context->ldev, context->renderpass, &context->swapchain_info, context->swapchain_image_views[i]);
   }

   vulkan_shader_modules shaders = vulkan_shaders_load(context->ldev, scratch);
   VkPipelineCache cache = 0; // TODO: enable
   VkPipelineLayout layout = vulkan_pipeline_layout_create(context->ldev);
   context->pipeline = vulkan_pipeline_create(context->ldev, context->renderpass, cache, layout, &shaders);

   // app callbacks
   hw->renderer.backends[vulkan_renderer_index] = context;
   hw->renderer.frame_present = vulkan_present;
   hw->renderer.frame_resize = vulkan_resize;
   hw->renderer.renderer_index = vulkan_renderer_index;

   return true;
}

bool vulkan_uninitialize(hw* hw)
{
#if 0
   vulkan_context* context = hw->renderer.backends[vulkan_renderer_index];

   vkDestroySwapchainKHR(context->ldev, context->swapchain, 0);
   vkDestroySurfaceKHR(context->instance, context->surface, 0);
   vkDestroyDevice(context->ldev, 0);
   vkDestroyInstance(context->instance, 0);
#endif

   return true;
}
