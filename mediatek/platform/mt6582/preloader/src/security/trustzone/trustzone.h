
#ifndef TRUSTZONE_H
#define TRUSTZONE_H

#define MOD "[TRUSTZONE]"

#define TEE_PARAMETER_ADDR              (0x100100)

/* TEE magic */
#define TEE_ARGUMENT_MAGIC              (0x4B54444DU)
/* TEE version */
#define TEE_ARGUMENT_VERSION            (0x00010000U)

/* SEC MEM magic */
#define SEC_MEM_MAGIC                   (0x3C562817U)
/* SEC MEM version */
#define SEC_MEM_VERSION                 (0x00010000U)
/* Tplay Table Size */
#define SEC_MEM_TPLAY_TABLE_SIZE        (0x1000)//4KB by default

typedef struct {
    unsigned int magic;           // Magic number
    unsigned int version;         // version
    unsigned int NWEntry;         // NW Entry point after t-base
    unsigned int NWBootArgs;      // NW boot args (propagated by t-base in r4 before jump)
    unsigned int NWBootArgsSize;  // NW boot args size (propagated by t-base in r5 before jump)
    unsigned int dRamBase;        // NonSecure DRAM start address
    unsigned int dRamSize;        // NonSecure DRAM size
    unsigned int secDRamBase;     // Secure DRAM start address
    unsigned int secDRamSize;     // Secure DRAM size
    unsigned int sRamBase;        // NonSecure Scratch RAM start address
    unsigned int sRamSize;        // NonSecure Scratch RAM size
    unsigned int secSRamBase;     // Secure Scratch RAM start address
    unsigned int secSRamSize;     // Secure Scratch RAM size
    unsigned int log_port;        // uart base address for logging
    unsigned int log_baudrate;    // uart baud rate
    unsigned int hwuid[4];        // HW Unique id for t-base used    
} tee_arg_t;

typedef struct {
    unsigned int magic;           // Magic number
    unsigned int version;         // version
    unsigned int svp_mem_start;   // MM sec mem pool start addr.
    unsigned int svp_mem_end;     // MM sec mem pool end addr.
    unsigned int tplay_table_size;  //tplay handle-to-physical table size, its start address should be (svp_mem_end - tplay_table_size);
} sec_mem_arg_t;

/**************************************************************************
 * EXPORTED FUNCTIONS
 **************************************************************************/

#endif /* TRUSTZONE_H */



