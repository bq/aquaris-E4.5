/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _OMAP_BOARD_H
#define _OMAP_BOARD_H
#include <linux/types.h>
#include <asm/arch/gpio-switch.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define OMAP_TAG_CLOCK 0x4f01
#define OMAP_TAG_MMC 0x4f02
#define OMAP_TAG_SERIAL_CONSOLE 0x4f03
#define OMAP_TAG_USB 0x4f04
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define OMAP_TAG_LCD 0x4f05
#define OMAP_TAG_GPIO_SWITCH 0x4f06
#define OMAP_TAG_UART 0x4f07
#define OMAP_TAG_FBMEM 0x4f08
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define OMAP_TAG_STI_CONSOLE 0x4f09
#define OMAP_TAG_CAMERA_SENSOR 0x4f0a
#define OMAP_TAG_BT 0x4f0b
#define OMAP_TAG_BOOT_REASON 0x4f80
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define OMAP_TAG_FLASH_PART 0x4f81
#define OMAP_TAG_VERSION_STR 0x4f82
struct omap_clock_config {
 u8 system_clock_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct omap_mmc_conf {
 unsigned enabled:1;
 unsigned nomux:1;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned cover:1;
 unsigned wire4:1;
 s16 power_pin;
 s16 switch_pin;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 s16 wp_pin;
};
struct omap_mmc_config {
 struct omap_mmc_conf mmc[2];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct omap_serial_console_config {
 u8 console_uart;
 u32 console_speed;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct omap_sti_console_config {
 unsigned enable:1;
 u8 channel;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct omap_camera_sensor_config {
 u16 reset_gpio;
 int (*power_on)(void * data);
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int (*power_off)(void * data);
};
struct omap_usb_config {
 unsigned register_host:1;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned register_dev:1;
 u8 otg;
 u8 hmc_mode;
 u8 rwc;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u8 pins[3];
};
struct omap_lcd_config {
 char panel_name[16];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 char ctrl_name[16];
 s16 nreset_gpio;
 u8 data_lines;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct device;
struct fb_info;
struct omap_backlight_config {
 int default_intensity;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int (*set_power)(struct device *dev, int state);
 int (*check_fb)(struct fb_info *fb);
};
struct omap_fbmem_config {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u32 start;
 u32 size;
};
struct omap_pwm_led_platform_data {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 const char *name;
 int intensity_timer;
 int blink_timer;
 void (*set_power)(struct omap_pwm_led_platform_data *self, int on_off);
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct omap_gpio_switch_config {
 char name[12];
 u16 gpio;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int flags:4;
 int type:4;
 int key_code:24;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct omap_uart_config {
 unsigned int enabled_uarts;
};
struct omap_flash_part_config {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 char part_table[0];
};
struct omap_boot_reason_config {
 char reason_str[12];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct omap_version_config {
 char component[12];
 char version[12];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct omap_board_config_entry {
 u16 tag;
 u16 len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u8 data[0];
};
struct omap_board_config_kernel {
 u16 tag;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 const void *data;
};
struct omap_bluetooth_config {
 u8 chip_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u8 bt_uart;
 u8 bd_addr[6];
 u8 bt_sysclk;
 int bt_wakeup_gpio;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int host_wakeup_gpio;
 int reset_gpio;
};
#define omap_get_config(tag, type)   ((const type *) __omap_get_config((tag), sizeof(type), 0))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define omap_get_nr_config(tag, type, nr)   ((const type *) __omap_get_config((tag), sizeof(type), (nr)))
#endif
