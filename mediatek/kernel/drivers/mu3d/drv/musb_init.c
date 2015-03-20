#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/prefetch.h>
#include <linux/usb/nop-usb-xceiv.h>

#include <asm/cacheflush.h>

#include "musb_core.h"
#include "ssusb_qmu.h"
#include <linux/mu3d/drv/SSUSB_DEV_c_header.h>
#include <linux/mu3d/hal/mu3d_hal_osal.h>
#include <linux/mu3d/hal/mu3d_hal_phy.h>
#include <linux/mu3d/drv/musb_comm.h>
#include <linux/mu3d/hal/mu3d_hal_usb_drv.h>

static struct musb_fifo_cfg __initdata mtu3d_cfg[] = {
{ .hw_ep_num =  1, .style = FIFO_TX, .maxpacket = 1024, },
{ .hw_ep_num =  1, .style = FIFO_RX, .maxpacket = 1024, },
{ .hw_ep_num =  2, .style = FIFO_TX, .maxpacket = 1024, },
{ .hw_ep_num =  2, .style = FIFO_RX, .maxpacket = 1024, },
{ .hw_ep_num =  3, .style = FIFO_TX, .maxpacket = 1024, },
{ .hw_ep_num =  3, .style = FIFO_RX, .maxpacket = 1024, },
{ .hw_ep_num =  4, .style = FIFO_TX, .maxpacket = 1024, },
{ .hw_ep_num =  4, .style = FIFO_RX, .maxpacket = 1024, },
{ .hw_ep_num =  5, .style = FIFO_TX, .maxpacket = 1024, },
{ .hw_ep_num =  5, .style = FIFO_RX, .maxpacket = 1024, },
{ .hw_ep_num =  6, .style = FIFO_TX, .maxpacket = 1024, },
{ .hw_ep_num =  6, .style = FIFO_RX, .maxpacket = 1024, },
{ .hw_ep_num =  7, .style = FIFO_TX, .maxpacket = 1024, },
{ .hw_ep_num =  7, .style = FIFO_RX, .maxpacket = 1024, },
{ .hw_ep_num =  8, .style = FIFO_TX, .maxpacket = 1024, },
{ .hw_ep_num =  8, .style = FIFO_RX, .maxpacket = 1024, },
};


struct mtu3d_glue {
	struct device		*dev;
	struct platform_device	*musb;
};
#define glue_to_musb(g)		platform_get_drvdata(g->musb)

#ifdef USE_SSUSB_QMU
//extern QMU_DONE_ISR_DATA param;
//extern struct tasklet_struct t_qmu_done;
#endif

struct platform_device *mu3d_musb;


static void mtu3d_musb_reg_init(void);
static int mtu3d_set_power(struct usb_phy *x, unsigned mA);

