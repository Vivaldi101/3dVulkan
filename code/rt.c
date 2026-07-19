#include "vk.h"

static bool rt_blas_geometry_build(arena s, vk_context* context, vk_rt_as* as)
{
   vk_geometry* geometry = &context->geometry;
   vk_device* devices = &context->devices;
   vk_buffer_hash_table* buffer_table = &context->buffer_table;
   const size mesh_draws_count = geometry->mesh_draws.count;
   const size alignment = 256;
   size total_acceleration_size = 0;
   size total_scratch_size = 0;

   VkAccelerationStructureGeometryKHR* acceleration_geometries =
      push(&s, typeof(*acceleration_geometries), mesh_draws_count);
   VkAccelerationStructureBuildGeometryInfoKHR* build_infos =
      push(&s, typeof(*build_infos), mesh_draws_count);
   VkAccelerationStructureBuildRangeInfoKHR* build_ranges =
      push(&s, typeof(*build_ranges), mesh_draws_count);
   VkAccelerationStructureBuildRangeInfoKHR** build_range_ptrs =
      push(&s, typeof(*build_range_ptrs), mesh_draws_count);
   size* acceleration_offsets =
      push(&s, size, mesh_draws_count);
   size* scratch_offsets =
      push(&s, size, mesh_draws_count);
   size* acceleration_sizes =
      push(&s, size, mesh_draws_count);

   vk_buffer* ib = buffer_hash_lookup(buffer_table, ib_buffer_name);
   vk_buffer* vb = buffer_hash_lookup(buffer_table, vb_buffer_name);
   VkDeviceAddress ib_address = buffer_device_address(ib, devices);
   VkDeviceAddress vb_address = buffer_device_address(vb, devices);

   for(size i = 0; i < mesh_draws_count; ++i)
   {
      vk_mesh_draw* draw = geometry->mesh_draws.data + i;

      VkAccelerationStructureBuildSizesInfoKHR size_info =
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

      VkAccelerationStructureGeometryKHR* ag = acceleration_geometries + i;

      // vertex format must match VK_FORMAT_R32G32B32_SFLOAT
      static_assert(offsetof(vertex, vz) == offsetof(vertex, vx) + sizeof(float) * 2);

      ag->sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      ag->geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
      ag->flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

      ag->geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
      ag->geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
      ag->geometry.triangles.vertexStride = sizeof(vertex);
      ag->geometry.triangles.maxVertex = ((u32)draw->index_count / 3) - 1;
      ag->geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
      ag->geometry.triangles.vertexData.deviceAddress = vb_address + (draw->vertex_offset * sizeof(vertex));
      ag->geometry.triangles.indexData.deviceAddress = ib_address + (draw->index_offset * sizeof(u32));

      VkAccelerationStructureBuildGeometryInfoKHR build_info =
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
      build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
      build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
      build_info.geometryCount = 1;
      build_info.pGeometries = ag;

      assert((draw->index_count % 3) == 0);
      u32 max_primitive_count = (u32)draw->index_count / 3;

      vkGetAccelerationStructureBuildSizesKHR(devices->logical,
                                              VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                              &build_info, &max_primitive_count, &size_info);

      acceleration_offsets[i] = total_acceleration_size;
      scratch_offsets[i] = total_scratch_size;
      acceleration_sizes[i] = size_info.accelerationStructureSize;

      total_acceleration_size = (total_acceleration_size + size_info.accelerationStructureSize + alignment - 1) & ~(alignment - 1);
      total_scratch_size = (total_scratch_size + size_info.buildScratchSize + alignment - 1) & ~(alignment - 1);
   }

   vk_buffer blas_buffer = {.size = total_acceleration_size};
   if(!vk_buffer_create_and_bind(&blas_buffer, devices,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
      return false;
   buffer_hash_insert(buffer_table, blas_buffer_name, blas_buffer);

   vk_buffer scratch_buffer = {.size = total_scratch_size};
   if(!vk_buffer_create_and_bind(&scratch_buffer, devices,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
      return false;
   VkDeviceAddress scratch_address = buffer_device_address(&scratch_buffer, devices);

   printf("BLAS Ray tracing acceleration structure size: \t%zu KB\n", total_acceleration_size / 1024);
   printf("BLAS Ray tracing build scratch size: \t\t%zu KB\n", total_scratch_size / 1024);

   for(size i = 0; i < mesh_draws_count; ++i)
   {
      vk_mesh_draw* draw = geometry->mesh_draws.data + i;

      VkAccelerationStructureCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
      create_info.buffer = blas_buffer.handle;
      create_info.offset = acceleration_offsets[i];
      create_info.size = acceleration_sizes[i];
      create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

      assert(blas_buffer.size >= create_info.size + create_info.offset);
      assert((create_info.offset & 0xff) == 0);

      VkAccelerationStructureKHR blas = 0;
      if(!vk_valid(vkCreateAccelerationStructureKHR(devices->logical, &create_info, &global_allocator.handle, &blas)))
         return false;

      array_add(as->blases, blas);

      u32 max_primitive_count = (u32)draw->index_count / 3;

      build_infos[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
      build_infos[i].type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      build_infos[i].flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
      build_infos[i].mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
      build_infos[i].dstAccelerationStructure = as->blases.data[i];
      build_infos[i].geometryCount = 1;
      build_infos[i].pGeometries = acceleration_geometries + i;
      build_infos[i].scratchData.deviceAddress = scratch_address + scratch_offsets[i];

      build_ranges[i].primitiveCount = max_primitive_count;
      build_ranges[i].primitiveOffset = 0;
      build_ranges[i].firstVertex = 0;
      build_ranges[i].transformOffset = 0;

      build_range_ptrs[i] = build_ranges + i;
   }

   if(!vk_valid(vkResetCommandPool(devices->logical, context->cmd.pool, 0)))
      return false;

   VkCommandBufferBeginInfo buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
   buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   if(!vk_valid(vkBeginCommandBuffer(context->cmd.buffer, &buffer_begin_info)))
      return false;

   vkCmdBuildAccelerationStructuresKHR(context->cmd.buffer, (u32)mesh_draws_count, build_infos, build_range_ptrs);

   if(!vk_valid(vkEndCommandBuffer(context->cmd.buffer)))
      return false;

   VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &context->cmd.buffer;

   if(!vk_valid(vkQueueSubmit(context->graphics_queue, 1, &submit_info, VK_NULL_HANDLE)))
      return false;

   if(!vk_valid(vkDeviceWaitIdle(devices->logical)))
      return false;

   vk_buffer_destroy(devices, &scratch_buffer);

   return true;
}

static bool rt_tlas_geometry_build(vk_context* context, vk_rt_as* as)
{
   vk_buffer_hash_table* buffer_table = &context->buffer_table;
   vk_geometry* geometry = &context->geometry;
   vk_device* devices = &context->devices;
   const size draw_count = geometry->mesh_draws.count;

   assert(context->rt_as.blases.count == draw_count);

   vk_buffer instance_buffer = {.size = draw_count * sizeof(VkAccelerationStructureInstanceKHR)};
   if(!vk_buffer_create_and_bind(&instance_buffer, devices,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
      return false;

   VkAccelerationStructureGeometryKHR ag = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
   ag.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
   ag.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
   ag.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
   ag.geometry.instances.data.deviceAddress = buffer_device_address(&instance_buffer, devices);

   VkAccelerationStructureBuildGeometryInfoKHR build_info =
   {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
   build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
   build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
   build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
   build_info.geometryCount = 1;
   build_info.pGeometries = &ag;

   VkAccelerationStructureBuildSizesInfoKHR size_info =
   {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

   vkGetAccelerationStructureBuildSizesKHR(devices->logical,
                                           VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                           &build_info, &(u32)draw_count, &size_info);

   assert(geometry->mesh_draws.count == geometry->mesh_instances.count); // TODO: Fix this

   VkAccelerationStructureInstanceKHR* instances = (VkAccelerationStructureInstanceKHR*)instance_buffer.data;

   for(size i = 0; i < draw_count; ++i)
   {
      VkAccelerationStructureInstanceKHR instance = {0};
      instance.mask = 0xFF;
      instance.instanceCustomIndex = (u32)i;
      instance.flags = VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR | VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

      mat4 wm = geometry->mesh_instances.data[i].world;
      // column -> row major
      VkTransformMatrixKHR transform =
      {{
         {wm.data[0], wm.data[4], wm.data[8],  wm.data[12]},
         {wm.data[1], wm.data[5], wm.data[9],  wm.data[13]},
         {wm.data[2], wm.data[6], wm.data[10], wm.data[14]}
      }};
      instance.transform = transform;

      VkAccelerationStructureDeviceAddressInfoKHR addr_info = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};

      addr_info.accelerationStructure = context->rt_as.blases.data[i];
      VkDeviceAddress blas_address = vkGetAccelerationStructureDeviceAddressKHR(devices->logical, &addr_info);

      instance.accelerationStructureReference = blas_address;

      instances[i] = instance;
   }

   vk_buffer tlas_buffer = {.size = size_info.accelerationStructureSize};
   if(!vk_buffer_create_and_bind(&tlas_buffer, devices,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
      return false;
   buffer_hash_insert(buffer_table, tlas_buffer_name, tlas_buffer);

   vk_buffer scratch_buffer = {.size = size_info.buildScratchSize};
   if(!vk_buffer_create_and_bind(&scratch_buffer, devices,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
      return false;
   VkDeviceAddress scratch_address = buffer_device_address(&scratch_buffer, devices);

   VkAccelerationStructureCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
   create_info.buffer = tlas_buffer.handle;
   create_info.size = tlas_buffer.size;
   create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

   if(!vk_valid(vkCreateAccelerationStructureKHR(devices->logical, &create_info, &global_allocator.handle, &as->tlas)))
      return false;

   build_info.dstAccelerationStructure = as->tlas;
   build_info.scratchData.deviceAddress = scratch_address;

   if(!vk_valid(vkResetCommandPool(devices->logical, context->cmd.pool, 0)))
      return false;

   VkCommandBufferBeginInfo buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
   buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   if(!vk_valid(vkBeginCommandBuffer(context->cmd.buffer, &buffer_begin_info)))
      return false;

   VkAccelerationStructureBuildRangeInfoKHR build_range = {0};
   build_range.primitiveCount = (u32)draw_count;

   VkAccelerationStructureBuildRangeInfoKHR* build_range_ptr = &build_range;
   vkCmdBuildAccelerationStructuresKHR(context->cmd.buffer, 1, &build_info, &build_range_ptr);

   if(!vk_valid(vkEndCommandBuffer(context->cmd.buffer)))
      return false;

   VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &context->cmd.buffer;

   if(!vk_valid(vkQueueSubmit(context->graphics_queue, 1, &submit_info, VK_NULL_HANDLE)))
      return false;

   if(!vk_valid(vkDeviceWaitIdle(devices->logical)))
      return false;

   vk_buffer_destroy(devices, &instance_buffer);
   vk_buffer_destroy(devices, &scratch_buffer);

   return true;
}

static bool rt_acceleration_structures_create(vk_context* context)
{
   context->rt_as.blases.arena = context->app_storage;
   array_resize(context->rt_as.blases, context->geometry.mesh_draws.count);

   arena s = context->scratch;
   if(!rt_blas_geometry_build(s, context, &context->rt_as))
      return false;
   if(!rt_tlas_geometry_build(context, &context->rt_as))
      return false;

   return true;
}
