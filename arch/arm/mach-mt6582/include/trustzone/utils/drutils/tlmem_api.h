/*
 * (c) TRUSTONIC LIMITED 2013
 */

 
#ifndef __TLAPIROT13_H__
#define __TLAPIROT13_H__

//#include "TlApi/TlApiCommon.h"
//#include "TlApi/TlApiError.h"

/* Marshaled function parameters.
 * structs and union of marshaling parameters via TlApi.
 *
 * @note The structs can NEVER be packed !
 * @note The structs can NOT used via sizeof(..) !
 */

 /*
  * Function id definitions
  */
#define FID_DR_OPEN_SESSION     1
#define FID_DR_CLOSE_SESSION    2
#define FID_DR_INIT_DATA        3
#define FID_DR_EXECUTE          4
//#define FID_DRV_ADD_FOO         5
//#define FID_DRV_SUB_FOO         6
/* .. add more when needed */

/* Marshaled function parameters.
 * structs and union of marshaling parameters via TlApi.
 *
 * @note The structs can NEVER be packed !
 * @note The structs can NOT used via sizeof(..) !
 */
#define QUERY_MAX_LEN 8

typedef struct {
    uint32_t    commandId;
    uint32_t    phy_addr;
    uint32_t    size;
    uint32_t    alignment;
    uint32_t    refcount;
    uint32_t    handle;
} tlApimem_t, *tlApimem_ptr;

#define DRUTILS_SEC_SYSTRACE_START  (1<<0)
#define DRUTILS_SEC_SYSTRACE_STOP   (1<<1)
#define DRUTILS_SEC_SYSTRACE_PAUSE  (1<<2)
#define DRUTILS_SEC_SYSTRACE_RESUME (1<<3)

typedef struct {	
    uint32_t commandId;
	unsigned long buf;
	unsigned long size;
	uint32_t head;
	uint32_t event;
} drutils_systrace_param_t, *drutils_systrace_param_ptr;

#endif // __TLAPIROT13_H__



