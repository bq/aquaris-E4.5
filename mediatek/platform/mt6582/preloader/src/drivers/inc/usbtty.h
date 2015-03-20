
#ifndef __USB_TTY_H__
#define __USB_TTY_H__

#include "platform.h"
#include "usbdcore.h"
#include "usbd.h"

#define NUM_CONFIGS    1
#define NUM_INTERFACES 2
#define NUM_ENDPOINTS  3
#define NUM_COMM_INTERFACES 1
#define NUM_DATA_INTERFACES 1
#define NUM_COMM_ENDPOINTS 1
#define NUM_DATA_ENDPOINTS 2

#define EP0_MAX_PACKET_SIZE 64
#define EP0_MAX_PACKET_SIZE_FULL 16

#define USBD_CONFIGURATION_STR      "USB CDC ACM for preloader"
#define USBD_DATA_INTERFACE_STR     "CDC ACM Data Interface"
#define USBD_COMM_INTERFACE_STR     "CDC ACM Communication Interface"

#define USBD_SERIAL_OUT_ENDPOINT      1
#define USBD_SERIAL_OUT_HS_PKTSIZE	512
#define USBD_SERIAL_OUT_FS_PKTSIZE   64
#define USBD_SERIAL_IN_ENDPOINT	      1
#define USBD_SERIAL_IN_HS_PKTSIZE	512
#define USBD_SERIAL_IN_FS_PKTSIZE	 64
#define USBD_INT_IN_ENDPOINT          3
#define USBD_INT_IN_HS_PKTSIZE       64
#define USBD_INT_IN_FS_PKTSIZE       64

#define USBDL_DEVICE_CLASS	    0x02
#define USBDL_DEVICE_SUBCLASS	0x00
#define USBDL_DEVICE_PROTOCOL	0x00

#define USBDL_COMM_INTERFACE_CLASS	   0x02 /* communication interface class */
#define USBDL_COMM_INTERFACE_SUBCLASS  0x02     /* Abstract Control Model subclass */
#define USBDL_COMM_INTERFACE_PROTOCOL  0x01     /* uses the Common AT commands Protocol */

#define USBDL_DATA_INTERFACE_CLASS    0x0a      /* data interface class */
#define USBDL_DATA_INTERFACE_SUBCLASS 0x00
#define USBDL_DATA_INTERFACE_PROTOCOL 0x00

#define USBD_BCD_DEVICE 0x0100
#define USBD_MAXPOWER	  0xfa

#define STR_MANUFACTURER 1
#define STR_PRODUCT	 2
#define STR_CONFIG	 3
#define STR_DATA_INTERFACE	 4
#define STR_COMM_INTERFACE   5

/* timeout of usb enumeration to avoid system hang when enum fail */
#define USB_ENUM_TIMEOUT            (CFG_USB_ENUM_TIMEOUT)

/* Buffers to hold input and output data */
#define USBTTY_BUFFER_SIZE 1024

extern struct urb *usb_alloc_urb (struct mt_dev *device,
                                  struct mt_ep *endpoint);
extern void config_usbtty (struct mt_dev *device);
extern int mt_usbtty_tstc (void);
extern int mt_usbtty_getc (void);
extern int mt_usbtty_getcn (int count, char *buf);
extern void mt_usbtty_putc (const char c, int flush);
extern void mt_usbtty_putcn (int count, char *buf, int flush);
extern void mt_usbtty_puts (const char *str);
extern int mt_usbtty_query_data_size (void);
extern bool mt_usb_connect (void);
extern void mt_usb_disconnect (void);
extern void usb_service_offline (void);
extern int tool_is_present (void);
extern void tool_state_update (int);
extern void enable_highspeed (void);

#endif


