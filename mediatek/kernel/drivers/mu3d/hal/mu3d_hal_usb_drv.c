#include <linux/mu3d/hal/mu3d_hal_osal.h>
#define _MTK_USB_DRV_EXT_
#include <linux/mu3d/hal/mu3d_hal_usb_drv.h>
#undef _MTK_USB_DRV_EXT_
#include <linux/mu3d/hal/mu3d_hal_hw.h>
#include <linux/mu3d/hal/mu3d_hal_qmu_drv.h>
#include <linux/mu3d/hal/mu3d_hal_comm.h>
#include <linux/mu3phy/mtk-phy.h>



struct USB_REQ *mu3d_hal_get_req(DEV_INT32 ep_num, USB_DIR dir){
    DEV_INT32 ep_index=0;

    if(dir == USB_TX) {
        ep_index = ep_num;
    } else if(dir == USB_RX) {
        ep_index = ep_num + MAX_EP_NUM;
    } else {
        BUG_ON(1);
    }

    return &g_u3d_req[ep_index];
}

/**
 * mu3d_hal_ssusb_en - disable ssusb power down & enable u2/u3 ports
 *
 */
void mu3d_hal_ssusb_en(void)
{
	os_clrmsk(U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);
	os_clrmsk(U3D_SSUSB_IP_PW_CTRL2, SSUSB_IP_DEV_PDN);
	#ifdef SUPPORT_U3
	os_clrmsk(U3D_SSUSB_U3_CTRL_0P, (SSUSB_U3_PORT_DIS | SSUSB_U3_PORT_PDN | SSUSB_U3_PORT_HOST_SEL));
	#endif
	os_clrmsk(U3D_SSUSB_U2_CTRL_0P, (SSUSB_U2_PORT_DIS | SSUSB_U2_PORT_PDN | SSUSB_U2_PORT_HOST_SEL));

	os_setmsk(U3D_SSUSB_REF_CK_CTRL, (SSUSB_REF_MAC_CK_GATE_EN | SSUSB_REF_PHY_CK_GATE_EN | SSUSB_REF_CK_GATE_EN | SSUSB_REF_MAC3_CK_GATE_EN));

	/* check U3D sys125,u3 mac,u2 mac clock status. */
	mu3d_hal_check_clk_sts();
}

/**
 * mu3d_hal_dft_reg - apply default register settings
 */
void mu3d_hal_dft_reg(void)
{
	DEV_UINT32 tmp;

	/* set sys_ck related registers */
	#ifdef NEVER
	//sys_ck = OSC 125MHz/2 = 62.5MHz
	os_setmsk(U3D_SSUSB_SYS_CK_CTRL, SSUSB_SYS_CK_DIV2_EN);
	//U2 MAC sys_ck = ceil(62.5) = 63
	os_writelmsk(U3D_USB20_TIMING_PARAMETER, 63, TIME_VALUE_1US);
	#ifdef SUPPORT_U3
	//U3 MAC sys_ck = ceil(62.5) = 63
	os_writelmsk(U3D_TIMING_PULSE_CTRL, 63, CNT_1US_VALUE);
	#endif
	#endif

	//USB 2.0 related
	//sys_ck
	os_writelmsk(U3D_USB20_TIMING_PARAMETER, U3D_MAC_SYS_CK, TIME_VALUE_1US);
	//ref_ck
 	os_writelmsk(U3D_SSUSB_U2_PHY_PLL, U3D_MAC_REF_CK, SSUSB_U2_PORT_1US_TIMER);
	//spec: >600ns
	tmp = div_and_rnd_up(600, (1000 / U3D_MAC_REF_CK)) + 1;
	os_writelmsk(U3D_SSUSB_IP_PW_CTRL0,
		(tmp<<SSUSB_IP_U2_ENTER_SLEEP_CNT_OFST), SSUSB_IP_U2_ENTER_SLEEP_CNT);

	//USB 3.0 related
#ifdef SUPPORT_U3
	//sys_ck
	os_writelmsk(U3D_TIMING_PULSE_CTRL, U3D_MAC_SYS_CK, CNT_1US_VALUE);
	//ref_ck
 	os_writelmsk(U3D_REF_CK_PARAMETER, U3D_MAC_REF_CK, REF_1000NS);
	//spec: >=300ns
	tmp = div_and_rnd_up(300, (1000 / U3D_MAC_REF_CK));
	os_writelmsk(U3D_UX_EXIT_LFPS_TIMING_PARAMETER,
		(tmp<<RX_UX_EXIT_LFPS_REF_OFST), RX_UX_EXIT_LFPS_REF);
#endif

#ifdef NEVER
	/* set ref_ck related registers */
	//U2 ref_ck = OSC 20MHz/2 = 10MHz
 	os_writelmsk(U3D_SSUSB_U2_PHY_PLL, 10, SSUSB_U2_PORT_1US_TIMER);
	//>600ns
	os_writelmsk(U3D_SSUSB_IP_PW_CTRL0,
		(7<<SSUSB_IP_U2_ENTER_SLEEP_CNT_OFST), SSUSB_IP_U2_ENTER_SLEEP_CNT);
	#ifdef SUPPORT_U3
	//U3 ref_ck = 20MHz/2 = 10MHz
 	os_writelmsk(U3D_REF_CK_PARAMETER, 10, REF_1000NS);
	//>=300ns
	os_writelmsk(U3D_UX_EXIT_LFPS_TIMING_PARAMETER,
		(3<<RX_UX_EXIT_LFPS_REF_OFST), RX_UX_EXIT_LFPS_REF);
	#endif
#endif /* NEVER */

	/* code to override HW default values, FPGA ONLY */
	#ifndef CONFIG_MTK_FPGA
	//enable debug probe
	os_writel(U3D_SSUSB_PRB_CTRL0, 0xffff);
	#endif

	//USB 2.0 related
	//response STALL to host if LPM BESL value is not in supporting range
	os_setmsk(U3D_POWER_MANAGEMENT, (LPM_BESL_STALL | LPM_BESLD_STALL));


	//USB 3.0 related
	#ifdef SUPPORT_U3
	#ifdef U2_U3_SWITCH_AUTO
	os_setmsk(U3D_USB2_TEST_MODE, U2U3_AUTO_SWITCH);
	#endif

	//device responses to u3_exit from host automatically
	os_writel(U3D_LTSSM_CTRL, os_readl(U3D_LTSSM_CTRL) &~ SOFT_U3_EXIT_EN);

	#ifndef CONFIG_MTK_FPGA
	os_writel(U3D_PIPE_LATCH_SELECT, 0);
	#endif

	#endif

	#if ISO_UPDATE_TEST & ISO_UPDATE_MODE
	os_setmsk(U3D_POWER_MANAGEMENT, ISO_UPDATE);
	#else
	os_clrmsk(U3D_POWER_MANAGEMENT, ISO_UPDATE);
	#endif

	#ifdef DIS_ZLP_CHECK_CRC32
	//disable CRC check of ZLP, for compatibility concern
	os_writel(U3D_LINK_CAPABILITY_CTRL, ZLP_CRC32_CHK_DIS);
	#endif
}

