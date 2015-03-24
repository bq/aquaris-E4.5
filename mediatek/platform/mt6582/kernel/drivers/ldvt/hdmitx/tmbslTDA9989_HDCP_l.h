/*
 * (c) NXP N.V. 2006
 */



#ifndef TMBSLTDA9989_HDCP_L_H
#define TMBSLTDA9989_HDCP_L_H

/*============================================================================*/
/*                       INCLUDE FILES                                        */
/*============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __LINUX_ARM_ARCH__

#define HDCP_F1 { \
   int rej_f2(tmHdmiTxobject_t *pDis);          \
   regVal = rej_f2(pDis);                       \
   }

#define HDCP_F2 { \
   tmErrorCode_t rej_f1(tmHdmiTxobject_t *pDis);    \
   err = rej_f1(pDis);                                  \
   RETIF(err != TM_OK, err);                            \
}

#define HDCP_F3 { \
   if (fInterruptStatus & (1 << HDMITX_CALLBACK_INT_R0)) \
      {                                                  \
         tmErrorCode_t rej_f3(tmUnitSelect_t txUnit);    \
         err = rej_f3(txUnit);                           \
         RETIF(err != TM_OK, err);                       \
      }                                                  \
}

#else

#define HDCP_F1 {regVal = 0;}

#define HDCP_F2 {}

#define HDCP_F3 {}

#endif /*TMFL_HDCP_SUPPORT*/

#ifdef __cplusplus
}
#endif

#endif /* TMBSLTDA9989_HDCP_L_H */
/*============================================================================*/
/*                            END OF FILE                                     */
/*============================================================================*/




