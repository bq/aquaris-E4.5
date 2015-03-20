
#include "sec_platform.h"
#include "sec_rom_info.h"
#include "sec_key.h"
#include "sec_error.h"

/**************************************************************************
*  MACRO
**************************************************************************/ 
#define MOD                             "SEC_K"

/**************************************************************************
*  DEFINITION
**************************************************************************/ 
#define SBC_PUBK_SEARCH_LEN             (0x300)  

/******************************************************************************
 * DEBUG
 ******************************************************************************/
#define SMSG                            dbg_print

/**************************************************************************
 *  GLOBAL VARIABLES
 **************************************************************************/
RSA_PUBK                                g_SBC_PUBK;

/**************************************************************************
 *  LOCAL VARIABLES
 **************************************************************************/

/**************************************************************************
 *  EXTERNAL VARIABLES
 **************************************************************************/
extern AND_ROMINFO_T  g_ROM_INFO;

#if 0
/**************************************************************************
 *  UTILITY
 **************************************************************************/
void dump_buf(U8* buf, U32 len)
{
    U32 i = 0;

    for (i =1; i <len+1; i++)
    {                
        SMSG("0x%x,",buf[i-1]);
        
        if(0 == (i%8))
            SMSG("\n");
    }

    if(0 != (len%8))
        SMSG("\n");    
}
#endif

/**************************************************************************
 *  INTERNAL FUNCTIONS
 **************************************************************************/
void sec_key_init (void)
{   
    U32 addr = 0;
    U32 ret = 0;
    U32 e_key_len = 2;

    /* ------------------ */
    /* check key length   */
    /* ------------------ */    
    COMPILE_ASSERT(AND_SEC_KEY_SIZE == sizeof(AND_SECKEY_T));          
    SMSG("[%s] SML KEY AC = %d\n",MOD,g_ROM_INFO.m_SEC_CTRL.m_sml_aes_key_ac_en);

    /* ------------------ */
    /* read sbc key       */
    /* ------------------ */    

    /* specify read address */
    addr = &g_ROM_INFO;
    addr = addr & 0xFFFFF000;
    addr = addr - SBC_PUBK_SEARCH_LEN;
    //SMSG("[%s] Search Addr '0x%x'\n",MOD,addr);
    
    ret = seclib_read_sbc_key (addr, SBC_PUBK_SEARCH_LEN, &g_SBC_PUBK);
    if(SEC_OK == ret)
    {        
        SMSG("[%s] SBC_PUBK Found\n",MOD);
        //SMSG("[%s] E_KEY :\n",MOD);    
        /* remove random data */
        memset(g_SBC_PUBK.E_Key + e_key_len, 0x0, sizeof(g_SBC_PUBK.E_Key) - 2*e_key_len);  
        //dump_buf(g_SBC_PUBK.E_Key,sizeof(g_SBC_PUBK.E_Key));
        //SMSG("[%s] N_KEY :\n",MOD);    
        //dump_buf(g_SBC_PUBK.N_Key,sizeof(g_SBC_PUBK.N_Key));        
    }
    else
    {
        SMSG("[%s] SBC_PUBK Not Found '0x%x'\n",MOD,ret);    
    }
    
}