static inline void mtu3d_u3_ltssm_intr_handler(struct musb *musb, u32 dwLtssmValue)
{
	static u32 soft_conn_num = 0;

	if (dwLtssmValue & SS_DISABLE_INTR) {
		os_printk(K_DEBUG, "LTSSM: SS_DISABLE_INTR [%d] & Set SOFT_CONN=1\n", ++soft_conn_num);
		//enable U2 link. after host reset, HS/FS EP0 configuration is applied in musb_g_reset
		os_clrmsk(U3D_SSUSB_U2_CTRL_0P, SSUSB_U2_PORT_PDN);
		os_setmsk(U3D_POWER_MANAGEMENT, SOFT_CONN);
	}

	if (dwLtssmValue & ENTER_U0_INTR) {
		os_printk(K_DEBUG, "LTSSM: ENTER_U0_INTR %d\n", musb->g.speed);
		//do not apply U3 EP0 setting again, if the speed is already U3
		//LTSSM may go to recovery and back to U0
		if (musb->g.speed != USB_SPEED_SUPER)
			musb_conifg_ep0(musb);
	}

	if (dwLtssmValue & VBUS_FALL_INTR) {
		os_printk(K_DEBUG, "LTSSM: VBUS_FALL_INTR\n");
		mu3d_hal_pdn_ip_port(1, 1, 1, 1);
		os_writel(U3D_USB3_CONFIG, 0);
	}

	if (dwLtssmValue & VBUS_RISE_INTR) {
		os_printk(K_DEBUG, "LTSSM: VBUS_RISE_INTR\n");
		os_writel(U3D_USB3_CONFIG, USB3_EN);
	}

	if (dwLtssmValue & ENTER_U3_INTR) {
		os_printk(K_DEBUG, "LTSSM: ENTER_U3_INTR\n");
		mu3d_hal_pdn_ip_port(0, 0, 1, 0);
	}

	if (dwLtssmValue & EXIT_U3_INTR) {
		os_printk(K_DEBUG, "LTSSM: EXIT_U3_INTR\n");
		mu3d_hal_pdn_ip_port(1, 0, 1, 0);
	}

#ifndef POWER_SAVING_MODE
	if (dwLtssmValue & U3_RESUME_INTR) {
		os_printk(K_DEBUG, "LTSSM: RESUME_INTR\n");
		mu3d_hal_pdn_ip_port(1, 0, 1, 0);
		os_writel(U3D_LINK_POWER_CONTROL, os_readl(U3D_LINK_POWER_CONTROL) | UX_EXIT);
	}
#endif

	/*7.5.12.2 Hot Reset Requirements
	* 1. A downstream port shall reset its Link Error Count as defined in Section 7.4.2.
	* 2. A downstream port shall reset its PM timers and the associated U1 and U2 timeout values to zero.
	* 3. The port Configuration information shall remain unchanged (refer to Section 8.4.6 for details).
	* 4. The port shall maintain its transmitter specifications defined in Table 6-10.
	* 5. The port shall maintain its low-impedance receiver termination (RRX-DC) defined in Table 6-13.
	*/
	if (dwLtssmValue & HOT_RST_INTR) {
		DEV_INT32 link_err_cnt;
		DEV_INT32 timeout_val;
		os_printk(K_DEBUG, "LTSSM: HOT_RST_INTR\n");
		/* Clear link error count */
		link_err_cnt=os_readl(U3D_LINK_ERR_COUNT);
		os_printk(K_DEBUG, "LTSSM: link_err_cnt=%x\n", link_err_cnt);
		os_writel(U3D_LINK_ERR_COUNT, CLR_LINK_ERR_CNT);

		/* Clear U1 & U2 Enable*/
		os_clrmsk(U3D_LINK_POWER_CONTROL, (SW_U1_ACCEPT_ENABLE|SW_U2_ACCEPT_ENABLE));

		musb->g.pwr_params.bU1Enabled = 0;
		musb->g.pwr_params.bU2Enabled = 0;

		/* Reset U1 & U2 timeout value*/
		timeout_val = os_readl(U3D_LINK_UX_INACT_TIMER);
		os_printk(K_DEBUG, "LTSSM: timer_val =%x\n", timeout_val);
		timeout_val &= ~ (U1_INACT_TIMEOUT_VALUE | DEV_U2_INACT_TIMEOUT_VALUE);
		os_writel(U3D_LINK_UX_INACT_TIMER, timeout_val);
	}

	if (dwLtssmValue & SS_INACTIVE_INTR) os_printk(K_DEBUG, "LTSSM: SS_INACTIVE_INTR\n");
	if (dwLtssmValue & RECOVERY_INTR) os_printk(K_DEBUG, "LTSSM: RECOVERY_INTR\n");

	/* A completion of a Warm Reset shall result in the following.
	* 1. A downstream port shall reset its Link Error Count.
	* 2. Port configuration information of an upstream port shall be reset to default values. Refer to
	*	 Sections 8.4.5 and 8.4.6 for details.
	* 3. The PHY level variables (such as Rx equalization settings) shall be reinitialized or retrained.
	* 4. The LTSSM of a port shall transition to U0 through RxDetect and Polling.
	*/
	if (dwLtssmValue & WARM_RST_INTR) {
		DEV_INT32 link_err_cnt;
		os_printk(K_DEBUG, "LTSSM: WARM_RST_INTR\n");
		/* Clear link error count */
		link_err_cnt=os_readl(U3D_LINK_ERR_COUNT);
		os_printk(K_DEBUG, "LTSSM: link_err_cnt=%x\n", link_err_cnt);
		os_writel(U3D_LINK_ERR_COUNT, CLR_LINK_ERR_CNT);
	}

	if (dwLtssmValue & ENTER_U2_INTR) os_printk(K_DEBUG, "LTSSM: ENTER_U2_INTR\n");
	if (dwLtssmValue & ENTER_U1_INTR) os_printk(K_DEBUG, "LTSSM: ENTER_U1_INTR\n");
	if (dwLtssmValue & RXDET_SUCCESS_INTR) os_printk(K_DEBUG, "LTSSM: RXDET_SUCCESS_INTR\n");

}

