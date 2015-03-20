/*
 * MUSB OTG driver virtual root hub support
 *
 * Copyright 2005 Mentor Graphics Corporation
 * Copyright (C) 2005-2006 by Texas Instruments
 * Copyright (C) 2006-2007 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/timer.h>

#include <asm/unaligned.h>

#include "musbfsh_core.h"
#include "musbfsh_host.h"


#ifdef MTK_USB_RUNTIME_SUPPORT
#include <cust_eint.h>
extern void mt_eint_unmask(unsigned int line);
extern void mt_eint_mask(unsigned int line);
extern void request_wakeup_md_timeout(unsigned int dev_id, unsigned int dev_sub_id);
#endif

static void musbfsh_port_suspend(struct musbfsh *musbfsh, bool do_suspend)
{
	u8		power;
	u8		intrusbe;
	u8		intrusb;
	void __iomem	*mbase = musbfsh->mregs;
	int retries = 0;

	/* NOTE:  this doesn't necessarily put PHY into low power mode,
	 * turning off its clock; that's a function of PHY integration and
	 * MUSBFSH_POWER_ENSUSPEND.  PHY may need a clock (sigh) to detect
	 * SE0 changing to connect (J) or wakeup (K) states.
	 */
	if (do_suspend) {
		/* clean MUSBFSH_INTR_SOF in MUSBFSH_INTRUSBE */
		intrusbe = musbfsh_readb(mbase, MUSBFSH_INTRUSBE);
		intrusbe &= ~MUSBFSH_INTR_SOF;
		musbfsh_writeb(mbase, MUSBFSH_INTRUSBE, intrusbe);
		mb();
		/* clean MUSBFSH_INTR_SOF in MUSBFSH_INTRUSB */
		intrusb = musbfsh_readb(mbase, MUSBFSH_INTRUSB);
		intrusb |= MUSBFSH_INTR_SOF;
		musbfsh_writeb(mbase, MUSBFSH_INTRUSB, intrusb);
		mb();
		retries = 10000;
		intrusb = musbfsh_readb(mbase, MUSBFSH_INTRUSB);
		while (!(intrusb & MUSBFSH_INTR_SOF)) {
			intrusb = musbfsh_readb(mbase, MUSBFSH_INTRUSB);
			if (retries-- < 1)
				break;
		}

		/* delay 10 us */
		udelay(10);

		/* set MUSBFSH_POWER_SUSPENDM in MUSBFSH_POWER_SUSPENDM */
		power = musbfsh_readb(mbase, MUSBFSH_POWER);

#ifdef MTK_USB_RUNTIME_SUPPORT
		/* mask remote wake up IRQ between port suspend and bus suspend.
		    hub.c will call set_port_feature first then usb_set_device_state, so if EINT comes between them, resume flow may see
		    device state without USB_STATE_SUSPENDED and do nothing.
		    So we postpone remote wake up IRQ untill the suspend flow is all done (when bus_suspend is called). Since suspend flow
		    may be interrupted (root hub is suspended, but not host controller), so we also unmaks EINT when resume is done.
		*/
		mt_eint_mask(CUST_EINT_MT6280_USB_WAKEUP_NUM);
#endif

		retries = 10000;

		power &= ~MUSBFSH_POWER_RESUME;
		power |= MUSBFSH_POWER_SUSPENDM;
		musbfsh_writeb(mbase, MUSBFSH_POWER, power);

		/* Needed for OPT A tests */
		power = musbfsh_readb(mbase, MUSBFSH_POWER);
		while (power & MUSBFSH_POWER_SUSPENDM) {
			power = musbfsh_readb(mbase, MUSBFSH_POWER);
			if (retries-- < 1)
				break;
		}
		mb();
		WARNING( "Root port suspended, power 0x%02x\n", power);

		musbfsh->port1_status |= USB_PORT_STAT_SUSPEND;
	} else {
		power = musbfsh_readb(mbase, MUSBFSH_POWER);
		if(!(power & MUSBFSH_POWER_SUSPENDM)) {
			WARNING("Root port resuming abort, power 0x%02x\n", power);
			if(power & MUSBFSH_POWER_RESUME)
				goto finish;
			else
				return;
		}

#ifdef MTK_USB_RUNTIME_SUPPORT
		//ERR("EINT to wake up MD for resume\n");
		//request_wakeup_md_timeout(0, 0); //wx, wakeup MD first
#endif
		power &= ~MUSBFSH_POWER_SUSPENDM;
		power |= MUSBFSH_POWER_RESUME;
		musbfsh_writeb(mbase, MUSBFSH_POWER, power);
		mb();
		WARNING("Root port resuming, power 0x%02x\n", power);
finish:
		/* later, GetPortStatus will stop RESUME signaling */
		musbfsh->port1_status |= MUSBFSH_PORT_STAT_RESUME;
		musbfsh->rh_timer = jiffies + msecs_to_jiffies(20);
	}
}

