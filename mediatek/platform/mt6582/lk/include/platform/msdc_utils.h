#ifndef _MSDC_UTILS_H_
#define _MSDC_UTILS_H_

#include <platform/msdc_types.h>

/* Debug message event */
#define MSG_EVT_NONE        0x00000000	/* No event */
#define MSG_EVT_DMA         0x00000001	/* DMA related event */
#define MSG_EVT_CMD         0x00000002	/* MSDC CMD related event */
#define MSG_EVT_RSP         0x00000004	/* MSDC CMD RSP related event */
#define MSG_EVT_INT         0x00000008	/* MSDC INT event */
#define MSG_EVT_CFG         0x00000010	/* MSDC CFG event */
#define MSG_EVT_FUC         0x00000020	/* Function event */
#define MSG_EVT_OPS         0x00000040	/* Read/Write operation event */
#define MSG_EVT_FIO         0x00000080	/* FIFO operation event */
#define MSG_EVT_PWR         0x00000100	/* MSDC power related event */
#define MSG_EVT_INF         0x01000000  /* information event */
#define MSG_EVT_WRN         0x02000000  /* Warning event */
#define MSG_EVT_ERR         0x04000000  /* Error event */

#define MSG_EVT_ALL         0xffffffff

//#define MSG_EVT_MASK       (MSG_EVT_ALL & ~MSG_EVT_DMA & ~MSG_EVT_WRN & ~MSG_EVT_RSP & ~MSG_EVT_INT & ~MSG_EVT_CMD & ~MSG_EVT_OPS)
//#define MSG_EVT_MASK       (MSG_EVT_ALL & ~MSG_EVT_FIO)
//#define MSG_EVT_MASK       (MSG_EVT_FIO)
#define MSG_EVT_MASK       (MSG_EVT_ALL)

#ifdef MSG_DEBUG
#define MSG(evt, fmt, args...) \
do {	\
    if ((MSG_EVT_##evt) & MSG_EVT_MASK) { \
        printf(fmt, ##args); \
    } \
} while(0)

#define MSG_FUNC(f)	MSG(FUC, "<FUNC>: %s\n", __FUNCTION__)
#else
#define MSG(evt, fmt, args...)
#define MSG_FUNC(f)
#endif

#undef BUG_ON
#define BUG_ON(x) \
    do { \
        if (x) { \
            while(1); \
        } \
    }while(0)

#undef WARN_ON
#define WARN_ON(x) \
    do { \
        if (x) { \
            MSG(WRN, "[WARN] %s LINE:%d\n", #x, __LINE__); \
        } \
    }while(0)

#define ERR_EXIT(expr, ret, expected_ret) \
    do { \
        (ret) = (expr);\
        if ((ret) != (expected_ret)) { \
            goto exit; \
        } \
    } while(0)

//#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))

#ifdef MSDC_INLINE_UTILS
/*
 * ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */

static unsigned int uffs(unsigned int x)
{
	unsigned int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

static unsigned int ntohl(unsigned int n)
{
    unsigned int t;
    unsigned char *b = (unsigned char*)&t;
    *b++ = ((n >> 24) & 0xFF);
    *b++ = ((n >> 16) & 0xFF);
    *b++ = ((n >> 8) & 0xFF);
    *b   = ((n) & 0xFF);
    return t;
}

#define set_field(reg,field,val) \
do {    \
    unsigned int tv = (unsigned int)(*(volatile u32*)(reg)); \
    tv &= ~(field); \
    tv |= ((val) << (uffs((unsigned int)field) - 1)); \
    (*(volatile u32*)(reg) = (u32)(tv)); \
} while(0)

#define get_field(reg,field,val) \
do {    \
    unsigned int tv = (unsigned int)(*(volatile u32*)(reg)); \
    val = ((tv & (field)) >> (uffs((unsigned int)field) - 1)); \
} while(0)

#else
extern unsigned int msdc_uffs(unsigned int x);
extern unsigned int msdc_ntohl(unsigned int n);
extern void msdc_set_field(volatile u32 *reg, u32 field, u32 val);
extern void msdc_get_field(volatile u32 *reg, u32 field, u32 *val);

#define uffs(x)                       msdc_uffs(x)
#define ntohl(n)                      msdc_ntohl(n)
#define set_field(r,f,v)              msdc_set_field((volatile u32*)r,f,v)
#define get_field(r,f,v)              msdc_get_field((volatile u32*)r,f,&v)

#endif

#ifndef printf
//#define printf(fmt, args...)          do{}while(0) //msdc_print(fmt, ##args)
#endif
//#define memcpy(dst,src,sz)          kal_mem_cpy(dst, src, sz)
//#define memset(p,v,s)               kal_mem_set(p, v, s)
//#define free(p)                     KAL_free(p)
//#define malloc(sz)                  KAL_malloc(sz,4, KAL_USER_MSDC)

#ifndef min
#define min(x, y)   (x < y ? x : y)
#endif
#ifndef max
#define max(x, y)   (x > y ? x : y)
#endif

#if 0
#define udelay(us)  \
    do { \
        volatile int count = us * 10; \
        while (count--); \
    }while(0)

#define mdelay(ms) \
    do { \
        unsigned long i; \
        for (i = 0; i < ms; i++) \
        udelay(1000); \
    }while(0)
#endif

#define WAIT_COND(cond,tmo,left) \
    do { \
        u32 t = tmo; \
        while (1) { \
            if ((cond) || (t == 0)) break; \
            if (t > 0) { mdelay(1); t--; } \
        } \
        left = t; \
    }while(0)

extern void msdc_print(char *fmt,...);

#endif /* _MSDC_UTILS_H_ */

