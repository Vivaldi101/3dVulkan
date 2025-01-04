#if !defined(_HW_MEMORY_H)
#define _HW_MEMORY_H

#if !defined(_WIN32)
#error "Cannot include the file on a non-Win32 platforms"
#endif

#include <windows.h>
#include "common.h"

// TODO: Rename these apis to fit the module_object_action way

#define HW_get_array_elements(stack, type) ((type *)stack.base)
#define HW_get_at(stack, type, index) ((type *)_getat_(stack, sizeof(type), index))
#define HW_get_last(stack, type) ((type *)_get_last_element_(stack, sizeof(type)))

#define HW_push_size(stack, size, type) ((type *)_push_(stack, size))  
#define HW_push_struct(stack, type) ((type *)HW_memory_buffer_push(stack, sizeof(type)))  
#define HW_push_count(stack, count, type) ((type *)HW_memory_buffer_push(stack, (count)*sizeof(type)))  
#define HW_push_string(stack, count) HW_push_count(stack, count, char)

#define HW_pop_struct(stack, type) ((type *)_pop_(stack, sizeof(type)))  
#define HW_pop_count(stack, count, type) ((type *)HW_memory_buffer_pop(stack, (count)*sizeof(type)))  
#define HW_pop_string(stack, count) HW_pop_count(stack, count, char)

#define HW_push_array(stack, count, type) ((type *)_push_(stack, (count) * sizeof(type)))
#define HW_pop_array(stack, count, type) ((type *)_pop_(stack, (count) * sizeof(type)))
#define HW_check_memory(cond) do { if (!(cond)) { MessageBoxA(0, "Out of memory in: " ##__FILE__, 0, 0); DebugBreak(); } } while(0)
#define HW_print_message_box(msg) MessageBoxA(0, msg, 0, 0)

cache_align typedef struct
{
   byte* base;
   size_t max_size, bytes_used;
} hw_memory_buffer;

hw_memory_buffer HW_memory_buffer_create(size_t num_bytes);

static void* HW_memory_buffer_top(hw_memory_buffer *buffer) 
{
    void *ptr = buffer->base + buffer->bytes_used;
    return ptr;
}

// FIXME: pass alignment
static void* HW_memory_buffer_push(hw_memory_buffer *buffer, size_t num_bytes) 
{
    void *result = buffer->base + buffer->bytes_used;
    buffer->bytes_used += num_bytes;
    
    return result;
}

static void* HW_memory_buffer_pop(hw_memory_buffer *buffer, size_t num_bytes) 
{
    void *result;
    buffer->bytes_used -= num_bytes;
    result = buffer->base + buffer->bytes_used;
    
    return result;
}

#endif
