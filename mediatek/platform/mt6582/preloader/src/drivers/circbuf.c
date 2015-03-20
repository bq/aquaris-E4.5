
#include "circbuf.h"
#include "usbtty.h"
#include "platform.h"

static char g_usb_input_buf[USBTTY_BUFFER_SIZE];
static char g_usb_output_buf[USBTTY_BUFFER_SIZE];

int buf_input_init (circbuf_t * buf, unsigned int size)
{
    ASSERT (buf != NULL);

    buf->size = 0;
    buf->totalsize = size;
    buf->data = (char *) g_usb_input_buf;

    ASSERT (buf->data != NULL);
    {
        int i = 0;
        for (i = 0; i < USBTTY_BUFFER_SIZE; i++)
            buf->data[i] = 0x00;
    }

    buf->top = buf->data;
    buf->tail = buf->data;
    buf->end = &(buf->data[size]);

    return 1;
}

int buf_output_init (circbuf_t * buf, unsigned int size)
{
    ASSERT (buf != NULL);

    buf->size = 0;
    buf->totalsize = size;
    buf->data = (char *) g_usb_output_buf;

    ASSERT (buf->data != NULL);

    {
        int i = 0;
        for (i = 0; i < USBTTY_BUFFER_SIZE; i++)
            buf->data[i] = 0x00;
    }

    buf->top = buf->data;
    buf->tail = buf->data;
    buf->end = &(buf->data[size]);

    return 1;
}

int buf_pop (circbuf_t * buf, char *dest, unsigned int len)
{
    unsigned int i;
    char *p = buf->top;
    char *end = buf->end;
    char *data = buf->data;
    char *q = dest;
    //u32 dma_con = 0;

    if (len == 0)
        return 0;

    /* Cap to number of bytes in buffer */
    if (len > buf->size)
        len = buf->size;

#if 0
    /* dma setting */
    __raw_writel (p, DMA_SRC (USB_FULL_DMA1_BASE));     /* SOURCE */
    __raw_writel (dest, DMA_DST (USB_FULL_DMA1_BASE));  /* DESTINATION */
    __raw_writel (end - p, DMA_WPPT (USB_FULL_DMA1_BASE));      /* wrapping point */
    __raw_writel (data, DMA_WPTO (USB_FULL_DMA1_BASE)); /* wrapping destination */

    __raw_writel (len, DMA_COUNT (USB_FULL_DMA1_BASE));
    dma_con = DMA_CON_WPEN | DMA_CON_BURST_16BEAT | DMA_CON_SINC
        | DMA_CON_DINC | DMA_CON_SIZE_BYTE;
    __raw_writel (dma_con, DMA_CON (USB_FULL_DMA1_BASE));

    __raw_writel (DMA_START_BIT, DMA_START (USB_FULL_DMA1_BASE));
    //printf("USB DMA Start!\n");
    while (__raw_readl (DMA_GLBSTA_L) & DMA_GLBSTA_RUN (1));
    //printf("USB DMA Complete\n");
    __raw_writel (DMA_STOP_BIT, DMA_START (USB_FULL_DMA1_BASE));
    __raw_writel (DMA_ACKINT_BIT, DMA_ACKINT (USB_FULL_DMA1_BASE));

    p += len;
    if (p >= end)
        p = data + (p - end);
#else
    for (i = 0; i < len; i++)
    {
        *q = *p;
        p++;
        if (p == end)
            p = data;
        q++;
    }
#endif

    /* Update 'top' pointer */
    buf->top = p;
    buf->size -= len;

    return len;
}

int buf_push (circbuf_t * buf, const char *src, unsigned int len)
{
    /* NOTE:  this function allows push to overwrite old data. */
    unsigned int i;
    //u32 dma_con = 0;
    char *p = buf->tail;
    char *end = buf->end;
    char *data = buf->data;
    char *q = (char *)src;
#if 0
    /* dma setting */
    __raw_writel (src, DMA_SRC (USB_FULL_DMA0_BASE));   /* source */
    __raw_writel (p, DMA_DST (USB_FULL_DMA0_BASE));     /* destination */
    __raw_writel (end - p, DMA_WPPT (USB_FULL_DMA0_BASE));      /* wrapping point */
    __raw_writel (data, DMA_WPTO (USB_FULL_DMA0_BASE)); /* wrapping destination */


    __raw_writel (len, DMA_COUNT (USB_FULL_DMA0_BASE));
    dma_con =
        DMA_CON_WPEN | DMA_CON_WPSD | DMA_CON_BURST_16BEAT | DMA_CON_SINC |
        DMA_CON_DINC | DMA_CON_SIZE_BYTE;

    __raw_writel (dma_con, DMA_CON (USB_FULL_DMA0_BASE));

    __raw_writel (DMA_START_BIT, DMA_START (USB_FULL_DMA0_BASE));
    //printf("USB DMA Start!\n");
    while (__raw_readl (DMA_GLBSTA_L) & DMA_GLBSTA_RUN (0));
    //printf("USB DMA Complete\n");
    __raw_writel (DMA_STOP_BIT, DMA_START (USB_FULL_DMA0_BASE));
    __raw_writel (DMA_ACKINT_BIT, DMA_ACKINT (USB_FULL_DMA0_BASE));

    p += len;
    if (p >= end)
    {
        p = data + (p - end);
    }
#else
    for (i = 0; i < len; i++)
    {
        *p = *q;
        p++;
        if (p == end)
            p = data;
        q++;
    }
#endif

    buf->size += len;

    /* Update 'tail' pointer */
    buf->tail = p;

    return len;
}