static inline void mtu3d_u2_common_intr_handler(u32 dwIntrUsbValue)
{
	if (dwIntrUsbValue & DISCONN_INTR) {
		mu3d_hal_pdn_ip_port(1, 0, 1, 1);

		os_printk(K_DEBUG, "[U2 DISCONN_INTR] Set SOFT_CONN=0\n");
		os_clrmsk(U3D_POWER_MANAGEMENT, SOFT_CONN);

		/*TODO-J: ADD musb_g_disconnect(musb);??*/
	}

	if (dwIntrUsbValue & LPM_INTR) 	{
		os_printk(K_DEBUG, "[U2 LPM interrupt]\n");

		if (!((os_readl(U3D_POWER_MANAGEMENT) & LPM_HRWE))) {
			mu3d_hal_pdn_ip_port(0, 0, 0, 1);
		}
	}

	if (dwIntrUsbValue & LPM_RESUME_INTR) {
		if (!(os_readl(U3D_POWER_MANAGEMENT) & LPM_HRWE)) {
			mu3d_hal_pdn_ip_port(1, 0, 0, 1);

			os_writel(U3D_USB20_MISC_CONTROL, os_readl(U3D_USB20_MISC_CONTROL) | LPM_U3_ACK_EN);
		}
	}

	if(dwIntrUsbValue & SUSPEND_INTR) {
		os_printk(K_DEBUG,"[U2 SUSPEND_INTR]\n");
		mu3d_hal_pdn_ip_port(0, 0, 0, 1);
	}

	if (dwIntrUsbValue & RESUME_INTR) {
		os_printk(K_NOTICE,"[U2 RESUME_INTR]\n");
		mu3d_hal_pdn_ip_port(1, 0, 0, 1);
	}

	if (dwIntrUsbValue & RESUME_INTR) {
		os_printk(K_NOTICE,"[U2 RESET_INTR]\n");
	}

}

static inline void mtu3d_link_intr_handler(u32 dwLinkIntValue)
{
	u32 dwTemp;

	dwTemp = os_readl(U3D_DEVICE_CONF) & SSUSB_DEV_SPEED;
	mu3d_hal_pdn_cg_en();

	switch (dwTemp)
	{
		case SSUSB_SPEED_FULL:
			os_printk(K_ALET,"USB Speed = Full Speed\n");
			//BESLCK = 4 < BESLCK_U3 = 10 < BESLDCK = 15
			os_writel(U3D_USB20_LPM_PARAMETER, 0xa4f0);
			os_setmsk(U3D_POWER_MANAGEMENT, (LPM_BESL_STALL|LPM_BESLD_STALL));
		break;
		case SSUSB_SPEED_HIGH:
			os_printk(K_ALET,"USB Speed = High Speed\n");
			//BESLCK = 4 < BESLCK_U3 = 10 < BESLDCK = 15
			os_writel(U3D_USB20_LPM_PARAMETER, 0xa4f0);
			os_setmsk(U3D_POWER_MANAGEMENT, (LPM_BESL_STALL|LPM_BESLD_STALL));
		break;
		case SSUSB_SPEED_SUPER:
			os_printk(K_ALET,"USB Speed = Super Speed\n");
		break;
		default:
			os_printk(K_ALET,"USB Speed = Invalid\n");
		break;
	}
}

