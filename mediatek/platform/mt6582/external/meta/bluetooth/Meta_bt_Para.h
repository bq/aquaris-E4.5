
#ifndef __META_BT_PARA_H_
#define __META_BT_PARA_H_

#define FT_CNF_OK     0
#define FT_CNF_FAIL   1

#include <stdbool.h>
#ifndef BOOL
#define BOOL  bool
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if 0
/* the FT interface Description: */

/*/////////////////////////////////////////////////////////////////////////
Description:
	the module use this function to send cnf data to PC. For the data of one frame, 
	!!!!Local_len + Peer_len <= 2031 bytes, Peer_len must <=2000 !!!!
Parameters:
	Para1: [in]--Local_buf is cmd data of XXX module
	Para2: [in]--Local_len is length of cmd data
	Para3: [in]--Peer_buf: if Peer_buf exist, Peer_buf is used to transfer bulk of data, like write/read file, transfer image/picture; 
                         !!!! if not exist, just use null ptr or else !!!!!
	Para4: [in]--Peer_len: Peer buf length, 
                         !!!! if peer buf don't exist, please set to 0!!!!!
return value:  TRUE: sucesess, otherwise indicate fail
/////////////////////////////////////////////////////////////////////////*/
BOOL WriteDataToPC(void *Local_buf,unsigned short Local_len,void *Peer_buf,unsigned short Peer_len);

#endif

/* The TestCase Enum define of BT_module */
typedef enum {
  BT_OP_HCI_SEND_COMMAND = 0,
  BT_OP_HCI_CLEAN_COMMAND,
  BT_OP_HCI_SEND_DATA,
  BT_OP_HCI_TX_PURE_TEST,
  BT_OP_HCI_RX_TEST_START,
  BT_OP_HCI_RX_TEST_END,
  BT_OP_HCI_TX_PURE_TEST_V2,
  BT_OP_HCI_RX_TEST_START_V2,
  BT_OP_ENABLE_NVRAM_ONLINE_UPDATE,
  BT_OP_DISABLE_NVRAM_ONLINE_UPDATE,
  BT_OP_ENABLE_PCM_CLK_SYNC_SIGNAL,
  BT_OP_DISABLE_PCM_CLK_SYNC_SIGNAL,
  BT_OP_GET_CHIP_ID,
  BT_OP_INIT,
  BT_OP_DEINIT,
  BT_OP_END
} BT_OP;

typedef enum {
  BT_CHIP_ID_MT6611 = 0,
  BT_CHIP_ID_MT6612,
  BT_CHIP_ID_MT6616,
  BT_CHIP_ID_MT6620,
  BT_CHIP_ID_MT6622,
  BT_CHIP_ID_MT6626,
  BT_CHIP_ID_MT6628,
  BT_CHIP_ID_MT6572,
  BT_CHIP_ID_MT6582,
  BT_CHIP_ID_MT6592,
  BT_CHIP_ID_MT6571,
  BT_CHIP_ID_MT6630
} BT_CHIP_ID;

/* The HCI command struct */
typedef struct {
  unsigned short      opcode;
  unsigned char       len;
  unsigned char       parms[256];
} BT_HCI_CMD;

typedef struct {
  unsigned short      con_hdl;
  unsigned short      len;
  unsigned char       buffer[1024];
} BT_HCI_BUFFER;

typedef union {
  BT_HCI_CMD          hcicmd;
  BT_HCI_BUFFER       hcibuf;
  unsigned int        dummy;
} BT_CMD;

typedef struct {
  unsigned char       event;
  unsigned short      handle;
  unsigned char       len;
  unsigned char       status;
  unsigned char       parms[256];
} BT_HCI_EVENT;

typedef union {
  BT_HCI_EVENT        hcievent;
  BT_HCI_BUFFER       hcibuf;
  unsigned int        dummy;
} BT_RESULT;

typedef struct {
  FT_H                header;
  BT_OP               op;
  BT_CMD              cmd;
} BT_REQ;

typedef struct {
  FT_H                header;
  BT_OP               op;
  unsigned int        bt_status;  //bt->FT
  unsigned char       eventtype;  //result type
  BT_RESULT           bt_result;  //result
  unsigned char       status;
} BT_CNF;

BOOL META_BT_init(void);
void META_BT_deinit(void);
void META_BT_OP(BT_REQ *req, char *peer_buff, unsigned short peer_len);

#ifdef __cplusplus
};
#endif
#endif


