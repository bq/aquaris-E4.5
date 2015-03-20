
#ifndef MT_EMI_H
#define MT_EMI_H

//#define DDR_RESERVE_MODE

typedef volatile unsigned int *V_UINT32P;
extern void mt6516_set_emi (void);
extern void mt6516_256M_mem_setting (void);
int get_dram_rank_nr (void);
int get_dram_rank_size (int dram_rank_size[]);
#define DDR1  1
#define LPDDR2  2
#define LPDDR3  3
#define PCDDR3  4
typedef struct
{
    int   sub_version;            // sub_version: 0x1 for new version
    int  type;                /* 0x0000 : Invalid
                                 0x0001 : Discrete DDR1
                                 0x0002 : Discrete LPDDR2
                                 0x0003 : Discrete LPDDR3
                                 0x0004 : Discrete PCDDR3
                                 0x0101 : MCP(NAND+DDR1)
                                 0x0102 : MCP(NAND+LPDDR2)
                                 0x0103 : MCP(NAND+LPDDR3)
                                 0x0104 : MCP(NAND+PCDDR3)
                                 0x0201 : MCP(eMMC+DDR1)
                                 0x0202 : MCP(eMMC+LPDDR2)
                                 0x0203 : MCP(eMMC+LPDDR3)
                                 0x0204 : MCP(eMMC+PCDDR3)
                              */
    int   id_length;              // EMMC and NAND ID checking length
    int   fw_id_length;              // FW ID checking length
    char  ID[16];
    char  fw_id[8];               // To save fw id
    int   EMI_CONA_VAL;           //@0x3000
    int   DRAMC_DRVCTL0_VAL;      //@0x40B8               -> customized TX I/O driving
    int   DRAMC_DRVCTL1_VAL;      //@0x40BC               -> customized TX I/O driving
    int   DRAMC_ACTIM_VAL;        //@0x4000
    int   DRAMC_GDDR3CTL1_VAL;    //@0x40F4
    int   DRAMC_CONF1_VAL;        //@0x4004
    int   DRAMC_DDR2CTL_VAL;      //@0x407C 
    int   DRAMC_TEST2_3_VAL;      //@0x4044
    int   DRAMC_CONF2_VAL;        //@0x4008
    int   DRAMC_PD_CTRL_VAL;      //@0x41DC
    int   DRAMC_PADCTL3_VAL;      //@0x4014               -> customized TX DQS delay
    int   DRAMC_DQODLY_VAL;       //@0x4200~0x420C        -> customized TX DQ delay
    int   DRAMC_ADDR_OUTPUT_DLY;  // for E1 DDR2 only
    int   DRAMC_CLK_OUTPUT_DLY;   // for E1 DDR2 only
    int   DRAMC_ACTIM1_VAL;       //@0x41E8
    int   DRAMC_MISCTL0_VAL;      //@0x40FC
    int   DRAMC_ACTIM05T_VAL;     //@0x41F8
    int   DRAM_RANK_SIZE[4];
    int   reserved[10];

    union
    {
        struct
        {
            int   LPDDR2_MODE_REG_1;
            int   LPDDR2_MODE_REG_2;
            int   LPDDR2_MODE_REG_3;
            int   LPDDR2_MODE_REG_5;
            int   LPDDR2_MODE_REG_10;
            int   LPDDR2_MODE_REG_63;
        };
        struct
        {
            int   DDR1_MODE_REG;
            int   DDR1_EXT_MODE_REG;
        };
        struct
        {
            int   PCDDR3_MODE_REG0;
            int   PCDDR3_MODE_REG1;
            int   PCDDR3_MODE_REG2;
            int   PCDDR3_MODE_REG3;
        };
        struct
        {
            int   LPDDR3_MODE_REG_1;
            int   LPDDR3_MODE_REG_2;
            int   LPDDR3_MODE_REG_3;
            int   LPDDR3_MODE_REG_5;			
            int   LPDDR3_MODE_REG_10;
            int   LPDDR3_MODE_REG_63;
        };
    };
} EMI_SETTINGS;

int mt_get_dram_type (void); 
/* 0: invalid */
/* 1: mDDR1 */
/* 2: LPDDR2 */
/* 3: LPDDR3 */
/* 4: PCDDR3 */
#endif


