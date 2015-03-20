/*******************************************************************************
 *
 * Filename:
 * ---------
 *  auddrv_btcvsd_ioctl.h
 *
 * Project:
 * --------
 *   Android Audio BTCVSD Driver
 *
 * Description:
 * ------------
 *   ioctl control message
 *
 * Author:
 * -------
 *
 *
 *
 *------------------------------------------------------------------------------
 * $Revision$
 * $Modtime:$
 * $Log:$
 *
 * .
 *
 *
 *
 *******************************************************************************/
#ifndef _AUDDRV_BTCVSD_IOCTL_MSG_H
#define _AUDDRV_BTCVSD_IOCTL_MSG_H


/*****************************************************************************
*                     C O M P I L E R   F L A G S
******************************************************************************
*/


/*****************************************************************************
*                          C O N S T A N T S
******************************************************************************
*/

//below is control message
#define AUD_DRV_BTCVSD_IOC_MAGIC 'C'

#define ALLOCATE_FREE_BTCVSD_BUF _IOWR(AUD_DRV_BTCVSD_IOC_MAGIC, 0xE0, unsigned int)
#define SET_BTCVSD_STATE         _IOWR(AUD_DRV_BTCVSD_IOC_MAGIC, 0xE1, unsigned int)
#define GET_BTCVSD_STATE         _IOWR(AUD_DRV_BTCVSD_IOC_MAGIC, 0xE2, unsigned int)



/*****************************************************************************
*                         D A T A   T Y P E S
******************************************************************************
*/

/*****************************************************************************
*                        F U N C T I O N   D E F I N I T I O N
******************************************************************************
*/


#endif

