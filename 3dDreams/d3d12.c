#include <d3d12.h>
#include <dxgi1_2.h>

#include "hw_arena.h"
#include "common.h"

#pragma comment(lib,	"d3d12.lib")
#pragma comment(lib,	"d3dcompiler.lib")
#pragma comment(lib,	"dxguid.lib")
#pragma comment(lib,	"dxgi.lib")

#define d3d_success(r) SUCCEEDED((r))

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
      hw_assert("render_commands not created");

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
   ID3D12Device* d3d12_device;
   ID3D12CommandQueue* d3d12_command_queue;
   //ID3D12Fence* fence;	todo: enable
   IDXGISwapChain1* dxgi_swap_chain;
   IDXGIFactory2* dxgi_factory;
   IDXGIAdapter1* dxgi_adapter;
   ID3D12CommandAllocator* d3d12_command_allocator;
   ID3D12PipelineState* pipeline_state;
   ID3D12Debug* debug;
   D3D12_COMMAND_QUEUE_DESC graphics_command_queue_desc = {0};
   DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {0};
   D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {0};
   D3D12_RASTERIZER_DESC rasterizer_desc = {0};
   u32 adapter_index;

   pre(hw->renderer.window.handle);

   d3d12_renderer* renderer = hw_arena_push_struct(&hw->memory, d3d12_renderer);

   hr |= D3D12GetDebugInterface(&IID_ID3D12Debug, (void**)&debug);
   if (!debug)
      hw_assert("debug not created");

   debug->lpVtbl->EnableDebugLayer(debug);

   hr |= CreateDXGIFactory1(&IID_IDXGIFactory, (void**)&dxgi_factory);
   if (!dxgi_factory)
      hw_assert("dxgi_factory not created");

   adapter_index = 0;

   while(dxgi_factory->lpVtbl->EnumAdapters1(dxgi_factory, adapter_index, &dxgi_adapter) != DXGI_ERROR_NOT_FOUND)
   {
      DXGI_ADAPTER_DESC1 adapter_desc = {0};

      dxgi_adapter->lpVtbl->GetDesc1(dxgi_adapter, &adapter_desc);

      hr |= D3D12CreateDevice((IUnknown*)dxgi_adapter, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, NULL);
      if (d3d_success(hr))
         break;

      ++adapter_index;
   }

   hr |= D3D12CreateDevice((IUnknown*)dxgi_adapter, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, (void**)&d3d12_device);
   if (!d3d12_device)
      hw_assert("d3d12_device not created");

   renderer->device = d3d12_device;

   hr |= d3d12_device->lpVtbl->CreateCommandQueue(d3d12_device, &graphics_command_queue_desc, &IID_ID3D12CommandQueue, (void**)&d3d12_command_queue);
   if (!d3d12_command_queue)
      hw_assert("d3d12_command_queue not created");

   renderer->queue = d3d12_command_queue;

   swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
   swap_chain_desc.SampleDesc.Count = 1;
   swap_chain_desc.BufferCount = 3;
   swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

   hr |= dxgi_factory->lpVtbl->CreateSwapChainForHwnd(dxgi_factory, (IUnknown*)d3d12_command_queue, hw->renderer.window.handle, &swap_chain_desc, 0, 0, &dxgi_swap_chain);
   if (!dxgi_swap_chain)
      hw_assert("dxgi_swap_chain not created");


   renderer->swap_chain = dxgi_swap_chain;

   hr |= d3d12_device->lpVtbl->CreateCommandAllocator(d3d12_device, graphics_command_queue_desc.Type, &IID_ID3D12CommandAllocator, (void**)&d3d12_command_allocator);
   if (!d3d12_command_allocator)
      hw_assert("d3d12_command_allocator not created");

   renderer->command_allocator = d3d12_command_allocator;

	// TODO: root signature and VS shader
   pso.pRootSignature = 0;
   pso.RasterizerState = rasterizer_desc;
   pso.SampleMask = UINT_MAX;
   pso.NumRenderTargets = 1;
   pso.SampleDesc.Count = 1;
   pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
   pso.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
   pso.RTVFormats[0] = swap_chain_desc.Format;

   hr |= d3d12_device->lpVtbl->CreateGraphicsPipelineState(d3d12_device, &pso, &IID_ID3D12PipelineState, (void**)&pipeline_state);
   if (!pipeline_state)
      hw_assert("pipeline_state not created");

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