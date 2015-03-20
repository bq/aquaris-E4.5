
#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include "typedefs.h"
#include "platform.h"

#if CFG_USB_DOWNLOAD 
extern int usbdl_handler(struct bldr_comport *comport, u32 hshk_tmo_ms);
#endif

#endif /* DOWNLOAD_H */