#ifdef SUPPORT_OTG
static inline void mtu3d_otg_intr_handler(u32 dwOtgIntValue)
{
	if (dwOtgIntValue & VBUS_CHG_INTR) {
		os_printk(K_NOTICE, "OTG: VBUS_CHG_INTR\n");
		os_setmsk(U3D_SSUSB_OTG_STS_CLR, SSUSB_VBUS_INTR_CLR);
	}

	//this interrupt is issued when B device becomes device
	if (dwOtgIntValue & SSUSB_CHG_B_ROLE_B) {
		os_printk(K_NOTICE, "OTG: CHG_B_ROLE_B\n");
		os_setmsk(U3D_SSUSB_OTG_STS_CLR, SSUSB_CHG_B_ROLE_B_CLR);

		//switch DMA module to device
		os_printk(K_NOTICE, "Switch DMA to device\n");
		os_clrmsk(U3D_SSUSB_U2_CTRL_0P, SSUSB_U2_PORT_HOST_SEL);
	}

	//this interrupt is issued when B device becomes host
	if (dwOtgIntValue & SSUSB_CHG_A_ROLE_B) {
		os_printk(K_NOTICE, "OTG: CHG_A_ROLE_B\n");
		os_setmsk(U3D_SSUSB_OTG_STS_CLR, SSUSB_CHG_A_ROLE_B_CLR);
	}

	//this interrupt is issued when IDDIG reads B
	if (dwOtgIntValue & SSUSB_ATTACH_B_ROLE) {
		os_printk(K_NOTICE, "OTG: CHG_ATTACH_B_ROLE\n");
		os_setmsk(U3D_SSUSB_OTG_STS_CLR, SSUSB_ATTACH_B_ROLE_CLR);

		//switch DMA module to device
		os_printk(K_NOTICE, "Switch DMA to device\n");
		os_clrmsk(U3D_SSUSB_U2_CTRL_0P, SSUSB_U2_PORT_HOST_SEL);
	}

}
#endif