static void musbfsh_port_reset(struct musbfsh *musbfsh, bool do_reset)
{
	u8		power;
	void __iomem	*mbase = musbfsh->mregs;

	/* NOTE:  caller guarantees it will turn off the reset when
	 * the appropriate amount of time has passed
	 */
	power = musbfsh_readb(mbase, MUSBFSH_POWER);
	WARNING("reset=%d power=0x%x\n", do_reset, power);
	if (do_reset) {
		if(power & MUSBFSH_POWER_SUSPENDM) {
			WARNING("reset a suspended device\n");

#ifdef MTK_USB_RUNTIME_SUPPORT
			//ERR("EINT to wake up MD for reset\n");
			//request_wakeup_md_timeout(0, 0); //wx, we may have to reset a suspended MD
#endif
			musbfsh_writeb(mbase, MUSBFSH_POWER,
				power | MUSBFSH_POWER_RESUME);
			mdelay(20);
			musbfsh_writeb(mbase, MUSBFSH_POWER,
				power & ~MUSBFSH_POWER_RESUME);
		}

		/*
		 * If RESUME is set, we must make sure it stays minimum 20 ms.
		 * Then we must clear RESUME and wait a bit to let musb start
		 * generating SOFs. If we don't do this, OPT HS A 6.8 tests
		 * fail with "Error! Did not receive an SOF before suspend
		 * detected".
		 */
		if (power &  MUSBFSH_POWER_RESUME) {
			WARNING("reset a resuming device\n");
			while (time_before(jiffies, musbfsh->rh_timer))
				mdelay(1);
			musbfsh_writeb(mbase, MUSBFSH_POWER,//stop the resume signal
				power & ~MUSBFSH_POWER_RESUME);
			mdelay(1);
		}

		musbfsh->ignore_disconnect = true;
		power &= 0xf0;
		musbfsh_writeb(mbase, MUSBFSH_POWER,
				power | MUSBFSH_POWER_RESET);
		mb();
		musbfsh->port1_status |= USB_PORT_STAT_RESET;
		musbfsh->port1_status &= ~USB_PORT_STAT_ENABLE;
		musbfsh->rh_timer = jiffies + msecs_to_jiffies(50);
	} else {
		INFO( "Root port reset stopped\n");
		musbfsh_writeb(mbase, MUSBFSH_POWER,
				power & ~MUSBFSH_POWER_RESET);
		mb();
		musbfsh->ignore_disconnect = false;

		power = musbfsh_readb(mbase, MUSBFSH_POWER);
		if (power & MUSBFSH_POWER_HSMODE) {
			INFO( "high-speed device connected\n");
			musbfsh->port1_status |= USB_PORT_STAT_HIGH_SPEED;
		}
#ifdef IC_USB
		USB11PHY_SET8(U1PHTCR2, force_usb11_dm_rpd | force_usb11_dp_rpd);
		USB11PHY_CLR8(U1PHTCR2, RG_USB11_DM_RPD | RG_USB11_DP_RPD);  // disconnect host port's pull down resistors on D+ and D-
		USB11PHY_SET8(U1PHTCR2, force_usb11_dp_rpu | RG_USB11_DP_RPU); // tell MAC there still is a device attached, ohterwise we will get disconnect interrupt
		WARNING("USB1.1 PHY special config for IC-USB 0x%X=%x\n", U1PHTCR2, USB11PHY_READ8(U1PHTCR2));
#endif
		musbfsh->port1_status &= ~USB_PORT_STAT_RESET;
		musbfsh->port1_status |= USB_PORT_STAT_ENABLE
					| (USB_PORT_STAT_C_RESET << 16)
					| (USB_PORT_STAT_C_ENABLE << 16);
		usb_hcd_poll_rh_status(musbfsh_to_hcd(musbfsh));//call back func to notify the hub thread the state of hub!

		musbfsh->vbuserr_retry = VBUSERR_RETRY_COUNT;
	}
}