/**
 * mu3d_hal_rst_dev - reset all device modules
 *
 */
void mu3d_hal_rst_dev(void){
	DEV_INT32 ret;

	os_printk(K_ERR, "%s\n", __func__);

	os_writel(U3D_SSUSB_DEV_RST_CTRL, SSUSB_DEV_SW_RST);
	os_writel(U3D_SSUSB_DEV_RST_CTRL, 0);

	mu3d_hal_check_clk_sts();

	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_DEV_QMU_RST_B_STS, SSUSB_DEV_QMU_RST_B_STS, 1, 10);
	if (ret == RET_FAIL)
        os_printk(K_ERR, "SSUSB_DEV_QMU_RST_B_STS NG\n");

	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_DEV_BMU_RST_B_STS, SSUSB_DEV_BMU_RST_B_STS, 1, 10);
	if (ret == RET_FAIL)
        os_printk(K_ERR, "SSUSB_DEV_BMU_RST_B_STS NG\n");

	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_DEV_RST_B_STS, SSUSB_DEV_RST_B_STS, 1, 10);
	if (ret == RET_FAIL)
        os_printk(K_ERR, "SSUSB_DEV_RST_B_STS NG\n");
}

/**
 * mu3d_hal_check_clk_sts - check sys125,u3 mac,u2 mac clock status
 *
 */
DEV_INT32 mu3d_hal_check_clk_sts(void){
	DEV_INT32 ret;

	os_printk(K_ERR, "%s\n", __func__);

	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_SYS125_RST_B_STS, SSUSB_SYS125_RST_B_STS, 1, 10);
	if (ret == RET_FAIL)
        os_printk(K_ERR, "SSUSB_SYS125_RST_B_STS NG\n");

	//do not check when SSUSB_U2_PORT_PDN = 1, because U2 port stays in reset state
	if (!(os_readl(U3D_SSUSB_U2_CTRL_0P) & SSUSB_U2_PORT_PDN))
	{
		ret = wait_for_value(U3D_SSUSB_IP_PW_STS2, SSUSB_U2_MAC_SYS_RST_B_STS, SSUSB_U2_MAC_SYS_RST_B_STS, 1, 10);
		if (ret == RET_FAIL)
			os_printk(K_ERR, "SSUSB_U2_MAC_SYS_RST_B_STS NG\n");
	}

#ifdef SUPPORT_U3
	//do not check when SSUSB_U3_PORT_PDN = 1, because U3 port stays in reset state
	if (!(os_readl(U3D_SSUSB_U3_CTRL_0P) & SSUSB_U3_PORT_PDN))
	{
		ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_U3_MAC_RST_B_STS, SSUSB_U3_MAC_RST_B_STS, 1, 10);
		if (ret == RET_FAIL)
	        os_printk(K_ERR, "SSUSB_U3_MAC_RST_B_STS NG\n");
	}
#endif

	os_printk(K_CRIT, "check clk pass!!\n");
	return RET_SUCCESS;
}

/**
 * mu3d_hal_link_up - u3d link up
 *
 */
DEV_INT32 mu3d_hal_link_up(DEV_INT32 latch_val)
{
	mu3d_hal_ssusb_en();
	mu3d_hal_rst_dev();
	os_ms_delay(50);
	os_writel(U3D_USB3_CONFIG, USB3_EN);
	os_writel(U3D_PIPE_LATCH_SELECT, latch_val);//set tx/rx latch sel

	return 0;
}


/**
 * mu3d_hal_initr_dis - disable all interrupts
 *
 */
void mu3d_hal_initr_dis(void){
	/* Disable Level 1 interrupts*/
	os_writel(U3D_LV1IECR, 0xFFFFFFFF);

	/* Disable Endpoint interrupts*/
	os_writel(U3D_EPIECR, 0xFFFFFFFF);

	/* Disable DMA interrupts*/
	os_writel(U3D_DMAIECR, 0xFFFFFFFF);

	/* Disable TxQ/RxQ Done interrupts*/
	os_writel(U3D_QIECR0, 0xFFFFFFFF);

	/* Disable TxQ/RxQ Error interrupts */
	os_writel(U3D_QIECR1, 0xFFFFFFFF);

	/* Disable TxQ/RxQ Empty indication*/
	os_writel(U3D_QEMIECR, 0xFFFFFFFF);

	/* Disable Tx GPD Error indication*/
	os_writel(U3D_TQERRIECR0, 0xFFFFFFFF);

	/* Disable Rx GPD Error indication*/
	os_writel(U3D_RQERRIECR0, 0xFFFFFFFF);

	/* Disable Rx ZLP Error indication*/
	os_writel(U3D_RQERRIECR1, 0xFFFFFFFF);

	/* Clear EP0 and Tx/Rx EPn interrupts status*/
	os_writel(U3D_EPISR, 0xFFFFFFFF);

	/* Clear EP0 and Tx/Rx EPn DMA interrupts status*/
	os_writel(U3D_DMAISR, 0xFFFFFFFF);

	/* Clear TxQ/RxQ Done interrupts status*/
	os_writel(U3D_QISAR0, 0xFFFFFFFF);

	/* Clear TxQ/RxQ Error interrupts status*/
	os_writel(U3D_QISAR1, 0xFFFFFFFF);

	/* Clear TxQ/RxQ Empty indication */
	os_writel(U3D_QEMIR, 0xFFFFFFFF);

	/* Clear TxQ GPD Error indication*/
	os_writel(U3D_TQERRIR0, 0xFFFFFFFF);

	/* Clear RxQ GPD Error indication*/
	os_writel(U3D_RQERRIR0, 0xFFFFFFFF);

	/* Clear RxQ ZLP Error indication*/
	os_writel(U3D_RQERRIR1, 0xFFFFFFFF);
}
/**
 * mu3d_hal_system_intr_en - enable system global interrupt
 *
 */
