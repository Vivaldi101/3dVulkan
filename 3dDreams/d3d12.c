#include <d3d12.h>
#include <dxgi1_2.h>

#include "common.h"

#pragma comment(lib,	"d3d12.lib")
#pragma comment(lib,	"dxguid.lib")
#pragma comment(lib,	"dxgi.lib")

#define d3d_success(r) SUCCEEDED((r))

void d3d12_present(hw_renderer* renderer)
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
   ID3D12Device* d3d12_device = NULL;
   ID3D12CommandQueue* d3d12_command_queue = NULL;
   D3D12_COMMAND_QUEUE_DESC graphics_command_queue_desc = {0};
   DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {0};
   //D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc = {0};
   ID3D12Fence* fence = NULL;
   IDXGISwapChain1* dxgi_swap_chain = NULL;
   IDXGIFactory2* dxgi_factory = NULL;
   ID3D12CommandAllocator* d3d12_command_allocator = NULL;

   //ID3D12PipelineState* pipeline_state;
   pre(hw->renderer.window.handle);

   hr |= D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, (void**)&d3d12_device);
   if (!d3d12_device)
      hw_assert("d3d12_device not created");

   hw->renderer.device = d3d12_device;

   hr |= d3d12_device->lpVtbl->CreateFence(d3d12_device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void**)&fence);
   if (!fence)
      hw_assert("fence not created");

   hr |= CreateDXGIFactory(&IID_IDXGIFactory, (void**)&dxgi_factory);
   if (!dxgi_factory)
      hw_assert("dxgi_factory not created");

   graphics_command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

   hr |= d3d12_device->lpVtbl->CreateCommandQueue(d3d12_device, &graphics_command_queue_desc, &IID_ID3D12CommandQueue, (void**)&d3d12_command_queue);
   if (!d3d12_command_queue)
      hw_assert("d3d12_command_queue not created");

   hw->renderer.queue = d3d12_command_queue;

   swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
   swap_chain_desc.SampleDesc.Count = 1;
   swap_chain_desc.BufferCount = 3;
   swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

   hr |= dxgi_factory->lpVtbl->CreateSwapChainForHwnd(dxgi_factory, (IUnknown*)d3d12_command_queue, hw->renderer.window.handle, &swap_chain_desc, 0, 0, &dxgi_swap_chain);
   if (!dxgi_swap_chain)
      hw_assert("dxgi_swap_chain not created");

   hw->renderer.swap_chain = dxgi_swap_chain;

   hr |= d3d12_device->lpVtbl->CreateCommandAllocator(d3d12_device, graphics_command_queue_desc.Type, &IID_ID3D12CommandAllocator, (void**)&d3d12_command_allocator);
   if (!d3d12_command_allocator)
      hw_assert("d3d12_command_allocator not created");

   hw->renderer.command_allocator = d3d12_command_allocator;

   //pipeline_state_desc.SampleMask = UINT_MAX;
   //pipeline_state_desc.NumRenderTargets = 1;
   //pipeline_state_desc.SampleDesc.Count = 1;
   //pipeline_state_desc.RTVFormats[0] = swap_chain_desc.Format;

   //hr |= device->CreateGraphicsPipelineState(d3d12_device, &pipeline_state_desc, &IID_ID3D12PipelineState, (void**)&pipeline_state);
   //if (!pipeline_state)
      //hw_assert("pipeline_state not created");

   hw->renderer.present = d3d12_present;
   hw->renderer.command_list_type = graphics_command_queue_desc.Type;

   post(d3d_success(hr));
   post(hw->renderer.device);
   post(hw->renderer.queue);
   post(hw->renderer.swap_chain);
   post(hw->renderer.command_allocator);
   post(hw->renderer.present);
}