static irqreturn_t generic_interrupt(int irq, void *__hci)
{
	unsigned long flags;
	irqreturn_t	retval = IRQ_NONE;
	struct musb	*musb = __hci;

	u32 dwL1Value = 0;
	u32 dwIntrUsbValue = 0;
	u32 dwDmaIntrValue = 0;
	u32 dwIntrEPValue = 0;
	u16 wIntrTxValue = 0;
	u16 wIntrRxValue = 0;
	u32 wIntrQMUValue = 0;
	u32 wIntrQMUDoneValue = 0;
	u32 dwLtssmValue = 0;
	u32 dwLinkIntValue = 0;

#ifdef SUPPORT_OTG
	u32 dwOtgIntValue = 0;
#endif

	spin_lock_irqsave(&musb->lock, flags);

	dwL1Value = os_readl(U3D_LV1ISR) & os_readl(U3D_LV1IER);

	if (dwL1Value & EP_CTRL_INTR) {
		dwLinkIntValue = os_readl(U3D_DEV_LINK_INTR) & os_readl(U3D_DEV_LINK_INTR_ENABLE);
		/* Write 1 clear*/
		os_writel(U3D_DEV_LINK_INTR, dwLinkIntValue);
		os_printk(K_DEBUG, "===L1[%x] LinkInt[%x]\n", dwL1Value, dwLinkIntValue);
	}

	if (dwL1Value & MAC2_INTR) {
		dwIntrUsbValue = os_readl(U3D_COMMON_USB_INTR) & os_readl(U3D_COMMON_USB_INTR_ENABLE);
		/* Write 1 clear*/
		os_writel(U3D_COMMON_USB_INTR, dwIntrUsbValue);
		os_printk(K_DEBUG, "===L1[%x] U2[%x]\n", dwL1Value, dwIntrUsbValue);
	}

	if (dwL1Value & DMA_INTR) {
		dwDmaIntrValue = os_readl(U3D_DMAISR) & os_readl(U3D_DMAIER);
		/* Write 1 clear*/
		os_writel(U3D_DMAISR, dwDmaIntrValue);
		os_printk(K_DEBUG, "===L1[%x] DMA[%x]\n", dwL1Value, dwDmaIntrValue);
	}

	if (dwL1Value & MAC3_INTR) {
		dwLtssmValue = os_readl(U3D_LTSSM_INTR) & os_readl(U3D_LTSSM_INTR_ENABLE);
		/* Write 1 clear*/
		os_writel(U3D_LTSSM_INTR, dwLtssmValue);
		os_printk(K_DEBUG, "===L1[%x] LTSSM[%x]\n", dwL1Value, dwLtssmValue);
	}

#ifdef USE_SSUSB_QMU
	if (dwL1Value & QMU_INTR) {
		wIntrQMUValue = os_readl(U3D_QISAR1) & os_readl(U3D_QIER1);
		wIntrQMUDoneValue = os_readl(U3D_QISAR0) & os_readl(U3D_QIER0);
		/* Write 1 clear */
		os_writel(U3D_QISAR0, wIntrQMUDoneValue);
		os_printk(K_DEBUG, "===L1[%x] QMUDone[Tx=%x,Rx=%x] QMU[%x]===\n",\
			dwL1Value, ((wIntrQMUDoneValue & 0xFFFF) >> 1), wIntrQMUDoneValue>>17, wIntrQMUValue);
	}
#endif

	if (dwL1Value & BMU_INTR) {
		dwIntrEPValue = os_readl(U3D_EPISR) & os_readl(U3D_EPIER);
		wIntrTxValue = dwIntrEPValue & 0xFFFF;
		wIntrRxValue = (dwIntrEPValue>>16);
		os_writel(U3D_EPISR, dwIntrEPValue);
		os_printk(K_DEBUG, "===L1[%x] Tx[%x] Rx[%x]===\n",\
			dwL1Value, wIntrTxValue, wIntrRxValue);

	}

	/*TODO: need to handle SetupEnd Interrupt!!!*/

	/*--Hanlde each interrupt--*/
	if (dwL1Value & (BMU_INTR | MAC2_INTR)) {
		musb->int_usb = dwIntrUsbValue;
		musb->int_tx = wIntrTxValue;
		musb->int_rx = wIntrRxValue;

		if (musb->int_usb || musb->int_tx || musb->int_rx)
			retval = musb_interrupt(musb);
	}

#ifdef SUPPORT_OTG
	dwOtgIntValue = os_readl(U3D_SSUSB_OTG_STS);
#endif


#ifdef USE_SSUSB_QMU
	if (wIntrQMUDoneValue) {
	 	qmu_done_interrupt(musb, wIntrQMUDoneValue);
	}

#ifdef NEVER
	//For speeding up QMU processing, let it be the first priority to be executed. Other interrupts will be handled at next run.
	//ported from Liang-yen's code
	if (wIntrQMUDoneValue) {
	 	//qmu_done_interrupt(musb, wIntrQMUDoneValue);
	 	param.musb = musb;
		param.wQmuVal = wIntrQMUDoneValue;
	 	tasklet_schedule(&t_qmu_done);
	}
#endif /* NEVER */

 	if (wIntrQMUValue) {
		qmu_exception_interrupt(musb, wIntrQMUValue);
 	}
#endif

	if (dwLtssmValue) {
		mtu3d_u3_ltssm_intr_handler(musb, dwLtssmValue);
	}

	if (dwIntrUsbValue) {
		mtu3d_u2_common_intr_handler(dwIntrUsbValue);
	}

	if (dwLinkIntValue & SSUSB_DEV_SPEED_CHG_INTR) {
		mtu3d_link_intr_handler(dwLinkIntValue);
	}

#ifdef SUPPORT_OTG
	if (dwOtgIntValue) {
		mtu3d_otg_intr_handler(dwOtgIntValue);
	}
#endif

	spin_unlock_irqrestore(&musb->lock, flags);

	return retval;

}