void mu3d_hal_system_intr_en(void)
{
	os_printk(K_ERR, "%s\n", __func__);

	DEV_UINT16 int_en;
	DEV_UINT32 ltssm_int_en;

	/* Disable All endpoint interrupt*/
	os_writel(U3D_EPIECR, os_readl(U3D_EPIER));

	/* Disable All DMA interrput*/
	os_writel(U3D_DMAIECR, os_readl(U3D_DMAIER));

	/* Disable U2 common USB interrupts*/
	os_writel(U3D_COMMON_USB_INTR_ENABLE, 0x00);

	/* Clear U2 common USB interrupts status*/
	os_writel(U3D_COMMON_USB_INTR, os_readl(U3D_COMMON_USB_INTR));

	/* Enable U2 common USB interrupt*/
	int_en = SUSPEND_INTR_EN|RESUME_INTR_EN|RESET_INTR_EN|CONN_INTR_EN|DISCONN_INTR_EN \
			|VBUSERR_INTR_EN|LPM_INTR_EN|LPM_RESUME_INTR_EN;
	os_writel(U3D_COMMON_USB_INTR_ENABLE, int_en);

	#ifdef SUPPORT_U3
	/* Disable U3 LTSSM interrupts*/
	os_writel(U3D_LTSSM_INTR_ENABLE, 0x00);
	os_printk(K_ERR, "U3D_LTSSM_INTR: %x\n", os_readl(U3D_LTSSM_INTR));

	/* Clear U3 LTSSM interrupts*/
	os_writel(U3D_LTSSM_INTR, os_readl(U3D_LTSSM_INTR));

	/* Enable U3 LTSSM interrupts*/
	ltssm_int_en = SS_INACTIVE_INTR_EN|SS_DISABLE_INTR_EN|COMPLIANCE_INTR_EN|LOOPBACK_INTR_EN \
		     |HOT_RST_INTR_EN|WARM_RST_INTR_EN|RECOVERY_INTR_EN|ENTER_U0_INTR_EN|ENTER_U1_INTR_EN \
		     |ENTER_U2_INTR_EN|ENTER_U3_INTR_EN|EXIT_U1_INTR_EN|EXIT_U2_INTR_EN|EXIT_U3_INTR_EN \
		     |RXDET_SUCCESS_INTR_EN|VBUS_RISE_INTR_EN|VBUS_FALL_INTR_EN|U3_LFPS_TMOUT_INTR_EN|U3_RESUME_INTR_EN;
	os_writel(U3D_LTSSM_INTR_ENABLE, ltssm_int_en);
	#endif

	#ifdef SUPPORT_OTG
	os_writel(U3D_SSUSB_OTG_INT_EN, 0x0);
	os_printk(K_ERR, "U3D_SSUSB_OTG_STS: %x\n", os_readl(U3D_SSUSB_OTG_STS));
	os_writel(U3D_SSUSB_OTG_STS_CLR, os_readl(U3D_SSUSB_OTG_STS));
	os_writel(U3D_SSUSB_OTG_INT_EN,
		SSUSB_VBUS_CHG_INT_B_EN | SSUSB_CHG_B_ROLE_B_INT_EN | \
		SSUSB_CHG_A_ROLE_B_INT_EN | SSUSB_ATTACH_B_ROLE_INT_EN);
	#endif

	/* Enable EP0 DMA interrupt*/
	os_writel(U3D_DMAIESR, EP0DMAIESR);

	/* Enable speed change interrupt*/
	os_writel(U3D_DEV_LINK_INTR_ENABLE, SSUSB_DEV_SPEED_CHG_INTR_EN);
}

/**
 * mu3d_hal_resume - power mode resume
 *
 */
void mu3d_hal_resume(void)
{
	#ifdef SUPPORT_U3
	if(os_readl(U3D_DEVICE_CONF) & HW_USB2_3_SEL){ //SS
		mu3d_hal_pdn_ip_port(1, 0, 1, 0);

		os_setmsk(U3D_LINK_POWER_CONTROL, UX_EXIT);
	}
	else
	#endif
	{ //hs fs
		mu3d_hal_pdn_ip_port(1, 0, 0, 1);

		os_setmsk(U3D_POWER_MANAGEMENT, RESUME);
    	os_ms_delay(10);
    	os_clrmsk(U3D_POWER_MANAGEMENT, RESUME);
	}
}

/**
 * mu3d_hal_u2dev_connect - u2 device softconnect
 *
 */
void mu3d_hal_u2dev_connect(){
	os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) | SOFT_CONN);
	os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) | SUSPENDM_ENABLE);
	os_printk(K_ERR, "SOFTCONN = 1\n");
}

/**
 * mu3d_hal_u3dev_en - enable U3D ss dev
 *
 */
void mu3d_hal_u3dev_en(void){
	os_writel(U3D_USB3_CONFIG, USB3_EN);
}

void mu3d_hal_u2dev_disconn(){
   os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) & ~SOFT_CONN);
   os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) & ~SUSPENDM_ENABLE);
}

/**
 * mu3d_hal_set_speed - enable ss or connect to hs/fs
 *@args - arg1: speed
 */
void mu3d_hal_set_speed(USB_SPEED speed){
 	#ifndef EXT_VBUS_DET
 	os_writel(U3D_MISC_CTRL, 0);
	#else
 	os_writel(U3D_MISC_CTRL, 0x3);
 	#endif

	printk("mu3d_hal_set_speed speed=%d\n", speed);

	/* clear ltssm state*/
    if(speed == SSUSB_SPEED_FULL){
        os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) & ~HS_ENABLE);
		mu3d_hal_u2dev_connect();
        g_u3d_setting.speed = SSUSB_SPEED_FULL;
    }
    else if(speed == SSUSB_SPEED_HIGH){
        os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) | HS_ENABLE);
		mu3d_hal_u2dev_connect();
        g_u3d_setting.speed = SSUSB_SPEED_HIGH;
    }
	#ifdef SUPPORT_U3
	else if(speed == SSUSB_SPEED_SUPER){
        g_u3d_setting.speed = SSUSB_SPEED_SUPER;
		mu3d_hal_u2dev_disconn();
		mu3d_hal_u3dev_en();
    }
	#endif
    else{
        os_printk(K_ALET,"Unsupported speed!!\n");
        BUG_ON(1);
    }
}

