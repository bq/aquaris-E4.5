
/******************************************************************************
*
* Filename:
* ---------
*     buffer.h
*/
#ifndef BUFFER_ADDR_H
#define BUFFER_ADDR_H

#include "platform.h"
#include "dram_buffer.h"


#define SEC_SECRO_BUFFER_START      sec_secro_buf
#define SEC_SECRO_BUFFER_LENGTH     DRAM_SEC_SECRO_BUFFER_LENGTH

#define SEC_WORKING_BUFFER_START    sec_working_buf
#define SEC_WORKING_BUFFER_LENGTH   DRAM_SEC_WORKING_BUFFER_LENGTH

#define SEC_UTIL_BUFFER_START       sec_util_buf
#define SEC_UTIL_BUFFER_LENGTH      DRAM_SEC_UTIL_BUFFER_LENGTH

/*SecLib.a use DRAM*/
#define SEC_LIB_HEAP_START          sec_lib_heap_buf
#define SEC_LIB_HEAP_LENGTH         DRAM_SEC_LIB_HEAP_LENGTH

/*For v3 verify check buffer */
#define SEC_IMG_BUFFER_START        sec_img_buf
#define SEC_IMG_BUFFER_LENGTH       DRAM_SEC_IMG_BUFFER_LENGTH

#define SEC_CHUNK_BUFFER_START      sec_chunk_buf
#define SEC_CHUNK_BUFFER_LENGTH     DRAM_SEC_CHUNK_BUFFER_LENGTH

/************************************/
/*preloader download DA use DRAM*/
#define DA_RAM_ADDR                 (CFG_DA_RAM_ADDR)
#define DA_RAM_LENGTH               (0x30000)
/*preloader validate DA use DRAM*/
#define DA_RAM_RELOCATE_ADDR        (CFG_DA_RAM_ADDR + DA_RAM_LENGTH)
#define DA_RAM_RELOCATE_LENGTH      (DA_RAM_LENGTH)

#if 0
extern u8 sec_secro_buf[SEC_SECRO_BUFFER_LENGTH];
extern u8 sec_working_buf[SEC_WORKING_BUFFER_LENGTH];
extern u8 sec_util_buf[SEC_UTIL_BUFFER_LENGTH];
extern u8 sec_lib_heap_buf[SEC_LIB_HEAP_LENGTH];
extern u8 sec_img_buf[SEC_IMG_BUFFER_LENGTH];
extern u8 sec_chunk_buf[SEC_CHUNK_BUFFER_LENGTH];
#endif

#define  sec_secro_buf    g_dram_buf->sec_secro_buf 
#define  sec_working_buf  g_sec_buf.sram_sec_working_buf
#define  sec_util_buf     g_dram_buf->sec_util_buf
#define  sec_lib_heap_buf g_dram_buf->sec_lib_heap_buf
#define  sec_img_buf      g_sec_buf.sram_sec_img_buf
#define  sec_chunk_buf    g_dram_buf->sec_chunk_buf
#endif





