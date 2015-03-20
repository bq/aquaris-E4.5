
#ifndef ANDROID_FM_AUDIOPLAYER_H
#define ANDROID_FM_AUDIOPLAYER_H


#include <utils/threads.h>

#include <media/MediaPlayerInterface.h>
#include <media/AudioTrack.h>
#include <media/AudioRecord.h>
//#ifdef MTK_5192_FM_LINEIN
#include "media/IATVCtrlService.h"
//#endif

namespace android
{

class FMAudioPlayer : public MediaPlayerInterface
{
public:
    FMAudioPlayer();
    ~FMAudioPlayer();

    virtual void        onFirstRef();
    virtual status_t    initCheck();
    virtual status_t    setDataSource(const char *path, const KeyedVector<String8, String8> *headers);
    virtual status_t    setDataSource(int fd, int64_t offset, int64_t length);
    virtual status_t    setVideoSurface(const sp<Surface>& surface)
    {
        return UNKNOWN_ERROR;
    }
    virtual status_t    setVideoSurfaceTexture(
        const sp<IGraphicBufferProducer>& bufferProducer)
    {
        return UNKNOWN_ERROR;
    }
    virtual status_t    prepare();
    virtual status_t    prepareAsync();
    virtual status_t    start();
    virtual status_t    stop();
    virtual status_t    seekTo(int msec);
    virtual status_t    pause();
    virtual bool        isPlaying();
    virtual status_t    getCurrentPosition(int *msec);
    virtual status_t    getDuration(int *msec);
    virtual status_t    release();
    virtual status_t    reset();
    virtual status_t    setLooping(int loop);
#ifndef FAKE_FM
    virtual status_t    setRender(bool enable);
#endif
    virtual player_type playerType()
    {
        return FM_AUDIO_PLAYER;
    }
    virtual status_t    invoke(const Parcel &request, Parcel *reply)
    {
        return INVALID_OPERATION;
    }
    virtual status_t    setParameter(int key, const Parcel &request)
    {
        return INVALID_OPERATION;
    }
    virtual status_t    getParameter(int key, Parcel *reply)
    {
        return INVALID_OPERATION;
    }

private:
    status_t    setdatasource(const char *path, int fd, int64_t offset, int64_t length);
    status_t    reset_nosync();
    status_t    createOutputTrack();
    static int  renderThread(void *);
	int         render();
	bool        createAudioRecord();
	bool        deleteAudioRecord();

#ifndef FAKE_FM
    void        setHwCallback(bool enable);
#endif

	sp<AudioRecord>      mAudioRecord;
    Mutex               mMutex;
    Condition           mCondition;
    FILE               *mFile;
    int64_t             mOffset;
    int64_t             mLength;
    char               *mAudioBuffer;
    char               *mDummyBuffer;
    int                 mPlayTime;
    int                 mDuration;
    uint32_t            mFmAudioSamplingRate;

    status_t            mState;
    int                 mStreamType;
    bool                mAndroidLoop;
    volatile bool       mExit;
    bool                mPaused;
		
	bool				mSetRender;
    volatile bool       mRender;
    pid_t               mRenderTid;
	bool 				flagRecordError;

    int mMutePause;

    //#ifdef MTK_5192_FM_LINEIN
    // Keeping pointer to ATVCtrlService
    sp<IATVCtrlService> spATVCtrlService;
    //#endif

};

}; // namespace android

#endif