/**
 * mu3d_hal_pdn_cg_en - enable U2/U3 pdn & cg
 *@args -
 */
void mu3d_hal_pdn_cg_en(void)
{
#ifdef POWER_SAVING_MODE
	DEV_UINT8 speed = (os_readl(U3D_DEVICE_CONF) & SSUSB_DEV_SPEED);

	switch (speed)
	{
		case SSUSB_SPEED_SUPER:
			os_printk(K_EMERG, "Device: SUPER SPEED LOW POWER\n");
 			os_setmsk(U3D_SSUSB_U2_CTRL_0P, SSUSB_U2_PORT_PDN);
			break;
		case SSUSB_SPEED_HIGH:
			os_printk(K_EMERG, "Device: HIGH SPEED LOW POWER\n");
			#ifdef SUPPORT_U3
 			os_setmsk(U3D_SSUSB_U3_CTRL_0P, SSUSB_U3_PORT_PDN);
			#endif
			break;
		case SSUSB_SPEED_FULL:
			os_printk(K_EMERG, "Device: FULL SPEED LOW POWER\n");
			#ifdef SUPPORT_U3
			os_setmsk(U3D_SSUSB_U3_CTRL_0P, SSUSB_U3_PORT_PDN);
			#endif
			break;
		default:
			os_printk(K_EMERG, "[ERROR] Are you kidding!?!?\n");
			break;
	}
#endif
}

void mu3d_hal_pdn_ip_port(DEV_UINT8 on, DEV_UINT8 touch_dis, DEV_UINT8 u3, DEV_UINT8 u2)
{
	#ifdef POWER_SAVING_MODE
	if (on)
	{
		os_clrmsk(U3D_SSUSB_IP_PW_CTRL2, SSUSB_IP_DEV_PDN);

		#ifdef SUPPORT_U3
		if (u3)
			os_clrmsk(U3D_SSUSB_U3_CTRL_0P, SSUSB_U3_PORT_PDN
				| ((touch_dis) ? SSUSB_U3_PORT_DIS : 0));
		#endif
		if (u2)
			os_clrmsk(U3D_SSUSB_U2_CTRL_0P, SSUSB_U2_PORT_PDN
				| ((touch_dis) ? SSUSB_U2_PORT_DIS : 0));

		while(!(os_readl(U3D_SSUSB_IP_PW_STS1) & SSUSB_SYSPLL_STABLE));
	}
	else
	{
		#ifdef SUPPORT_U3
		if (u3)
			os_setmsk(U3D_SSUSB_U3_CTRL_0P, SSUSB_U3_PORT_PDN
				| ((touch_dis) ? SSUSB_U3_PORT_DIS : 0));
		#endif
		if (u2)
			os_setmsk(U3D_SSUSB_U2_CTRL_0P, SSUSB_U2_PORT_PDN
				| ((touch_dis) ? SSUSB_U2_PORT_DIS : 0));

		os_setmsk(U3D_SSUSB_IP_PW_CTRL2, SSUSB_IP_DEV_PDN);
	}
	#endif
}

/**
 * mu3d_hal_det_speed - detect device speed
 *@args - arg1: speed
 */
void mu3d_hal_det_speed(USB_SPEED speed, DEV_UINT8 det_speed){
	DEV_UINT8 temp;
	DEV_UINT16 cnt_down = 10000;

	printk("===Start polling===\n");

	//#ifdef EXT_VBUS_DET
	if (!det_speed)
	{
		while (!(os_readl(U3D_DEV_LINK_INTR) & SSUSB_DEV_SPEED_CHG_INTR))
		{
			os_ms_delay(1);
			cnt_down--;

			if (cnt_down == 0) {
				printk("WTF!!!!!!!!!!!!!!!!!!!");
				return;
			}
		}
	}
	else
	{
		while(!(os_readl(U3D_DEV_LINK_INTR) & SSUSB_DEV_SPEED_CHG_INTR));
	}
	//#else
	//while(!(os_readl(U3D_DEV_LINK_INTR) & SSUSB_DEV_SPEED_CHG_INTR));
	//#endif

	printk("===Polling End===\n");

	os_writel(U3D_DEV_LINK_INTR, SSUSB_DEV_SPEED_CHG_INTR);

	temp = (os_readl(U3D_DEVICE_CONF) & SSUSB_DEV_SPEED);
	switch (temp)
	{
		case SSUSB_SPEED_SUPER:
			os_printk(K_EMERG, "Device: SUPER SPEED\n");
			break;
		case SSUSB_SPEED_HIGH:
			os_printk(K_EMERG, "Device: HIGH SPEED\n");
			break;
		case SSUSB_SPEED_FULL:
			os_printk(K_EMERG, "Device: FULL SPEED\n");
			break;
		case SSUSB_SPEED_INACTIVE:
			os_printk(K_EMERG, "Device: INACTIVE\n");
			break;
	}

	if (temp != speed)
	{
		os_printk(K_EMERG, "desired speed: %d, detected speed: %d\n", speed, temp);
		os_ms_delay(5000);
		//while(1);
	}
}

/**
 * mu3d_hal_write_fifo - pio write one packet
 *@args - arg1: ep number, arg2: data length,  arg3: data buffer, arg4: max packet size
 */
