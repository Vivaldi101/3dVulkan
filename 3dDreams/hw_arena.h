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

#define arena_push_struct(arena, type) (hw_buffer_push(arena, sizeof(type), sizeof(type)))  
#define arena_push_count(arena, count, type) (hw_buffer_push(arena, (count)*sizeof(type), sizeof(type))) 
#define arena_push_size(arena, count, type) (hw_buffer_push(arena, (count)*sizeof(byte), sizeof(type)))  
#define arena_push_string(arena, count) arena_push_count(arena, count, char)
#define arena_push_pointer_strings(arena, count) hw_buffer_push(arena, (count)*sizeof(const char*), sizeof(const char*))

#define arena_pop_struct(arena, type) ((type *)_pop_(arena, sizeof(type)))  
#define arena_pop_count(arena, count, type) ((type *)hw_buffer_pop(arena, (count)*sizeof(type)))  
#define arena_pop_string(arena, count) arena_pop_count(arena, count, char)

#define sub_arena_reset(arena) hw_sub_buffer_reset(arena)
#define sub_arena_create(arena) hw_sub_memory_buffer_create(arena)

#define arena_create(arena) hw_buffer_create(arena)

#define arena_is_empty(arena) hw_buffer_is_empty(arena)
#define arena_is_full(arena) hw_buffer_is_full(arena)
#define arena_is_set(arena, elements) hw_buffer_is_set(arena, elements)

cache_align typedef struct hw_arena
{
   byte* base;
   usize max_size, bytes_used;	// in bytes
   usize element_count;				// actual item count
} hw_arena;

#define arena_get_data(arena, type) ((type*)(arena)->base)

static void hw_global_reserve_available()
{
   MEMORYSTATUSEX memory_status;
   memory_status.dwLength = sizeof(memory_status);

   GlobalMemoryStatusEx(&memory_status);

   global_allocate(0, memory_status.ullAvailPhys, MEM_RESERVE, PAGE_READWRITE);
}

static void* hw_virtual_memory_commit(void* address, usize size)
{
   return global_allocate(address, size, MEM_COMMIT, PAGE_READWRITE); // only commit whats needed
}

static void hw_virtual_allocate_init()
{
   typedef LPVOID(*VirtualAllocPtr)(LPVOID, usize, DWORD, DWORD);

   HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
   if (hKernel32)
      global_allocate = (VirtualAllocPtr)(GetProcAddress(hKernel32, "VirtualAlloc"));

   hw_global_reserve_available(); // reserve all range
	post(global_allocate);
}

static hw_arena hw_buffer_create(usize byte_count) 
{
   hw_arena result = {0};
   pre(global_allocate);

   void* data = hw_virtual_memory_commit(0, byte_count);

   result.base = data;
   result.max_size = byte_count;
   result.bytes_used = 0;

   return result;
}

static hw_arena hw_sub_memory_buffer_create(const hw_arena *buffer)
{
   hw_arena result = {0};
   pre(buffer->max_size >= buffer->bytes_used);

   result.bytes_used = 0;
   result.max_size = buffer->max_size - buffer->bytes_used;
   result.base = (byte*)buffer->base + buffer->bytes_used;

   post(result.bytes_used == 0);
   post(result.max_size == buffer->max_size - buffer->bytes_used);
   post(result.base == (byte*)buffer->base + buffer->bytes_used);

   return result;
}

static void* hw_buffer_top(hw_arena *buffer) 
{
   void *ptr = (byte*)buffer->base + buffer->bytes_used;
   return ptr;
}

// TODO: element_size and count?
static hw_arena hw_buffer_push(hw_arena *buffer, usize bytes, usize element_size) 
{
   hw_arena result = {0};
   if(buffer->bytes_used + bytes > buffer->max_size)
      return (hw_arena){0};

   result.base = (byte*)buffer->base + buffer->bytes_used;
   result.max_size = bytes;
   result.element_count += bytes / element_size;
   buffer->bytes_used += bytes;

   return result;
}

static void* hw_buffer_pop(hw_arena *buffer, usize bytes) 
{
   void *result;
   pre(buffer->bytes_used >= bytes);

   buffer->bytes_used -= bytes;
   result = (byte*)buffer->base + buffer->bytes_used;

   return result;
}

static void hw_sub_buffer_reset(hw_arena* buffer) 
{
	memset(buffer, 0, sizeof(hw_arena)); // we only reset the structure but wont clear the memory since its commited top-level
}

static bool hw_buffer_is_empty(hw_arena *buffer) 
{
	return buffer->bytes_used == 0;
}

static bool hw_buffer_is_full(hw_arena *buffer) 
{
	return buffer->bytes_used == buffer->max_size;
}

static bool hw_buffer_is_set(hw_arena *buffer, usize element_count) 
{
	return buffer->element_count == element_count; 
}

#endif
