
#ifndef __META_LCDFT_PARA_H__
#define __META_LCDFT_PARA_H__

#include "FT_Public.h"

typedef struct {
    unsigned long time_duration;
} LCDFt_REQ;
 
typedef struct {
    bool status;
} LCDFt_CNF;
 
bool Meta_LCDFt_Init(void);
LCDFt_CNF Meta_LCDFt_OP(LCDFt_REQ req);
bool Meta_LCDFt_Deinit(void);

#endif  // __META_LCDFT_PARA_H__


