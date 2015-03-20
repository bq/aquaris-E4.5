#ifndef _OS_TYPES_H
#define _OS_TYPES_H
#if !defined(__KERNEL__)
#include <sys/time.h>
#include <sys/uio.h>
#include "osal/sii_common/sii_types.h"
#endif
#include "osal/sii_common/sii_ids.h"
#include "osal/sii_common/sii_inline.h"
#define OS_NO_WAIT          0
#define OS_INFINITE_WAIT    -1
#define OS_MAX_TIMEOUT      (1000*60*60*24) 
typedef struct timeval SiiOsTimeVal_t;
typedef struct iovec SiiOsIoVec_t;
typedef uint32_t SiiOsIpAddr_t;
typedef enum
{
    SII_OS_STATUS_SUCCESS                = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_SUCCESS),             
    SII_OS_STATUS_WARN_PENDING           = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_WARN_PENDING),        
    SII_OS_STATUS_WARN_BREAK             = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_WARN_BREAK),          
    SII_OS_STATUS_WARN_INCOMPLETE        = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_WARN_INCOMPLETE),     
    SII_OS_STATUS_ERR_INVALID_HANDLE     = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_ERR_INVALID_HANDLE),  
    SII_OS_STATUS_ERR_INVALID_PARAM      = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_ERR_INVALID_PARAM),   
    SII_OS_STATUS_ERR_INVALID_OP         = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_ERR_INVALID_OP),      
    SII_OS_STATUS_ERR_NOT_AVAIL          = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_ERR_NOT_AVAIL),       
    SII_OS_STATUS_ERR_IN_USE             = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_ERR_IN_USE),          
    SII_OS_STATUS_ERR_TIMEOUT            = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_ERR_TIMEOUT),         
    SII_OS_STATUS_ERR_FAILED             = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_ERR_FAILED),          
    SII_OS_STATUS_ERR_NOT_IMPLEMENTED    = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_ERR_NOT_IMPLEMENTED), 
    SII_OS_STATUS_ERR_SEM_COUNT_EXCEEDED = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_ERR_CUSTOM1),         
    SII_OS_STATUS_ERR_QUEUE_EMPTY        = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_ERR_CUSTOM2),         
    SII_OS_STATUS_ERR_QUEUE_FULL         = SII_STATUS_SET_GROUP(SII_GROUP_OSAL, SII_STATUS_ERR_CUSTOM3),         
    SII_OS_STATUS_LAST
} SiiOsStatus_t;
static SII_INLINE const char * SiiOsStatusString(const SiiOsStatus_t status)
{
    switch (status)
    {
        case SII_OS_STATUS_SUCCESS:
            return "Success";
        case SII_OS_STATUS_WARN_PENDING:
            return "Warning-Operation Pending";
        case SII_OS_STATUS_WARN_BREAK:
            return "Warning-Break";
        case SII_OS_STATUS_WARN_INCOMPLETE:
            return "Warning-Operation Incomplete";
        case SII_OS_STATUS_ERR_INVALID_HANDLE:
            return "Error-Invalid Handle";
        case SII_OS_STATUS_ERR_INVALID_PARAM:
            return "Error-Invalid Parameter";
        case SII_OS_STATUS_ERR_INVALID_OP:
            return "Error-Invalid Operation";
        case SII_OS_STATUS_ERR_NOT_AVAIL:
            return "Error-Resource Not Available";
        case SII_OS_STATUS_ERR_IN_USE:
            return "Error-Resource In Use";
        case SII_OS_STATUS_ERR_TIMEOUT:
            return "Error-Timeout Expired";
        case SII_OS_STATUS_ERR_FAILED:
            return "Error-General Failure";
        case SII_OS_STATUS_ERR_NOT_IMPLEMENTED:
            return "Error-Not Implemented";
        case SII_OS_STATUS_ERR_SEM_COUNT_EXCEEDED:
            return "Error-Semaphore Count Exceeded";
        case SII_OS_STATUS_ERR_QUEUE_EMPTY:
            return "Error-Queue Empty";
		case SII_OS_STATUS_ERR_QUEUE_FULL:
			return "Error-Queue Full";	
        default:
            return "UNKNOWN";
    }
}
#endif 
