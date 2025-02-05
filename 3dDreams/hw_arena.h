#if !defined(_HW_MEMORY_H)
#define _HW_MEMORY_H

#include <windows.h>

#include "common.h"

#if defined(_WIN32)
typedef LPVOID(*VirtualAllocPtr)(LPVOID, SIZE_T, DWORD, DWORD);
static VirtualAllocPtr global_allocate;
#elif
// TODO: other plats have their own global allocators
#endif

#define arena_push_struct(arena, type) ((type *)hw_buffer_push(arena, sizeof(type)))  
#define arena_push_count(arena, count, type) ((type *)hw_buffer_push(arena, (count)*sizeof(type)))  
#define arena_push_size(arena, count) ((void *)hw_buffer_push(arena, (count)*sizeof(byte)))  
#define arena_push_string(arena, count) arena_push_count(arena, count, char)

#define arena_pop_struct(arena, type) ((type *)_pop_(arena, sizeof(type)))  
#define arena_pop_count(arena, count, type) ((type *)hw_buffer_pop(arena, (count)*sizeof(type)))  
#define arena_pop_string(arena, count) arena_pop_count(arena, count, char)

#define sub_arena_clear(arena) hw_buffer_clear(arena)
#define sub_arena_create(arena) hw_sub_memory_buffer_create(arena)

#define arena_create(arena) hw_buffer_create(arena)

cache_align typedef struct hw_buffer
{
   byte* base;
   size_t max_size, bytes_used;
} hw_buffer;

cache_align typedef struct hw_buffer_stub
{
   byte blob[1024];
} hw_buffer_stub;

static hw_buffer_stub global_hw_buffer_stub;

static void* hw_arena_get_stub()
{
   return &global_hw_buffer_stub;
}

static void hw_virtual_allocate_init()
{
   typedef LPVOID(*VirtualAllocPtr)(LPVOID, SIZE_T, DWORD, DWORD);

   HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
   if (hKernel32)
      global_allocate = (VirtualAllocPtr)(GetProcAddress(hKernel32, "VirtualAlloc"));

   post(global_allocate);
}

static hw_buffer hw_buffer_create(size_t num_bytes) 
{
    hw_buffer result = {0};
    pre(global_allocate);

    void* ptr = global_allocate(0, num_bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
    result.base = (byte *)ptr;
    result.max_size = num_bytes;
    result.bytes_used = 0;
    
    return result;
}

static hw_buffer hw_sub_memory_buffer_create(const hw_buffer *buffer)
{
   hw_buffer result = {0};
   pre(buffer->max_size > buffer->bytes_used);

   result.bytes_used = 0;
   result.max_size = buffer->max_size - buffer->bytes_used;
   result.base = buffer->base + buffer->bytes_used;

   post(result.bytes_used == 0);
   post(result.max_size == buffer->max_size - buffer->bytes_used);
   post(result.base == buffer->base + buffer->bytes_used);

   return result;
}

static void* hw_buffer_top(hw_buffer *buffer) 
{
   void *ptr = buffer->base + buffer->bytes_used;
   return ptr;
}

// FIXME: pass alignment
static void* hw_buffer_push(hw_buffer *buffer, size_t bytes) 
{
   void* result;
   if(buffer->bytes_used + bytes > buffer->max_size)
		return hw_arena_get_stub();

   result = buffer->base + buffer->bytes_used;
   buffer->bytes_used += bytes;

   return result;
}

static void* hw_buffer_pop(hw_buffer *buffer, size_t bytes) 
{
   void *result;
   pre(buffer->bytes_used >= bytes);

   buffer->bytes_used -= bytes;
   result = buffer->base + buffer->bytes_used;

   return result;
}

static void hw_buffer_clear(hw_buffer *buffer) 
{
   memset(buffer, 0, sizeof(hw_buffer));
}

#endif
