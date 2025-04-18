#pragma once

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include "common.h"


#define set_arena_type(t) typedef t arena_type;
#define scratch_invariant(s, a, t) ((s) <= scratch_left((a), typeof(*(t))))

#define arena_full(a)      ((a)->beg == (a)->end)   // or empty for stub arenas
#define arena_loop(i, a, p) for(size (i) = 0; (i) < scratch_left((a), *(p)); ++(i))
#define arena_offset(i, a, t) (t*)a.beg + (i)

#define scratch_size(a) (size)((a).end - (a).beg)
#define arena_size(a) (size)((a)->end - (a)->beg)

#define scratch_left(a,t)  (size)(scratch_size(a) / sizeof(t))
#define arena_left(a,t)    (size)(arena_size(a) / sizeof(t))

#define arena_stub(p, a) ((p) == (a)->end)
#define scratch_stub(p, a) arena_stub(p, &a)

#define arena_end(a, p) ((byte*)(a)->end == (byte*)(p))
#define scratch_end(a, p) ((byte*)(a).end == (byte*)(p))

#define arena_end_count(p, a, n) (void*)(p)!=(a)->end?(p)+(n):(p)
#define scratch_end_count(p, a, n) (void*)(p)!=(a).end?(p)+(n):(p)

#define scratch_clear(a) memset((a).beg, 0, scratch_size((a)))

// TODO: Different news for scratch and storage arenas

#define newx(a,b,c,d,e,...) e
#define new(...)            newx(__VA_ARGS__,new4,new3,new2)(__VA_ARGS__)
#define new2(a, t)          (t*)alloc(a, sizeof(t), __alignof(t), 1, 0)
#define new3(a, t, n)       (t*)alloc(a, sizeof(t), __alignof(t), n, 0)
#define new4(a, t, n, f)    (t*)alloc(a, sizeof(t), __alignof(t), n, f)

#define newxsize(a,b,c,d,e,...) e
#define newsize(...)            newxsize(__VA_ARGS__,new4size,new3size,new2size)(__VA_ARGS__)
#define new2size(a, t)          alloc(a, t, __alignof(t), 1, 0)
#define new3size(a, t, n)       alloc(a, t, __alignof(t), n, 0)
#define new4size(a, t, n, f)    alloc(a, t, __alignof(t), n, f)

#define sizeof(x)       (size)sizeof(x)
#define countof(a)      (sizeof(a) / sizeof(*(a)))
#define lengthof(s)     (countof(s) - 1)
#define amountof(a, t)  ((a) * sizeof(t))

// arena result type
typedef struct
{
   void* data;
   size count;
} arena_result;

typedef struct arena
{
   byte* beg;
   byte* end;  // one past the end
} arena;

static void* alloc(arena* a, size alloc_size, size align, size count, u32 flag)
{
   // align allocation to next aligned boundary
   void* p = (void*)(((uptr)a->beg + (align - 1)) & (-align));

   if(count <= 0 || count > (a->end - (byte*)p) / alloc_size) // empty or overflow
      return a->end;

   a->beg = (byte*)p + (count * alloc_size);          // advance arena 

   post(((uptr)p & (align - 1)) == 0);   // aligned result

   return p;
}

static arena_result arena_alloc(arena scratch, size objsize, size count)
{
   arena_result result = {};

   size last_count = scratch_size(scratch) / objsize;
   result.data = newsize(&scratch, objsize, count);
   result.count = last_count - (scratch_size(scratch) / objsize);

   return result;
}
