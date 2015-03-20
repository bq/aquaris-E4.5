#ifndef MTK_QMU_H
#define MTK_QMU_H

#include <linux/mu3d/hal/mu3d_hal_osal.h>
#include <linux/mu3d/hal/mu3d_hal_hw.h>
#include <linux/mu3d/hal/mu3d_hal_comm.h>
#include <linux/mu3d/hal/mu3d_hal_usb_drv.h>



typedef struct _TGPD {
    DEV_UINT8	flag;
    DEV_UINT8	chksum;
    DEV_UINT16	DataBufferLen; /*Rx Allow Length*/
    struct _TGPD*	pNext;
    DEV_UINT8*	pBuf;
    DEV_UINT16	bufLen;
    DEV_UINT8	ExtLength;
    DEV_UINT8	ZTepFlag;
}TGPD, *PGPD;

typedef struct _TBD {
    DEV_UINT8  flag;
    DEV_UINT8  chksum;
    DEV_UINT16  DataBufferLen; /*Rx Allow Length*/
    struct _TBD *pNext;
    DEV_UINT8 *pBuf;
    DEV_UINT16 bufLen;
    DEV_UINT8  extLen;
    DEV_UINT8  reserved;
}TBD, *PBD;

typedef struct _GPD_RANGE {
    PGPD	pNext;
    PGPD	pStart;
    PGPD	pEnd;
}GPD_R, *RGPD;

typedef struct _BD_RANGE {
    PBD	pNext;
    PBD	pStart;
    PBD	pEnd;
}BD_R, *RBD;


#define AT_GPD_EXT_LEN					0//256
#define AT_BD_EXT_LEN 				0//256
#define MAX_GPD_NUM					10//30
#define MAX_BD_NUM					50//500
//DVT+
#define STRESS_IOC_TH 				8
#define STRESS_GPD_TH 				24
#define RANDOM_STOP_DELAY 				80
#define STRESS_DATA_LENGTH 			1024*64//1024*16
//DVT-
#define GPD_BUF_SIZE 65532
#define BD_BUF_SIZE 32768 //set to half of 64K of max size

#define IS_BDP 1

#define MAX_QMU_EP 					MAX_EP_NUM /*The better way is to read U3D_CAP_EPINFO*/

