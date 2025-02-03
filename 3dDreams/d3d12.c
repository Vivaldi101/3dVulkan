#include <d3d12.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>

#include "hw_arena.h"
#include "common.h"

#pragma comment(lib,	"d3d12.lib")
#pragma comment(lib,	"d3dcompiler.lib")
#pragma comment(lib,	"dxguid.lib")
#pragma comment(lib,	"dxgi.lib")

#define d3d_success(r) SUCCEEDED((r))

enum 
{
	FRAME_BUFFER_COUNT = 3
};

typedef struct d3d12_renderer
{
	void* device;
	void* queue;
	void* swap_chain;
	void* command_allocator;
   usize command_list_type;
} d3d12_renderer;

void d3d12_present(d3d12_renderer* renderer)
{
   HRESULT hr = S_OK;
   ID3D12GraphicsCommandList* render_commands;

   pre(renderer->device);
   pre(renderer->queue);
   pre(renderer->swap_chain);
   pre(renderer->command_allocator);

   hr |= ((ID3D12Device*)renderer->device)->lpVtbl->CreateCommandList(
      renderer->device,
      0, 
      renderer->command_list_type,
      renderer->command_allocator,
      0,
      &IID_ID3D12CommandList,
      (void**)&render_commands);

   if (!render_commands)
      hw_message("render_commands not created");

   render_commands->lpVtbl->Close(render_commands);

   ((ID3D12CommandQueue*)renderer->queue)->lpVtbl->ExecuteCommandLists(
      renderer->queue,
      1,
      (ID3D12CommandList**)&render_commands);

   ((IDXGISwapChain1*)renderer->swap_chain)->lpVtbl->Present(renderer->swap_chain, 0, 0);
   render_commands->lpVtbl->Release(render_commands);

   post(d3d_success(hr));
}

