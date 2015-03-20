
#ifndef _KAL_NON_SPECIFIC_GENERAL_TYPES_H
#define _KAL_NON_SPECIFIC_GENERAL_TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef unsigned char           kal_uint8;
typedef signed char             kal_int8;
typedef char                    kal_char;
typedef unsigned short          kal_wchar;

typedef unsigned short      kal_uint16;
typedef signed short       kal_int16;

typedef unsigned int            kal_uint32;
typedef signed int              kal_int32;



typedef enum 
{
  KAL_FALSE=0,
  KAL_TRUE
} kal_bool;
 
//#define DRV_ISP_6238_SERIES

#define SIM_ON_PC

#define ASSERT(n) { if (!(n)) { printf("ASSERT ERROR\n"); } }
#define kal_mem_cpy(a,b,c) memcpy(a,b,c)
#define kal_mem_set(a,b,c) memset(a,b,c)


#define drv_get_current_time() 0;
#define drv_get_duration_ms(a) 0;

#endif  /* _KAL_NON_SPECIFIC_GENERAL_TYPES_H */


