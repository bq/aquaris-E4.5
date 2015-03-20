#include <utils/Log.h>
#include <utils/Errors.h>
#include <fcntl.h>
#include <math.h>

#include "MediaTypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <cutils/xlog.h>	// For XLOG?().

//#include "lens_custom_cfg.h"
//#include "msdk_lens_exp.h"
#include "camera_custom_lens.h"
//#include "lens.h"
//nclude "image_sensor.h"
#include "kd_imgsensor.h"

extern PFUNC_GETLENSDEFAULT pDummy_getDefaultData;

#if defined(SENSORDRIVE)
extern PFUNC_GETLENSDEFAULT pSensorDrive_getDefaultData;
#endif
#if defined(BU6424AF)
extern PFUNC_GETLENSDEFAULT pOV13850AF_getDefaultData;
#endif
#if defined(FM50AF)
extern PFUNC_GETLENSDEFAULT pFM50AF_getDefaultData;
extern PFUNC_GETLENSDEFAULT pOV8865AF_getDefaultData;
extern PFUNC_GETLENSDEFAULT pDW9714A_getDefaultData;
#endif

MSDK_LENS_INIT_FUNCTION_STRUCT LensList[MAX_NUM_OF_SUPPORT_LENS] =
{
#if defined(SENSORDRIVE)
	{DUMMY_SENSOR_ID, SENSOR_DRIVE_LENS_ID, "kd_camera_hw", pSensorDrive_getDefaultData},	
    //  for backup lens, need assign correct SensorID
    //{OV3640_SENSOR_ID, SENSOR_DRIVE_LENS_ID, "kd_camera_hw", pSensorDrive_getDefaultData},
#endif

#if defined(FM50AF)
    {OV8865_SENSOR_ID, FM50AF_LENS_ID, "FM50AF", pOV8865AF_getDefaultData},
	//{DUMMY_SENSOR_ID, FM50AF_LENS_ID, "FM50AF", pFM50AF_getDefaultData},
	{T4K04_SENSOR_ID, FM50AF_LENS_ID, "FM50AF", pFM50AF_getDefaultData},
	{OV12830_SENSOR_ID, OV12830AF_LENS_ID, "FM50AF", pDW9714A_getDefaultData},
#endif
#if defined(BU6424AF)
	//{OV12830_SENSOR_ID, OV12830AF_LENS_ID, "FM50AF", pOV13850AF_getDefaultData},
    {OV13850_SENSOR_ID, BU6424AF_LENS_ID, "BU6424AF", pOV13850AF_getDefaultData},
 #endif
#if defined(OV8825AF)
		{OV8825_SENSOR_ID, OV8825AF_LENS_ID, "OV8825AF", pOV8825AF_getDefaultData},
#endif
	{DUMMY_SENSOR_ID, DUMMY_LENS_ID, "Dummy", pDummy_getDefaultData},
    //  for new added lens, need assign correct SensorID

};


UINT32 GetLensInitFuncList(PMSDK_LENS_INIT_FUNCTION_STRUCT pLensList)
{
	char xbuf[128];
	int fd = open("/sys/devices/platform/image_sensor/currSensorName",O_RDONLY);
	read(fd,xbuf,128);
	close(fd);

	XLOGD("[GetLensInitFuncList]current working sensor:%s  ",xbuf);  
	memcpy(pLensList, &LensList[0], sizeof(MSDK_LENS_INIT_FUNCTION_STRUCT)* MAX_NUM_OF_SUPPORT_LENS);
       	return MHAL_NO_ERROR;
} // GetLensInitFuncList()






