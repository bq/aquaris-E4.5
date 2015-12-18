#ifndef __DDP_TDSHP_H__
#define __DDP_TDSHP_H__

#include "ddp_reg.h"
#include "ddp_drv.h"

#define TDS_HSYNC_WIDTH 							0xf24
#define TDS_ACTIVE_WIDTH_IN_VBLANK 	0xf3C
#define DTDS_IN_MTX_C00 							0xf50
#define DTDS_IN_MTX_C01 							0xf54
#define DTDS_IN_MTX_C02 							0xf58
#define DTDS_IN_MTX_C10 							0xf5C
#define DTDS_IN_MTX_C11 							0xf60
#define DTDS_IN_MTX_C12 							0xf64
#define DTDS_IN_MTX_C20 							0xf68
#define DTDS_IN_MTX_C21 							0xf6C
#define DTDS_IN_MTX_C22 							0xf70
#define DTDS_IN_MTX_IN_OFFSET0 				0xf74
#define DTDS_IN_MTX_IN_OFFSET1 				0xf78
#define DTDS_IN_MTX_IN_OFFSET2 				0xf7C
#define DTDS_IN_MTX_OUT_OFFSET0 			0xf80
#define DTDS_IN_MTX_OUT_OFFSET1 			0xf84
#define DTDS_IN_MTX_OUT_OFFSET2 			0xf88
#define DTDS_OUT_MTX_C00 						0xf90
#define DTDS_OUT_MTX_C01 						0xf94
#define DTDS_OUT_MTX_C02 						0xf98
#define DTDS_OUT_MTX_C10 						0xf9C
#define DTDS_OUT_MTX_C11 						0xfA0
#define DTDS_OUT_MTX_C12 						0xfA4
#define DTDS_OUT_MTX_C20 						0xfA8
#define DTDS_OUT_MTX_C21 						0xfAC
#define DTDS_OUT_MTX_C22 						0xfB0
#define DTDS_OUT_MTX_IN_OFFSET0 			0xfB4
#define DTDS_OUT_MTX_IN_OFFSET1 			0xfB8
#define DTDS_OUT_MTX_IN_OFFSET2 			0xfBC
#define DTDS_OUT_MTX_OUT_OFFSET0 		0xfC0
#define DTDS_OUT_MTX_OUT_OFFSET1 		0xfC4
#define DTDS_OUT_MTX_OUT_OFFSET2 		0xfC8
#define DTDS_CONFIG 									0xf10




#define SHARP_TUNING_INDEX 1

//TDSHP Param: (order: THSHP+PBC-REMOVED from DTV's XML)
void DpEngine_SHARPonInit(void);
void DpEngine_SHARPonConfig(unsigned int srcWidth,unsigned int srcHeight);

DISPLAY_TDSHP_T *get_TDSHP_index(void);



//--------------------------------------------------------


#endif