#define TGPD_FLAGS_HWO              	0x01
#define TGPD_IS_FLAGS_HWO(_pd)      	(((TGPD *)_pd)->flag & TGPD_FLAGS_HWO)
#define TGPD_SET_FLAGS_HWO(_pd)     	(((TGPD *)_pd)->flag |= TGPD_FLAGS_HWO)
#define TGPD_CLR_FLAGS_HWO(_pd)     	(((TGPD *)_pd)->flag &= (~TGPD_FLAGS_HWO))
#define TGPD_FORMAT_BDP             	0x02
#define TGPD_IS_FORMAT_BDP(_pd)     	(((TGPD *)_pd)->flag & TGPD_FORMAT_BDP)
#define TGPD_SET_FORMAT_BDP(_pd)    	(((TGPD *)_pd)->flag |= TGPD_FORMAT_BDP)
#define TGPD_CLR_FORMAT_BDP(_pd)    	(((TGPD *)_pd)->flag &= (~TGPD_FORMAT_BDP))
#define TGPD_FORMAT_BPS             	0x04
#define TGPD_IS_FORMAT_BPS(_pd)     	(((TGPD *)_pd)->flag & TGPD_FORMAT_BPS)
#define TGPD_SET_FORMAT_BPS(_pd)    	(((TGPD *)_pd)->flag |= TGPD_FORMAT_BPS)
#define TGPD_CLR_FORMAT_BPS(_pd)    	(((TGPD *)_pd)->flag &= (~TGPD_FORMAT_BPS))
#define TGPD_SET_FLAG(_pd, _flag)   	((TGPD *)_pd)->flag = (((TGPD *)_pd)->flag&(~TGPD_FLAGS_HWO))|(_flag)
#define TGPD_GET_FLAG(_pd)             	(((TGPD *)_pd)->flag & TGPD_FLAGS_HWO)
#define TGPD_SET_CHKSUM(_pd, _n)    	((TGPD *)_pd)->chksum = mu3d_hal_cal_checksum((DEV_UINT8 *)_pd, _n)-1
#define TGPD_SET_CHKSUM_HWO(_pd, _n)    ((TGPD *)_pd)->chksum = mu3d_hal_cal_checksum((DEV_UINT8 *)_pd, _n)-1
#define TGPD_GET_CHKSUM(_pd)        	((TGPD *)_pd)->chksum
#define TGPD_SET_FORMAT(_pd, _fmt)  	((TGPD *)_pd)->flag = (((TGPD *)_pd)->flag&(~TGPD_FORMAT_BDP))|(_fmt)
#define TGPD_GET_FORMAT(_pd)        	((((TGPD *)_pd)->flag & TGPD_FORMAT_BDP)>>1)
#define TGPD_SET_DataBUF_LEN(_pd, _len) ((TGPD *)_pd)->DataBufferLen = _len
#define TGPD_ADD_DataBUF_LEN(_pd, _len) ((TGPD *)_pd)->DataBufferLen += _len
#define TGPD_GET_DataBUF_LEN(_pd)       ((TGPD *)_pd)->DataBufferLen
#define TGPD_SET_NEXT(_pd, _next)   	((TGPD *)_pd)->pNext = (TGPD *)_next;
#define TGPD_GET_NEXT(_pd)          	(TGPD *)((TGPD *)_pd)->pNext
#define TGPD_SET_TBD(_pd, _tbd)     	((TGPD *)_pd)->pBuf = (DEV_UINT8 *)_tbd;\
                                    	TGPD_SET_FORMAT_BDP(_pd)
#define TGPD_GET_TBD(_pd)           	(TBD *)((TGPD *)_pd)->pBuf
#define TGPD_SET_DATA(_pd, _data)   	((TGPD *)_pd)->pBuf = (DEV_UINT8 *)_data
#define TGPD_GET_DATA(_pd)          	(DEV_UINT8*)((TGPD *)_pd)->pBuf
#define TGPD_SET_BUF_LEN(_pd, _len) 	((TGPD *)_pd)->bufLen = _len
#define TGPD_ADD_BUF_LEN(_pd, _len) 	((TGPD *)_pd)->bufLen += _len
#define TGPD_GET_BUF_LEN(_pd)       	((TGPD *)_pd)->bufLen
#define TGPD_SET_EXT_LEN(_pd, _len) 	((TGPD *)_pd)->ExtLength = _len
#define TGPD_GET_EXT_LEN(_pd)        	((TGPD *)_pd)->ExtLength
#define TGPD_SET_EPaddr(_pd, _EP)  		((TGPD *)_pd)->ZTepFlag =(((TGPD *)_pd)->ZTepFlag&0xF0)|(_EP)
#define TGPD_GET_EPaddr(_pd)        	((TGPD *)_pd)->ZTepFlag & 0x0F
#define TGPD_FORMAT_TGL             	0x10
#define TGPD_IS_FORMAT_TGL(_pd)     	(((TGPD *)_pd)->ZTepFlag & TGPD_FORMAT_TGL)
#define TGPD_SET_FORMAT_TGL(_pd)    	(((TGPD *)_pd)->ZTepFlag |=TGPD_FORMAT_TGL)
#define TGPD_CLR_FORMAT_TGL(_pd)    	(((TGPD *)_pd)->ZTepFlag &= (~TGPD_FORMAT_TGL))
#define TGPD_FORMAT_ZLP             	0x20
#define TGPD_IS_FORMAT_ZLP(_pd)     	(((TGPD *)_pd)->ZTepFlag & TGPD_FORMAT_ZLP)
#define TGPD_SET_FORMAT_ZLP(_pd)    	(((TGPD *)_pd)->ZTepFlag |=TGPD_FORMAT_ZLP)
#define TGPD_CLR_FORMAT_ZLP(_pd)    	(((TGPD *)_pd)->ZTepFlag &= (~TGPD_FORMAT_ZLP))
#define TGPD_FORMAT_IOC             	0x80
#define TGPD_IS_FORMAT_IOC(_pd)     	(((TGPD *)_pd)->flag & TGPD_FORMAT_IOC)
#define TGPD_SET_FORMAT_IOC(_pd)    	(((TGPD *)_pd)->flag |=TGPD_FORMAT_IOC)
#define TGPD_CLR_FORMAT_IOC(_pd)    	(((TGPD *)_pd)->flag &= (~TGPD_FORMAT_IOC))
#define TGPD_SET_TGL(_pd, _TGL)  		((TGPD *)_pd)->ZTepFlag |=(( _TGL) ? 0x10: 0x00)
#define TGPD_GET_TGL(_pd)        		((TGPD *)_pd)->ZTepFlag & 0x10 ? 1:0
#define TGPD_SET_ZLP(_pd, _ZLP)  		((TGPD *)_pd)->ZTepFlag |= ((_ZLP) ? 0x20: 0x00)
#define TGPD_GET_ZLP(_pd)        		((TGPD *)_pd)->ZTepFlag & 0x20 ? 1:0
#define TGPD_GET_EXT(_pd)           	((DEV_UINT8 *)_pd + sizeof(TGPD))