DEV_INT32 mu3d_hal_write_fifo(DEV_INT32 ep_num,DEV_INT32 length,DEV_UINT8 *buf,DEV_INT32 maxp){

	DEV_UINT32 residue;
	DEV_UINT32 count;
	DEV_UINT32 temp;

	os_printk(K_DEBUG, "%s epnum=%d, len=%d, buf=%p, maxp=%d\n", __func__, ep_num, length, buf, maxp);

	#if (BUS_MODE==PIO_MODE)
	/* Here is really tricky, need to print this log to pass EPn PIO loopback
	 * No time to figure out why. Sorry~
	 */
	os_printk(K_DEBUG, "write_fifo=ep_num=%d, length=%d, buf=%p, maxp=%d\n",ep_num, length, buf, maxp);
	#endif

	count = residue = length;

	while(residue > 0) {

		if(residue==1) {
			temp=((*buf)&0xFF);
			os_writeb(USB_FIFO(ep_num), temp);
			buf += 1;
			residue -= 1;
		} else if(residue==2) {
			temp=((*buf)&0xFF)+(((*(buf+1))<<8)&0xFF00);
			os_writew(USB_FIFO(ep_num), temp);
			buf += 2;
			residue -= 2;
		} else if(residue==3) {
			temp=((*buf)&0xFF)+(((*(buf+1))<<8)&0xFF00);
			os_writew(USB_FIFO(ep_num), temp);
			buf += 2;

			temp=((*buf)&0xFF);
			os_writeb(USB_FIFO(ep_num), temp);
			buf += 1;
			residue -= 3;
		} else {
			temp=((*buf)&0xFF)+(((*(buf+1))<<8)&0xFF00)+(((*(buf+2))<<16)&0xFF0000)+(((*(buf+3))<<24)&0xFF000000);
			os_writel(USB_FIFO(ep_num), temp);
			buf += 4;
			residue -= 4;
		}
	}

#ifdef NEVER
    while(residue > 0){

		if(residue==1){
			temp=((*buf)&0xFF);
			os_writel(U3D_RISC_SIZE, RISC_SIZE_1B);
			unit=1;
		}
		else if(residue==2){
			temp=((*buf)&0xFF)+(((*(buf+1))<<8)&0xFF00);
			os_writel(U3D_RISC_SIZE, RISC_SIZE_2B);
			unit=2;
		}
		else if(residue==3){
			temp=((*buf)&0xFF)+(((*(buf+1))<<8)&0xFF00);
			os_writel(U3D_RISC_SIZE, RISC_SIZE_2B);
			unit=2;
		}
		else{
			temp=((*buf)&0xFF)+(((*(buf+1))<<8)&0xFF00)+(((*(buf+2))<<16)&0xFF0000)+(((*(buf+3))<<24)&0xFF000000);
			unit=4;
		}
		os_writel(USB_FIFO(ep_num), temp);
        buf=buf+unit;
		residue-=unit;

    }
	if(os_readl(U3D_RISC_SIZE)!=RISC_SIZE_4B){
		os_writel(U3D_RISC_SIZE, RISC_SIZE_4B);
	}
#endif /* NEVER */

	if (ep_num == 0) {
		if (count == 0) {
			os_printk(K_DEBUG,"USB_EP0_DATAEND %8X+\n", os_readl(U3D_EP0CSR));
			os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR) | EP0_DATAEND | EP0_TXPKTRDY);
			os_printk(K_DEBUG,"USB_EP0_DATAEND %8X-\n", os_readl(U3D_EP0CSR));
		} else {
#ifdef AUTOSET
			if (count < maxp) {
				os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR) | EP0_TXPKTRDY);
				os_printk(K_DEBUG,"EP0_TXPKTRDY\n");
			}
#else
			os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR)|EP0_TXPKTRDY);
#endif
		}
	} else {
		if (count == 0) {
			USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_TXPKTRDY);
		} else {
#ifdef AUTOSET
			if(count < maxp) {
				USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_TXPKTRDY);
				os_printk(K_DEBUG,"short packet\n");
			}
#else
			USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_TXPKTRDY);
#endif
		}
	}
	return count;
}

/**
 * mu3d_hal_read_fifo - pio read one packet
 *@args - arg1: ep number,  arg2: data buffer
 */
DEV_INT32 mu3d_hal_read_fifo(DEV_INT32 ep_num,DEV_UINT8 *buf){
    DEV_UINT16 count, residue;
	DEV_UINT32 temp;
 	DEV_UINT8 *bp = buf ;

	if (ep_num==0) {
		residue = count = os_readl(U3D_RXCOUNT0);
	} else {
		residue = count = (USB_ReadCsr32(U3D_RX1CSR3, ep_num)>>EP_RX_COUNT_OFST);
	}

	os_printk(K_DEBUG, "%s, req->buf=%p, cnt=%d\n", __func__, buf, count);

    while(residue > 0) {

	temp= os_readl(USB_FIFO(ep_num));

	/*Store the first byte*/
	*bp = temp&0xFF;

	/*Store the 2nd byte, If have*/
        if(residue>1)
		*(bp+1) = (temp>>8)&0xFF;

	/*Store the 3rd byte, If have*/
        if(residue>2)
		*(bp+2) = (temp>>16)&0xFF;

	/*Store the 4th byte, If have*/
        if(residue>3)
		*(bp+3) = (temp>>24)&0xFF;

        if(residue>4) {
		bp = bp + 4;
        	residue = residue - 4;
       	} else {
		residue = 0;
	}
    }

#ifdef AUTOCLEAR
	if (ep_num==0) {
		if (!count) {
			os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR)|EP0_RXPKTRDY);
			os_printk(K_DEBUG, "EP0_RXPKTRDY\n");
		}
	} else {
		if(!count){
			USB_WriteCsr32(U3D_RX1CSR0, ep_num, USB_ReadCsr32(U3D_RX1CSR0, ep_num) | RX_RXPKTRDY);
			os_printk(K_ALET,"ZLP\n");
		}
	}
#else
	if (ep_num==0) {
		os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR)|EP0_RXPKTRDY);
		os_printk(K_DEBUG, "EP0_RXPKTRDY\n");
	} else {
		USB_WriteCsr32(U3D_RX1CSR0, ep_num, USB_ReadCsr32(U3D_RX1CSR0, ep_num) | RX_RXPKTRDY);
		os_printk(K_DEBUG, "RX_RXPKTRDY\n");
	}
#endif
    return count;
}

/**
 * mu3d_hal_write_fifo_burst - pio write n packets with polling buffer full (epn only)
 *@args - arg1: ep number, arg2: u3d req
 */
