#if !defined(_APP_H)
#define _APP_H

#include "common.h"

cache_align typedef struct app_input
{
   hw_input_type input_type;
   union { i32 pos[2]; u64 key; };
} app_input;

void app_start(int argc, const char** argv, hw* hw);

#endif
