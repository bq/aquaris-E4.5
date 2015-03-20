/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ccmni_pfp.h
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *   MT6516 Cross Chip Modem Network Interface - Packet Framing Protocol
 *
 * Author:
 * -------
 *   Stanley Chou (mtk01411)
 *
 ****************************************************************************/
#ifndef __CCCI_CCMNI_PFP_H__
#define __CCCI_CCMNI_PFP_H__
/* Compile Option to decide if DYNAMIC_MULTIPLE_FRAME encode/decode is supported or not */

#define MAX_PDP_CONT_NUM          3
#define PFP_FRAME_START_FLAG      0xF9
#define PFP_FRAME_MAGIC_NUM       0x66
/* MAX_PFP_LEN_FIELD_VALUE is the maximum size of one IP Packet */
#define MAX_PFP_LEN_FIELD_VALUE   1500
#define FREE_RAW_DATA_BUF_SIZE    2048
#define FREE_COOKED_DATA_BUF_SIZE 2048
#define SUPPORT_FRAME_NUM 1
#define SUPPORT_PKT_NUM 1024

enum frame_flag_t
{
    FRAME_START = 0,
    FRAME_CONTINUOUS,    
    FRAME_END    
};

enum unframe_state_t
{
    PARSE_PFP_FRAME_START_FLAG_STATE = 0,
    PARSE_PFP_FRAME_MAGIC_NUM_STATE,
    PARSE_PFP_FRAME_LENGTH_FIELD_STATE,
    PARSE_PFP_FRAME_GET_DATA_STATE
};

/* Following implementations are designed for PFP(Packet Frame Protocol) */ 
typedef struct
{
    int            frame_size;
    unsigned char *frame_data;
} frame_format_t;

typedef struct 
{
    int num_frames;
    int pending_data_flag;
    int consumed_length;
#ifdef __SUPPORT_DYNAMIC_MULTIPLE_FRAME__
    frame_format_t *frame_list;
#else
    frame_format_t frame_list[SUPPORT_FRAME_NUM];
#endif
} frame_info_t;

typedef struct _complete_ippkt_t
{
    int            pkt_size;   
    unsigned char *pkt_data;
#ifndef __SUPPORT_DYNAMIC_MULTIPLE_FRAME__
    int            entry_used;
#endif
    struct _complete_ippkt_t *next;
} complete_ippkt_t;

typedef struct _packet_info_t
{
    int                  num_complete_packets;
    int                  consumed_length;
    int                  try_decode_again;
    enum unframe_state_t parse_data_state;
    complete_ippkt_t    *pkt_list;
} packet_info_t;

typedef struct _ccmni_record_t
{
    /* Next expected state to be parsed while entering the pfp_unframe() again */ 
    enum unframe_state_t unframe_state;
    /* Record the latest parsed Packet length for getting the data */ 
    int pkt_size;
    /* For fast to find the last node of pkt_list to insert a new parsed IP Pkt into this pkt_list */ 
    complete_ippkt_t *last_pkt_node;
} ccmni_record_t;

extern ccmni_record_t ccmni_dev[];

/* The following buffers are used for testing purpose */
/* Store one IP Packet data */
extern unsigned char frame_cooked_data  [];
/* Pack the IP Packet into a Frame sent to Modem */
extern unsigned char frame_raw_data     [];
extern unsigned char unframe_raw_data   [];
extern unsigned char unframe_cooked_data[];


void          pfp_reset  (int ccmni_inx);
frame_info_t  pfp_frame  (unsigned char* raw_data, unsigned char* cooked_data,
                          int cooked_size, int frame_flag, int ccmni_inx);
packet_info_t pfp_unframe(unsigned char* cooked_data, int cooked_data_buf_size,
                          unsigned char* raw_data, int raw_size, int ccmni_inx);
void          traverse_pkt_list(complete_ippkt_t *node);

#ifndef __SUPPORT_DYNAMIC_MULTIPLE_FRAME__
complete_ippkt_t* get_one_available_complete_ippkt_entry(void);
void          release_one_used_complete_ippkt_entry(complete_ippkt_t* entry);   
#endif

#endif // __CCCI_CCMNI_PFP_H__ 