void mtu3d_musb_enable(struct musb *musb)
{
	printk("%s\n", __func__);

#ifdef NEVER
	unsigned long flags;

	if (musb->power == true)
		return;

	flags = musb_readl(mtk_musb->mregs, USB_L1INTM);

	// mask ID pin, so "open clock" and "set flag" won't be interrupted. ISR may call clock_disable.
	musb_writel(mtk_musb->mregs, USB_L1INTM, (~IDDIG_INT_STATUS) & flags);

  if(platform_init_first){
  	DBG(0,"usb init first\n\r");
  	musb->is_host = true;
  }

	if (!mtk_usb_power) {
		if (down_interruptible(&power_clock_lock))
			xlog_printk(ANDROID_LOG_ERROR, "USB20", "%s: busy, Couldn't get power_clock_lock\n" \
				, __func__);
#ifndef CONFIG_MTK_FPGA
		enable_pll(UNIVPLL, "USB_PLL");

		printk("%s, enable UPLL before connect\n", __func__);
#endif

		mdelay(10);

		usb_phy_recover();

		mtk_usb_power = true;

		up(&power_clock_lock);
	}
	musb->power = true;

	musb_writel(mtk_musb->mregs,USB_L1INTM,flags);
#endif /* NEVER */
}

void mtu3d_musb_disable(struct musb *musb)
{
	printk("%s\n", __func__);

#ifdef NEVER
	if(musb->power == false)
        return;

	if(platform_init_first){
  	DBG(0,"usb init first\n\r");
  	musb->is_host = false;
  	platform_init_first = false;
  }

	if (mtk_usb_power) {
		if (down_interruptible(&power_clock_lock))
			xlog_printk(ANDROID_LOG_ERROR, "USB20", "%s: busy, Couldn't get power_clock_lock\n" \
				, __func__);

	//Modification for ALPS00408742
	mtk_usb_power = false;
	smp_mb();
	//printk("%s, line %d: %d, %d, before savecurrent\n", __func__, __LINE__, mtk_usb_power, musb->power);
	//Modification for ALPS00408742
	usb_phy_savecurrent();

#ifndef CONFIG_MTK_FPGA

	disable_pll(UNIVPLL,"USB_PLL");

	//printk("%s, disable VUSB and UPLL before disconnect\n", __func__);
#endif
	mtk_usb_power = false;

	//Modification for ALPS00408742
	smp_mb();
	//Modification for ALPS00408742
	up(&power_clock_lock);
	}

	musb->power = false;
#endif /* NEVER */
}

static int mtu3d_musb_init(struct musb *musb)
{
	os_printk(K_DEBUG, "%s\n", __func__);

	usb_nop_xceiv_register();
	musb->xceiv = usb_get_phy(USB_PHY_TYPE_USB2);
	if (IS_ERR_OR_NULL(musb->xceiv))
		goto unregister;

	mtu3d_musb_reg_init();

	if (is_peripheral_enabled(musb))
		musb->xceiv->set_power = mtu3d_set_power;

	musb->isr = generic_interrupt;
	return 0;

unregister:
	usb_nop_xceiv_unregister();
	return -ENODEV;

}

static int mtu3d_set_power(struct usb_phy *x, unsigned mA)
{
	os_printk(K_DEBUG, "%s\n", __func__);
	return 0;
}

static int mtu3d_musb_exit(struct musb *musb)
{
#ifdef NEVER
	struct device *dev = musb->controller;
	struct musb_hdrc_platform_data *plat = dev->platform_data;
#endif /* NEVER */

	usb_put_phy(musb->xceiv);
	usb_nop_xceiv_unregister();

	return 0;
}

static void mtu3d_musb_reg_init()
{
	int ret = 1;
	os_printk(K_DEBUG, "%s\n", __func__);

	/*
	 * FIXME:
	 * Do i really need this?
	 * IDDIG function does not work on FPGA. Need to set B role manually
	 */
#ifdef SUPPORT_OTG
	os_setmsk(U3D_SSUSB_OTG_STS, SSUSB_CHG_B_ROLE_B);
#endif


	/* initialize PHY related data structure */
	if (!u3phy_ops)
		ret = u3phy_init();

	if (ret || u3phy_ops) {
		u3phy_ops->init(u3phy);

		/* USB 2.0 slew rate calibration */
		u3phy_ops->u2_slew_rate_calibration(u3phy);

		/* disable ip power down, disable U2/U3 ip power down */
		mu3d_hal_ssusb_en();

		/* reset U3D all dev module. */
		mu3d_hal_rst_dev();
	} else {
		os_printk(K_EMERG, "%s: PHY initialization fail!\n", __func__);
		BUG_ON(1);
	}
}

