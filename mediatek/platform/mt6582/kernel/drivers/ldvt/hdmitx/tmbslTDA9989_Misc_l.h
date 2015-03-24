/*
 * (c)  Koninklijke Philips Electronics N.V. 2002
 */



#ifndef TMBSLTDA9989_MISC_L_H
#define TMBSLTDA9989_MISC_L_H

/*============================================================================*/
/*                       INCLUDE FILES                                        */
/*============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*                       MACRO DEFINITIONS                                    */
/*============================================================================*/


/*============================================================================*/
/*                       ENUM OR TYPE DEFINITIONS                             */
/*============================================================================*/

/*============================================================================*/
/*                       EXTERN DATA DEFINITION                               */
/*============================================================================*/

#ifdef TMFL_TDA9989_PIXEL_CLOCK_ON_DDC
extern    CONST_DAT UInt8 kndiv_im[];
extern    CONST_DAT UInt8 kclk_div[];
#endif /* TMFL_TDA9989_PIXEL_CLOCK_ON_DDC */


/*============================================================================*/
/*                       EXTERN FUNCTION PROTOTYPES                           */
/*============================================================================*/

tmErrorCode_t hotPlugRestore ( tmUnitSelect_t  txUnit );

#ifdef TMFL_TDA9989_PIXEL_CLOCK_ON_DDC
tmErrorCode_t hotPlugRestore ( tmUnitSelect_t  txUnit );
#endif /* TMFL_TDA9989_PIXEL_CLOCK_ON_DDC */

#ifdef __cplusplus
}
#endif

#endif /* TMBSLTDA9989_MISC_L_H */
/*============================================================================*/
/*                            END OF FILE                                     */
/*============================================================================*/




