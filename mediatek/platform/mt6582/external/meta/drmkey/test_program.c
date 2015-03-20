
#include <cutils/log.h>
#include <tz_cross/keyblock.h>

int main(void)
{
    DRMKeyID id = WIDEVINE_ID;
    int ret = 0;
    unsigned char* enckbdrm = NULL;
    unsigned int inlength = 0;
    unsigned char* kbdrm = NULL;
    unsigned int outlength = 0;
    for(; id<DRM_KEY_MAX; id++) 
    {
        ret = get_encrypt_drmkey(id,&enckbdrm,&inlength);
        if(ret!=0) {
            printf("KeyID:%d get fail!\n",id);
            continue;
        }
        
        /*don't call decrypt_drmkey and free_drmkey while TEE is enable*/
        ret = decrypt_drmkey (enckbdrm,inlength, &kbdrm, &outlength);
        if(ret!=0) {
            printf("KeyID:%d decrypt fail!\n",id);
            continue;
        }
        free_drmkey (kbdrm);
        /*don't call decrypt_drmkey and free_drmkey while TEE is enable*/
        
        free_encrypt_drmkey (enckbdrm);
    }
    return 0;
}

