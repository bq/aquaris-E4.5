#ifndef _PMT_H
#define _PMT_H

#ifdef CONFIG_MTK_EMMC_SUPPORT

#define MAX_PARTITION_NAME_LEN 64
typedef struct
{
    unsigned char name[MAX_PARTITION_NAME_LEN];     /* partition name */
    unsigned long long size;     						/* partition size */
    unsigned long long offset;       					/* partition start */
    unsigned long long mask_flags;       				/* partition flags */
} pt_resident;

struct pt_info {
    int sequencenumber:8;
    int tool_or_sd_update:4;
    int pt_next:4;
    int mirror_pt_dl:4;         //mirror download OK
    int mirror_pt_has_space:4;
    int pt_changed:4;
    int pt_has_space:4;
};

struct DM_PARTITION_INFO {
    char part_name[MAX_PARTITION_NAME_LEN];     /* the name of partition */
    unsigned long long start_addr;              /* the start address of partition */
    unsigned long long part_len;                /* the length of partition */
    unsigned char visible;                      /* visible is 0: this partition is hidden and NOT downloadable */
                                                /* visible is 1: this partition is visible and downloadable */                        
    unsigned char dl_selected;                  /* dl_selected is 0: this partition is NOT selected to download */
                                                /* dl_selected is 1: this partition is selected to download */
};

struct DM_PARTITION_INFO_PACKET {
    unsigned int pattern;
    unsigned int part_num;                      /* The actual number of partitions */
    struct DM_PARTITION_INFO part_info[PART_MAX_COUNT];
};

#define XBR_COUNT 8

struct MBR_EBR_struct {
    char xbr_name[8];
    int part_index[4];  // 4 slots per xbr
};

#endif

#define DM_ERR_OK       (0)
#define ERR_NO_EXIST    (1)

#define PT_SIG          (0x50547631) //"PTv1"
#define MPT_SIG         (0x4D505431) //"MPT1"
#define PT_SIG_SIZE 4

#define is_valid_pt(buf) ((*(u32 *)(buf))==PT_SIG)
#define is_valid_mpt(buf) ((*(u32 *)(buf))==MPT_SIG)

#endif
