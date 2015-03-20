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

#ifndef TSF_TUNING_CUSTOM_H_
#define TSF_TUNING_CUSTOM_H_

MVOID *getTSFTrainingData(void);

MVOID *getTSFTuningData(void);

MUINT32 isEnableTSF(MINT32 const i4SensorDev);

MUINT32 getTSFD65Idx(void);

const MINT32* getTSFAWBForceInput(void);

#endif /* TSF_TUNING_CUSTOM_H_ */
