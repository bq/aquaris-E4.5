
#ifndef META_H
#define META_H

#define HSHK_TOKEN_SZ           (8)             /* handshake token size */
#define HSHK_COM_READY          "READY"         /* com ready for communication */
#define HSHK_DISCON_SZ          (10)
#define HSHK_DISCON             "DISCONNECT"    /* com disconnect for communication */

#define	META_STR_REQ            "METAMETA"      /* META request */

#if CFG_WORLD_PHONE_SUPPORT
#define META_STR_ACK            "ATEMATEX"      /* META ack Response */
#else
#define META_STR_ACK            "ATEMATEM"      /* META ack Response */
#endif

#define META_LOCK               "LOCK"          /* META lock */
#define META_FORBIDDEN_ACK      "METAFORB"
#define META_ARG_VER_STR        "ATEM0001"

#define META_ADV_REQ            "ADVEMETA"
#define META_ADV_ACK            "ATEMEVDX"

#define FACTORY_STR_REQ         "FACTFACT"
#define FACTORY_STR_ACK         "TCAFTCAF"

#define ATE_STR_REQ             "FACTORYM"      /* ATE request */
#define ATE_STR_ACK             "MYROTCAF"      /* ATE ack response */

#define SWITCH_MD_REQ           "SWITCHMD"      /* switch MD request */
#define SWITCH_MD_ACK           "DMHCTIWS"      /* switch MD ack response */

#define ATCMD_PREFIX            "AT+"
#define ATCMD_NBOOT_REQ         ATCMD_PREFIX"NBOOT"    /* AT command to trigger normal boot */
#define ATCMD_OK                ATCMD_PREFIX"OK"
#define ATCMD_UNKNOWN           ATCMD_PREFIX"UNKONWN"

#define FB_STR_REQ              "FASTBOOT"
#define FB_STR_ACK              "TOOBTSAF"

typedef struct {
    unsigned int len;           /* the length of parameter */
    unsigned int ver;           /* the version of parameter */
} para_header_t;

typedef struct {
    para_header_t header;       /* the header of parameter */
    unsigned char usb_type;     /* 0: single interface device, 1: composite device */
    unsigned char usb_num;      /* usb com port number */
    unsigned char md0_type;     /* md image type for md1 */
    unsigned char md1_type;     /* md image type for md1 */
} para_v1_t;

typedef union {
    para_header_t header;
    para_v1_t     v0001;
} para_t;

#endif /* META_H */



