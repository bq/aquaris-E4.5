
#ifndef __CIRCBUF_H__
#define __CIRCBUF_H__

#define USB_BUFFER_VERIFY                       1


typedef struct circbuf
{
    unsigned int size;
    unsigned int totalsize;

    char *top;
    char *tail;

    char *data;
    char *end;
} circbuf_t;

int buf_input_init (circbuf_t * buf, unsigned int size);
int buf_output_init (circbuf_t * buf, unsigned int size);
int buf_pop (circbuf_t * buf, char *dest, unsigned int len);
int buf_push (circbuf_t * buf, const char *src, unsigned int len);

#endif


