
#ifndef USBPHY_H_
#define USBPHY_H_

#if CFG_FPGA_PLATFORM
#include "i2c.h"
#endif

#define USB20_PHY_BASE					(USBSIF_BASE + 0x0800)
#define USB11_PHY_BASE					(USBSIF_BASE + 0x0900)
#define PERI_GLOBALCON_PDN0_SET 		(PERICFG_BASE+0x008)
#define USB0_PDN						1<<10

#if CFG_FPGA_PLATFORM
#define USB_I2C_ID	I2C2	/* 0 - 6 */
#define PATH_NORMAL	0
#define PATH_PMIC	1

extern U32 usb_i2c_read8 (U8 addr, U8 *dataBuffer);
extern U32 usb_i2c_write8 (U8 addr, U8 value);

#define USBPHY_I2C_READ8(addr, buffer)	 usb_i2c_read8(addr, buffer)
#define USBPHY_I2C_WRITE8(addr, value)	 usb_i2c_write8(addr, value)
#endif

#define USBPHY_READ8(offset)		__raw_readb(USB20_PHY_BASE+offset)
#define USBPHY_WRITE8(offset, value)	__raw_writeb(value, USB20_PHY_BASE+offset)
#define USBPHY_SET8(offset, mask)	USBPHY_WRITE8(offset, USBPHY_READ8(offset) | mask)
#define USBPHY_CLR8(offset, mask)	USBPHY_WRITE8(offset, USBPHY_READ8(offset) & ~mask)

#define USB_SET_BIT(BS,REG)			mt65xx_reg_sync_writel((__raw_readl(REG) | (U32)(BS)), (REG))
#define USB_CLR_BIT(BS,REG)			mt65xx_reg_sync_writel((__raw_readl(REG) & (~(U32)(BS))), (REG))

#define USB11PHY_READ8(offset)		__raw_readb(USB11_PHY_BASE+offset)
#define USB11PHY_WRITE8(offset, value)	__raw_writeb(value, USB11_PHY_BASE+offset)
#define USB11PHY_SET8(offset, mask)	USB11PHY_WRITE8(offset, USB11PHY_READ8(offset) | mask)
#define USB11PHY_CLR8(offset, mask)	USB11PHY_WRITE8(offset, USB11PHY_READ8(offset) & ~mask)

#endif



