#ifndef ES_REG_H
#define ES_REG_H



/******************************************************************************
 * CONSTANT DEFINITIONS                                                       
 ******************************************************************************/
#define INCORRECT_REG_INDEX          0xFFFFFFFF    /* incorrect register index */


/******************************************************************************
 * TYPE DEFINITIONS                                                            
******************************************************************************/
typedef enum
{   
    E_SPARE0 = 1,
    E_SPARE1,
    E_SPARE2,
    E_HW_RES0,
    E_HW_RES1,
    E_HW_RES2,
    E_MAX
} REG_INDEX;

typedef enum
{   
    MASK_R_SPARE0_ADC_CALI_EN = 0,
    MASK_R_SPARE0_DEGC_CALI,        
    MASK_R_SPARE0_O_SLOPE_SIGN,      
    MASK_R_SPARE0_O_VGS,
    MASK_R_SPARE0_O_VTS,
    MASK_R_SPARE0_O_SLOPE,
    MASK_R_SPARE1_ADC_OE,
    MASK_R_SPARE1_NON_ANALOG_PART,
    MASK_R_SPARE2_ADC_GE,
    MASK_R_SPARE2_NON_ANALOG_PART,
    MASK_R_HW_RES0_AUXADC_DAT,
    MASK_R_HW_RES0_NON_ANALOG_PART,
    MASK_R_HW_RES1_MIPI_DAT0,
    MASK_R_HW_RES1_MIPI_DAT1,
    MASK_R_HW_RES1_MIPI_DAT2,
    MASK_R_HW_RES1_MIPI_DAT3,
    MASK_R_HW_RES1_VDAC_EN,
    MASK_R_HW_RES1_VDAC_TRIM_VAL,
    MASK_R_HW_RES1_VDAC_DAT_REV,
    MASK_R_HW_RES2_BGR_CTRL,
    MASK_R_HW_RES2_BGR_RSEL,
    MASK_R_HW_RES2_BGR_CTRL_EN,
    MASK_R_HW_RES2_BGR_RSEL_EN,
    MASK_R_HW_RES2_NON_ANALOG_PART,
} REG_MASK_INDEX;


/******************************************************************************
 * EXPORT FUNCTION
 ******************************************************************************/
extern unsigned int seclib_get_param(REG_INDEX reg_index, REG_MASK_INDEX mask);


#endif /* ES_REG_H*/