DEV_INT32 mu3d_hal_write_fifo_burst(DEV_INT32 ep_num,DEV_INT32 length,DEV_UINT8 *buf,DEV_INT32 maxp){

    DEV_UINT32 residue, count, actual;
	DEV_UINT32 temp;
    DEV_UINT8 *bp ;

	os_printk(K_DEBUG,"%s ep_num=%d, lenght=%d, buf=%p, maxp=%d\n",__func__, ep_num,length,buf,maxp);

	#if (BUS_MODE==PIO_MODE)
	/* Here is really tricky, need to print this log to pass EPn PIO loopback
	 * No time to figure out why. Sorry~
	 */
	printk("write_fifo_burst=ep_num=%d, lenght=%d, buf=%p, maxp=%d\n",ep_num,length,buf,maxp);
	#endif

	actual = 0;

#ifdef AUTOSET
 	while(!(USB_ReadCsr32(U3D_TX1CSR0, ep_num) & TX_FIFOFULL)) {
#endif

		if(length - actual > maxp) {
			count = residue = maxp;
		} else {
			count = residue = length - actual;
		}

		bp = buf + actual;

		while(residue > 0) {

			if(residue==1) {
				temp=((*buf)&0xFF);
				os_writeb(USB_FIFO(ep_num), temp);
				buf += 1;
				residue -= 1;
			} else if(residue==2) {
				temp=((*buf)&0xFF)+(((*(buf+1))<<8)&0xFF00);
				os_writew(USB_FIFO(ep_num), temp);
				buf += 2;
				residue -= 2;
			} else if(residue==3) {
				temp=((*buf)&0xFF)+(((*(buf+1))<<8)&0xFF00);
				os_writew(USB_FIFO(ep_num), temp);
				buf += 2;

				temp=((*buf)&0xFF);
				os_writeb(USB_FIFO(ep_num), temp);
				buf += 1;
				residue -= 3;
			} else {
				temp=((*buf)&0xFF)+(((*(buf+1))<<8)&0xFF00)+(((*(buf+2))<<16)&0xFF0000)+(((*(buf+3))<<24)&0xFF000000);
				os_writel(USB_FIFO(ep_num), temp);
				buf += 4;
				residue -= 4;
			}
		}
#ifdef NEVER
    	while(residue > 0){

			if(residue==1){
				temp=((*bp)&0xFF);
				os_writel(U3D_RISC_SIZE, RISC_SIZE_1B);
				unit=1;
			}
			else if(residue==2){
				temp=((*bp)&0xFF)+(((*(bp+1))<<8)&0xFF00);
				os_writel(U3D_RISC_SIZE, RISC_SIZE_2B);
				unit=2;
			}
			else if(residue==3){
				temp=((*bp)&0xFF)+(((*(bp+1))<<8)&0xFF00);
				os_writel(U3D_RISC_SIZE, RISC_SIZE_2B);
				unit=2;
			}
			else{
				temp=((*bp)&0xFF)+(((*(bp+1))<<8)&0xFF00)+(((*(bp+2))<<16)&0xFF0000)+(((*(bp+3))<<24)&0xFF000000);
				unit=4;
			}
			os_writel(USB_FIFO(ep_num), temp);
        	bp=bp+unit;
			residue-=unit;
    	}
		if(os_readl(U3D_RISC_SIZE)!=RISC_SIZE_4B){
			os_writel(U3D_RISC_SIZE, RISC_SIZE_4B);
		}
#endif /* NEVER */
    	actual += count;

		if(length == 0) {
			USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_TXPKTRDY);
			return actual;
		} else {
#ifdef AUTOSET
			if((count < maxp) && (count > 0)) {
				USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_TXPKTRDY);
				os_printk(K_DEBUG,"short packet\n");
				return actual;
			}
			if(count == 0) {
				return actual;
			}
#else
			USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_TXPKTRDY);
#endif
		}
#ifdef AUTOSET
	}
#endif
	return actual;
}

/**
 * mu3d_hal_read_fifo_burst - pio read n packets with polling buffer empty (epn only)
 *@args - arg1: ep number, arg2: data buffer
 */
DEV_INT32 mu3d_hal_read_fifo_burst(DEV_INT32 ep_num,DEV_UINT8 *buf){

    DEV_UINT16 count, residue;
	DEV_UINT32 temp, actual;
 	DEV_UINT8 *bp;

	os_printk(K_INFO,"mu3d_hal_read_fifo_burst\n");
 	os_printk(K_ALET,"req->buf :%x\n", (DEV_UINT32)buf);
	actual = 0;
#ifdef AUTOCLEAR
	while(!(USB_ReadCsr32(U3D_RX1CSR0, ep_num) & RX_FIFOEMPTY)){
#endif
		residue = count = (USB_ReadCsr32(U3D_RX1CSR3, ep_num)>>EP_RX_COUNT_OFST);
 		os_printk(K_INFO,"count :%d ; req->actual :%d \n",count,actual);
		bp = buf + actual;

    	while(residue > 0){
			temp= os_readl(USB_FIFO(ep_num));
       		*bp = temp&0xFF;
			*(bp+1) = (temp>>8)&0xFF;
			*(bp+2) = (temp>>16)&0xFF;
			*(bp+3) = (temp>>24)&0xFF;
        	bp=bp+4;
        	if(residue>4){
        		residue=residue-4;
       		}
			else{
				residue=0;
			}
		}
    	actual += count;

#ifdef AUTOCLEAR
		if(!count){
			USB_WriteCsr32(U3D_RX1CSR0, ep_num, USB_ReadCsr32(U3D_RX1CSR0, ep_num) | RX_RXPKTRDY);
			os_printk(K_ALET,"zlp\n");
			os_printk(K_ALET,"actual :%d\n",actual);
			return actual;
		}
#else
		if(ep_num==0){
			os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR)|EP0_RXPKTRDY);
		}
		else{
			USB_WriteCsr32(U3D_RX1CSR0, ep_num, USB_ReadCsr32(U3D_RX1CSR0, ep_num) | RX_RXPKTRDY);
		}
#endif
#ifdef AUTOCLEAR
	}
#endif

	return actual;
}


/**
* mu3d_hal_unfigured_ep -
 *@args -
 */