#define TBD_FLAGS_EOL               	0x01
#define TBD_IS_FLAGS_EOL(_bd)       	(((TBD *)_bd)->flag & TBD_FLAGS_EOL)
#define TBD_SET_FLAGS_EOL(_bd)      	(((TBD *)_bd)->flag |= TBD_FLAGS_EOL)
#define TBD_CLR_FLAGS_EOL(_bd)      	(((TBD *)_bd)->flag &= (~TBD_FLAGS_EOL))
#define TBD_SET_FLAG(_bd, _flag)    	((TBD *)_bd)->flag = (DEV_UINT8)_flag
#define TBD_GET_FLAG(_bd)           	((TBD *)_bd)->flag
#define TBD_SET_CHKSUM(_pd, _n)     	((TBD *)_pd)->chksum = mu3d_hal_cal_checksum((DEV_UINT8 *)_pd, _n)
#define TBD_GET_CHKSUM(_pd)         	((TBD *)_pd)->chksum
#define TBD_SET_DataBUF_LEN(_pd, _len) 	((TBD *)_pd)->DataBufferLen = _len
#define TBD_GET_DataBUF_LEN(_pd)       	((TBD *)_pd)->DataBufferLen
#define TBD_SET_NEXT(_bd, _next)    	((TBD *)_bd)->pNext = (TBD *)_next
#define TBD_GET_NEXT(_bd)           	(TBD *)((TBD *)_bd)->pNext
#define TBD_SET_DATA(_bd, _data)    	((TBD *)_bd)->pBuf = (DEV_UINT8 *)_data
#define TBD_GET_DATA(_bd)           	(DEV_UINT8*)((TBD *)_bd)->pBuf
#define TBD_SET_BUF_LEN(_bd, _len)  	((TBD *)_bd)->bufLen = _len
#define TBD_ADD_BUF_LEN(_bd, _len)  	((TBD *)_bd)->bufLen += _len
#define TBD_GET_BUF_LEN(_bd)        	((TBD *)_bd)->bufLen
#define TBD_SET_EXT_LEN(_bd, _len)  	((TBD *)_bd)->extLen = _len
#define TBD_ADD_EXT_LEN(_bd, _len)  	((TBD *)_bd)->extLen += _len
#define TBD_GET_EXT_LEN(_bd)        	((TBD *)_bd)->extLen
#define TBD_GET_EXT(_bd)            	((DEV_UINT8 *)_bd + sizeof(TBD))



#undef EXTERN

#ifdef _MTK_QMU_DRV_EXT_
#define EXTERN
#else
#define EXTERN extern
#endif


