#include "common.h"
#include "arena.h"
#include <stdio.h>

align_struct file_result
{
   byte* data;
	size file_size;
} file_result;

static file_result win32_file_read(arena* file_arena, const char* path)
{
   file_result result = {};

   HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
   if(file == INVALID_HANDLE_VALUE)
      return (file_result) {};

   LARGE_INTEGER file_size;
   if(!GetFileSizeEx(file, &file_size))
      return (file_result) {};

   u32 file_size_32 = (u32)(file_size.QuadPart);
   result.data = newsize(file_arena, file_size_32);
   if(arena_end(file_arena, result.data))
      return (file_result) {};

   DWORD bytes_read = 0;
   if(!(ReadFile(file, result.data, file_size_32, &bytes_read, 0) && (file_size_32 == bytes_read)))
      return (file_result) {};

   result.file_size = bytes_read;

   CloseHandle(file);

   return result;
}

