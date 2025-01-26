#include <d3d12.h>
#include <dxgi1_2.h>

#include "common.h"

#pragma comment(lib,	"d3d12.lib")
#pragma comment(lib,	"dxguid.lib")
#pragma comment(lib,	"dxgi.lib")

#define d3d_success(r)  SUCCEEDED((r))
#define device			      d3d12_device->lpVtbl
#define factory			   dxgi_factory->lpVtbl
#define swap_chain			dxgi_swap_chain->lpVtbl

static ID3D12Device*        d3d12_device;
static ID3D12CommandQueue*  d3d12_command_queue;

static IDXGIFactory2*   dxgi_factory;
static IDXGISwapChain1* dxgi_swap_chain;

void d3d12_present()
{
   swap_chain->Present(dxgi_swap_chain, 0, 0);
}

void d3d12_initialize(hw* hw)
{
   pre(hw->window.handle);

   HRESULT hr = S_OK;
   D3D12_COMMAND_QUEUE_DESC graphics_command_queue_desc = { 0 };
   DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = { 0 };

   hr = CreateDXGIFactory(&IID_IDXGIFactory, (void**)&dxgi_factory);
   hr |= D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, (void**)&d3d12_device);

   if (!d3d12_device)
      hw_assert("d3d12_device not created");

   graphics_command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
   hr |= device->CreateCommandQueue(d3d12_device, &graphics_command_queue_desc, &IID_ID3D12CommandQueue, (void**)&d3d12_command_queue);

   if (!d3d12_command_queue)
      hw_assert("d3d12_command_queue not created");

   swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
   swap_chain_desc.SampleDesc.Count = 1;
   swap_chain_desc.BufferCount = 3;
   swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
   hr |= factory->CreateSwapChainForHwnd(dxgi_factory, (IUnknown*)d3d12_command_queue, hw->window.handle, &swap_chain_desc, 0, 0, &dxgi_swap_chain);

   if (!dxgi_swap_chain)
      hw_assert("dxgi_swap_chain not created");

   hw->renderer.present = d3d12_present;
   post(d3d_success(hr));
   post(hw->renderer.present);
}