void mu3d_hal_unfigured_ep(void){
    DEV_UINT32 i, tx_ep_num, rx_ep_num;
	struct USB_EP_SETTING *ep_setting;

	g_TxFIFOadd = USB_TX_FIFO_START_ADDRESS;
	g_RxFIFOadd = USB_RX_FIFO_START_ADDRESS;

#ifdef HARDCODE_EP
	tx_ep_num = MAX_QMU_EP;//os_readl(U3D_CAP_EPINFO) & CAP_TX_EP_NUM;
	rx_ep_num = MAX_QMU_EP;//(os_readl(U3D_CAP_EPINFO) & CAP_RX_EP_NUM) >> 8;
#else
	tx_ep_num = os_readl(U3D_CAP_EPINFO) & CAP_TX_EP_NUM;
	rx_ep_num = (os_readl(U3D_CAP_EPINFO) & CAP_RX_EP_NUM) >> 8;
#endif

	for(i=1; i<=tx_ep_num; i++){

		USB_WriteCsr32(U3D_TX1CSR0, i, USB_ReadCsr32(U3D_TX1CSR0, i) & (~0x7FF));
		USB_WriteCsr32(U3D_TX1CSR1, i, 0);
  		USB_WriteCsr32(U3D_TX1CSR2, i, 0);
		ep_setting = &g_u3d_setting.ep_setting[i];
		ep_setting->fifoaddr = 0;
		ep_setting->enabled = 0;
	}

	for(i=1; i<=rx_ep_num; i++){
		USB_WriteCsr32(U3D_RX1CSR0, i, USB_ReadCsr32(U3D_RX1CSR0, i) & (~0x7FF));
		USB_WriteCsr32(U3D_RX1CSR1, i, 0);
		USB_WriteCsr32(U3D_RX1CSR2, i, 0);
		ep_setting = &g_u3d_setting.ep_setting[i+MAX_EP_NUM];
		ep_setting->fifoaddr = 0;
		ep_setting->enabled = 0;
	}
}
/**
* mu3d_hal_ep_enable - configure ep
*@args - arg1: ep number, arg2: dir, arg3: transfer type, arg4: max packet size, arg5: interval, arg6: slot, arg7: burst, arg8: mult
*/
void mu3d_hal_ep_enable(DEV_UINT8 ep_num, USB_DIR dir, TRANSFER_TYPE type, DEV_INT32 maxp,
                  DEV_INT8 interval, DEV_INT8 slot,DEV_INT8 burst, DEV_INT8 mult) {

	DEV_INT32 ep_index=0;
	DEV_INT32 used_before;
	DEV_UINT8 fifosz=0, max_pkt, binterval;
	DEV_INT32 csr0, csr1, csr2;
	struct USB_EP_SETTING *ep_setting;

	/*Enable Burst, NumP=0, EoB*/
	os_writel( U3D_USB3_EPCTRL_CAP, os_readl(U3D_USB3_EPCTRL_CAP) | TX_NUMP_0_EN | SET_EOB_EN | TX_BURST_EN);

	if(slot > MAX_SLOT) {
		os_printk(K_ALET, "!!!!!!!!!!!!!!Configure wrong slot number!!!!!!!!!(MAX=%d, Not=%d)\n", MAX_SLOT, slot);
		slot = MAX_SLOT;
	}

	if(type == USB_CTRL) {

		ep_setting = &g_u3d_setting.ep_setting[0];
		ep_setting->fifosz = maxp;
		ep_setting->maxp = maxp;
		csr0 = os_readl(U3D_EP0CSR) & EP0_W1C_BITS;
		csr0 |= maxp;
		os_writel(U3D_EP0CSR, csr0);

		os_setmsk(U3D_USB2_RX_EP_DATAERR_INTR, BIT16); //EP0 data error interrupt
		return;
	}

	if(dir == USB_TX) {
		ep_index = ep_num;
	} else if(dir == USB_RX) {
		ep_index = ep_num + MAX_EP_NUM;
	} else {
		BUG_ON(1);
	}

	ep_setting = &g_u3d_setting.ep_setting[ep_index];
	used_before = ep_setting->fifoaddr;

	if(ep_setting->enabled)
		return;

	binterval = interval;
	if(dir == USB_TX) {
		if((g_TxFIFOadd + maxp * (slot + 1) > os_readl(U3D_CAP_EPNTXFFSZ)) && (!used_before)) {
			os_printk(K_ALET,"mu3d_hal_ep_enable, FAILED: sram exhausted\n");
			os_printk(K_ALET,"g_FIFOadd :%x\n",g_TxFIFOadd );
			os_printk(K_ALET,"maxp :%d\n",maxp );
			os_printk(K_ALET,"mult :%d\n",slot );
			BUG_ON(1);
		}
	} else {
		if((g_RxFIFOadd + maxp * (slot + 1) > os_readl(U3D_CAP_EPNRXFFSZ)) && (!used_before)) {
			os_printk(K_ALET,"mu3d_hal_ep_enable, FAILED: sram exhausted\n");
			os_printk(K_ALET,"g_FIFOadd :%x\n",g_RxFIFOadd );
			os_printk(K_ALET,"maxp :%d\n",maxp );
			os_printk(K_ALET,"mult :%d\n",slot );
			BUG_ON(1);
		}
	}

	ep_setting->transfer_type = type;

	if(dir == USB_TX) {
		if(!ep_setting->fifoaddr) {
			ep_setting->fifoaddr = g_TxFIFOadd;
		}
	} else {
		if(!ep_setting->fifoaddr) {
			ep_setting->fifoaddr = g_RxFIFOadd;
		}
	}

	ep_setting->fifosz = maxp;
	ep_setting->maxp = maxp;
	ep_setting->dir = dir;
	ep_setting->enabled = 1;

	switch(maxp){
		case 8:
			fifosz = USB_FIFOSZ_SIZE_16;
			break;
		case 16:
			fifosz = USB_FIFOSZ_SIZE_16;
			break;
		case 32:
			fifosz = USB_FIFOSZ_SIZE_32;
			break;
		case 64:
			fifosz = USB_FIFOSZ_SIZE_64;
			break;
		case 128:
			fifosz = USB_FIFOSZ_SIZE_128;
			break;
		case 256:
			fifosz = USB_FIFOSZ_SIZE_256;
			break;
		case 512:
			fifosz = USB_FIFOSZ_SIZE_512;
			break;
		case 1023:
		case 1024:
			fifosz = USB_FIFOSZ_SIZE_1024;
			break;
		case 2048:
			fifosz = USB_FIFOSZ_SIZE_1024;
			break;
		case 3072:
		case 4096:
			fifosz = USB_FIFOSZ_SIZE_1024;
			break;
		case 8192:
			fifosz = USB_FIFOSZ_SIZE_1024;
			break;
		case 16384:
			fifosz = USB_FIFOSZ_SIZE_1024;
			break;
		case 32768:
			fifosz = USB_FIFOSZ_SIZE_1024;
			break;
		}

	if(dir == USB_TX) {
		//CSR0
		csr0 = USB_ReadCsr32(U3D_TX1CSR0, ep_num) &~ TX_TXMAXPKTSZ;
		csr0 |= (maxp & TX_TXMAXPKTSZ);
#if (BUS_MODE==PIO_MODE)
#ifdef AUTOSET
		csr0 |= TX_AUTOSET;
#endif
		csr0 &= ~TX_DMAREQEN;
#endif

		//CSR1
		max_pkt = (burst+1)*(mult+1)-1;
		csr1 = (burst & SS_TX_BURST);
		csr1 |= (slot<<TX_SLOT_OFST) & TX_SLOT;
		csr1 |= (max_pkt<<TX_MAX_PKT_OFST) & TX_MAX_PKT;
		csr1 |= (mult<<TX_MULT_OFST) & TX_MULT;

		//CSR2
		csr2 = (g_TxFIFOadd>>4) & TXFIFOADDR;
		csr2 |= (fifosz<<TXFIFOSEGSIZE_OFST) & TXFIFOSEGSIZE;

		if(type == USB_BULK) {
			csr1 |= TYPE_BULK;
		} else if(type == USB_INTR) {
			csr1 |= TYPE_INT;
			csr2 |= (binterval<<TXBINTERVAL_OFST)&TXBINTERVAL;
		} else if(type == USB_ISO) {
			csr1 |= TYPE_ISO;
			csr2 |= (binterval<<TXBINTERVAL_OFST)&TXBINTERVAL;
		}

#ifdef USE_SSUSB_QMU
		os_writel(U3D_EPIECR, os_readl(U3D_EPIECR)|(BIT0<<ep_num));
#endif
		USB_WriteCsr32(U3D_TX1CSR0, ep_num, csr0);
		USB_WriteCsr32(U3D_TX1CSR1, ep_num, csr1);
		USB_WriteCsr32(U3D_TX1CSR2, ep_num, csr2);

		os_printk(K_INFO,"[CSR]U3D_TX1CSR0 :%x\n",USB_ReadCsr32(U3D_TX1CSR0, ep_num));
		os_printk(K_INFO,"[CSR]U3D_TX1CSR1 :%x\n",USB_ReadCsr32(U3D_TX1CSR1, ep_num));
		os_printk(K_INFO,"[CSR]U3D_TX1CSR2 :%x\n",USB_ReadCsr32(U3D_TX1CSR2, ep_num));

	}else if(dir == USB_RX) {
		//CSR0
		csr0 = USB_ReadCsr32(U3D_RX1CSR0, ep_num) &~ RX_RXMAXPKTSZ;
		csr0 |= (maxp & RX_RXMAXPKTSZ);
#if (BUS_MODE==PIO_MODE)
#ifdef AUTOCLEAR
		csr0 |= RX_AUTOCLEAR;
#endif
		csr0 &= ~RX_DMAREQEN;
#endif

		//CSR1
		max_pkt = (burst+1)*(mult+1)-1;
		csr1 = (burst & SS_RX_BURST);
		csr1 |= (slot<<RX_SLOT_OFST) & RX_SLOT;
		csr1 |= (max_pkt<<RX_MAX_PKT_OFST) & RX_MAX_PKT;
		csr1 |= (mult<<RX_MULT_OFST) & RX_MULT;

		//CSR2
		csr2 = (g_RxFIFOadd>>4) & RXFIFOADDR;
		csr2 |= (fifosz<<RXFIFOSEGSIZE_OFST) & RXFIFOSEGSIZE;

		if(type==USB_BULK) {
			csr1 |= TYPE_BULK;
		} else if(type==USB_INTR) {
			csr1 |= TYPE_INT;
			csr2 |= (binterval<<RXBINTERVAL_OFST)&RXBINTERVAL;
		} else if(type==USB_ISO) {
			csr1 |= TYPE_ISO;
			csr2 |= (binterval<<RXBINTERVAL_OFST)&RXBINTERVAL;
		}

#ifdef USE_SSUSB_QMU
		os_writel(U3D_EPIECR, os_readl(U3D_EPIECR)|(BIT16<<ep_num));
#endif
		USB_WriteCsr32(U3D_RX1CSR0, ep_num, csr0);
		USB_WriteCsr32(U3D_RX1CSR1, ep_num, csr1);
		USB_WriteCsr32(U3D_RX1CSR2, ep_num, csr2);

		os_printk(K_INFO,"[CSR]U3D_RX1CSR0 :%x\n",USB_ReadCsr32(U3D_RX1CSR0, ep_num));
		os_printk(K_INFO,"[CSR]U3D_RX1CSR1 :%x\n",USB_ReadCsr32(U3D_RX1CSR1, ep_num));
		os_printk(K_INFO,"[CSR]U3D_RX1CSR2 :%x\n",USB_ReadCsr32(U3D_RX1CSR2, ep_num));

		os_setmsk(U3D_USB2_RX_EP_DATAERR_INTR, BIT16<<ep_num); //EPn data error interrupt
	} else {
		os_printk(K_ERR, "WHAT THE DIRECTION IS?!?!\n");
		BUG_ON(1);
	}

	if(dir == USB_TX){
		if(maxp == 1023){
			g_TxFIFOadd += (1024 * (slot + 1));
		} else {
			g_TxFIFOadd += (maxp * (slot + 1));
		}
	} else {
		if(maxp == 1023) {
			g_RxFIFOadd += (1024 * (slot + 1));
		} else {
			g_RxFIFOadd += (maxp * (slot + 1));
		}
	}

	return;
}

