#pragma once

#include <stddef.h>
#include <malloc.h>
#include <stdint.h>
#include <assert.h>

#define aligned_count(s, t) (size)((f32)(s) / sizeof(t)+0.5f)
#define scratch_count_left(a, s) (size)(((a).end - (a).beg) / (s))
#define arena_end(a, p) ((byte*)(a)->end == (byte*)(p))
#define scratch_end(a, p) ((byte*)(a).end == (byte*)(p))
#define scratch_size_left(a)  (size)(((a).end - (a).beg) / sizeof(byte))
#define scratch_left(a,t)  (size)(((a).end - (a).beg) / sizeof(t))
#define arena_left(a,t)    (size)(((a)->end - (a)->beg) / sizeof(t))
#define arena_full(a)      ((a)->beg == (a)->end)   // or empty for stub arenas
#define arena_loop(i, a, p) for(size (i) = 0; (i) < scratch_left((a), *(p)); ++(i))
#define arena_offset(i, a, t) (t*)a.beg + (i)

#define scratch_count(a, s, t)  ((s) - scratch_left(a, t))
#define arena_count(a, s, t)  ((s) - arena_left(a, t))

#define scratch_size(a) (size)((a).end - (a).beg)
#define arena_size(a) (size)((a)->end - (a)->beg)

#define arena_stub(p, a) ((p) == (a)->end)
#define scratch_stub(p, a) arena_stub(p, &a)

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

typedef struct
{
   void* data;
   size count;
} arena_data;

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

   assert(((uptr)p & (align - 1)) == 0);   // aligned result

   return p;
}

static arena_data arena_alloc(arena* a, size objsize, size count)
{
   arena_data result = {};

   size last_count = arena_size(a) / objsize;
   result.data = newsize(a, objsize, count);
   result.count = last_count - (arena_size(a) / objsize);

   return result;
}

static arena arena_new(size cap)
{
   arena a = {0}; // stub arena
   if(cap > 0)
   {
      a.beg = calloc(1, cap); // might change to OS specific alloc later
      a.end = a.beg ? a.beg + cap : 0;
   }

   return a;
}

static void arena_free(arena* a)
{
   free(a->beg);
}
