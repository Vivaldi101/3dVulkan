#include "hash.h"

static u32 hash(const char* key)
{
   uint32_t result = 2166136261U;
   while(*key)
   {
      result *= 16777619U;
      result ^= (uint8_t)(*key);
      key++;
   }

   return result;
}

static vk_shader_module vk_shader_hash_lookup(vk_shader_hash_table* table, const char* key)
{
   u32 index = hash(key) % table->max_count;
   u32 old_index = index;

   while(table->keys[index] && strcmp(table->keys[index], key) < 0)
   {
      index = (index + 1) % table->max_count;
      if(index == old_index) break; // wrap around
   }

   assert(index == old_index || !table->keys[index] || strcmp(table->keys[index], key) >= 0);

   if(table->keys[index] && strcmp(table->keys[index], key) == 0)
      return table->values[index];

   return (vk_shader_module){0};
}

static void vk_shader_hash_log_module_name(void* ctx, vk_shader_module_name shader_module)
{
   (void)ctx;
   printf("Shader module '%s': \t'%p'\n", shader_module.name, shader_module.module.handle);
}

static void vk_shader_hash_insert(vk_shader_hash_table* table, const char* key, vk_shader_module value)
{
   if(table->count == table->max_count)
      return;

   u32 index = hash(key) % table->max_count;

   while(table->keys[index])
   {
      if(strcmp(table->keys[index], key) > 0)
      {
         const char* tmp_key = table->keys[index];
         vk_shader_module tmp_value = table->values[index];

         table->keys[index] = key;
         table->values[index] = value;

         key = tmp_key;
         value = tmp_value;
      }
      else if(strcmp(table->keys[index], key) == 0)
      {
         table->values[index] = value;
         return;
      }

      index = (index + 1) % table->max_count;
   }

   table->keys[index] = key;
   table->values[index] = value;
   table->count++;
}

static void vk_shader_hash_function(vk_shader_hash_table* table, void(*p)(void* ctx, vk_shader_module_name module), void* ctx)
{
   u32 index = 0;
   u32 count = 0;

   while(count != table->count)
   {
      if(table->keys[index])
      {
         vk_shader_module_name m = {table->values[index], table->keys[index]};
         p(ctx, m);
         ++count;
      }
      index = (index + 1) % table->max_count;
   }
}
