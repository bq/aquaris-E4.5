 
#ifndef ANDROID_VIBSPK_AUDIOPLAYER_H
#define ANDROID_VIBSPK_AUDIOPLAYER_H

#include <utils/threads.h>

#include <media/MediaPlayerInterface.h>
#include <media/AudioTrack.h>
#include <media/AudioRecord.h>


namespace android
{
extern bool VIBRATOR_SPKON(unsigned int timeoutms);
extern bool VIBRATOR_SPKOFF();
extern int VIBRATOR_SPKEXIST();

class VibSpkAudioPlayer
{
public:
    VibSpkAudioPlayer(audio_stream_type_t streamType, float volume, bool threadCanCallJava);
    ~VibSpkAudioPlayer();
    static VibSpkAudioPlayer *getInstance();
    bool start();
    void stop();
    unsigned short      mState;
    
private:
    unsigned short      mLoopCounter; // Current tone loopback count
    int                 mSamplingRate;  // AudioFlinger Sampling rate
    sp<AudioTrack>      mpAudioTrack;  // Pointer to audio track used for playback
    Mutex               mLock;          // Mutex to control concurent access to ToneGenerator object from audio callback and application API
    Mutex               mCbkCondLock; // Mutex associated to mWaitCbkCond
    Condition           mWaitCbkCond; // condition enabling interface to wait for audio callback completion after a change is requested
    float               mVolume;  // Volume applied to audio track
    audio_stream_type_t mStreamType; // Audio stream used for output
    unsigned int        mProcessSize;  // Size of audio blocks generated at a time by audioCallback() (in PCM frames).
    bool                initAudioTrack();
    static void         audioCallback(int event, void* user, void *info);
    bool                prepareWave();
    unsigned int        numWaves(unsigned int segmentIdx);
    void                clearWaveGens();
    bool                mThreadCanCallJava;
};
	
};   //namespace android

#endif   //ANDROID_VIBSPK_AUDIOPLAYER_H

