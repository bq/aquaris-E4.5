
#ifndef _MTK_HAL_CAMADAPTER_INC_IMGBUFPROVIDERSMANAGER_H_
#define _MTK_HAL_CAMADAPTER_INC_IMGBUFPROVIDERSMANAGER_H_
//


namespace android {
/*******************************************************************************
*   Image Buffer Providers Manager
*******************************************************************************/
class ImgBufProvidersManager : public RefBase
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.
                                    ImgBufProvidersManager()
                                        : mRWLockPvdrs()
                                        , mvPvdrs()
                                    {
                                        mvPvdrs.setCapacity(IImgBufProvider::eID_TOTAL_NUM);
                                        for (uint_t i = 0; i < mvPvdrs.capacity(); i++)
                                        {
                                            mvPvdrs.push_back(NULL);
                                        }
                                    }

public:     ////                    Attributes.
    //
    inline  size_t                  getProvidersSize() const
                                    {
                                        return mvPvdrs.size();
                                    }
    //
    inline  void                    setProvider(int32_t const i4PvdrId, sp<IImgBufProvider>const& rpPvdr)
                                    {
                                        RWLock::AutoWLock _l(mRWLockPvdrs);
                                        mvPvdrs.editItemAt(i4PvdrId) = rpPvdr;
                                    }
    //
    inline sp<IImgBufProvider>      getProvider(int32_t const i4PvdrId) const
                                    {
                                        RWLock::AutoRLock _l(mRWLockPvdrs);
                                        return  mvPvdrs.itemAt(i4PvdrId);
                                    }

    inline String8                  queryFormat(int32_t const i4PvdrId) const
                                    {
                                        String8 s8Format = String8::empty();
                                        ImgBufQueNode node;
                                        sp<IImgBufProvider> pImgBufPvdr = getProvider(i4PvdrId);

                                        if  ( pImgBufPvdr != 0 && pImgBufPvdr->queryProvider(node) ) {
                                            s8Format = node.getImgBuf()->getImgFormat();
                                        }
                                        return  s8Format;
                                    }

public:     ////                    Operations.

    //  get [Display] Image Buffer Provider
    sp<IImgBufProvider>             getDisplayPvdr() const  { return getProvider(IImgBufProvider::eID_DISPLAY); }
    //  get [Record Callback] Image Buffer Provider
    sp<IImgBufProvider> const       getRecCBPvdr() const    { return getProvider(IImgBufProvider::eID_REC_CB); }
    //  get [Preview Callback] Image Buffer Provider
    sp<IImgBufProvider> const       getPrvCBPvdr() const    { return getProvider(IImgBufProvider::eID_PRV_CB); }
    //  get [Face Detection] Image Buffer Provider
    sp<IImgBufProvider> const       getFDBufPvdr() const    { return getProvider(IImgBufProvider::eID_FD); }
    //  get [genaric preview feature] Image Buffer Provider    
    sp<IImgBufProvider> const       getGenericBufPvdr()const{ return getProvider(IImgBufProvider::eID_GENERIC); }
    

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
private:    ////                    Data Members.
    mutable RWLock                  mRWLockPvdrs;
    Vector< sp<IImgBufProvider> >   mvPvdrs;

};


}; // namespace android
#endif  //_MTK_HAL_CAMADAPTER_INC_IMGBUFPROVIDERSMANAGER_H_