static const struct musb_platform_ops mtu3d_ops = {
	.init		= mtu3d_musb_init,
	.exit		= mtu3d_musb_exit,

	.enable   = mtu3d_musb_enable,
	.disable  = mtu3d_musb_disable
};

static u64 usb_dmamask = DMA_BIT_MASK(32);

static int mtu3d_probe(struct platform_device *pdev)
{
	struct musb_hdrc_platform_data	*pdata = pdev->dev.platform_data;
	struct platform_device		*musb;
	struct mtu3d_glue		*glue;

	int ret = -ENOMEM;

	os_printk(K_DEBUG, "%s\n", __func__);

	glue = kzalloc(sizeof(*glue), GFP_KERNEL);
	if (!glue) {
		dev_err(&pdev->dev, "failed to allocate glue context\n");
		goto err0;
	}

	musb = platform_device_alloc(MUSB_DRIVER_NAME, PLATFORM_DEVID_NONE);
	if (!musb) {
		dev_err(&pdev->dev, "failed to allocate musb device\n");
		goto err1;
	}

	musb->dev.parent		= &pdev->dev;
	musb->dev.dma_mask		= &usb_dmamask;
	musb->dev.coherent_dma_mask	= usb_dmamask;

	mu3d_musb = musb;

	glue->dev			= &pdev->dev;
	glue->musb			= musb;

	pdata->platform_ops	= &mtu3d_ops;

	/* FIXME:
	 * the ep cfg should be placed at mt_dev.c.
	 * However, each item of the array is 8 showing from ICE.
	 * But the actul size of each item is 6 by sizeof().
	 * Move the array here, the array algin issue is fixed.
	 * So put here temp until we know the reason.
	 */
	pdata->config->fifo_cfg = mtu3d_cfg;
	pdata->config->fifo_cfg_size = ARRAY_SIZE(mtu3d_cfg);

	platform_set_drvdata(pdev, glue);

	ret = platform_device_add_resources(musb, pdev->resource,
			pdev->num_resources);
	if (ret) {
		dev_err(&pdev->dev, "failed to add resources\n");
		goto err2;
	}

	ret = platform_device_add_data(musb, pdata, sizeof(*pdata));
	if (ret) {
		dev_err(&pdev->dev, "failed to add platform_data\n");
		goto err2;
	}

	ret = platform_device_add(musb);
	if (ret) {
		dev_err(&pdev->dev, "failed to register musb device\n");
		goto err2;
	}

	return 0;

err2:
	platform_device_put(musb);

err1:
	kfree(glue);

err0:
	return ret;
}

static int __exit mtu3d_remove(struct platform_device *pdev)
{
	struct mtu3d_glue		*glue = platform_get_drvdata(pdev);

	platform_device_del(glue->musb);
	platform_device_put(glue->musb);
	kfree(glue);

	return 0;
}

#ifdef CONFIG_PM
static int mtu3d_suspend(struct device *dev)
{
	return 0;
}

static int mtu3d_resume(struct device *dev)
{

	mtu3d_musb_reg_init();

	return 0;
}

static struct dev_pm_ops mtu3d_pm_ops = {
	.suspend	= mtu3d_suspend,
	.resume		= mtu3d_resume,
};

#define DEV_PM_OPS	&mtu3d_pm_ops
#else
#define DEV_PM_OPS	NULL
#endif

static struct platform_driver mtu3d_driver = {
	.probe		= mtu3d_probe,
	.remove		= mtu3d_remove,
	.driver		= {
		.name	= "musb-mtu3d",
		.pm	= DEV_PM_OPS,
	},
};

MODULE_DESCRIPTION("mtu3d MUSB Glue Layer");
MODULE_AUTHOR("MediaTek");
MODULE_LICENSE("GPL v2");
module_platform_driver(mtu3d_driver);