void d3d12_initialize(hw* hw)
{
   HRESULT hr = S_OK;

   // interfaces
   ID3D12Device* device;
   ID3D12CommandQueue* d3d12_command_queue;
   IDXGISwapChain1* swap_chain;
   IDXGIFactory2* dxgi_factory;
   IDXGIAdapter1* dxgi_adapter;
   ID3D12CommandAllocator* d3d12_command_allocator;
   ID3D12PipelineState* pipeline_state;
   ID3D12Debug* debug;

	// shaders
   ID3DBlob* vertex_shader;
   //ID3DBlob* pixel_shader; TODO: enable once all works
   u32 compiler_flags;

	// descriptors
   D3D12_COMMAND_QUEUE_DESC graphics_command_queue_desc = {0};
   D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {0};
   D3D12_RASTERIZER_DESC rasterizer_desc = {0};
   D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {0};
   D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {0};
   DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {0};

   // signatures
   void* root_signature;
   ID3DBlob* root_signature_blob;
   ID3DBlob* root_signature_blob_error;

   // select the d3d12 capable gpu
   u32 adapter_index;

   //ID3D12Resource* rtv_heap_desc;

   pre(hw->renderer.window.handle);

   d3d12_renderer* renderer = hw_arena_push_struct(&hw->memory, d3d12_renderer);

   hr = D3D12GetDebugInterface(&IID_ID3D12Debug, (void**)&debug);
   if (!debug)
      hw_message("debug not created");

   debug->lpVtbl->EnableDebugLayer(debug);

   hr = CreateDXGIFactory1(&IID_IDXGIFactory, (void**)&dxgi_factory);
   if (!dxgi_factory)
      hw_message("dxgi_factory not created");

   adapter_index = 0;

   while(dxgi_factory->lpVtbl->EnumAdapters1(dxgi_factory, adapter_index, &dxgi_adapter) != DXGI_ERROR_NOT_FOUND)
   {
      DXGI_ADAPTER_DESC1 adapter_desc = {0};

      dxgi_adapter->lpVtbl->GetDesc1(dxgi_adapter, &adapter_desc);

      hr = D3D12CreateDevice((IUnknown*)dxgi_adapter, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, NULL);
      if (d3d_success(hr))
         break;

      ++adapter_index;
   }

   hr = D3D12CreateDevice((IUnknown*)dxgi_adapter, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, (void**)&device);
   if (!device)
      hw_message("device not created");

   renderer->device = device;

   hr = device->lpVtbl->CreateCommandQueue(device, &graphics_command_queue_desc, &IID_ID3D12CommandQueue, (void**)&d3d12_command_queue);
   if (!d3d12_command_queue)
      hw_message("d3d12_command_queue not created");

   renderer->queue = d3d12_command_queue;

   swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
   swap_chain_desc.SampleDesc.Count = 1;
   swap_chain_desc.BufferCount = FRAME_BUFFER_COUNT;
   swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

   hr = dxgi_factory->lpVtbl->CreateSwapChainForHwnd(dxgi_factory, (IUnknown*)d3d12_command_queue, hw->renderer.window.handle, &swap_chain_desc, 0, 0, &swap_chain);
   if (!swap_chain)
      hw_message("swap_chain not created");

   renderer->swap_chain = swap_chain;

   rtv_heap_desc.NumDescriptors = FRAME_BUFFER_COUNT;
   rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

   for(u32 frame = 0; frame < FRAME_BUFFER_COUNT; ++frame)
   {
      //D3D12_RENDER_TARGET_VIEW_DESC handle;
      //ID3D12Resource* buffer_resource;
		//device->lpVtbl->CreateRenderTargetView(device, 
         //swap_chain->lpVtbl->GetBuffer(swap_chain, frame, &IID_IDXGISwapChain1, (void**)&buffer_resource), &handle);
   }

   hr = device->lpVtbl->CreateCommandAllocator(device, graphics_command_queue_desc.Type, &IID_ID3D12CommandAllocator, (void**)&d3d12_command_allocator);
   if (!d3d12_command_allocator)
      hw_message("d3d12_command_allocator not created");

   renderer->command_allocator = d3d12_command_allocator;

	compiler_flags = D3DCOMPILE_DEBUG;
   hr = D3DCompileFromFile(L"C:\\Work\\3dDreams\\3dDreams\\shader.hlsl", 0, 0, "vs_main", "vs_5_0", compiler_flags, 0, &vertex_shader, 0);

   hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_signature_blob, &root_signature_blob_error);

   hr = device->lpVtbl->CreateRootSignature(device,
      0,
      root_signature_blob->lpVtbl->GetBufferPointer(root_signature_blob),
      root_signature_blob->lpVtbl->GetBufferSize(root_signature_blob),
      &IID_ID3D12RootSignature, (void**)&root_signature);
   if (!root_signature)
      hw_message("d3d12_command_allocator not created");

	pso.pRootSignature = root_signature;
   pso.VS.pShaderBytecode = vertex_shader->lpVtbl->GetBufferPointer(vertex_shader);
   pso.VS.BytecodeLength = vertex_shader->lpVtbl->GetBufferSize(vertex_shader);
   pso.RasterizerState = rasterizer_desc;
   pso.SampleMask = UINT_MAX;
   pso.NumRenderTargets = 1;
   pso.SampleDesc.Count = 1;
   pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
   pso.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
   pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
   pso.RTVFormats[0] = swap_chain_desc.Format;

   hr = device->lpVtbl->CreateGraphicsPipelineState(device, &pso, &IID_ID3D12PipelineState, (void**)&pipeline_state);
   if (!pipeline_state)
      hw_message("pipeline_state not created");


   hw->renderer.backends[d3d12_renderer_index] = renderer;
   hw->renderer.frame_present = d3d12_present;
   hw->renderer.renderer_index = d3d12_renderer_index;

   renderer->command_list_type = graphics_command_queue_desc.Type;

   post(d3d_success(hr));
   post(hw->renderer.frame_present);
   post(hw->renderer.backends[d3d12_renderer_index]);
   post(hw->renderer.renderer_index == d3d12_renderer_index);
   post(renderer->device);
   post(renderer->queue);
   post(renderer->swap_chain);
   post(renderer->command_allocator);
}