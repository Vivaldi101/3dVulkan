#if !defined(_APP_H)
#define _APP_H

#include "common.h"

#if 1
align_struct app_input
{
   hw_input_type input_type;
   union { i32 pos[2]; u64 key; } ;
} app_input;
#endif

void app_start(int argc, const char** argv, hw* hw);

#endif
