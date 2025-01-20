#if !defined(_PRIORITY_QUEUE_H)
#define _PRIORITY_QUEUE_H

#include "common.h"
#include <assert.h>

#define priority_queue_lc(i) (2*(i))+1
#define priority_queue_rc(i) (2*(i))+2
#define priority_queue_root(i) (usize)(((f32)(i)/2.0f)-0.5f)

typedef enum priority_queue_criteria 
{ 
   PRIORITY_QUEUE_MAX, PRIORITY_QUEUE_MIN 
} priority_queue_criteria;

typedef struct priority_queue
{
   priority_queue_type elements[priority_queue_max_count];
   priority_queue_criteria criteria;
   usize count;
} priority_queue;

static void priority_queue_swap(priority_queue_type* a, priority_queue_type* b)
{
   priority_queue_type t;
   t = *b;
   *b = *a;
   *a = t;
}

static void priority_queue_invariant(priority_queue* queue) 
{
   usize root, lc, rc;
   usize i;

   for(i = 0; i < queue->count; ++i)
   {
      if(priority_queue_rc(i) < queue->count)
      {
         root = queue->elements[i].index;
         lc = queue->elements[priority_queue_lc(i)].index;
         rc = queue->elements[priority_queue_rc(i)].index;

         assert(root >= lc && root >= rc);
      }
      else break;
   }
}

// Assumes now that it is max heap
// TODO: Use the boolean for min/max option

static void priority_queue_insert(priority_queue* queue, priority_queue_type data)
{
   // indexes
   usize parent, child;

   priority_queue_invariant(queue);
   assert(queue->count < priority_queue_max_count);

   queue->count++;
   queue->elements[queue->count-1] = data;

   child = queue->count-1;
   parent = priority_queue_root(child);

   while(parent >= 0)
   {
      if(queue->elements[parent].index < queue->elements[child].index)
      {
         priority_queue_swap(&queue->elements[parent], &queue->elements[child]);
         child = parent;
         parent = priority_queue_root(parent);
      }
      else break;
   }

   priority_queue_invariant(queue);
}

static priority_queue_type priority_queue_remove(priority_queue* queue)
{
   priority_queue_type result;
   // indexes
   usize parent, child;

   priority_queue_invariant(queue);
   assert(queue->count > 0);

   result = queue->elements[0];
   queue->elements[0] = queue->elements[queue->count-1];
   queue->count--;
   parent = 0;
   child = 1;

   while(child+1 < queue->count)
   {
      if(queue->elements[child].index < queue->elements[child+1].index)
         child = child + 1;
      if(queue->elements[child].index > queue->elements[parent].index)
      {
         priority_queue_swap(&queue->elements[parent], &queue->elements[child]);
         parent = child;
         child = priority_queue_lc(child);
      }
      else break;
   }

   priority_queue_invariant(queue);

   return result;
}

#endif

// wp(child+1 < count)
// child < count - 1

// 2i + 1
// 2i + 2

// 2(2i + 1)
// 4i + 2

// 2(2i + 2)
// 4i + 4

// 4i + 4 - (2i + 2)
// 4i + 4 - 2i - 2
// 2i + 2

// 4i + 2 - (2i + 1)
// 4i + 2 - 2i - 1
// 2i + 2 - 1
// 2i + 1