void musbfsh_root_disconnect(struct musbfsh *musbfsh)
{
    INFO("musbfsh_root_disconnect++\r\n");
	musbfsh->port1_status = USB_PORT_STAT_POWER
			| (USB_PORT_STAT_C_CONNECTION << 16);

	usb_hcd_poll_rh_status(musbfsh_to_hcd(musbfsh));
	musbfsh->is_active = 0;
}


/*---------------------------------------------------------------------*/

/* Caller may or may not hold musbfsh->lock */
int musbfsh_hub_status_data(struct usb_hcd *hcd, char *buf)
{
	struct musbfsh	*musbfsh = hcd_to_musbfsh(hcd);
	int		retval = 0;

	INFO("musbfsh_hub_status_data++\r\n");
	/* called in_irq() via usb_hcd_poll_rh_status() */
	if (musbfsh->port1_status & 0xffff0000) {
		*buf = 0x02;
		retval = 1;
	}
	return retval;
}

int musbfsh_hub_control(
	struct usb_hcd	*hcd,
	u16		typeReq,
	u16		wValue,
	u16		wIndex,
	char		*buf,
	u16		wLength)
{
	struct musbfsh *musbfsh = hcd_to_musbfsh(hcd);
	u32		temp;
	int		retval = 0;
	unsigned long	flags;

	INFO("musbfsh_hub_control++,typeReq=0x%x,wValue=0x%x,wIndex=0x%x\r\n",typeReq,wValue,wIndex);
	spin_lock_irqsave(&musbfsh->lock, flags);

	if (unlikely(!HCD_HW_ACCESSIBLE(hcd))) {
		spin_unlock_irqrestore(&musbfsh->lock, flags);
		return -ESHUTDOWN;
	}

	/* hub features:  always zero, setting is a NOP
	 * port features: reported, sometimes updated when host is active
	 * no indicators
	 */
	switch (typeReq) {
	case ClearHubFeature:
	case SetHubFeature:
		switch (wValue) {
		case C_HUB_OVER_CURRENT:
		case C_HUB_LOCAL_POWER:
			break;
		default:
			goto error;
		}
		break;
	case ClearPortFeature:
		if ((wIndex & 0xff) != 1)//wIndex indicate the port number, here it is should be 1
			goto error;

		switch (wValue) {
		case USB_PORT_FEAT_ENABLE:
			break;
		case USB_PORT_FEAT_SUSPEND:
			musbfsh_port_suspend(musbfsh, false);//here is clearing the suspend
			break;
		case USB_PORT_FEAT_POWER:
			musbfsh_platform_set_vbus(musbfsh, 0);//only power off the vbus
			break;
		case USB_PORT_FEAT_C_CONNECTION:
		case USB_PORT_FEAT_C_ENABLE:
		case USB_PORT_FEAT_C_OVER_CURRENT:
		case USB_PORT_FEAT_C_RESET:
		case USB_PORT_FEAT_C_SUSPEND:
			break;
		default:
			goto error;
		}
		INFO("clear feature %d\n", wValue);
		musbfsh->port1_status &= ~(1 << wValue);
		break;
	case GetHubDescriptor:
		{
		struct usb_hub_descriptor *desc = (void *)buf;

		desc->bDescLength = 9;
		desc->bDescriptorType = 0x29;
		desc->bNbrPorts = 1;
		desc->wHubCharacteristics = cpu_to_le16(
				  0x0001	/* per-port power switching */
				| 0x0010	/* no overcurrent reporting */
				);
		desc->bPwrOn2PwrGood = 5;	/* msec/2 */
		desc->bHubContrCurrent = 0;

		/* workaround bogus struct definition */
		desc->u.hs.DeviceRemovable[0] = 0x02;	/* port 1 */
		desc->u.hs.DeviceRemovable[1] = 0xff;
		}
		break;
	case GetHubStatus:
		temp = 0;
		*(__le32 *) buf = cpu_to_le32(temp);
		break;
	case GetPortStatus:
		if (wIndex != 1)
			goto error;

		/* finish RESET signaling? */
		if ((musbfsh->port1_status & USB_PORT_STAT_RESET)
				&& time_after_eq(jiffies, musbfsh->rh_timer))
			musbfsh_port_reset(musbfsh, false);//stop the reset because the timeout of reset.

		/* finish RESUME signaling? */
		if ((musbfsh->port1_status & MUSBFSH_PORT_STAT_RESUME)
				&& time_after_eq(jiffies, musbfsh->rh_timer)) {
			u8		power;

			power = musbfsh_readb(musbfsh->mregs, MUSBFSH_POWER);
			power &= ~MUSBFSH_POWER_RESUME;
			WARNING( "Root port resume stopped, power 0x%02x\n",power);
			musbfsh_writeb(musbfsh->mregs, MUSBFSH_POWER, power);

#ifdef MTK_USB_RUNTIME_SUPPORT
			//mt_eint_unmask(CUST_EINT_MT6280_USB_WAKEUP_NUM);
#endif

			/* ISSUE:  DaVinci (RTL 1.300) disconnects after
			 * resume of high speed peripherals (but not full
			 * speed ones).
			 */

			musbfsh->is_active = 1;
			musbfsh->port1_status &= ~(USB_PORT_STAT_SUSPEND
					| MUSBFSH_PORT_STAT_RESUME);
			musbfsh->port1_status |= USB_PORT_STAT_C_SUSPEND << 16;
			usb_hcd_poll_rh_status(musbfsh_to_hcd(musbfsh));
		}

		put_unaligned(cpu_to_le32(musbfsh->port1_status
					& ~MUSBFSH_PORT_STAT_RESUME),
				(__le32 *) buf);

		/* port change status is more interesting */
		INFO("port status %08x,devctl=0x%x\n",musbfsh->port1_status,musbfsh_readb(musbfsh->mregs, MUSBFSH_DEVCTL));
		break;
	case SetPortFeature:
		if ((wIndex & 0xff) != 1)
			goto error;

		switch (wValue) {
		case USB_PORT_FEAT_POWER:
			/* NOTE: this controller has a strange state machine
			 * that involves "requesting sessions" according to
			 * magic side effects from incompletely-described
			 * rules about startup...
			 *
			 * This call is what really starts the host mode; be
			 * very careful about side effects if you reorder any
			 * initialization logic, e.g. for OTG, or change any
			 * logic relating to VBUS power-up.
			 */
			INFO("musbfsh_start is called in hub control\n");
			musbfsh_start(musbfsh);
			break;
		case USB_PORT_FEAT_RESET:
			musbfsh_port_reset(musbfsh, true);//enable the reset, but not finish
			break;
		case USB_PORT_FEAT_SUSPEND:
			musbfsh_port_suspend(musbfsh, true);
			break;
		case USB_PORT_FEAT_TEST:
#if 0
			if (unlikely(is_host_active(musbfsh)))
				goto error;

			wIndex >>= 8;
			switch (wIndex) {
			case 1:
				pr_debug("TEST_J\n");
				temp = MUSBFSH_TEST_J;
				break;
			case 2:
				pr_debug("TEST_K\n");
				temp = MUSBFSH_TEST_K;
				break;
			case 3:
				pr_debug("TEST_SE0_NAK\n");
				temp = MUSBFSH_TEST_SE0_NAK;
				break;
			case 4:
				pr_debug("TEST_PACKET\n");
				temp = MUSBFSH_TEST_PACKET;
				musbfsh_load_testpacket(musbfsh);
				break;
			case 5:
				pr_debug("TEST_FORCE_ENABLE\n");
				temp = MUSBFSH_TEST_FORCE_HOST
					| MUSBFSH_TEST_FORCE_FS;

				musbfsh_writeb(musbfsh->mregs, MUSBFSH_DEVCTL,
						MUSBFSH_DEVCTL_SESSION);
				break;
			case 6:
				pr_debug("TEST_FIFO_ACCESS\n");
				temp = MUSBFSH_TEST_FIFO_ACCESS;
				break;
			default:
				goto error;
			}
			musbfsh_writeb(musbfsh->mregs, MUSBFSH_TESTMODE, temp);
#endif
			break;
		default:
			goto error;
		}
		INFO("set feature %d\n", wValue);
		musbfsh->port1_status |= 1 << wValue;
		break;

	default:
error:
		/* "protocol stall" on error */
		retval = -EPIPE;
	}
	spin_unlock_irqrestore(&musbfsh->lock, flags);
	return retval;
}
