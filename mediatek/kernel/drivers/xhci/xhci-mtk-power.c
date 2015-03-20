#include "xhci.h"
#include <linux/xhci/xhci-mtk-power.h>
#include <linux/xhci/xhci-mtk.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/printk.h>

extern void mu3d_hal_u3dev_en(void);
extern void mu3d_hal_u3dev_dis(void);

static struct xhci_hcd	*mtk_xhci = NULL;

#define mtk_power_log(fmt, args...) \
    printk("%s(%d): " fmt, __func__, __LINE__, ##args)


static bool wait_for_value(int addr, int msk, int value, int ms_intvl, int count){
	int i;

	for (i = 0; i < count; i++){
		if((readl((void __iomem *)addr) & msk) == value)
			return true;
		mdelay(ms_intvl);
	}

	return false;
}

static void mtk_chk_usb_ip_ck_sts(void){
    int ret;
	int num_u3_port;
	int num_u2_port;

    num_u3_port = SSUSB_U3_PORT_NUM(readl((void __iomem *)SSUSB_IP_CAP));
	num_u2_port = SSUSB_U2_PORT_NUM(readl((void __iomem *)SSUSB_IP_CAP));

	ret = wait_for_value(SSUSB_IP_PW_STS1, SSUSB_SYS125_RST_B_STS, SSUSB_SYS125_RST_B_STS, 1, 10);
	if (ret == false)
        mtk_power_log("sys125_ck is still active!!!\n");

	/* do not check when SSUSB_U2_PORT_PDN = 1, because U2 port stays in reset state */
	if (num_u2_port && !(readl(SSUSB_U2_CTRL(0)) & SSUSB_U2_PORT_PDN)){
		ret = wait_for_value(SSUSB_IP_PW_STS2, SSUSB_U2_MAC_SYS_RST_B_STS, SSUSB_U2_MAC_SYS_RST_B_STS, 1, 10);
		if (ret == false)
			mtk_power_log("mac2_sys_ck is still active!!!\n");
	}

	/* do not check when SSUSB_U3_PORT_PDN = 1, because U3 port stays in reset state */
	if (num_u3_port && !(readl(SSUSB_U3_CTRL(0)) & SSUSB_U3_PORT_PDN)){
		ret = wait_for_value(SSUSB_IP_PW_STS1, SSUSB_U3_MAC_RST_B_STS, SSUSB_U3_MAC_RST_B_STS, 1, 10);
		if (ret == false)
	        mtk_power_log("mac3_mac_ck is still active!!!\n");
	}
}

#ifdef CONFIG_USB_MTK_DUALMODE
void mtk_xhci_set(struct xhci_hcd *xhci){
    mtk_xhci = xhci;
}

void mtk_port_reset_switch(bool tohost, bool skip_u3dev){
	int i;
	u32 temp;
	int num_u3_port;
	int num_u2_port;

    num_u3_port = SSUSB_U3_PORT_NUM(readl((void __iomem *)SSUSB_IP_CAP));
	num_u2_port = SSUSB_U2_PORT_NUM(readl((void __iomem *)SSUSB_IP_CAP));

    if(tohost)
        writel(readl((void __iomem *)SSUSB_IP_PW_CTRL_1) & (~SSUSB_IP_PDN), (void __iomem *)SSUSB_IP_PW_CTRL_1);

	for(i=0; i<num_u3_port; i++){
		temp = readl((void __iomem *)SSUSB_U3_CTRL(i));
		temp = temp | SSUSB_U3_PORT_PDN;
        temp = temp | SSUSB_U3_PORT_DIS;
		writel(temp, (void __iomem *)SSUSB_U3_CTRL(i));
	}

	for(i=0; i<num_u2_port; i++){
		temp = readl((void __iomem *)SSUSB_U2_CTRL(i));
		temp = temp | SSUSB_U2_PORT_PDN;
		temp = temp | SSUSB_U2_PORT_DIS;
		writel(temp, (void __iomem *)SSUSB_U2_CTRL(i));
	}

    mtk_chk_usb_ip_ck_sts();

	/* diable all u3 port power down and disable bits --> power on and enable all u3 ports */
	for(i=0; i<num_u3_port; i++){
		temp = readl((void __iomem *)SSUSB_U3_CTRL(i));
		temp = temp & (~SSUSB_U3_PORT_PDN);
        temp = temp & (~SSUSB_U3_PORT_DIS);
		temp = (tohost)? temp | SSUSB_U3_PORT_HOST_SEL
                       : temp & (~SSUSB_U3_PORT_HOST_SEL);
		writel(temp, (void __iomem *)SSUSB_U3_CTRL(i));
	}

    /* diable all u2 port power down and disable bits --> power on and enable all u2 ports */
	for(i=0; i<num_u2_port; i++){
		temp = readl((void __iomem *)SSUSB_U2_CTRL(i));
		temp = temp & (~SSUSB_U2_PORT_PDN);
		temp = temp & (~SSUSB_U2_PORT_DIS);
		temp = (tohost)? temp | SSUSB_U2_PORT_HOST_SEL
                       : temp & (~SSUSB_U2_PORT_HOST_SEL);
		writel(temp, (void __iomem *)SSUSB_U2_CTRL(i));
	}

    if(!skip_u3dev)
        mu3d_hal_u3dev_dis();

    if(!tohost)
        writel(readl((void __iomem *)SSUSB_IP_PW_CTRL_1) | (SSUSB_IP_PDN), (void __iomem *)SSUSB_IP_PW_CTRL_1);

	mtk_chk_usb_ip_ck_sts();

    if(!skip_u3dev)
        mu3d_hal_u3dev_en();
}

/* set 0 to PORT_POWER of PORT_STATUS register of each port */
void mtk_switch2host(){
    mtk_port_reset_switch(true, true);
    /* assert port power bit to drive drv_vbus */
    enableXhciAllPortPower(mtk_xhci);
}

void mtk_switch2device(bool skip_u3dev){
	int i;
	u32 temp;
	int num_u3_port;
	int num_u2_port;

    /* deassert port power bit to drop off vbus */
    disableXhciAllPortPower(mtk_xhci);

    mtk_port_reset_switch(false, skip_u3dev);
}

#endif

/* set 1 to PORT_POWER of PORT_STATUS register of each port */
void enableXhciAllPortPower(struct xhci_hcd *xhci){
	int i;
	u32 port_id, temp;
	u32 __iomem *addr;
	int num_u3_port;
	int num_u2_port;

    num_u3_port = SSUSB_U3_PORT_NUM(readl((void __iomem *)SSUSB_IP_CAP));
	num_u2_port = SSUSB_U2_PORT_NUM(readl((void __iomem *)SSUSB_IP_CAP));

	for(i=1; i<=num_u3_port; i++){
		port_id=i;
		addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
		temp = xhci_readl(xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp |= PORT_POWER;
		xhci_writel(xhci, temp, addr);
        while(!(xhci_readl(xhci, addr) & PORT_POWER));
	}

	for(i=1; i<=num_u2_port; i++){
		port_id=i+num_u3_port;
		addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
		temp = xhci_readl(xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp |= PORT_POWER;
		xhci_writel(xhci, temp, addr);
        while(!(xhci_readl(xhci, addr) & PORT_POWER));
	}
}

/* set 0 to PORT_POWER of PORT_STATUS register of each port */
void disableXhciAllPortPower(struct xhci_hcd *xhci){
	int i;
	u32 port_id, temp;
	u32 __iomem *addr;
	int num_u3_port;
	int num_u2_port;

    num_u3_port = SSUSB_U3_PORT_NUM(readl((void __iomem *)SSUSB_IP_CAP));
	num_u2_port = SSUSB_U2_PORT_NUM(readl((void __iomem *)SSUSB_IP_CAP));

	for(i=1; i<=num_u3_port; i++){
		port_id=i;
		addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
		temp = xhci_readl(xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp &= ~PORT_POWER;
		xhci_writel(xhci, temp, addr);
        while(xhci_readl(xhci, addr) & PORT_POWER);
	}

	for(i=1; i<=num_u2_port; i++){
		port_id=i+num_u3_port;
		addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
		temp = xhci_readl(xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp &= ~PORT_POWER;
		xhci_writel(xhci, temp, addr);
        while(xhci_readl(xhci, addr) & PORT_POWER);
	}
}

void enableAllClockPower(bool is_reset){
	int i;
	u32 temp;
	int num_u3_port;
	int num_u2_port;

    num_u3_port = SSUSB_U3_PORT_NUM(readl((void __iomem *)SSUSB_IP_CAP));
	num_u2_port = SSUSB_U2_PORT_NUM(readl((void __iomem *)SSUSB_IP_CAP));

	/* reset whole ip */
    if(is_reset){
    	writel(readl((void __iomem *)SSUSB_IP_PW_CTRL) | (SSUSB_IP_SW_RST), (void __iomem *)SSUSB_IP_PW_CTRL);
    	writel(readl((void __iomem *)SSUSB_IP_PW_CTRL) & (~SSUSB_IP_SW_RST), (void __iomem *)SSUSB_IP_PW_CTRL);
    }

    /* disable ip host power down bit --> power on host ip */
	writel(readl((void __iomem *)SSUSB_IP_PW_CTRL_1) & (~SSUSB_IP_PDN), (void __iomem *)SSUSB_IP_PW_CTRL_1);

	/* diable all u3 port power down and disable bits --> power on and enable all u3 ports */
	for(i=0; i<num_u3_port; i++){
		temp = readl((void __iomem *)SSUSB_U3_CTRL(i));
		temp = temp & (~SSUSB_U3_PORT_PDN) & (~SSUSB_U3_PORT_DIS) | SSUSB_U3_PORT_HOST_SEL;
		writel(temp, (void __iomem *)SSUSB_U3_CTRL(i));
	}

    /*
     * FIXME: clock is the correct 30MHz, if the U3 device is enabled
     */
    #if 0
    temp = readl(SSUSB_U3_CTRL(0));
    temp = temp & (~SSUSB_U3_PORT_PDN) & (~SSUSB_U3_PORT_DIS) & (~SSUSB_U3_PORT_HOST_SEL);
    writel(temp, SSUSB_U3_CTRL(0));
    #endif

    /* diable all u2 port power down and disable bits --> power on and enable all u2 ports */
	for(i=0; i<num_u2_port; i++){
		temp = readl((void __iomem *)SSUSB_U2_CTRL(i));
		temp = temp & (~SSUSB_U2_PORT_PDN) & (~SSUSB_U2_PORT_DIS) | SSUSB_U2_PORT_HOST_SEL;
		writel(temp, (void __iomem *)SSUSB_U2_CTRL(i));
	}
	//msleep(100);
	mtk_chk_usb_ip_ck_sts();
}

#if 0
//called after HC initiated
void disableAllClockPower(void){
	int i;
	u32 temp;
	int num_u3_port;
	int num_u2_port;

	num_u3_port = SSUSB_U3_PORT_NUM(readl(SSUSB_IP_CAP));
	num_u2_port = SSUSB_U2_PORT_NUM(readl(SSUSB_IP_CAP));

	//disable target ports
	for(i=0; i<num_u3_port; i++){
        temp = readl((void __iomem *)SSUSB_U3_CTRL(i));
		temp = temp | SSUSB_U3_PORT_PDN & (~SSUSB_U3_PORT_HOST_SEL);
        writel(temp, (void __iomem *)SSUSB_U3_CTRL(i));
	}

	for(i=0; i<num_u2_port; i++){
        temp = readl((void __iomem *)SSUSB_U2_CTRL(i));
		temp = temp | SSUSB_U2_PORT_PDN & (~SSUSB_U2_PORT_HOST_SEL);
        writel(temp, (void __iomem *)SSUSB_U2_CTRL(i));
	}

    writel(readl((void __iomem *)SSUSB_IP_PW_CTRL_1) | (SSUSB_IP_PDN), (void __iomem *)SSUSB_IP_PW_CTRL_1);
	//msleep(100);
	mtk_chk_usb_ip_ck_sts();
}
#endif


//(X)disable clock/power of a port
//(X)if all ports are disabled, disable IP ctrl power
//disable all ports and IP clock/power, this is just mention HW that the power/clock of port
//and IP could be disable if suspended.
//If doesn't not disable all ports at first, the IP clock/power will never be disabled
//(some U2 and U3 ports are binded to the same connection, that is, they will never enter suspend at the same time
//port_index: port number
//port_rev: 0x2 - USB2.0, 0x3 - USB3.0 (SuperSpeed)
void disablePortClockPower(int port_index, int port_rev){
	u32 temp;
	int real_index;

	real_index = port_index;

	if(port_rev == 0x3){
		temp = readl((void __iomem *)SSUSB_U3_CTRL(real_index));
		temp = temp | (SSUSB_U3_PORT_PDN);
		writel(temp, (void __iomem *)SSUSB_U3_CTRL(real_index));
	}
	else if(port_rev == 0x2){
		temp = readl((void __iomem *)SSUSB_U2_CTRL(real_index));
		temp = temp | (SSUSB_U2_PORT_PDN);
		writel(temp, (void __iomem *)SSUSB_U2_CTRL(real_index));
	}

	writel(readl((void __iomem *)SSUSB_IP_PW_CTRL_1) | (SSUSB_IP_PDN), (void __iomem *)SSUSB_IP_PW_CTRL_1);
}

//if IP ctrl power is disabled, enable it
//enable clock/power of a port
//port_index: port number
//port_rev: 0x2 - USB2.0, 0x3 - USB3.0 (SuperSpeed)
void enablePortClockPower(int port_index, int port_rev){
	u32 temp;
	int real_index;

	real_index = port_index;

	writel(readl((void __iomem *)SSUSB_IP_PW_CTRL_1) & (~SSUSB_IP_PDN), (void __iomem *)SSUSB_IP_PW_CTRL_1);

	if(port_rev == 0x3){
		temp = readl((void __iomem *)SSUSB_U3_CTRL(real_index));
		temp = temp & (~SSUSB_U3_PORT_PDN);
		writel(temp, (void __iomem *)SSUSB_U3_CTRL(real_index));
	}
	else if(port_rev == 0x2){
		temp = readl((void __iomem *)SSUSB_U2_CTRL(real_index));
		temp = temp & (~SSUSB_U2_PORT_PDN);
		writel(temp, (void __iomem *)SSUSB_U2_CTRL(real_index));
	}
}
