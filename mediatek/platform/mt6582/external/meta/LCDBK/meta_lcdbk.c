
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "meta_lcdbk.h"
#include <utils/Log.h>

#undef LOG_TAG 
#define LOG_TAG "META"

//#define USE_PMIC_IOCTL

#ifdef USE_PMIC_IOCTL
#define TEST_PMIC_PRINT 0
#define PMIC_READ 1
#define PMIC_WRITE 2
#define SET_PMIC_LCDBK 3

int set_lcdbk_data[1] = {32}; /* brightness*/
int meta_lcdbk_fd = 0;
#endif

int adwBrightness[10] = {255, 0, 30, 65, 100, 135, 170, 205, 240, 255};

LCDLevel_CNF Meta_LCDBK_OP(LCDLevel_REQ dwBrightness)	
{
#ifdef USE_PMIC_IOCTL
	int dwVurrentBrightDuty;
	int ret;
	LCDLevel_CNF lcdbk_cnf;
	lcdbk_cnf.status=true;	

	/* CHecking the range of  lcd_light_level */
	if ( dwBrightness.lcd_light_level < 0 || dwBrightness.lcd_light_level > 9 ) {
		printf("brightness is not correct ! \n");
		lcdbk_cnf.status=false;
		return lcdbk_cnf;
	}
	/* open file */
	meta_lcdbk_fd = open("/dev/MT6326-pmic",O_RDWR, 0);
	if (meta_lcdbk_fd == -1) {
		printf("Open /dev/MT6326-pmic : ERROR \n");
		lcdbk_cnf.status=false;
		return lcdbk_cnf;
	}	

	/* Gemini Phone */
	dwVurrentBrightDuty = adwBrightness[dwBrightness.lcd_light_level];
	if(dwVurrentBrightDuty >=32)
		dwVurrentBrightDuty=0;
	set_lcdbk_data[0]=dwVurrentBrightDuty;
	ret = ioctl(meta_lcdbk_fd, SET_PMIC_LCDBK, set_lcdbk_data);
	if (ret == -1) {
		printf("Meta_LCDBK_OP : ERROR \n");
		lcdbk_cnf.status=false;					
	}
	
	//printf("Meta_LCDBK_OP : Set Brightness %d \n", set_lcdbk_data[0]);

	close(meta_lcdbk_fd);
	
	return lcdbk_cnf;
#else // !USE_PMIC_IOCTL
	LCDLevel_CNF lcdbk_cnf;
	int fd = -1, level;
#define BUF_LEN 16
	char wbuf[BUF_LEN] = {'\0'};
	char rbuf[BUF_LEN] = {'\0'};

	lcdbk_cnf.status = false;

#define BRIGHTNESS_FILE "/sys/class/leds/lcd-backlight/brightness"
	fd = open(BRIGHTNESS_FILE, O_RDWR, 0);
	if (fd == -1) {
		LOGE("Can't open %s\n", BRIGHTNESS_FILE);
		goto EXIT;
	}
	level = adwBrightness[dwBrightness.lcd_light_level];
	sprintf(wbuf, "%d\n", level);
	if (write(fd, wbuf, strlen(wbuf)) == -1) {
		LOGE("Can't write %s\n", BRIGHTNESS_FILE);
		goto EXIT;
	}
	close(fd);
	fd = open(BRIGHTNESS_FILE, O_RDWR, 0);
	if (fd == -1) {
		LOGE("Can't open %s\n", BRIGHTNESS_FILE);
		goto EXIT;
	}
	if (read(fd, rbuf, BUF_LEN) == -1) {
		LOGE("Can't read %s\n", BRIGHTNESS_FILE);
		goto EXIT;
	}
	if (!strncmp(wbuf, rbuf, BUF_LEN))
		lcdbk_cnf.status = true;

EXIT:
		if (fd != -1)
			close(fd);
		return lcdbk_cnf;
#endif
}

BOOL Meta_LCDBK_Init()
{
	return true;
}

BOOL Meta_LCDBK_Deinit()
{
	return true;
}



