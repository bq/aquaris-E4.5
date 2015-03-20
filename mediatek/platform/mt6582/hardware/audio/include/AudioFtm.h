/*******************************************************************************
 *
 * Filename:
 * ---------
 *   AudioFtm.h
 *
 * Project:
 * --------
 *   Android Audio Driver
 *
 * Description:
 * ------------
 *   Factory Mode
 *
 * Author:
 * -------
 *   Chipeng Chang (mtk02308)
 *
 *------------------------------------------------------------------------------
 * $Revision: #5 $
 * $Modtime:$
 * $Log:$
 *
 *
 *******************************************************************************/

#ifndef ANDROID_AUDIO_FTM_H
#define ANDROID_AUDIO_FTM_H

/*****************************************************************************
*                     C O M P I L E R   F L A G S
******************************************************************************
*/

/*****************************************************************************
*                E X T E R N A L   R E F E R E N C E S
******************************************************************************
*/
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include "AudioDef.h"

#include "AudioFtmBase.h"

#include "AudioVolumeFactory.h"
#include "AudioAnalogControlFactory.h"
#include "AudioDigitalControlFactory.h"

#include "AudioMTKStreamInManager.h"
#include "AudioDigitalType.h"
#include "AudioAnalogType.h"

/*****************************************************************************
*                          C O N S T A N T S
******************************************************************************
*/

/*****************************************************************************
*                         M A C R O
******************************************************************************
*/

/*****************************************************************************
*                  R E G I S T E R       D E F I N I T I O N
******************************************************************************
*/

/*****************************************************************************
*                        F U N C T I O N   D E F I N I T I O N
******************************************************************************
*/

/*****************************************************************************
*                         D A T A   T Y P E S
******************************************************************************
*/

namespace android
{


/*****************************************************************************
*                        C L A S S   D E F I N I T I O N
******************************************************************************
*/

class AudioFtm : public AudioFtmBase
{
    public:
        static AudioFtm *getInstance();
        ~AudioFtm();

        bool AudFtm_Init();

        // for FM Tx and HDMI  factory mode & Meta mode
        bool WavGen_SW_SineWave(bool Enable, uint32 Freq, int type);
        bool WavGen_SWPattern(bool Enable, uint32 Freq, int type);
        static void *FmTx_thread_create(void *arg);
        void FmTx_thread_digital_out(void);
        void FmTx_thread_analog_out(void);
        static void *HDMI_thread_create(void *arg);
        void HDMI_thread_I2SOutput(void);
        void WavGen_AudioRead(char *pBuffer, unsigned int bytes);
        unsigned int WavGen_AudioWrite(void *buffer, unsigned int bytes);

        // for factory mode & Meta mode (Digital part)
        void Afe_Enable_SineWave(bool bEnable);

        // for factory mode & Meta mode (Analog part)
        void FTM_AnaLpk_on(void);
        void FTM_AnaLpk_off(void);

        void Audio_Set_Speaker_Vol(int level);
        void Audio_Set_Speaker_On(int Channel);
        void Audio_Set_Speaker_Off(int Channel);
        void Audio_Set_HeadPhone_On(int Channel);
        void Audio_Set_HeadPhone_Off(int Channel);
        void Audio_Set_Earpiece_On();
        void Audio_Set_Earpiece_Off();

        int RecieverTest(char receiver_test);
        int LouderSPKTest(char left_channel, char right_channel);
        int EarphoneTest(char bEnable);
        int EarphoneTestLR(char bLR);
        int Pmic_I2s_out(char echoflag);

        // FM loopback test
        int FMLoopbackTest(char bEnable);
        int Audio_FM_I2S_Play(char bEnable);
        int Audio_MATV_I2S_Play(char bEnable);

        // speaker OC test
        int Audio_READ_SPK_OC_STA(void);
        int LouderSPKOCTest(char left_channel, char right_channel);

        void FTMI2SDacOutSet(uint32 SampleRate);

        void FTMPMICLoopbackTest(bool bEnable);
        void FTMPMICEarpieceLoopbackTest(bool bEnable);
        void FTMPMICDualModeLoopbackTest(bool bEnable);
        int HDMI_SineGenPlayback(bool bEnable, int dSamplingRate);

        /// Loopback
        int PhoneMic_Receiver_Loopback(char echoflag);
        int PhoneMic_EarphoneLR_Loopback(char echoflag);
        int PhoneMic_SpkLR_Loopback(char echoflag);
        int HeadsetMic_EarphoneLR_Loopback(char bEnable, char bHeadsetMic);
        int HeadsetMic_SpkLR_Loopback(char echoflag);

        int PhoneMic_Receiver_Acoustic_Loopback(int Acoustic_Type, int *Acoustic_Status_Flag, int bHeadset_Output);


        /// Vibration Speaker
        int      SetVibSpkCalibrationParam(void *cali_param);
        uint32_t GetVibSpkCalibrationStatus();
        void     SetVibSpkEnable(bool enable, uint32_t freq);
        void     SetVibSpkRampControl(uint8_t rampcontrol);
        
        bool     ReadAuxadcData(int channel, int *value);



    private:

        int RequestClock(void);
        int ReleaseClock(void);


        pthread_t m_WaveThread;
        bool mAudioSinWave_thread;
        unsigned int IdxAudioPattern;
        unsigned int SizeAudioPattern;
        unsigned char *g_i2VDL_DATA;
        char *mAudioBuffer;
        AudioMTKVolumeInterface *mAudioVolumeInstance;
        AudioAnalogControlInterface *mAudioAnalogInstance;
        AudioDigitalControlInterface *mAudioDigitalInstance;
        //AudioMTKStreamInManager *mStreamInManager;    //ship marked
        AudioResourceManagerInterface *mAudioResourceManager;

        static AudioFtm *UniqueAudioFtmInstance;
        AudioFtm();
        AudioFtm(const AudioFtm &);             // intentionally undefined
        AudioFtm &operator=(const AudioFtm &);  // intentionally undefined

        int    mFd;
        int    mSamplingRate;
        int    mSineWaveStatus;
};

}; // namespace android

#endif



