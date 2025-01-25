#include <d3d12.h>
#include <dxgi1_2.h>

#include "common.h"

#pragma comment(lib,	"d3d12.lib")
#pragma comment(lib,	"dxguid.lib")

#define d3d_success(r)  SUCCEEDED((r))
#define d3d_f			d3d12_device->lpVtbl
#define dxgi_f			dxgi_factory->lpVtbl

ID3D12Device*			d3d12_device;
ID3D12CommandQueue*		d3d12_queue;
IDXGIFactory*			dxgi_factory;

void d3d_initialize(hw* hw)
{
   HRESULT hr;
   D3D12_COMMAND_QUEUE_DESC graphics_command_queue_desc = {0};
   DXGI_SWAP_CHAIN_DESC swap_chain_desc = {0};

   hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, (void**)&d3d12_device);
   d3d_success(hr);

   graphics_command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
   hr = d3d_f->CreateCommandQueue(d3d12_device, &graphics_command_queue_desc, &IID_ID3D12CommandQueue, &d3d12_queue);
   
   swap_chain_desc.SampleDesc.Count = 1;
   swap_chain_desc.BufferCount = 3;
   swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

   //hr = dxgi_f->CreateSwapChain(dxgi_factory, d3d12_device, &swap_chain_desc, 0);

   // postcond
   post(d3d12_device);
   post(d3d12_queue);
   post(d3d_success(hr));
   //post(dxgi_factory);
}