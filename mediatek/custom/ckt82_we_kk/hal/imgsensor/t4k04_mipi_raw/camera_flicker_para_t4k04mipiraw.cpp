#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_t4k04.h"
#include "camera_info_t4k04.h"
#include "camera_custom_AEPlinetable.h"
#include "camera_custom_flicker_para.h"
#include <cutils/xlog.h>


static void get_flicker_para_by_preview(FLICKER_CUST_PARA* para)
{
  int i;
  int freq[9] = {70,100, 120, 140, 170, 200, 220, 240, 260};
  FLICKER_CUST_STATISTICS EV50_L50 = {-159, 2869, 361, -649};
  FLICKER_CUST_STATISTICS EV50_L60 = {862, 714, 1448, -546};
  FLICKER_CUST_STATISTICS EV60_L50 = {1097, 949, 2584, -671};
  FLICKER_CUST_STATISTICS EV60_L60 = {-210, 5611, 167, -797};
  for(i=0;i<9;i++)
  {
    para->flickerFreq[i]=freq[i];
  }
  para->flickerGradThreshold=42;
  para->flickerSearchRange=32;
  para->minPastFrames=3;
  para->maxPastFrames=56;
  para->EV50_L50=EV50_L50;
  para->EV50_L60=EV50_L60;
  para->EV60_L50=EV60_L50;
  para->EV60_L60=EV60_L60;
  para->EV50_thresholds[0]=-30;
  para->EV50_thresholds[1]=16;
  para->EV60_thresholds[0]=-30;
  para->EV60_thresholds[1]=16;
  para->freq_feature_index[0]=2;
  para->freq_feature_index[1]=1;
}

static void get_flicker_para_by_ZSD(FLICKER_CUST_PARA* para)
{
  int i;
  int freq[9] = {70,80, 90, 100, 110, 120, 130, 140, 170};
  FLICKER_CUST_STATISTICS EV50_L50 = {-68, 1541, 501, -531};
  FLICKER_CUST_STATISTICS EV50_L60 = {855, 428, 532, -338};
  FLICKER_CUST_STATISTICS EV60_L50 = {956, 561, 746, -417};
  FLICKER_CUST_STATISTICS EV60_L60 = {-36, 1247, 464, -486};
  for(i=0;i<9;i++)
  {
    para->flickerFreq[i]=freq[i];
  }
  para->flickerGradThreshold=42;
  para->flickerSearchRange=32;
  para->minPastFrames=3;
  para->maxPastFrames=14;
  para->EV50_L50=EV50_L50;
  para->EV50_L60=EV50_L60;
  para->EV60_L50=EV60_L50;
  para->EV60_L60=EV60_L60;
  para->EV50_thresholds[0]=-30;
  para->EV50_thresholds[1]=8;
  para->EV60_thresholds[0]=-30;
  para->EV60_thresholds[1]=8;
  para->freq_feature_index[0]=5;
  para->freq_feature_index[1]=3;
}


typedef NSFeature::RAWSensorInfo<SENSOR_ID> SensorInfoSingleton_T;
namespace NSFeature {
template <>
UINT32
SensorInfoSingleton_T::
impGetFlickerPara(MINT32 sensorMode, MVOID*const pDataBuf) const
{
	XLOGD("impGetFlickerPara+ mode=%d", sensorMode);
	XLOGD("prv=%d, vdo=%d, cap=%d, zsd=%d",
	    (int)e_sensorModePreview, (int)e_sensorModeVideoPreview, (int)e_sensorModeCapture, (int)e_sensorModeZsd );
	FLICKER_CUST_PARA* para;
	para =  (FLICKER_CUST_PARA*)pDataBuf;
	if(sensorMode==e_sensorModePreview)
		get_flicker_para_by_preview(para);
	else if(sensorMode==e_sensorModeZsd||
	   sensorMode==e_sensorModeVideoPreview ||
	   sensorMode==e_sensorModeCapture)
	{
		get_flicker_para_by_ZSD(para);
	}
	else
	{
		XLOGD("impGetFlickerPara ERROR ln=%d", __LINE__);
		return -1;
	}
	XLOGD("impGetFlickerPara-");
	return 0;
}
}

