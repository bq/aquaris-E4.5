/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "camera_custom_types.h"
#include "string.h"
#ifdef WIN32
#else
#include "camera_custom_nvram.h"
#endif
#include "flash_feature.h"
#include "flash_param.h"
#include "flash_tuning_custom.h"

#include <cutils/xlog.h>
#include "flash_feature.h"
#include "flash_param.h"
#include "flash_tuning_custom.h"
#include <kd_camera_feature.h>

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int getDefaultStrobeNVRam_sub(void* data, int* ret_size);

int getDefaultStrobeNVRam_main(void* data, int* ret_size)
{
	//static NVRAM_CAMERA_STROBE_STRUCT strobeNVRam;
	NVRAM_CAMERA_STROBE_STRUCT* p;
	p = (NVRAM_CAMERA_STROBE_STRUCT*)data;

	static short engTab[]=
    {
        100,200,600,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,1,
		1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,2,
		1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,3,
		1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,4,
		1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,5,
		1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,6,
		1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,7,
		1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,8,
    };

	//version
	p->u4Version = NVRAM_CAMERA_STROBE_FILE_VERSION;
	//eng tab
	memcpy(p->engTab.yTab, engTab, 256*sizeof(short));
	p->engTab.exp =20000;
	p->engTab.afe_gain = 1024;
	p->engTab.isp_gain = 1024;
	p->engTab.distance = 300; //mm
	//tuning para
	p->tuningPara[0].yTar = 188;
	p->tuningPara[0].antiIsoLevel = -5;
	p->tuningPara[0].antiExpLevel = -8;
	p->tuningPara[0].antiStrobeLevel = -0;
	p->tuningPara[0].antiUnderLevel = 0;
	p->tuningPara[0].antiOverLevel = 0;
	p->tuningPara[0].foregroundLevel = 3;
	p->tuningPara[0].isRefAfDistance = 0;
	p->tuningPara[0].accuracyLevel = 10;

	p->tuningPara[1].yTar = 188;
	p->tuningPara[1].antiIsoLevel = -5;
	p->tuningPara[1].antiExpLevel = -8;
	p->tuningPara[1].antiStrobeLevel = -10;
	p->tuningPara[1].antiUnderLevel = 0;
	p->tuningPara[1].antiOverLevel = 3;
	p->tuningPara[1].foregroundLevel = 3;
	p->tuningPara[1].isRefAfDistance = 0;
	p->tuningPara[1].accuracyLevel = 10;

	p->tuningPara[2].yTar = 188;
	p->tuningPara[2].antiIsoLevel = -5;
	p->tuningPara[2].antiExpLevel = -8;
	p->tuningPara[2].antiStrobeLevel = -10;
	p->tuningPara[2].antiUnderLevel = 0;
	p->tuningPara[2].antiOverLevel = 3;
	p->tuningPara[2].foregroundLevel = 3;
	p->tuningPara[2].isRefAfDistance = 0;
	p->tuningPara[2].accuracyLevel = 10;

	p->tuningPara[3].yTar = 188;
	p->tuningPara[3].antiIsoLevel = -5;
	p->tuningPara[3].antiExpLevel = -8;
	p->tuningPara[3].antiStrobeLevel = -10;
	p->tuningPara[3].antiUnderLevel = 0;
	p->tuningPara[3].antiOverLevel = 3;
	p->tuningPara[3].foregroundLevel = 3;
	p->tuningPara[3].isRefAfDistance = 0;
	p->tuningPara[3].accuracyLevel = 10;
	//is eng level used (or by firmware)
	p->isTorchEngUpdate =0;
	p->isNormaEnglUpdate =0;
	p->isLowBatEngUpdate =0;
	p->isBurstEngUpdate =0;
	//eng level
	memset(&p->engLevel, 0, sizeof(FLASH_ENG_LEVEL));

	*ret_size = sizeof(NVRAM_CAMERA_STROBE_STRUCT);
	return 0;
}


int getDefaultStrobeNVRam(int sensorType, void* data, int* ret_size)
{

	if(sensorType==(int)DUAL_CAMERA_SUB_SENSOR)
	{
		XLOGD("getDefaultStrobeNVRam ln=%d sensorId=%d",__LINE__, sensorType);
		return getDefaultStrobeNVRam_sub(data, ret_size);
	}
	else //DUAL_CAMERA_MAIN_SENSOR
	{
		XLOGD("getDefaultStrobeNVRam ln=%d sensorId=%d",__LINE__, sensorType);
		return getDefaultStrobeNVRam_main(data, ret_size);


	}
}