EXTERN DEV_UINT8 is_bdp;
//DVT+
EXTERN DEV_UINT32 gpd_buf_size;
EXTERN DEV_UINT16 bd_buf_size;
EXTERN DEV_UINT8 bBD_Extension;
EXTERN DEV_UINT8 bGPD_Extension;
EXTERN DEV_UINT32 g_dma_buffer_size;
//DVT+
EXTERN PGPD Rx_gpd_head[15];
EXTERN PGPD Tx_gpd_head[15];
EXTERN PGPD Rx_gpd_end[15];
EXTERN PGPD Tx_gpd_end[15];
EXTERN PGPD Rx_gpd_last[15];
EXTERN PGPD Tx_gpd_last[15];
EXTERN GPD_R Rx_gpd_List[15];
EXTERN GPD_R Tx_gpd_List[15];
EXTERN BD_R Rx_bd_List[15];
EXTERN BD_R Tx_bd_List[15];
EXTERN DEV_UINT32 Rx_gpd_Offset[15];
EXTERN DEV_UINT32 Tx_gpd_Offset[15];
EXTERN DEV_UINT32 Rx_bd_Offset[15];
EXTERN DEV_UINT32 Tx_bd_Offset[15];


EXTERN void mu3d_hal_resume_qmu(DEV_INT32 Q_num,  USB_DIR dir);
EXTERN void mu3d_hal_stop_qmu(DEV_INT32 Q_num,  USB_DIR dir);
EXTERN TGPD* mu3d_hal_prepare_tx_gpd(TGPD* gpd, dma_addr_t pBuf, DEV_UINT32 data_length, DEV_UINT8 ep_num, DEV_UINT8 _is_bdp, DEV_UINT8 isHWO,DEV_UINT8 ioc, DEV_UINT8 bps,DEV_UINT8 zlp);
EXTERN TGPD* mu3d_hal_prepare_rx_gpd(TGPD*gpd, dma_addr_t pBuf, DEV_UINT32 data_len, DEV_UINT8 ep_num, DEV_UINT8 _is_bdp, DEV_UINT8 isHWO, DEV_UINT8 ioc, DEV_UINT8 bps,DEV_UINT32 cMaxPacketSize);
EXTERN void mu3d_hal_insert_transfer_gpd(DEV_INT32 ep_num,USB_DIR dir, dma_addr_t buf, DEV_UINT32 count, DEV_UINT8 isHWO, DEV_UINT8 ioc, DEV_UINT8 bps,DEV_UINT8 zlp, DEV_UINT32 cMaxPacketSize );
EXTERN void mu3d_hal_alloc_qmu_mem(void);
EXTERN void mu3d_hal_init_qmu(void);
EXTERN void mu3d_hal_start_qmu(DEV_INT32 Q_num,  USB_DIR dir);
EXTERN void mu3d_hal_flush_qmu(DEV_INT32 Q_num,  USB_DIR dir);
EXTERN void mu3d_hal_restart_qmu(DEV_INT32 Q_num,  USB_DIR dir);
EXTERN void mu3d_hal_send_stall(DEV_INT32 Q_num,  USB_DIR dir);
EXTERN DEV_UINT8 mu3d_hal_cal_checksum(DEV_UINT8 *data, DEV_INT32 len);
EXTERN dma_addr_t mu3d_hal_gpd_virt_to_phys(void *vaddr,USB_DIR dir,DEV_UINT32 num);

EXTERN PBD get_bd(USB_DIR dir,DEV_UINT32 num);
EXTERN dma_addr_t bd_virt_to_phys(void *vaddr,USB_DIR dir,DEV_UINT32 num);
EXTERN void *bd_phys_to_virt(void *paddr,USB_DIR dir,DEV_UINT32 num);

EXTERN PGPD get_gpd(USB_DIR dir,DEV_UINT32 num);
EXTERN void *gpd_phys_to_virt(void *paddr,USB_DIR dir,DEV_UINT32 num);
EXTERN void gpd_ptr_align(USB_DIR dir,DEV_UINT32 num,PGPD ptr);

#undef EXTERN

#endif
