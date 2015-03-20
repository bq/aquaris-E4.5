/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef __LINUX_IF_PPPOX_H
#define __LINUX_IF_PPPOX_H
#include <linux/types.h>
#include <asm/byteorder.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#include <linux/if_pppol2tp.h>
#include <linux/if_pppolac.h>
#include <linux/if_pppopns.h>
#ifndef AF_PPPOX
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define AF_PPPOX 24
#define PF_PPPOX AF_PPPOX
#endif
typedef __be16 sid_t;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct pppoe_addr {
 sid_t sid;
 unsigned char remote[ETH_ALEN];
 char dev[IFNAMSIZ];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct pptp_addr {
 __be16 call_id;
 struct in_addr sin_addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define PX_PROTO_OE 0
#define PX_PROTO_OL2TP 1
#define PX_PROTO_PPTP 2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define PX_PROTO_OLAC 3
#define PX_PROTO_OPNS 4
#define PX_MAX_PROTO 5
struct sockaddr_pppox {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 sa_family_t sa_family;
 unsigned int sa_protocol;
 union {
 struct pppoe_addr pppoe;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct pptp_addr pptp;
 } sa_addr;
} __attribute__((packed));
struct sockaddr_pppol2tp {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 sa_family_t sa_family;
 unsigned int sa_protocol;
 struct pppol2tp_addr pppol2tp;
} __attribute__((packed));
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct sockaddr_pppol2tpv3 {
 sa_family_t sa_family;
 unsigned int sa_protocol;
 struct pppol2tpv3_addr pppol2tp;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__((packed));
#define PPPOEIOCSFWD _IOW(0xB1 ,0, size_t)
#define PPPOEIOCDFWD _IO(0xB1 ,1)
#define PADI_CODE 0x09
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define PADO_CODE 0x07
#define PADR_CODE 0x19
#define PADS_CODE 0x65
#define PADT_CODE 0xa7
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct pppoe_tag {
 __be16 tag_type;
 __be16 tag_len;
 char tag_data[0];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__ ((packed));
#define PTT_EOL __cpu_to_be16(0x0000)
#define PTT_SRV_NAME __cpu_to_be16(0x0101)
#define PTT_AC_NAME __cpu_to_be16(0x0102)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define PTT_HOST_UNIQ __cpu_to_be16(0x0103)
#define PTT_AC_COOKIE __cpu_to_be16(0x0104)
#define PTT_VENDOR __cpu_to_be16(0x0105)
#define PTT_RELAY_SID __cpu_to_be16(0x0110)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define PTT_SRV_ERR __cpu_to_be16(0x0201)
#define PTT_SYS_ERR __cpu_to_be16(0x0202)
#define PTT_GEN_ERR __cpu_to_be16(0x0203)
struct pppoe_hdr {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#ifdef __LITTLE_ENDIAN_BITFIELD
 __u8 ver : 4;
 __u8 type : 4;
#elif defined(__BIG_ENDIAN_BITFIELD)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 type : 4;
 __u8 ver : 4;
#else
#error "Please fix <asm/byteorder.h>"
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
 __u8 code;
 __be16 sid;
 __be16 length;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct pppoe_tag tag[0];
} __attribute__((packed));
#define PPPOE_SES_HLEN 8
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
