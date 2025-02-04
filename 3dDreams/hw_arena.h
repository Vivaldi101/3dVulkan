#if !defined(_hw_MEMORY_H)
#define _hw_MEMORY_H

#if !defined(_WIN32)
#error "Cannot include the file on a non-Win32 platforms"
#endif
#include <windows.h>

#include <assert.h>
#include "common.h"

// TODO: We need the concept of an empty stub struct in case of overflow allocation
#define hw_arena_push_struct(arena, type) ((type *)hw_buffer_push(arena, sizeof(type)))  
#define hw_arena_push_count(arena, count, type) ((type *)hw_buffer_push(arena, (count)*sizeof(type)))  
#define hw_arena_push_size(arena, count) ((void *)hw_buffer_push(arena, (count)*sizeof(byte)))  
#define hw_arena_push_string(arena, count) hw_arena_push_count(arena, count, char)

#define hw_arena_pop_struct(arena, type) ((type *)_pop_(arena, sizeof(type)))  
#define hw_arena_pop_count(arena, count, type) ((type *)hw_buffer_pop(arena, (count)*sizeof(type)))  
#define hw_arena_pop_string(arena, count) hw_arena_pop_count(arena, count, char)

#define hw_sub_arena_clear(arena) hw_buffer_clear(arena)
#define hw_sub_arena_create(arena) hw_sub_memory_buffer_create(arena)

#define hw_arena_create(arena) hw_buffer_create(arena)

#define hw_check_memory(cond) do { if (!(cond)) { MessageBoxA(0, "Out of memory in: " ##__FILE__, 0, 0); DebugBreak(); } } while(0)
#define hw_print_message_box(msg) MessageBoxA(0, msg, 0, 0)

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

static hw_buffer hw_buffer_create(size_t num_bytes) 
{
    hw_buffer result = {0};
    void *ptr = VirtualAlloc(0, num_bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
    result.base = (byte *)ptr;
    result.max_size = num_bytes;
    result.bytes_used = 0;
    
    return result;
}

static hw_buffer hw_sub_memory_buffer_create(const hw_buffer *buffer)
{
   hw_buffer result = {0};

   assert(buffer->max_size > buffer->bytes_used);

   result.bytes_used = 0;
   result.max_size = buffer->max_size - buffer->bytes_used;
   result.base = buffer->base + buffer->bytes_used;

   assert(result.bytes_used == 0);
   assert(result.max_size == buffer->max_size - buffer->bytes_used);
   assert(result.base == buffer->base + buffer->bytes_used);

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
   assert(buffer->bytes_used >= bytes);

   buffer->bytes_used -= bytes;
   result = buffer->base + buffer->bytes_used;

   return result;
}

static void hw_buffer_clear(hw_buffer *buffer) 
{
   memset(buffer, 0, sizeof(hw_buffer));
}

#endif
