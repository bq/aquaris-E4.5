
#ifndef TMBSLTDA9989_LOCAL_H
#define TMBSLTDA9989_LOCAL_H

/*============================================================================*/
/*                       INCLUDE FILES                                        */
/*============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*                       MACRO DEFINITIONS                                    */
/*============================================================================*/
#ifndef DUMMY_ACCESS
#ifdef _WINDOWS
#define DUMMY_ACCESS(x) x
#else
#define DUMMY_ACCESS(x)
#endif
#endif /* DUMMY_ACCESS */

/** \name Versions
 *  A group of macros to set the software component number and version
 */
/*@{*/
/** Compatibility number */
#define HDMITX_BSL_COMP_NUM             1

/** Major software version 1 to 255 */
#define HDMITX_BSL_MAJOR_VER            5

/** Minor software version 0 to 9 */
#define HDMITX_BSL_MINOR_VER            4
/*@}*/

/** \name ErrorChecks
 *  A group of checks ensuring that public error codes match DVP standard errors
 */
/*@{*/
/** SW interface compatibility error */
#if TMBSL_ERR_HDMI_COMPATIBILITY != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_COMPATIBILITY)
#error
#endif

/** SW major version error */
#if TMBSL_ERR_HDMI_MAJOR_VERSION != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_MAJOR_VERSION)
#error
#endif

/** SW component version error */
#if TMBSL_ERR_HDMI_COMP_VERSION != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_COMP_VERSION)
#error
#endif

/** Invalid device unit number */
#if TMBSL_ERR_HDMI_BAD_UNIT_NUMBER != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_BAD_UNIT_NUMBER)
#error
#endif

/** Invalid input parameter other than unit number */
#if TMBSL_ERR_HDMI_BAD_PARAMETER != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_BAD_PARAMETER)
#error
#endif

/** Inconsistent input parameters */
#if TMBSL_ERR_HDMI_INCONSISTENT_PARAMS != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_INCONSISTENT_PARAMS)
#error
#endif

/** Component is not initialized */
#if TMBSL_ERR_HDMI_NOT_INITIALIZED != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_NOT_INITIALIZED)
#error
#endif

/** Command not supported for current device */
#if TMBSL_ERR_HDMI_NOT_SUPPORTED != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_NOT_SUPPORTED)
#error
#endif

/** Initialization failed */
#if TMBSL_ERR_HDMI_INIT_FAILED != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_INIT_FAILED)
#error
#endif

/** Component is busy and cannot do a new operation */
#if TMBSL_ERR_HDMI_BUSY != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_BUSY)
#error
#endif

/** I2C read error */
#if TMBSL_ERR_HDMI_I2C_READ != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_READ)
#error
#endif

/** I2C write error */
#if TMBSL_ERR_HDMI_I2C_WRITE != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_WRITE)
#error
#endif

/** Assertion failure */
#if TMBSL_ERR_HDMI_ASSERTION != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_ASSERTION)
#error
#endif

/** Bad EDID block checksum */
#if TMBSL_ERR_HDMI_INVALID_CHECKSUM != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_INVALID_STATE)
#error
#endif

/** No connection to HPD pin */
#if TMBSL_ERR_HDMI_NULL_CONNECTION != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_NULL_CONNECTION)
#error
#endif

/** Not allowed in DVI mode */
#if TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_OPERATION_NOT_PERMITTED)
#error
#endif

/** Ressource not available*/
#if TMBSL_ERR_HDMI_RESOURCE_NOT_AVAILABLE != \
(TMBSL_ERR_HDMI_BASE + TM_ERR_NO_RESOURCES)
#error
#endif
 

/*@}*/

/**
 * A macro to check a condition and if true return a result
 */
#ifdef TMFL_LINUX_OS_KERNEL_DRIVER
#include <linux/kernel.h>
#define RETIF(cond, rslt)       if ((cond)){printk(KERN_INFO "%s %d\n",__func__,__LINE__);return (rslt);}
#else
#define RETIF(cond, rslt)       if ((cond)){return (rslt);}
#endif

/**
 * A macro to check a condition and if true return 
 * TMBSL_ERR_HDMI_BAD_PARAMETER.
 * To save code space, it can be compiled out by defining NO_RETIF_BADPARAM on
 * the compiler command line.
 */
#ifdef NO_RETIF_BADPARAM
#define RETIF_BADPARAM(cond)
#else
#define RETIF_BADPARAM(cond)  if ((cond)){return TMBSL_ERR_HDMI_BAD_PARAMETER;}
#endif

/**
 * A macro to check the result of a register API and if not TM_OK to return it.
 * To save code space, it can be compiled out by defining NO_RETIF_REG_FAIL on
 * the compiler command line.
 */
#ifdef NO_RETIF_REG_FAIL
#define RETIF_REG_FAIL(result)
#else
#define RETIF_REG_FAIL(result)  if ((result) != TM_OK){return (result);}
#endif

/**
 * Check the consistancy of BSL structure
 */
/* #define BSL_CONSISTANCY(_x_,_y_) do { \ */
/* #if ((_x_) != (_y_) * (_x_)[0])    \ */
/* #error "BSL HAS WRONG ARRAY SIZE"  \ */
/* #endif \ */
/*    } while (0) */


/*============================================================================*/
/*                       ENUM OR TYPE DEFINITIONS                             */
/*============================================================================*/

/**
 * Driver events and states used for diagnosis
 */
typedef enum _tmbslTDA9989Event
{
    EV_DEINIT               = 0,
    EV_INIT                 = 1,
    EV_STANDBY              = 2,
    EV_SLEEP                = 3,
    EV_ON                   = 4,
    EV_UNPLUGGED            = 5,
    EV_PLUGGEDIN            = 6,
    EV_RESUME_UNPLUGGED     = 7,
    EV_RESUME_PLUGGEDIN     = 8,
    EV_GETBLOCKDATA         = 9,
    EV_SETINOUT             = 10,
    EV_OUTDISABLE           = 11,
    EV_HDCP_RUN             = 12,
    EV_HDCP_BKSV_NREPEAT    = 13,
    EV_HDCP_BKSV_NSECURE    = 14,
    EV_HDCP_BKSV_REPEAT     = 15,
    EV_HDCP_BSTATUS_GOOD    = 16,
    EV_HDCP_KSV_SECURE      = 17,
    EV_HDCP_T0              = 18,
    EV_HDCP_STOP            = 19,
    EV_SINKON               = 20,
    EV_SINKOFF              = 21,
    EV_INVALID              = 22
} tmbslTDA9989Event_t;

typedef enum _tmbslTDA9989State
{
    ST_UNINITIALIZED        = 0,
    ST_STANDBY              = 1,
    ST_SLEEP                = 2,
    ST_DISCONNECTED         = 3,
    ST_AWAIT_EDID           = 4,
    ST_SINK_CONNECTED       = 5,
    ST_VIDEO_NO_HDCP        = 6,
    ST_HDCP_WAIT_RX         = 7,
    ST_HDCP_WAIT_BSTATUS    = 8,
    ST_HDCP_WAIT_SHA_1      = 9,
    ST_HDCP_AUTHENTICATED   = 10,
    ST_AWAIT_RX_SENSE       = 11,
    ST_INVALID              = 12,
    ST_NUM                  = 13
} tmbslTDA9989State_t;

/**
 * An enum to index into the Device Instance Data shadowReg array
 */
enum _eShad
{
    E_SP00_INT_FLAGS_0  = 0,
    E_SP00_INT_FLAGS_1  = 1,
    E_SP00_INT_FLAGS_2  = 2,
    E_SP00_VIP_CNTRL_0  = 3,
    E_SP00_VIP_CNTRL_1  = 4,
    E_SP00_VIP_CNTRL_2  = 5,
    E_SP00_VIP_CNTRL_3  = 6,
    E_SP00_VIP_CNTRL_4  = 7,
    E_SP00_VIP_CNTRL_5  = 8,
    E_SP00_MAT_CONTRL   = 9,
    E_SP00_TBG_CNTRL_0  = 10,
    E_SP00_TBG_CNTRL_1  = 11,
    E_SP00_HVF_CNTRL_0  = 12,
    E_SP00_HVF_CNTRL_1  = 13,
    E_SP00_TIMER_H      = 14,
    E_SP00_DEBUG_PROBE  = 15,
    E_SP00_AIP_CLKSEL   = 16,
    E_SP01_SC_VIDFORMAT = 17,
    E_SP01_SC_CNTRL     = 18,
    E_SP01_TBG_CNTRL_0  = 19,
#ifdef TMFL_HDCP_SUPPORT
    E_SP12_CTRL    = 20,
    E_SP12_BCAPS   = 21,
    E_SNUM              = 22,   /* Number of shadow registers */
    E_SNONE             = 22    /* Index value indicating no shadow register */
#else /* TMFL_HDCP_SUPPORT */
    E_SNUM              = 20,   /* Number of shadow registers */
    E_SNONE             = 20    /* Index value indicating no shadow register */
#endif /* TMFL_HDCP_SUPPORT */
};

/**
 * Page list
 * These are indexes to the allowed register page numbers
 */
enum _ePage
{
    E_PAGE_00      = 0,
    E_PAGE_01      = 1,
    E_PAGE_02      = 2,
    E_PAGE_09      = 3,
    E_PAGE_10      = 4,
    E_PAGE_11      = 5,
    E_PAGE_12      = 6,
    E_PAGE_13      = 7,
    E_PAGE_NUM     = 8,         /* Number of pages */
    E_PAGE_INVALID = 8          /* Index value indicating invalid page */
};

/**
 * Macros to initialize and access the following register list enum _eReg
 */
/* Pack shadow index s, page index p and register address a into UInt16 */
#define SPA(s,p,a)       (UInt16)(((s)<<11)|((p)<<8)|(a))
/* Unpacks shadow index s from UInt16 */
#define SPA2SHAD(spa)    (UInt8)((spa)>>11)
/* Unpacks page index p from UInt16 */
#define SPA2PAGE(spa)    (UInt8)(((spa)>>8)&0x0007)
/* Unpacks register address a from UInt16 */
#define SPA2ADDR(spa)    (UInt8)((spa)&0x00FF)

/**
 * Register list
 *
 * Each register symbol has these fields: E_REG_page_register_access
 *
 * The symbols have a 16-bit value as follows, including an index to
 * the Device Instance Data shadowReg[] array:
 *
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 * |15 |14 |13 |12 |11 |10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 * |  Shadow Index     |Page Index |       Register Address        |
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *
 */
enum _eReg
{
    /*************************************************************************/
    /** Rows formatted in "HDMI Driver - Register List.xls" and pasted here **/
    /*************************************************************************/
    E_REG_MIN_ADR                    = 0x00, /* First register on all pages   */
    E_REG_CURPAGE_ADR_W              = 0xFF, /* Address register on all pages */
    /*CEC Registers*/

    E_REG_CEC_INTERRUPTSTATUS_R       = 0xEE,
    E_REG_CEC_RXSHPDINTENA_RW         = 0xFC,
    E_REG_CEC_RXSHPDINT_R             = 0xFD,
    E_REG_CEC_RXSHPDLEV_R             = 0xFE,
    E_REG_CEC_ENAMODS_RW              = 0xFF,
    E_REG_CEC_FRO_IM_CLK_CTRL_RW      = 0xFB,

    E_REG_P00_VERSION_R              = SPA(E_SNONE            , E_PAGE_00, 0x00),
    E_REG_P00_MAIN_CNTRL0_RW         = SPA(E_SNONE            , E_PAGE_00, 0x01),
    E_REG_P00_VERSION_MSB_RW         = SPA(E_SNONE            , E_PAGE_00, 0x02),
    E_REG_P00_PACKAGE_TYPE_R         = SPA(E_SNONE            , E_PAGE_00, 0x03),
    E_REG_P00_SR_REG_W               = SPA(E_SNONE            , E_PAGE_00, 0x0A),
    E_REG_P00_DDC_DISABLE_RW         = SPA(E_SNONE            , E_PAGE_00, 0x0B),
    E_REG_P00_CCLK_ON_RW             = SPA(E_SNONE            , E_PAGE_00, 0x0C),
    E_REG_P00_I2C_MASTER_RW          = SPA(E_SNONE            , E_PAGE_00, 0x0D),
#ifdef TMFL_HDCP_OPTIMIZED_POWER
    E_REG_FEAT_POWER_DOWN            = SPA(E_SNONE            , E_PAGE_00, 0x0E),
#endif

    E_REG_P00_INT_FLAGS_0_RW         = SPA(E_SP00_INT_FLAGS_0 , E_PAGE_00, 0x0F),
    E_REG_P00_INT_FLAGS_1_RW         = SPA(E_SP00_INT_FLAGS_1 , E_PAGE_00, 0x10),
    E_REG_P00_INT_FLAGS_2_RW         = SPA(E_SP00_INT_FLAGS_2 , E_PAGE_00, 0x11),
    /*E_REG_P00_INT_FLAGS_3_R          = SPA(E_SNONE            , E_PAGE_00, 0x12),*/
    E_REG_P00_SW_INT_W               = SPA(E_SNONE            , E_PAGE_00, 0x15),
    E_REG_P00_ENA_ACLK_RW            = SPA(E_SNONE            , E_PAGE_00, 0x16),
    E_REG_P00_ENA_VP_0_RW            = SPA(E_SNONE            , E_PAGE_00, 0x18),
    E_REG_P00_ENA_VP_1_RW            = SPA(E_SNONE            , E_PAGE_00, 0x19),
    E_REG_P00_ENA_VP_2_RW            = SPA(E_SNONE            , E_PAGE_00, 0x1A),
    E_REG_P00_ENA_AP_RW              = SPA(E_SNONE            , E_PAGE_00, 0x1E),
    E_REG_P00_VIP_CNTRL_0_W          = SPA(E_SP00_VIP_CNTRL_0 , E_PAGE_00, 0x20),
    E_REG_P00_VIP_CNTRL_1_W          = SPA(E_SP00_VIP_CNTRL_1 , E_PAGE_00, 0x21),
    E_REG_P00_VIP_CNTRL_2_W          = SPA(E_SP00_VIP_CNTRL_2 , E_PAGE_00, 0x22),
    E_REG_P00_VIP_CNTRL_3_W          = SPA(E_SP00_VIP_CNTRL_3 , E_PAGE_00, 0x23),
    E_REG_P00_VIP_CNTRL_4_W          = SPA(E_SP00_VIP_CNTRL_4 , E_PAGE_00, 0x24),
    E_REG_P00_VIP_CNTRL_5_W          = SPA(E_SP00_VIP_CNTRL_5 , E_PAGE_00, 0x25),
    E_REG_P00_MUX_AP_RW              = SPA(E_SNONE            , E_PAGE_00, 0x26),
    E_REG_P00_MUX_VP_VIP_OUT_RW      = SPA(E_SNONE            , E_PAGE_00, 0x27),
    E_REG_P00_MAT_CONTRL_W           = SPA(E_SP00_MAT_CONTRL  , E_PAGE_00, 0x80),
    E_REG_P00_MAT_OI1_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x81),
    E_REG_P00_MAT_OI1_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x82),
    E_REG_P00_MAT_OI2_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x83),
    E_REG_P00_MAT_OI2_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x84),
    E_REG_P00_MAT_OI3_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x85),
    E_REG_P00_MAT_OI3_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x86),
    E_REG_P00_MAT_P11_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x87),
    E_REG_P00_MAT_P11_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x88),
    E_REG_P00_MAT_P12_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x89),
    E_REG_P00_MAT_P12_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x8A),
    E_REG_P00_MAT_P13_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x8B),
    E_REG_P00_MAT_P13_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x8C),
    E_REG_P00_MAT_P21_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x8D),
    E_REG_P00_MAT_P21_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x8E),
    E_REG_P00_MAT_P22_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x8F),
    E_REG_P00_MAT_P22_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x90),
    E_REG_P00_MAT_P23_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x91),
    E_REG_P00_MAT_P23_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x92),
    E_REG_P00_MAT_P31_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x93),
    E_REG_P00_MAT_P31_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x94),
    E_REG_P00_MAT_P32_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x95),
    E_REG_P00_MAT_P32_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x96),
    E_REG_P00_MAT_P33_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x97),
    E_REG_P00_MAT_P33_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x98),
    E_REG_P00_MAT_OO1_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x99),
    E_REG_P00_MAT_OO1_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x9A),   
    E_REG_P00_MAT_OO2_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x9B),
    E_REG_P00_MAT_OO2_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x9C),
    E_REG_P00_MAT_OO3_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x9D),
    E_REG_P00_MAT_OO3_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0x9E),
    E_REG_P00_VIDFORMAT_W            = SPA(E_SNONE            , E_PAGE_00, 0xA0),
    E_REG_P00_REFPIX_MSB_W           = SPA(E_SNONE            , E_PAGE_00, 0xA1),
    E_REG_P00_REFPIX_LSB_W           = SPA(E_SNONE            , E_PAGE_00, 0xA2),
    E_REG_P00_REFLINE_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0xA3),
    E_REG_P00_REFLINE_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0xA4),
    E_REG_P00_NPIX_MSB_W             = SPA(E_SNONE            , E_PAGE_00, 0xA5),
    E_REG_P00_NPIX_LSB_W             = SPA(E_SNONE            , E_PAGE_00, 0xA6),
    E_REG_P00_NLINE_MSB_W            = SPA(E_SNONE            , E_PAGE_00, 0xA7),
    E_REG_P00_NLINE_LSB_W            = SPA(E_SNONE            , E_PAGE_00, 0xA8),
    E_REG_P00_VS_LINE_STRT_1_MSB_W   = SPA(E_SNONE            , E_PAGE_00, 0xA9),
    E_REG_P00_VS_LINE_STRT_1_LSB_W   = SPA(E_SNONE            , E_PAGE_00, 0xAA),
    E_REG_P00_VS_PIX_STRT_1_MSB_W    = SPA(E_SNONE            , E_PAGE_00, 0xAB),
    E_REG_P00_VS_PIX_STRT_1_LSB_W    = SPA(E_SNONE            , E_PAGE_00, 0xAC),
    E_REG_P00_VS_LINE_END_1_MSB_W    = SPA(E_SNONE            , E_PAGE_00, 0xAD),
    E_REG_P00_VS_LINE_END_1_LSB_W    = SPA(E_SNONE            , E_PAGE_00, 0xAE),
    E_REG_P00_VS_PIX_END_1_MSB_W     = SPA(E_SNONE            , E_PAGE_00, 0xAF),
    E_REG_P00_VS_PIX_END_1_LSB_W     = SPA(E_SNONE            , E_PAGE_00, 0xB0),
    E_REG_P00_VS_LINE_STRT_2_MSB_W   = SPA(E_SNONE            , E_PAGE_00, 0xB1),
    E_REG_P00_VS_LINE_STRT_2_LSB_W   = SPA(E_SNONE            , E_PAGE_00, 0xB2),
    E_REG_P00_VS_PIX_STRT_2_MSB_W    = SPA(E_SNONE            , E_PAGE_00, 0xB3),
    E_REG_P00_VS_PIX_STRT_2_LSB_W    = SPA(E_SNONE            , E_PAGE_00, 0xB4),
    E_REG_P00_VS_LINE_END_2_MSB_W    = SPA(E_SNONE            , E_PAGE_00, 0xB5),
    E_REG_P00_VS_LINE_END_2_LSB_W    = SPA(E_SNONE            , E_PAGE_00, 0xB6),
    E_REG_P00_VS_PIX_END_2_MSB_W     = SPA(E_SNONE            , E_PAGE_00, 0xB7),
    E_REG_P00_VS_PIX_END_2_LSB_W     = SPA(E_SNONE            , E_PAGE_00, 0xB8),
    E_REG_P00_HS_PIX_START_MSB_W     = SPA(E_SNONE            , E_PAGE_00, 0xB9),
    E_REG_P00_HS_PIX_START_LSB_W     = SPA(E_SNONE            , E_PAGE_00, 0xBA),
    E_REG_P00_HS_PIX_STOP_MSB_W      = SPA(E_SNONE            , E_PAGE_00, 0xBB),
    E_REG_P00_HS_PIX_STOP_LSB_W      = SPA(E_SNONE            , E_PAGE_00, 0xBC),
    E_REG_P00_VWIN_START_1_MSB_W     = SPA(E_SNONE            , E_PAGE_00, 0xBD),
    E_REG_P00_VWIN_START_1_LSB_W     = SPA(E_SNONE            , E_PAGE_00, 0xBE),
    E_REG_P00_VWIN_END_1_MSB_W       = SPA(E_SNONE            , E_PAGE_00, 0xBF),
    E_REG_P00_VWIN_END_1_LSB_W       = SPA(E_SNONE            , E_PAGE_00, 0xC0),
    E_REG_P00_VWIN_START_2_MSB_W     = SPA(E_SNONE            , E_PAGE_00, 0xC1),
    E_REG_P00_VWIN_START_2_LSB_W     = SPA(E_SNONE            , E_PAGE_00, 0xC2),
    E_REG_P00_VWIN_END_2_MSB_W       = SPA(E_SNONE            , E_PAGE_00, 0xC3),
    E_REG_P00_VWIN_END_2_LSB_W       = SPA(E_SNONE            , E_PAGE_00, 0xC4),
    E_REG_P00_DE_START_MSB_W         = SPA(E_SNONE            , E_PAGE_00, 0xC5),
    E_REG_P00_DE_START_LSB_W         = SPA(E_SNONE            , E_PAGE_00, 0xC6),
    E_REG_P00_DE_STOP_MSB_W          = SPA(E_SNONE            , E_PAGE_00, 0xC7),
    E_REG_P00_DE_STOP_LSB_W          = SPA(E_SNONE            , E_PAGE_00, 0xC8),
    E_REG_P00_COLBAR_WIDTH_W         = SPA(E_SNONE            , E_PAGE_00, 0xC9),
    E_REG_P00_TBG_CNTRL_0_W          = SPA(E_SP00_TBG_CNTRL_0 , E_PAGE_00, 0xCA),
    E_REG_P00_TBG_CNTRL_1_W          = SPA(E_SP00_TBG_CNTRL_1 , E_PAGE_00, 0xCB),
    E_REG_P00_VBL_OFFSET_START_W     = SPA(E_SNONE            , E_PAGE_00, 0xCC),
    E_REG_P00_VBL_OFFSET_END_W       = SPA(E_SNONE            , E_PAGE_00, 0xCD),
    E_REG_P00_HBL_OFFSET_START_W     = SPA(E_SNONE            , E_PAGE_00, 0xCE),
    E_REG_P00_HBL_OFFSET_END_W       = SPA(E_SNONE            , E_PAGE_00, 0xCF),
    E_REG_P00_DWIN_RE_DE_W           = SPA(E_SNONE            , E_PAGE_00, 0xD0),
    E_REG_P00_DWIN_FE_DE_W           = SPA(E_SNONE            , E_PAGE_00, 0xD1),
#ifdef TMFL_RGB_DDR_12BITS
    E_REG_P00_VSPACE_START_MSB_W     = SPA(E_SNONE            , E_PAGE_00, 0xD2),
    E_REG_P00_VSPACE_START_LSB_W     = SPA(E_SNONE            , E_PAGE_00, 0xD3),
    E_REG_P00_VSPACE_END_MSB_W       = SPA(E_SNONE            , E_PAGE_00, 0xD4),
    E_REG_P00_VSPACE_END_LSB_W       = SPA(E_SNONE            , E_PAGE_00, 0xD5),
    E_REG_P00_ENABLE_SPACE_W         = SPA(E_SNONE            , E_PAGE_00, 0xD6),
    E_REG_P00_VSPACE_Y_DATA_W        = SPA(E_SNONE            , E_PAGE_00, 0xD7),
    E_REG_P00_VSPACE_U_DATA_W        = SPA(E_SNONE            , E_PAGE_00, 0xD8),
    E_REG_P00_VSPACE_V_DATA_W        = SPA(E_SNONE            , E_PAGE_00, 0xD9),
#endif

    E_REG_P00_TIMER_RI_PJ_RW         = SPA(E_SNONE            , E_PAGE_00, 0xE1),
    E_REG_P00_BCAPS_POLL_RW          = SPA(E_SNONE            , E_PAGE_00, 0xE2),
    E_REG_P00_100us_RW               = SPA(E_SNONE            , E_PAGE_00, 0xE3),

    E_REG_P00_HVF_CNTRL_0_W          = SPA(E_SP00_HVF_CNTRL_0 , E_PAGE_00, 0xE4),
    E_REG_P00_HVF_CNTRL_1_W          = SPA(E_SP00_HVF_CNTRL_1 , E_PAGE_00, 0xE5),       

    E_REG_P00_TIMER_H_W              = SPA(E_SP00_TIMER_H     , E_PAGE_00, 0xE8),
    E_REG_P00_TIMER_M_W              = SPA(E_SNONE            , E_PAGE_00, 0xE9),
    E_REG_P00_TIMER_L_W              = SPA(E_SNONE            , E_PAGE_00, 0xEA),
    E_REG_P00_TIMER_2SEC_W           = SPA(E_SNONE            , E_PAGE_00, 0xEB),
    E_REG_P00_TIMER_5SEC_W           = SPA(E_SNONE            , E_PAGE_00, 0xEC),
    E_REG_P00_NDIV_IM_W              = SPA(E_SNONE            , E_PAGE_00, 0xEE),
    E_REG_P00_NDIV_PF_W              = SPA(E_SNONE            , E_PAGE_00, 0xEF),   
    E_REG_P00_RPT_CNTRL_W            = SPA(E_SNONE            , E_PAGE_00, 0xF0),
    E_REG_P00_LEAD_OFF_W             = SPA(E_SNONE            , E_PAGE_00, 0xF1),
    E_REG_P00_TRAIL_OFF_W            = SPA(E_SNONE            , E_PAGE_00, 0xF2),
    E_REG_P00_MISR_EXP_0_RW          = SPA(E_SNONE            , E_PAGE_00, 0xF3),
    E_REG_P00_MISR_EXP_1_RW          = SPA(E_SNONE            , E_PAGE_00, 0xF4),
    E_REG_P00_MISR_EXP_2_RW          = SPA(E_SNONE            , E_PAGE_00, 0xF5),
    E_REG_P00_MISR_0_R               = SPA(E_SNONE            , E_PAGE_00, 0xF6),
    E_REG_P00_MISR_1_R               = SPA(E_SNONE            , E_PAGE_00, 0xF7),                 
    E_REG_P00_DEBUG_PROBE_W          = SPA(E_SP00_DEBUG_PROBE , E_PAGE_00, 0xF8),
    E_REG_P00_GHOST_XADDR_W          = SPA(E_SNONE            , E_PAGE_00, 0xF9),
    E_REG_P00_MISR_2_R               = SPA(E_SNONE            , E_PAGE_00, 0xFA),
    E_REG_P00_I2S_FORMAT_RW          = SPA(E_SNONE            , E_PAGE_00, 0xFC),
    E_REG_P00_AIP_CLKSEL_W           = SPA(E_SP00_AIP_CLKSEL  , E_PAGE_00, 0xFD),
    E_REG_P00_GHOST_ADDR_W           = SPA(E_SNONE            , E_PAGE_00, 0xFE),
    E_REG_P01_SC_VIDFORMAT_W         = SPA(E_SP01_SC_VIDFORMAT, E_PAGE_01, 0x00),

    E_REG_P01_SC_CNTRL_W             = SPA(E_SP01_SC_CNTRL    , E_PAGE_01, 0x01),
    E_REG_P01_SC_DELTA_PHASE_V_W     = SPA(E_SNONE            , E_PAGE_01, 0x02),
    E_REG_P01_SC_DELTA_PHASE_H_W     = SPA(E_SNONE            , E_PAGE_01, 0x03),
    E_REG_P01_SC_START_PHASE_H_W     = SPA(E_SNONE            , E_PAGE_01, 0x04),
    E_REG_P01_SC_NPIX_IN_LSB_W       = SPA(E_SNONE            , E_PAGE_01, 0x05),
    E_REG_P01_SC_NPIX_IN_MSB_W       = SPA(E_SNONE            , E_PAGE_01, 0x06),
    E_REG_P01_SC_NPIX_OUT_LSB_W      = SPA(E_SNONE            , E_PAGE_01, 0x07),
    E_REG_P01_SC_NPIX_OUT_MSB_W      = SPA(E_SNONE            , E_PAGE_01, 0x08),
    E_REG_P01_SC_NLINE_IN_LSB_W      = SPA(E_SNONE            , E_PAGE_01, 0x09),
    E_REG_P01_SC_NLINE_IN_MSB_W      = SPA(E_SNONE            , E_PAGE_01, 0x0A),
    E_REG_P01_SC_NLINE_OUT_LSB_W     = SPA(E_SNONE            , E_PAGE_01, 0x0B),
    E_REG_P01_SC_NLINE_OUT_MSB_W     = SPA(E_SNONE            , E_PAGE_01, 0x0C),
    E_REG_P01_SC_NLINE_SKIP_W        = SPA(E_SNONE            , E_PAGE_01, 0x0D),   
    E_REG_P01_SC_SAMPLE_BUFFILL_R    = SPA(E_SNONE            , E_PAGE_01, 0x0E),
    E_REG_P01_SC_MAX_BUFFILL_P_0_R   = SPA(E_SNONE            , E_PAGE_01, 0x0F),
    E_REG_P01_SC_MAX_BUFFILL_P_1_R   = SPA(E_SNONE            , E_PAGE_01, 0x10),
    E_REG_P01_SC_MAX_BUFFILL_D_0_R   = SPA(E_SNONE            , E_PAGE_01, 0x11),
    E_REG_P01_SC_MAX_BUFFILL_D_1_R   = SPA(E_SNONE            , E_PAGE_01, 0x12),
    E_REG_P01_SC_SAMPLE_FIFOFILL_R   = SPA(E_SNONE            , E_PAGE_01, 0x13),
    E_REG_P01_SC_MAX_FIFOFILL_PI_R   = SPA(E_SNONE            , E_PAGE_01, 0x14),
    E_REG_P01_SC_MIN_FIFOFILL_PO1_R  = SPA(E_SNONE            , E_PAGE_01, 0x15),
    E_REG_P01_SC_MIN_FIFOFILL_PO2_R  = SPA(E_SNONE            , E_PAGE_01, 0x16),
    E_REG_P01_SC_MIN_FIFOFILL_PO3_R  = SPA(E_SNONE            , E_PAGE_01, 0x17),   
    E_REG_P01_SC_MIN_FIFOFILL_PO4_R  = SPA(E_SNONE            , E_PAGE_01, 0x18),
    E_REG_P01_SC_MAX_FIFOFILL_DI_R   = SPA(E_SNONE            , E_PAGE_01, 0x19),
    E_REG_P01_SC_MAX_FIFOFILL_DO_R   = SPA(E_SNONE            , E_PAGE_01, 0x1A),
    E_REG_P01_SC_VS_LUT_0_W          = SPA(E_SNONE            , E_PAGE_01, 0x1B),
    E_REG_P01_SC_VS_LUT_1_W          = SPA(E_SNONE            , E_PAGE_01, 0x1C),
    E_REG_P01_SC_VS_LUT_2_W          = SPA(E_SNONE            , E_PAGE_01, 0x1D),
    E_REG_P01_SC_VS_LUT_3_W          = SPA(E_SNONE            , E_PAGE_01, 0x1E),
    E_REG_P01_SC_VS_LUT_4_W          = SPA(E_SNONE            , E_PAGE_01, 0x1F),
    E_REG_P01_SC_VS_LUT_5_W          = SPA(E_SNONE            , E_PAGE_01, 0x20),
    E_REG_P01_SC_VS_LUT_6_W          = SPA(E_SNONE            , E_PAGE_01, 0x21),
    E_REG_P01_SC_VS_LUT_7_W          = SPA(E_SNONE            , E_PAGE_01, 0x22),
    E_REG_P01_SC_VS_LUT_8_W          = SPA(E_SNONE            , E_PAGE_01, 0x23),
    E_REG_P01_SC_VS_LUT_9_W          = SPA(E_SNONE            , E_PAGE_01, 0x24),
    E_REG_P01_SC_VS_LUT_10_W         = SPA(E_SNONE            , E_PAGE_01, 0x25),
    E_REG_P01_SC_VS_LUT_11_W         = SPA(E_SNONE            , E_PAGE_01, 0x26),
    E_REG_P01_SC_VS_LUT_12_W         = SPA(E_SNONE            , E_PAGE_01, 0x27),
    E_REG_P01_SC_VS_LUT_13_W         = SPA(E_SNONE            , E_PAGE_01, 0x28),
    E_REG_P01_SC_VS_LUT_14_W         = SPA(E_SNONE            , E_PAGE_01, 0x29),
    E_REG_P01_SC_VS_LUT_15_W         = SPA(E_SNONE            , E_PAGE_01, 0x2A),
    E_REG_P01_SC_VS_LUT_16_W         = SPA(E_SNONE            , E_PAGE_01, 0x2B),
    E_REG_P01_SC_VS_LUT_17_W         = SPA(E_SNONE            , E_PAGE_01, 0x2C),
    E_REG_P01_SC_VS_LUT_18_W         = SPA(E_SNONE            , E_PAGE_01, 0x2D),
    E_REG_P01_SC_VS_LUT_19_W         = SPA(E_SNONE            , E_PAGE_01, 0x2E),
    E_REG_P01_SC_VS_LUT_20_W         = SPA(E_SNONE            , E_PAGE_01, 0x2F),
    E_REG_P01_SC_VS_LUT_21_W         = SPA(E_SNONE            , E_PAGE_01, 0x30),
    E_REG_P01_SC_VS_LUT_22_W         = SPA(E_SNONE            , E_PAGE_01, 0x31),
    E_REG_P01_SC_VS_LUT_23_W         = SPA(E_SNONE            , E_PAGE_01, 0x32),
    E_REG_P01_SC_VS_LUT_24_W         = SPA(E_SNONE            , E_PAGE_01, 0x33),
    E_REG_P01_SC_VS_LUT_25_W         = SPA(E_SNONE            , E_PAGE_01, 0x34),
    E_REG_P01_SC_VS_LUT_26_W         = SPA(E_SNONE            , E_PAGE_01, 0x35),
    E_REG_P01_SC_VS_LUT_27_W         = SPA(E_SNONE            , E_PAGE_01, 0x36),
    E_REG_P01_SC_VS_LUT_28_W         = SPA(E_SNONE            , E_PAGE_01, 0x37),
    E_REG_P01_SC_VS_LUT_29_W         = SPA(E_SNONE            , E_PAGE_01, 0x38),
    E_REG_P01_SC_VS_LUT_30_W         = SPA(E_SNONE            , E_PAGE_01, 0x39),
    E_REG_P01_SC_VS_LUT_31_W         = SPA(E_SNONE            , E_PAGE_01, 0x3A),
    E_REG_P01_SC_VS_LUT_32_W         = SPA(E_SNONE            , E_PAGE_01, 0x3B),
    E_REG_P01_SC_VS_LUT_33_W         = SPA(E_SNONE            , E_PAGE_01, 0x3C),
    E_REG_P01_SC_VS_LUT_34_W         = SPA(E_SNONE            , E_PAGE_01, 0x3D),
    E_REG_P01_SC_VS_LUT_35_W         = SPA(E_SNONE            , E_PAGE_01, 0x3E),   
    E_REG_P01_SC_VS_LUT_36_W         = SPA(E_SNONE            , E_PAGE_01, 0x3F),
    E_REG_P01_SC_VS_LUT_37_W         = SPA(E_SNONE            , E_PAGE_01, 0x40),
    E_REG_P01_SC_VS_LUT_38_W         = SPA(E_SNONE            , E_PAGE_01, 0x41),
    E_REG_P01_SC_VS_LUT_39_W         = SPA(E_SNONE            , E_PAGE_01, 0x42),
    E_REG_P01_SC_VS_LUT_40_W         = SPA(E_SNONE            , E_PAGE_01, 0x43),
    E_REG_P01_SC_VS_LUT_41_W         = SPA(E_SNONE            , E_PAGE_01, 0x44),
    E_REG_P01_SC_VS_LUT_42_W         = SPA(E_SNONE            , E_PAGE_01, 0x45),
    E_REG_P01_SC_VS_LUT_43_W         = SPA(E_SNONE            , E_PAGE_01, 0x46),
    E_REG_P01_SC_VS_LUT_44_W         = SPA(E_SNONE            , E_PAGE_01, 0x47),
    E_REG_P01_SC_LAT_SCO_RW          = SPA(E_SNONE            , E_PAGE_01, 0x48),
    E_REG_P01_VIDFORMAT_W            = SPA(E_SNONE            , E_PAGE_01, 0xA0),
    E_REG_P01_REFPIX_MSB_W           = SPA(E_SNONE            , E_PAGE_01, 0xA1),   
    E_REG_P01_REFPIX_LSB_W           = SPA(E_SNONE            , E_PAGE_01, 0xA2),
    E_REG_P01_REFLINE_MSB_W          = SPA(E_SNONE            , E_PAGE_01, 0xA3),
    E_REG_P01_REFLINE_LSB_W          = SPA(E_SNONE            , E_PAGE_01, 0xA4),
    E_REG_P01_NPIX_MSB_W             = SPA(E_SNONE            , E_PAGE_01, 0xA5),
    E_REG_P01_NPIX_LSB_W             = SPA(E_SNONE            , E_PAGE_01, 0xA6),
    E_REG_P01_NLINE_MSB_W            = SPA(E_SNONE            , E_PAGE_01, 0xA7),
    E_REG_P01_NLINE_LSB_W            = SPA(E_SNONE            , E_PAGE_01, 0xA8),
    E_REG_P01_VWIN_START_1_MSB_W     = SPA(E_SNONE            , E_PAGE_01, 0xBD),
    E_REG_P01_VWIN_START_1_LSB_W     = SPA(E_SNONE            , E_PAGE_01, 0xBE),
    E_REG_P01_VWIN_END_1_MSB_W       = SPA(E_SNONE            , E_PAGE_01, 0xBF),
    E_REG_P01_VWIN_END_1_LSB_W       = SPA(E_SNONE            , E_PAGE_01, 0xC0),
    E_REG_P01_VWIN_START_2_MSB_W     = SPA(E_SNONE            , E_PAGE_01, 0xC1),
    E_REG_P01_VWIN_START_2_LSB_W     = SPA(E_SNONE            , E_PAGE_01, 0xC2),
    E_REG_P01_VWIN_END_2_MSB_W       = SPA(E_SNONE            , E_PAGE_01, 0xC3),
    E_REG_P01_VWIN_END_2_LSB_W       = SPA(E_SNONE            , E_PAGE_01, 0xC4),
    E_REG_P01_DE_START_MSB_W         = SPA(E_SNONE            , E_PAGE_01, 0xC5),
    E_REG_P01_DE_START_LSB_W         = SPA(E_SNONE            , E_PAGE_01, 0xC6),
    E_REG_P01_DE_STOP_MSB_W          = SPA(E_SNONE            , E_PAGE_01, 0xC7),
    E_REG_P01_DE_STOP_LSB_W          = SPA(E_SNONE            , E_PAGE_01, 0xC8),
    E_REG_P01_TBG_CNTRL_0_W          = SPA(E_SP01_TBG_CNTRL_0 , E_PAGE_01, 0xCA),
    E_REG_P02_PLL_SERIAL_1_RW        = SPA(E_SNONE            , E_PAGE_02, 0x00),
    E_REG_P02_PLL_SERIAL_2_RW        = SPA(E_SNONE            , E_PAGE_02, 0x01),
    E_REG_P02_PLL_SERIAL_3_RW        = SPA(E_SNONE            , E_PAGE_02, 0x02),
    E_REG_P02_SERIALIZER_RW          = SPA(E_SNONE            , E_PAGE_02, 0x03),
    E_REG_P02_BUFFER_OUT_RW          = SPA(E_SNONE            , E_PAGE_02, 0x04),
    E_REG_P02_PLL_SCG1_RW            = SPA(E_SNONE            , E_PAGE_02, 0x05),
    E_REG_P02_PLL_SCG2_RW            = SPA(E_SNONE            , E_PAGE_02, 0x06),
    E_REG_P02_PLL_SCGN1_RW           = SPA(E_SNONE            , E_PAGE_02, 0x07),
    E_REG_P02_PLL_SCGN2_RW           = SPA(E_SNONE            , E_PAGE_02, 0x08),
    E_REG_P02_PLL_SCGR1_RW           = SPA(E_SNONE            , E_PAGE_02, 0x09),
    E_REG_P02_PLL_SCGR2_RW           = SPA(E_SNONE            , E_PAGE_02, 0x0A),
    E_REG_P02_VAI_PLL_R              = SPA(E_SNONE            , E_PAGE_02, 0x0D),
    E_REG_P02_AUDIO_DIV_RW           = SPA(E_SNONE            , E_PAGE_02, 0x0E),
    E_REG_P02_TEST1_RW               = SPA(E_SNONE            , E_PAGE_02, 0x0F),
    /*E_REG_P02_TEST2_RW               = SPA(E_SNONE            , E_PAGE_02, 0x10),*/
    E_REG_P02_SEL_CLK_RW             = SPA(E_SNONE            , E_PAGE_02, 0x11),
    E_REG_P02_ANA_GENERAL_RW         = SPA(E_SNONE            , E_PAGE_02, 0x12),
    E_REG_P02_BUFFER_OUT2_RW         = SPA(E_SNONE            , E_PAGE_02, 0x13),
    E_REG_P02_SRL_TSTPAT0_RW         = SPA(E_SNONE            , E_PAGE_02, 0x14),
    E_REG_P02_SRL_TSTPAT1_RW         = SPA(E_SNONE            , E_PAGE_02, 0x15),
    E_REG_P02_SRL_TSTPAT2_RW         = SPA(E_SNONE            , E_PAGE_02, 0x16),
    E_REG_P02_SRL_TSTPAT3_RW         = SPA(E_SNONE            , E_PAGE_02, 0x17),

    E_REG_P09_EDID_DATA_0_R          = SPA(E_SNONE            , E_PAGE_09, 0x00),
    E_REG_P09_EDID_DATA_1_R          = SPA(E_SNONE            , E_PAGE_09, 0x01),
    E_REG_P09_EDID_DATA_2_R          = SPA(E_SNONE            , E_PAGE_09, 0x02),
    E_REG_P09_EDID_DATA_3_R          = SPA(E_SNONE            , E_PAGE_09, 0x03),
    E_REG_P09_EDID_DATA_4_R          = SPA(E_SNONE            , E_PAGE_09, 0x04),
    E_REG_P09_EDID_DATA_5_R          = SPA(E_SNONE            , E_PAGE_09, 0x05),
    E_REG_P09_EDID_DATA_6_R          = SPA(E_SNONE            , E_PAGE_09, 0x06),
    E_REG_P09_EDID_DATA_7_R          = SPA(E_SNONE            , E_PAGE_09, 0x07),
    E_REG_P09_EDID_DATA_8_R          = SPA(E_SNONE            , E_PAGE_09, 0x08),
    E_REG_P09_EDID_DATA_9_R          = SPA(E_SNONE            , E_PAGE_09, 0x09),
    E_REG_P09_EDID_DATA_10_R         = SPA(E_SNONE            , E_PAGE_09, 0x0A),
    E_REG_P09_EDID_DATA_11_R         = SPA(E_SNONE            , E_PAGE_09, 0x0B),
    E_REG_P09_EDID_DATA_12_R         = SPA(E_SNONE            , E_PAGE_09, 0x0C),
    E_REG_P09_EDID_DATA_13_R         = SPA(E_SNONE            , E_PAGE_09, 0x0D),
    E_REG_P09_EDID_DATA_14_R         = SPA(E_SNONE            , E_PAGE_09, 0x0E),
    E_REG_P09_EDID_DATA_15_R         = SPA(E_SNONE            , E_PAGE_09, 0x0F),
    E_REG_P09_EDID_DATA_16_R         = SPA(E_SNONE            , E_PAGE_09, 0x10),
    E_REG_P09_EDID_DATA_17_R         = SPA(E_SNONE            , E_PAGE_09, 0x11),
    E_REG_P09_EDID_DATA_18_R         = SPA(E_SNONE            , E_PAGE_09, 0x12),
    E_REG_P09_EDID_DATA_19_R         = SPA(E_SNONE            , E_PAGE_09, 0x13),
    E_REG_P09_EDID_DATA_20_R         = SPA(E_SNONE            , E_PAGE_09, 0x14),
    E_REG_P09_EDID_DATA_21_R         = SPA(E_SNONE            , E_PAGE_09, 0x15),
    E_REG_P09_EDID_DATA_22_R         = SPA(E_SNONE            , E_PAGE_09, 0x16),
    E_REG_P09_EDID_DATA_23_R         = SPA(E_SNONE            , E_PAGE_09, 0x17),
    E_REG_P09_EDID_DATA_24_R         = SPA(E_SNONE            , E_PAGE_09, 0x18),
    E_REG_P09_EDID_DATA_25_R         = SPA(E_SNONE            , E_PAGE_09, 0x19),
    E_REG_P09_EDID_DATA_26_R         = SPA(E_SNONE            , E_PAGE_09, 0x1A),
    E_REG_P09_EDID_DATA_27_R         = SPA(E_SNONE            , E_PAGE_09, 0x1B),
    E_REG_P09_EDID_DATA_28_R         = SPA(E_SNONE            , E_PAGE_09, 0x1C),
    E_REG_P09_EDID_DATA_29_R         = SPA(E_SNONE            , E_PAGE_09, 0x1D),
    E_REG_P09_EDID_DATA_30_R         = SPA(E_SNONE            , E_PAGE_09, 0x1E),
    E_REG_P09_EDID_DATA_31_R         = SPA(E_SNONE            , E_PAGE_09, 0x1F),
    E_REG_P09_EDID_DATA_32_R         = SPA(E_SNONE            , E_PAGE_09, 0x20),
    E_REG_P09_EDID_DATA_33_R         = SPA(E_SNONE            , E_PAGE_09, 0x21),
    E_REG_P09_EDID_DATA_34_R         = SPA(E_SNONE            , E_PAGE_09, 0x22),
    E_REG_P09_EDID_DATA_35_R         = SPA(E_SNONE            , E_PAGE_09, 0x23),
    E_REG_P09_EDID_DATA_36_R         = SPA(E_SNONE            , E_PAGE_09, 0x24),
    E_REG_P09_EDID_DATA_37_R         = SPA(E_SNONE            , E_PAGE_09, 0x25),
    E_REG_P09_EDID_DATA_38_R         = SPA(E_SNONE            , E_PAGE_09, 0x26),
    E_REG_P09_EDID_DATA_39_R         = SPA(E_SNONE            , E_PAGE_09, 0x27),
    E_REG_P09_EDID_DATA_40_R         = SPA(E_SNONE            , E_PAGE_09, 0x28),
    E_REG_P09_EDID_DATA_41_R         = SPA(E_SNONE            , E_PAGE_09, 0x29),
    E_REG_P09_EDID_DATA_42_R         = SPA(E_SNONE            , E_PAGE_09, 0x2A),
    E_REG_P09_EDID_DATA_43_R         = SPA(E_SNONE            , E_PAGE_09, 0x2B),
    E_REG_P09_EDID_DATA_44_R         = SPA(E_SNONE            , E_PAGE_09, 0x2C),   
    E_REG_P09_EDID_DATA_45_R         = SPA(E_SNONE            , E_PAGE_09, 0x2D),
    E_REG_P09_EDID_DATA_46_R         = SPA(E_SNONE            , E_PAGE_09, 0x2E),
    E_REG_P09_EDID_DATA_47_R         = SPA(E_SNONE            , E_PAGE_09, 0x2F),
    E_REG_P09_EDID_DATA_48_R         = SPA(E_SNONE            , E_PAGE_09, 0x30),
    E_REG_P09_EDID_DATA_49_R         = SPA(E_SNONE            , E_PAGE_09, 0x31),
    E_REG_P09_EDID_DATA_50_R         = SPA(E_SNONE            , E_PAGE_09, 0x32),
    E_REG_P09_EDID_DATA_51_R         = SPA(E_SNONE            , E_PAGE_09, 0x33),
    E_REG_P09_EDID_DATA_52_R         = SPA(E_SNONE            , E_PAGE_09, 0x34),
    E_REG_P09_EDID_DATA_53_R         = SPA(E_SNONE            , E_PAGE_09, 0x35),
    E_REG_P09_EDID_DATA_54_R         = SPA(E_SNONE            , E_PAGE_09, 0x36),
    E_REG_P09_EDID_DATA_55_R         = SPA(E_SNONE            , E_PAGE_09, 0x37),
    E_REG_P09_EDID_DATA_56_R         = SPA(E_SNONE            , E_PAGE_09, 0x38),
    E_REG_P09_EDID_DATA_57_R         = SPA(E_SNONE            , E_PAGE_09, 0x39),
    E_REG_P09_EDID_DATA_58_R         = SPA(E_SNONE            , E_PAGE_09, 0x3A),
    E_REG_P09_EDID_DATA_59_R         = SPA(E_SNONE            , E_PAGE_09, 0x3B),
    E_REG_P09_EDID_DATA_60_R         = SPA(E_SNONE            , E_PAGE_09, 0x3C),
    E_REG_P09_EDID_DATA_61_R         = SPA(E_SNONE            , E_PAGE_09, 0x3D),       
    E_REG_P09_EDID_DATA_62_R         = SPA(E_SNONE            , E_PAGE_09, 0x3E),
    E_REG_P09_EDID_DATA_63_R         = SPA(E_SNONE            , E_PAGE_09, 0x3F),
    E_REG_P09_EDID_DATA_64_R         = SPA(E_SNONE            , E_PAGE_09, 0x40),
    E_REG_P09_EDID_DATA_65_R         = SPA(E_SNONE            , E_PAGE_09, 0x41),
    E_REG_P09_EDID_DATA_66_R         = SPA(E_SNONE            , E_PAGE_09, 0x42),
    E_REG_P09_EDID_DATA_67_R         = SPA(E_SNONE            , E_PAGE_09, 0x43),
    E_REG_P09_EDID_DATA_68_R         = SPA(E_SNONE            , E_PAGE_09, 0x44),
    E_REG_P09_EDID_DATA_69_R         = SPA(E_SNONE            , E_PAGE_09, 0x45),
    E_REG_P09_EDID_DATA_70_R         = SPA(E_SNONE            , E_PAGE_09, 0x46),
    E_REG_P09_EDID_DATA_71_R         = SPA(E_SNONE            , E_PAGE_09, 0x47),
    E_REG_P09_EDID_DATA_72_R         = SPA(E_SNONE            , E_PAGE_09, 0x48),
    E_REG_P09_EDID_DATA_73_R         = SPA(E_SNONE            , E_PAGE_09, 0x49),
    E_REG_P09_EDID_DATA_74_R         = SPA(E_SNONE            , E_PAGE_09, 0x4A),
    E_REG_P09_EDID_DATA_75_R         = SPA(E_SNONE            , E_PAGE_09, 0x4B),
    E_REG_P09_EDID_DATA_76_R         = SPA(E_SNONE            , E_PAGE_09, 0x4C),
    E_REG_P09_EDID_DATA_77_R         = SPA(E_SNONE            , E_PAGE_09, 0x4D),
    E_REG_P09_EDID_DATA_78_R         = SPA(E_SNONE            , E_PAGE_09, 0x4E),
    E_REG_P09_EDID_DATA_79_R         = SPA(E_SNONE            , E_PAGE_09, 0x4F),
    E_REG_P09_EDID_DATA_80_R         = SPA(E_SNONE            , E_PAGE_09, 0x50),
    E_REG_P09_EDID_DATA_81_R         = SPA(E_SNONE            , E_PAGE_09, 0x51),
    E_REG_P09_EDID_DATA_82_R         = SPA(E_SNONE            , E_PAGE_09, 0x52),
    E_REG_P09_EDID_DATA_83_R         = SPA(E_SNONE            , E_PAGE_09, 0x53),
    E_REG_P09_EDID_DATA_84_R         = SPA(E_SNONE            , E_PAGE_09, 0x54),
    E_REG_P09_EDID_DATA_85_R         = SPA(E_SNONE            , E_PAGE_09, 0x55),   
    E_REG_P09_EDID_DATA_86_R         = SPA(E_SNONE            , E_PAGE_09, 0x56),
    E_REG_P09_EDID_DATA_87_R         = SPA(E_SNONE            , E_PAGE_09, 0x57),
    E_REG_P09_EDID_DATA_88_R         = SPA(E_SNONE            , E_PAGE_09, 0x58),
    E_REG_P09_EDID_DATA_89_R         = SPA(E_SNONE            , E_PAGE_09, 0x59),
    E_REG_P09_EDID_DATA_90_R         = SPA(E_SNONE            , E_PAGE_09, 0x5A),
    E_REG_P09_EDID_DATA_91_R         = SPA(E_SNONE            , E_PAGE_09, 0x5B),
    E_REG_P09_EDID_DATA_92_R         = SPA(E_SNONE            , E_PAGE_09, 0x5C),
    E_REG_P09_EDID_DATA_93_R         = SPA(E_SNONE            , E_PAGE_09, 0x5D),
    E_REG_P09_EDID_DATA_94_R         = SPA(E_SNONE            , E_PAGE_09, 0x5E),
    E_REG_P09_EDID_DATA_95_R         = SPA(E_SNONE            , E_PAGE_09, 0x5F),
    E_REG_P09_EDID_DATA_96_R         = SPA(E_SNONE            , E_PAGE_09, 0x60),
    E_REG_P09_EDID_DATA_97_R         = SPA(E_SNONE            , E_PAGE_09, 0x61),
    E_REG_P09_EDID_DATA_98_R         = SPA(E_SNONE            , E_PAGE_09, 0x62),
    E_REG_P09_EDID_DATA_99_R         = SPA(E_SNONE            , E_PAGE_09, 0x63),
    E_REG_P09_EDID_DATA_100_R        = SPA(E_SNONE            , E_PAGE_09, 0x64),
    E_REG_P09_EDID_DATA_101_R        = SPA(E_SNONE            , E_PAGE_09, 0x65),
    E_REG_P09_EDID_DATA_102_R        = SPA(E_SNONE            , E_PAGE_09, 0x66),
    E_REG_P09_EDID_DATA_103_R        = SPA(E_SNONE            , E_PAGE_09, 0x67),
    E_REG_P09_EDID_DATA_104_R        = SPA(E_SNONE            , E_PAGE_09, 0x68),
    E_REG_P09_EDID_DATA_105_R        = SPA(E_SNONE            , E_PAGE_09, 0x69),
    E_REG_P09_EDID_DATA_106_R        = SPA(E_SNONE            , E_PAGE_09, 0x6A),
    E_REG_P09_EDID_DATA_107_R        = SPA(E_SNONE            , E_PAGE_09, 0x6B),
    E_REG_P09_EDID_DATA_108_R        = SPA(E_SNONE            , E_PAGE_09, 0x6C),
    E_REG_P09_EDID_DATA_109_R        = SPA(E_SNONE            , E_PAGE_09, 0x6D),
    E_REG_P09_EDID_DATA_110_R        = SPA(E_SNONE            , E_PAGE_09, 0x6E),
    E_REG_P09_EDID_DATA_111_R        = SPA(E_SNONE            , E_PAGE_09, 0x6F),
    E_REG_P09_EDID_DATA_112_R        = SPA(E_SNONE            , E_PAGE_09, 0x70),
    E_REG_P09_EDID_DATA_113_R        = SPA(E_SNONE            , E_PAGE_09, 0x71),
    E_REG_P09_EDID_DATA_114_R        = SPA(E_SNONE            , E_PAGE_09, 0x72),
    E_REG_P09_EDID_DATA_115_R        = SPA(E_SNONE            , E_PAGE_09, 0x73),
    E_REG_P09_EDID_DATA_116_R        = SPA(E_SNONE            , E_PAGE_09, 0x74),
    E_REG_P09_EDID_DATA_117_R        = SPA(E_SNONE            , E_PAGE_09, 0x75),
    E_REG_P09_EDID_DATA_118_R        = SPA(E_SNONE            , E_PAGE_09, 0x76),
    E_REG_P09_EDID_DATA_119_R        = SPA(E_SNONE            , E_PAGE_09, 0x77),
    E_REG_P09_EDID_DATA_120_R        = SPA(E_SNONE            , E_PAGE_09, 0x78),
    E_REG_P09_EDID_DATA_121_R        = SPA(E_SNONE            , E_PAGE_09, 0x79),
    E_REG_P09_EDID_DATA_122_R        = SPA(E_SNONE            , E_PAGE_09, 0x7A),
    E_REG_P09_EDID_DATA_123_R        = SPA(E_SNONE            , E_PAGE_09, 0x7B),
    E_REG_P09_EDID_DATA_124_R        = SPA(E_SNONE            , E_PAGE_09, 0x7C),
    E_REG_P09_EDID_DATA_125_R        = SPA(E_SNONE            , E_PAGE_09, 0x7D),
    E_REG_P09_EDID_DATA_126_R        = SPA(E_SNONE            , E_PAGE_09, 0x7E),
    E_REG_P09_EDID_DATA_127_R        = SPA(E_SNONE            , E_PAGE_09, 0x7F),
    E_REG_P09_EDID_CTRL_RW           = SPA(E_SNONE            , E_PAGE_09, 0xFA),
    E_REG_P09_DDC_ADDR_RW            = SPA(E_SNONE            , E_PAGE_09, 0xFB),
    E_REG_P09_DDC_OFFS_RW            = SPA(E_SNONE            , E_PAGE_09, 0xFC),
    E_REG_P09_DDC_SEGM_ADDR_RW       = SPA(E_SNONE            , E_PAGE_09, 0xFD),
    E_REG_P09_DDC_SEGM_RW            = SPA(E_SNONE            , E_PAGE_09, 0xFE),   
    
    E_REG_P10_IF1_HB0_RW             = SPA(E_SNONE            , E_PAGE_10, 0x20),
    E_REG_P10_IF1_HB1_RW             = SPA(E_SNONE            , E_PAGE_10, 0x21),
    E_REG_P10_IF1_HB2_RW             = SPA(E_SNONE            , E_PAGE_10, 0x22),
    E_REG_P10_IF1_PB0_RW             = SPA(E_SNONE            , E_PAGE_10, 0x23),
    E_REG_P10_IF1_PB1_RW             = SPA(E_SNONE            , E_PAGE_10, 0x24),
    E_REG_P10_IF1_PB2_RW             = SPA(E_SNONE            , E_PAGE_10, 0x25),
    E_REG_P10_IF1_PB3_RW             = SPA(E_SNONE            , E_PAGE_10, 0x26),
    E_REG_P10_IF1_PB4_RW             = SPA(E_SNONE            , E_PAGE_10, 0x27),
    E_REG_P10_IF1_PB5_RW             = SPA(E_SNONE            , E_PAGE_10, 0x28),
    E_REG_P10_IF1_PB6_RW             = SPA(E_SNONE            , E_PAGE_10, 0x29),
    E_REG_P10_IF1_PB7_RW             = SPA(E_SNONE            , E_PAGE_10, 0x2A),
    E_REG_P10_IF1_PB8_RW             = SPA(E_SNONE            , E_PAGE_10, 0x2B),
    E_REG_P10_IF1_PB9_RW             = SPA(E_SNONE            , E_PAGE_10, 0x2C),
    E_REG_P10_IF1_PB10_RW            = SPA(E_SNONE            , E_PAGE_10, 0x2D),
    E_REG_P10_IF1_PB11_RW            = SPA(E_SNONE            , E_PAGE_10, 0x2E),
    E_REG_P10_IF1_PB12_RW            = SPA(E_SNONE            , E_PAGE_10, 0x2F),
    E_REG_P10_IF1_PB13_RW            = SPA(E_SNONE            , E_PAGE_10, 0x30),
    E_REG_P10_IF1_PB14_RW            = SPA(E_SNONE            , E_PAGE_10, 0x31),
    E_REG_P10_IF1_PB15_RW            = SPA(E_SNONE            , E_PAGE_10, 0x32),
    E_REG_P10_IF1_PB16_RW            = SPA(E_SNONE            , E_PAGE_10, 0x33),
    E_REG_P10_IF1_PB17_RW            = SPA(E_SNONE            , E_PAGE_10, 0x34),
    E_REG_P10_IF1_PB18_RW            = SPA(E_SNONE            , E_PAGE_10, 0x35),
    E_REG_P10_IF1_PB19_RW            = SPA(E_SNONE            , E_PAGE_10, 0x36),
    E_REG_P10_IF1_PB20_RW            = SPA(E_SNONE            , E_PAGE_10, 0x37),
    E_REG_P10_IF1_PB21_RW            = SPA(E_SNONE            , E_PAGE_10, 0x38),
    E_REG_P10_IF1_PB22_RW            = SPA(E_SNONE            , E_PAGE_10, 0x39),
    E_REG_P10_IF1_PB23_RW            = SPA(E_SNONE            , E_PAGE_10, 0x3A),
    E_REG_P10_IF1_PB24_RW            = SPA(E_SNONE            , E_PAGE_10, 0x3B),
    E_REG_P10_IF1_PB25_RW            = SPA(E_SNONE            , E_PAGE_10, 0x3C),
    E_REG_P10_IF1_PB26_RW            = SPA(E_SNONE            , E_PAGE_10, 0x3D),
    E_REG_P10_IF1_PB27_RW            = SPA(E_SNONE            , E_PAGE_10, 0x3E),
    E_REG_P10_IF2_HB0_RW             = SPA(E_SNONE            , E_PAGE_10, 0x40),
    E_REG_P10_IF2_HB1_RW             = SPA(E_SNONE            , E_PAGE_10, 0x41),
    E_REG_P10_IF2_HB2_RW             = SPA(E_SNONE            , E_PAGE_10, 0x42),
    E_REG_P10_IF2_PB0_RW             = SPA(E_SNONE            , E_PAGE_10, 0x43),
    E_REG_P10_IF2_PB1_RW             = SPA(E_SNONE            , E_PAGE_10, 0x44),
    E_REG_P10_IF2_PB2_RW             = SPA(E_SNONE            , E_PAGE_10, 0x45),
    E_REG_P10_IF2_PB3_RW             = SPA(E_SNONE            , E_PAGE_10, 0x46),
    E_REG_P10_IF2_PB4_RW             = SPA(E_SNONE            , E_PAGE_10, 0x47),
    E_REG_P10_IF2_PB5_RW             = SPA(E_SNONE            , E_PAGE_10, 0x48),
    E_REG_P10_IF2_PB6_RW             = SPA(E_SNONE            , E_PAGE_10, 0x49),
    E_REG_P10_IF2_PB7_RW             = SPA(E_SNONE            , E_PAGE_10, 0x4A),
    E_REG_P10_IF2_PB8_RW             = SPA(E_SNONE            , E_PAGE_10, 0x4B),
    E_REG_P10_IF2_PB9_RW             = SPA(E_SNONE            , E_PAGE_10, 0x4C),
    E_REG_P10_IF2_PB10_RW            = SPA(E_SNONE            , E_PAGE_10, 0x4D),
    E_REG_P10_IF2_PB11_RW            = SPA(E_SNONE            , E_PAGE_10, 0x4E),
    E_REG_P10_IF2_PB12_RW            = SPA(E_SNONE            , E_PAGE_10, 0x4F),
    E_REG_P10_IF2_PB13_RW            = SPA(E_SNONE            , E_PAGE_10, 0x50),
    E_REG_P10_IF2_PB14_RW            = SPA(E_SNONE            , E_PAGE_10, 0x51),
    E_REG_P10_IF2_PB15_RW            = SPA(E_SNONE            , E_PAGE_10, 0x52),
    E_REG_P10_IF2_PB16_RW            = SPA(E_SNONE            , E_PAGE_10, 0x53),
    E_REG_P10_IF2_PB17_RW            = SPA(E_SNONE            , E_PAGE_10, 0x54),
    E_REG_P10_IF2_PB18_RW            = SPA(E_SNONE            , E_PAGE_10, 0x55),
    E_REG_P10_IF2_PB19_RW            = SPA(E_SNONE            , E_PAGE_10, 0x56),
    E_REG_P10_IF2_PB20_RW            = SPA(E_SNONE            , E_PAGE_10, 0x57),
    E_REG_P10_IF2_PB21_RW            = SPA(E_SNONE            , E_PAGE_10, 0x58),
    E_REG_P10_IF2_PB22_RW            = SPA(E_SNONE            , E_PAGE_10, 0x59),
    E_REG_P10_IF2_PB23_RW            = SPA(E_SNONE            , E_PAGE_10, 0x5A),
    E_REG_P10_IF2_PB24_RW            = SPA(E_SNONE            , E_PAGE_10, 0x5B),
    E_REG_P10_IF2_PB25_RW            = SPA(E_SNONE            , E_PAGE_10, 0x5C),
    E_REG_P10_IF2_PB26_RW            = SPA(E_SNONE            , E_PAGE_10, 0x5D),
    E_REG_P10_IF2_PB27_RW            = SPA(E_SNONE            , E_PAGE_10, 0x5E),
    E_REG_P10_IF3_HB0_RW             = SPA(E_SNONE            , E_PAGE_10, 0x60),
    E_REG_P10_IF3_HB1_RW             = SPA(E_SNONE            , E_PAGE_10, 0x61),
    E_REG_P10_IF3_HB2_RW             = SPA(E_SNONE            , E_PAGE_10, 0x62),
    E_REG_P10_IF3_PB0_RW             = SPA(E_SNONE            , E_PAGE_10, 0x63),
    E_REG_P10_IF3_PB1_RW             = SPA(E_SNONE            , E_PAGE_10, 0x64),
    E_REG_P10_IF3_PB2_RW             = SPA(E_SNONE            , E_PAGE_10, 0x65),
    E_REG_P10_IF3_PB3_RW             = SPA(E_SNONE            , E_PAGE_10, 0x66),
    E_REG_P10_IF3_PB4_RW             = SPA(E_SNONE            , E_PAGE_10, 0x67),
    E_REG_P10_IF3_PB5_RW             = SPA(E_SNONE            , E_PAGE_10, 0x68),
    E_REG_P10_IF3_PB6_RW             = SPA(E_SNONE            , E_PAGE_10, 0x69),
    E_REG_P10_IF3_PB7_RW             = SPA(E_SNONE            , E_PAGE_10, 0x6A),
    E_REG_P10_IF3_PB8_RW             = SPA(E_SNONE            , E_PAGE_10, 0x6B),
    E_REG_P10_IF3_PB9_RW             = SPA(E_SNONE            , E_PAGE_10, 0x6C),
    E_REG_P10_IF3_PB10_RW            = SPA(E_SNONE            , E_PAGE_10, 0x6D),
    E_REG_P10_IF3_PB11_RW            = SPA(E_SNONE            , E_PAGE_10, 0x6E),
    E_REG_P10_IF3_PB12_RW            = SPA(E_SNONE            , E_PAGE_10, 0x6F),
    E_REG_P10_IF3_PB13_RW            = SPA(E_SNONE            , E_PAGE_10, 0x70),   
    E_REG_P10_IF3_PB14_RW            = SPA(E_SNONE            , E_PAGE_10, 0x71),
    E_REG_P10_IF3_PB15_RW            = SPA(E_SNONE            , E_PAGE_10, 0x72),
    E_REG_P10_IF3_PB16_RW            = SPA(E_SNONE            , E_PAGE_10, 0x73),
    E_REG_P10_IF3_PB17_RW            = SPA(E_SNONE            , E_PAGE_10, 0x74),
    E_REG_P10_IF3_PB18_RW            = SPA(E_SNONE            , E_PAGE_10, 0x75),
    E_REG_P10_IF3_PB19_RW            = SPA(E_SNONE            , E_PAGE_10, 0x76),
    E_REG_P10_IF3_PB20_RW            = SPA(E_SNONE            , E_PAGE_10, 0x77),
    E_REG_P10_IF3_PB21_RW            = SPA(E_SNONE            , E_PAGE_10, 0x78),
    E_REG_P10_IF3_PB22_RW            = SPA(E_SNONE            , E_PAGE_10, 0x79),
    E_REG_P10_IF3_PB23_RW            = SPA(E_SNONE            , E_PAGE_10, 0x7A),
    E_REG_P10_IF3_PB24_RW            = SPA(E_SNONE            , E_PAGE_10, 0x7B),
    E_REG_P10_IF3_PB25_RW            = SPA(E_SNONE            , E_PAGE_10, 0x7C),
    E_REG_P10_IF3_PB26_RW            = SPA(E_SNONE            , E_PAGE_10, 0x7D),
    E_REG_P10_IF3_PB27_RW            = SPA(E_SNONE            , E_PAGE_10, 0x7E),
    E_REG_P10_IF4_HB0_RW             = SPA(E_SNONE            , E_PAGE_10, 0x80),
    E_REG_P10_IF4_HB1_RW             = SPA(E_SNONE            , E_PAGE_10, 0x81),
    E_REG_P10_IF4_HB2_RW             = SPA(E_SNONE            , E_PAGE_10, 0x82),
    E_REG_P10_IF4_PB0_RW             = SPA(E_SNONE            , E_PAGE_10, 0x83),
    E_REG_P10_IF4_PB1_RW             = SPA(E_SNONE            , E_PAGE_10, 0x84),
    E_REG_P10_IF4_PB2_RW             = SPA(E_SNONE            , E_PAGE_10, 0x85),
    E_REG_P10_IF4_PB3_RW             = SPA(E_SNONE            , E_PAGE_10, 0x86),
    E_REG_P10_IF4_PB4_RW             = SPA(E_SNONE            , E_PAGE_10, 0x87),
    E_REG_P10_IF4_PB5_RW             = SPA(E_SNONE            , E_PAGE_10, 0x88),   
    E_REG_P10_IF4_PB6_RW             = SPA(E_SNONE            , E_PAGE_10, 0x89),
    E_REG_P10_IF4_PB7_RW             = SPA(E_SNONE            , E_PAGE_10, 0x8A),
    E_REG_P10_IF4_PB8_RW             = SPA(E_SNONE            , E_PAGE_10, 0x8B),
    E_REG_P10_IF4_PB9_RW             = SPA(E_SNONE            , E_PAGE_10, 0x8C),
    E_REG_P10_IF4_PB10_RW            = SPA(E_SNONE            , E_PAGE_10, 0x8D),
    E_REG_P10_IF4_PB11_RW            = SPA(E_SNONE            , E_PAGE_10, 0x8E),
    E_REG_P10_IF4_PB12_RW            = SPA(E_SNONE            , E_PAGE_10, 0x8F),
    E_REG_P10_IF4_PB13_RW            = SPA(E_SNONE            , E_PAGE_10, 0x90),
    E_REG_P10_IF4_PB14_RW            = SPA(E_SNONE            , E_PAGE_10, 0x91),
    E_REG_P10_IF4_PB15_RW            = SPA(E_SNONE            , E_PAGE_10, 0x92),
    E_REG_P10_IF4_PB16_RW            = SPA(E_SNONE            , E_PAGE_10, 0x93),
    E_REG_P10_IF4_PB17_RW            = SPA(E_SNONE            , E_PAGE_10, 0x94),
    E_REG_P10_IF4_PB18_RW            = SPA(E_SNONE            , E_PAGE_10, 0x95),
    E_REG_P10_IF4_PB19_RW            = SPA(E_SNONE            , E_PAGE_10, 0x96),
    E_REG_P10_IF4_PB20_RW            = SPA(E_SNONE            , E_PAGE_10, 0x97),
    E_REG_P10_IF4_PB21_RW            = SPA(E_SNONE            , E_PAGE_10, 0x98),
    E_REG_P10_IF4_PB22_RW            = SPA(E_SNONE            , E_PAGE_10, 0x99),
    E_REG_P10_IF4_PB23_RW            = SPA(E_SNONE            , E_PAGE_10, 0x9A),
    E_REG_P10_IF4_PB24_RW            = SPA(E_SNONE            , E_PAGE_10, 0x9B),
    E_REG_P10_IF4_PB25_RW            = SPA(E_SNONE            , E_PAGE_10, 0x9C),
    E_REG_P10_IF4_PB26_RW            = SPA(E_SNONE            , E_PAGE_10, 0x9D),
    E_REG_P10_IF4_PB27_RW            = SPA(E_SNONE            , E_PAGE_10, 0x9E),
    E_REG_P10_IF5_HB0_RW             = SPA(E_SNONE            , E_PAGE_10, 0xA0),
    E_REG_P10_IF5_HB1_RW             = SPA(E_SNONE            , E_PAGE_10, 0xA1),
    E_REG_P10_IF5_HB2_RW             = SPA(E_SNONE            , E_PAGE_10, 0xA2),
    E_REG_P10_IF5_PB0_RW             = SPA(E_SNONE            , E_PAGE_10, 0xA3),
    E_REG_P10_IF5_PB1_RW             = SPA(E_SNONE            , E_PAGE_10, 0xA4),
    E_REG_P10_IF5_PB2_RW             = SPA(E_SNONE            , E_PAGE_10, 0xA5),
    E_REG_P10_IF5_PB3_RW             = SPA(E_SNONE            , E_PAGE_10, 0xA6),
    E_REG_P10_IF5_PB4_RW             = SPA(E_SNONE            , E_PAGE_10, 0xA7),
    E_REG_P10_IF5_PB5_RW             = SPA(E_SNONE            , E_PAGE_10, 0xA8),
    E_REG_P10_IF5_PB6_RW             = SPA(E_SNONE            , E_PAGE_10, 0xA9),
    E_REG_P10_IF5_PB7_RW             = SPA(E_SNONE            , E_PAGE_10, 0xAA),
    E_REG_P10_IF5_PB8_RW             = SPA(E_SNONE            , E_PAGE_10, 0xAB),
    E_REG_P10_IF5_PB9_RW             = SPA(E_SNONE            , E_PAGE_10, 0xAC),
    E_REG_P10_IF5_PB10_RW            = SPA(E_SNONE            , E_PAGE_10, 0xAD),
    E_REG_P10_IF5_PB11_RW            = SPA(E_SNONE            , E_PAGE_10, 0xAE),
    E_REG_P10_IF5_PB12_RW            = SPA(E_SNONE            , E_PAGE_10, 0xAF),
    E_REG_P10_IF5_PB13_RW            = SPA(E_SNONE            , E_PAGE_10, 0xB0),
    E_REG_P10_IF5_PB14_RW            = SPA(E_SNONE            , E_PAGE_10, 0xB1),
    E_REG_P10_IF5_PB15_RW            = SPA(E_SNONE            , E_PAGE_10, 0xB2),
    E_REG_P10_IF5_PB16_RW            = SPA(E_SNONE            , E_PAGE_10, 0xB3),
    E_REG_P10_IF5_PB17_RW            = SPA(E_SNONE            , E_PAGE_10, 0xB4),
    E_REG_P10_IF5_PB18_RW            = SPA(E_SNONE            , E_PAGE_10, 0xB5),
    E_REG_P10_IF5_PB19_RW            = SPA(E_SNONE            , E_PAGE_10, 0xB6),
    E_REG_P10_IF5_PB20_RW            = SPA(E_SNONE            , E_PAGE_10, 0xB7),
    E_REG_P10_IF5_PB21_RW            = SPA(E_SNONE            , E_PAGE_10, 0xB8),
    E_REG_P10_IF5_PB22_RW            = SPA(E_SNONE            , E_PAGE_10, 0xB9),
    E_REG_P10_IF5_PB23_RW            = SPA(E_SNONE            , E_PAGE_10, 0xBA),
    E_REG_P10_IF5_PB24_RW            = SPA(E_SNONE            , E_PAGE_10, 0xBB),
    E_REG_P10_IF5_PB25_RW            = SPA(E_SNONE            , E_PAGE_10, 0xBC),
    E_REG_P10_IF5_PB26_RW            = SPA(E_SNONE            , E_PAGE_10, 0xBD),
    E_REG_P10_IF5_PB27_RW            = SPA(E_SNONE            , E_PAGE_10, 0xBE),
    E_REG_P11_AIP_CNTRL_0_RW         = SPA(E_SNONE            , E_PAGE_11, 0x00),
    E_REG_P11_CA_I2S_RW              = SPA(E_SNONE            , E_PAGE_11, 0x01),
    E_REG_P11_CA_DSD_RW              = SPA(E_SNONE            , E_PAGE_11, 0x02),
    E_REG_P11_OBA_PH_RW              = SPA(E_SNONE            , E_PAGE_11, 0x03),
    E_REG_P11_LATENCY_RD_RW          = SPA(E_SNONE            , E_PAGE_11, 0x04),
    E_REG_P11_ACR_CTS_0_RW           = SPA(E_SNONE            , E_PAGE_11, 0x05),
    E_REG_P11_ACR_CTS_1_RW           = SPA(E_SNONE            , E_PAGE_11, 0x06),
    E_REG_P11_ACR_CTS_2_RW           = SPA(E_SNONE            , E_PAGE_11, 0x07),
    E_REG_P11_ACR_N_0_RW             = SPA(E_SNONE            , E_PAGE_11, 0x08),
    E_REG_P11_ACR_N_1_RW             = SPA(E_SNONE            , E_PAGE_11, 0x09),
    E_REG_P11_ACR_N_2_RW             = SPA(E_SNONE            , E_PAGE_11, 0x0A),
    E_REG_P11_GC_AVMUTE_RW           = SPA(E_SNONE            , E_PAGE_11, 0x0B),
    E_REG_P11_CTS_N_RW               = SPA(E_SNONE            , E_PAGE_11, 0x0C),
    E_REG_P11_ENC_CNTRL_RW           = SPA(E_SNONE            , E_PAGE_11, 0x0D),
    E_REG_P11_DIP_FLAGS_RW           = SPA(E_SNONE            , E_PAGE_11, 0x0E),   
    E_REG_P11_DIP_IF_FLAGS_RW        = SPA(E_SNONE            , E_PAGE_11, 0x0F),
    E_REG_P11_CH_STAT_B_0_RW         = SPA(E_SNONE            , E_PAGE_11, 0x14),
    E_REG_P11_CH_STAT_B_1_RW         = SPA(E_SNONE            , E_PAGE_11, 0x15),
    E_REG_P11_CH_STAT_B_3_RW         = SPA(E_SNONE            , E_PAGE_11, 0x16),
    E_REG_P11_CH_STAT_B_4_RW         = SPA(E_SNONE            , E_PAGE_11, 0x17),
    E_REG_P11_CH_STAT_B_2_ap0_l_RW   = SPA(E_SNONE            , E_PAGE_11, 0x18),
    E_REG_P11_CH_STAT_B_2_ap0_r_RW   = SPA(E_SNONE            , E_PAGE_11, 0x19),
    E_REG_P11_CH_STAT_B_2_ap1_l_RW   = SPA(E_SNONE            , E_PAGE_11, 0x1A),
    E_REG_P11_CH_STAT_B_2_ap1_r_RW   = SPA(E_SNONE            , E_PAGE_11, 0x1B),
    E_REG_P11_CH_STAT_B_2_ap2_l_RW   = SPA(E_SNONE            , E_PAGE_11, 0x1C),
    E_REG_P11_CH_STAT_B_2_ap2_r_RW   = SPA(E_SNONE            , E_PAGE_11, 0x1D),
    E_REG_P11_CH_STAT_B_2_ap3_l_RW   = SPA(E_SNONE            , E_PAGE_11, 0x1E),
    E_REG_P11_CH_STAT_B_2_ap3_r_RW   = SPA(E_SNONE            , E_PAGE_11, 0x1F),
    E_REG_P11_ISRC1_HB0_RW           = SPA(E_SNONE            , E_PAGE_11, 0x20),
    E_REG_P11_ISRC1_HB1_RW           = SPA(E_SNONE            , E_PAGE_11, 0x21),
    E_REG_P11_ISRC1_HB2_RW           = SPA(E_SNONE            , E_PAGE_11, 0x22),
    E_REG_P11_ISRC1_PB0_RW           = SPA(E_SNONE            , E_PAGE_11, 0x23),
    E_REG_P11_ISRC1_PB1_RW           = SPA(E_SNONE            , E_PAGE_11, 0x24),
    E_REG_P11_ISRC1_PB2_RW           = SPA(E_SNONE            , E_PAGE_11, 0x25),
    E_REG_P11_ISRC1_PB3_RW           = SPA(E_SNONE            , E_PAGE_11, 0x26),
    E_REG_P11_ISRC1_PB4_RW           = SPA(E_SNONE            , E_PAGE_11, 0x27),
    E_REG_P11_ISRC1_PB5_RW           = SPA(E_SNONE            , E_PAGE_11, 0x28),
    E_REG_P11_ISRC1_PB6_RW           = SPA(E_SNONE            , E_PAGE_11, 0x29),
    E_REG_P11_ISRC1_PB7_RW           = SPA(E_SNONE            , E_PAGE_11, 0x2A),
    E_REG_P11_ISRC1_PB8_RW           = SPA(E_SNONE            , E_PAGE_11, 0x2B),
    E_REG_P11_ISRC1_PB9_RW           = SPA(E_SNONE            , E_PAGE_11, 0x2C),
    E_REG_P11_ISRC1_PB10_RW          = SPA(E_SNONE            , E_PAGE_11, 0x2D),
    E_REG_P11_ISRC1_PB11_RW          = SPA(E_SNONE            , E_PAGE_11, 0x2E),
    E_REG_P11_ISRC1_PB12_RW          = SPA(E_SNONE            , E_PAGE_11, 0x2F),
    E_REG_P11_ISRC1_PB13_RW          = SPA(E_SNONE            , E_PAGE_11, 0x30),
    E_REG_P11_ISRC1_PB14_RW          = SPA(E_SNONE            , E_PAGE_11, 0x31),
    E_REG_P11_ISRC1_PB15_RW          = SPA(E_SNONE            , E_PAGE_11, 0x32),
    E_REG_P11_ISRC1_PB16_RW          = SPA(E_SNONE            , E_PAGE_11, 0x33),
    E_REG_P11_ISRC1_PB17_RW          = SPA(E_SNONE            , E_PAGE_11, 0x34),
    E_REG_P11_ISRC1_PB18_RW          = SPA(E_SNONE            , E_PAGE_11, 0x35),
    E_REG_P11_ISRC1_PB19_RW          = SPA(E_SNONE            , E_PAGE_11, 0x36),
    E_REG_P11_ISRC1_PB20_RW          = SPA(E_SNONE            , E_PAGE_11, 0x37),
    E_REG_P11_ISRC1_PB21_RW          = SPA(E_SNONE            , E_PAGE_11, 0x38),
    E_REG_P11_ISRC1_PB22_RW          = SPA(E_SNONE            , E_PAGE_11, 0x39),   
    E_REG_P11_ISRC1_PB23_RW          = SPA(E_SNONE            , E_PAGE_11, 0x3A)    ,
    E_REG_P11_ISRC1_PB24_RW          = SPA(E_SNONE            , E_PAGE_11, 0x3B)    ,
    E_REG_P11_ISRC1_PB25_RW          = SPA(E_SNONE            , E_PAGE_11, 0x3C),
    E_REG_P11_ISRC1_PB26_RW          = SPA(E_SNONE            , E_PAGE_11, 0x3D),
    E_REG_P11_ISRC1_PB27_RW          = SPA(E_SNONE            , E_PAGE_11, 0x3E),
    E_REG_P11_ISRC2_HB0_RW           = SPA(E_SNONE            , E_PAGE_11, 0x40),
    E_REG_P11_ISRC2_HB1_RW           = SPA(E_SNONE            , E_PAGE_11, 0x41),
    E_REG_P11_ISRC2_HB2_RW           = SPA(E_SNONE            , E_PAGE_11, 0x42),
    E_REG_P11_ISRC2_PB0_RW           = SPA(E_SNONE            , E_PAGE_11, 0x43),
    E_REG_P11_ISRC2_PB1_RW           = SPA(E_SNONE            , E_PAGE_11, 0x44),
    E_REG_P11_ISRC2_PB2_RW           = SPA(E_SNONE            , E_PAGE_11, 0x45),
    E_REG_P11_ISRC2_PB3_RW           = SPA(E_SNONE            , E_PAGE_11, 0x46),
    E_REG_P11_ISRC2_PB4_RW           = SPA(E_SNONE            , E_PAGE_11, 0x47),
    E_REG_P11_ISRC2_PB5_RW           = SPA(E_SNONE            , E_PAGE_11, 0x48),
    E_REG_P11_ISRC2_PB6_RW           = SPA(E_SNONE            , E_PAGE_11, 0x49),
    E_REG_P11_ISRC2_PB7_RW           = SPA(E_SNONE            , E_PAGE_11, 0x4A),
    E_REG_P11_ISRC2_PB8_RW           = SPA(E_SNONE            , E_PAGE_11, 0x4B),
    E_REG_P11_ISRC2_PB9_RW           = SPA(E_SNONE            , E_PAGE_11, 0x4C),
    E_REG_P11_ISRC2_PB10_RW          = SPA(E_SNONE            , E_PAGE_11, 0x4D),
    E_REG_P11_ISRC2_PB11_RW          = SPA(E_SNONE            , E_PAGE_11, 0x4E),
    E_REG_P11_ISRC2_PB12_RW          = SPA(E_SNONE            , E_PAGE_11, 0x4F),
    E_REG_P11_ISRC2_PB13_RW          = SPA(E_SNONE            , E_PAGE_11, 0x50),
    E_REG_P11_ISRC2_PB14_RW          = SPA(E_SNONE            , E_PAGE_11, 0x51),
    E_REG_P11_ISRC2_PB15_RW          = SPA(E_SNONE            , E_PAGE_11, 0x52),
    E_REG_P11_ISRC2_PB16_RW          = SPA(E_SNONE            , E_PAGE_11, 0x53),
    E_REG_P11_ISRC2_PB17_RW          = SPA(E_SNONE            , E_PAGE_11, 0x54),
    E_REG_P11_ISRC2_PB18_RW          = SPA(E_SNONE            , E_PAGE_11, 0x55),
    E_REG_P11_ISRC2_PB19_RW          = SPA(E_SNONE            , E_PAGE_11, 0x56),
    E_REG_P11_ISRC2_PB20_RW          = SPA(E_SNONE            , E_PAGE_11, 0x57),
    E_REG_P11_ISRC2_PB21_RW          = SPA(E_SNONE            , E_PAGE_11, 0x58),
    E_REG_P11_ISRC2_PB22_RW          = SPA(E_SNONE            , E_PAGE_11, 0x59),
    E_REG_P11_ISRC2_PB23_RW          = SPA(E_SNONE            , E_PAGE_11, 0x5A),
    E_REG_P11_ISRC2_PB24_RW          = SPA(E_SNONE            , E_PAGE_11, 0x5B),
    E_REG_P11_ISRC2_PB25_RW          = SPA(E_SNONE            , E_PAGE_11, 0x5C),
    E_REG_P11_ISRC2_PB26_RW          = SPA(E_SNONE            , E_PAGE_11, 0x5D),
    E_REG_P11_ISRC2_PB27_RW          = SPA(E_SNONE            , E_PAGE_11, 0x5E),
    E_REG_P11_ACP_HB0_RW             = SPA(E_SNONE            , E_PAGE_11, 0x60),
    E_REG_P11_ACP_HB1_RW             = SPA(E_SNONE            , E_PAGE_11, 0x61),
    E_REG_P11_ACP_HB2_RW             = SPA(E_SNONE            , E_PAGE_11, 0x62),
    E_REG_P11_ACP_PB0_RW             = SPA(E_SNONE            , E_PAGE_11, 0x63),
    E_REG_P11_ACP_PB1_RW             = SPA(E_SNONE            , E_PAGE_11, 0x64),
    E_REG_P11_ACP_PB2_RW             = SPA(E_SNONE            , E_PAGE_11, 0x65),
    E_REG_P11_ACP_PB3_RW             = SPA(E_SNONE            , E_PAGE_11, 0x66),
    E_REG_P11_ACP_PB4_RW             = SPA(E_SNONE            , E_PAGE_11, 0x67),
    E_REG_P11_ACP_PB5_RW             = SPA(E_SNONE            , E_PAGE_11, 0x68),
    E_REG_P11_ACP_PB6_RW             = SPA(E_SNONE            , E_PAGE_11, 0x69),
    E_REG_P11_ACP_PB7_RW             = SPA(E_SNONE            , E_PAGE_11, 0x6A),
    E_REG_P11_ACP_PB8_RW             = SPA(E_SNONE            , E_PAGE_11, 0x6B),
    E_REG_P11_ACP_PB9_RW             = SPA(E_SNONE            , E_PAGE_11, 0x6C),
    E_REG_P11_ACP_PB10_RW            = SPA(E_SNONE            , E_PAGE_11, 0x6D),
    E_REG_P11_ACP_PB11_RW            = SPA(E_SNONE            , E_PAGE_11, 0x6E),
    E_REG_P11_ACP_PB12_RW            = SPA(E_SNONE            , E_PAGE_11, 0x6F),
    E_REG_P11_ACP_PB13_RW            = SPA(E_SNONE            , E_PAGE_11, 0x70),
    E_REG_P11_ACP_PB14_RW            = SPA(E_SNONE            , E_PAGE_11, 0x71),
    E_REG_P11_ACP_PB15_RW            = SPA(E_SNONE            , E_PAGE_11, 0x72),
    E_REG_P11_ACP_PB16_RW            = SPA(E_SNONE            , E_PAGE_11, 0x73),
    E_REG_P11_ACP_PB17_RW            = SPA(E_SNONE            , E_PAGE_11, 0x74),
    E_REG_P11_ACP_PB18_RW            = SPA(E_SNONE            , E_PAGE_11, 0x75),
    E_REG_P11_ACP_PB19_RW            = SPA(E_SNONE            , E_PAGE_11, 0x76),
    E_REG_P11_ACP_PB20_RW            = SPA(E_SNONE            , E_PAGE_11, 0x77),
    E_REG_P11_ACP_PB21_RW            = SPA(E_SNONE            , E_PAGE_11, 0x78),
    E_REG_P11_ACP_PB22_RW            = SPA(E_SNONE            , E_PAGE_11, 0x79),
    E_REG_P11_ACP_PB23_RW            = SPA(E_SNONE            , E_PAGE_11, 0x7A),
    E_REG_P11_ACP_PB24_RW            = SPA(E_SNONE            , E_PAGE_11, 0x7B),
    E_REG_P11_ACP_PB25_RW            = SPA(E_SNONE            , E_PAGE_11, 0x7C),
    E_REG_P11_ACP_PB26_RW            = SPA(E_SNONE            , E_PAGE_11, 0x7D),
    E_REG_P11_ACP_PB27_RW            = SPA(E_SNONE            , E_PAGE_11, 0x7E),
    E_REG_P13_GMD_0_HB0_RW           = SPA(E_SNONE            , E_PAGE_13, 0x00),
    E_REG_P13_GMD_0_HB1_RW           = SPA(E_SNONE            , E_PAGE_13, 0x01),
    E_REG_P13_GMD_0_HB2_RW           = SPA(E_SNONE            , E_PAGE_13, 0x02),
    E_REG_P13_GMD_0_PB0_RW           = SPA(E_SNONE            , E_PAGE_13, 0x03),
    E_REG_P13_GMD_0_PB1_RW           = SPA(E_SNONE            , E_PAGE_13, 0x04),
    E_REG_P13_GMD_0_PB2_RW           = SPA(E_SNONE            , E_PAGE_13, 0x05),
    E_REG_P13_GMD_0_PB3_RW           = SPA(E_SNONE            , E_PAGE_13, 0x06),
    E_REG_P13_GMD_0_PB4_RW           = SPA(E_SNONE            , E_PAGE_13, 0x07),
    E_REG_P13_GMD_0_PB5_RW           = SPA(E_SNONE            , E_PAGE_13, 0x08),
    E_REG_P13_GMD_0_PB6_RW           = SPA(E_SNONE            , E_PAGE_13, 0x09),
    E_REG_P13_GMD_0_PB7_RW           = SPA(E_SNONE            , E_PAGE_13, 0x0A),
    E_REG_P13_GMD_0_PB8_RW           = SPA(E_SNONE            , E_PAGE_13, 0x0B),
    E_REG_P13_GMD_0_PB9_RW           = SPA(E_SNONE            , E_PAGE_13, 0x0C),
    E_REG_P13_GMD_0_PB10_RW          = SPA(E_SNONE            , E_PAGE_13, 0x0D),
    E_REG_P13_GMD_0_PB11_RW          = SPA(E_SNONE            , E_PAGE_13, 0x0E),
    E_REG_P13_GMD_0_PB12_RW          = SPA(E_SNONE            , E_PAGE_13, 0x0F),
    E_REG_P13_GMD_0_PB13_RW          = SPA(E_SNONE            , E_PAGE_13, 0x10),
    E_REG_P13_GMD_0_PB14_RW          = SPA(E_SNONE            , E_PAGE_13, 0x11),
    E_REG_P13_GMD_0_PB15_RW          = SPA(E_SNONE            , E_PAGE_13, 0x12),
    E_REG_P13_GMD_0_PB16_RW          = SPA(E_SNONE            , E_PAGE_13, 0x13),
    E_REG_P13_GMD_0_PB17_RW          = SPA(E_SNONE            , E_PAGE_13, 0x14),
    E_REG_P13_GMD_0_PB18_RW          = SPA(E_SNONE            , E_PAGE_13, 0x15),
    E_REG_P13_GMD_0_PB19_RW          = SPA(E_SNONE            , E_PAGE_13, 0x16),
    E_REG_P13_GMD_0_PB20_RW          = SPA(E_SNONE            , E_PAGE_13, 0x17),
    E_REG_P13_GMD_0_PB21_RW          = SPA(E_SNONE            , E_PAGE_13, 0x18),
    E_REG_P13_GMD_0_PB22_RW          = SPA(E_SNONE            , E_PAGE_13, 0x19),
    E_REG_P13_GMD_0_PB23_RW          = SPA(E_SNONE            , E_PAGE_13, 0x1A),
    E_REG_P13_GMD_0_PB24_RW          = SPA(E_SNONE            , E_PAGE_13, 0x1B),
    E_REG_P13_GMD_0_PB25_RW          = SPA(E_SNONE            , E_PAGE_13, 0x1C),
    E_REG_P13_GMD_0_PB26_RW          = SPA(E_SNONE            , E_PAGE_13, 0x1D),
    E_REG_P13_GMD_0_PB27_RW          = SPA(E_SNONE            , E_PAGE_13, 0x1E),
    E_REG_P13_GMD_CONTROL_RW         = SPA(E_SNONE            , E_PAGE_13, 0x1F),
    E_REG_P13_GMD_1_HB0_RW           = SPA(E_SNONE            , E_PAGE_13, 0x20),
    E_REG_P13_GMD_1_HB1_RW           = SPA(E_SNONE            , E_PAGE_13, 0x21),
    E_REG_P13_GMD_1_HB2_RW           = SPA(E_SNONE            , E_PAGE_13, 0x22),
    E_REG_P13_GMD_1_PB0_RW           = SPA(E_SNONE            , E_PAGE_13, 0x23),
    E_REG_P13_GMD_1_PB1_RW           = SPA(E_SNONE            , E_PAGE_13, 0x24),
    E_REG_P13_GMD_1_PB2_RW           = SPA(E_SNONE            , E_PAGE_13, 0x25),
    E_REG_P13_GMD_1_PB3_RW           = SPA(E_SNONE            , E_PAGE_13, 0x26),
    E_REG_P13_GMD_1_PB4_RW           = SPA(E_SNONE            , E_PAGE_13, 0x27),
    E_REG_P13_GMD_1_PB5_RW           = SPA(E_SNONE            , E_PAGE_13, 0x28),
    E_REG_P13_GMD_1_PB6_RW           = SPA(E_SNONE            , E_PAGE_13, 0x29),
    E_REG_P13_GMD_1_PB7_RW           = SPA(E_SNONE            , E_PAGE_13, 0x2A),
    E_REG_P13_GMD_1_PB8_RW           = SPA(E_SNONE            , E_PAGE_13, 0x2B),
    E_REG_P13_GMD_1_PB9_RW           = SPA(E_SNONE            , E_PAGE_13, 0x2C),
    E_REG_P13_GMD_1_PB10_RW          = SPA(E_SNONE            , E_PAGE_13, 0x2D),
    E_REG_P13_GMD_1_PB11_RW          = SPA(E_SNONE            , E_PAGE_13, 0x2E),
    E_REG_P13_GMD_1_PB12_RW          = SPA(E_SNONE            , E_PAGE_13, 0x2F),
    E_REG_P13_GMD_1_PB13_RW          = SPA(E_SNONE            , E_PAGE_13, 0x30),
    E_REG_P13_GMD_1_PB14_RW          = SPA(E_SNONE            , E_PAGE_13, 0x31),
    E_REG_P13_GMD_1_PB15_RW          = SPA(E_SNONE            , E_PAGE_13, 0x32),
    E_REG_P13_GMD_1_PB16_RW          = SPA(E_SNONE            , E_PAGE_13, 0x33),
    E_REG_P13_GMD_1_PB17_RW          = SPA(E_SNONE            , E_PAGE_13, 0x34),
    E_REG_P13_GMD_1_PB18_RW          = SPA(E_SNONE            , E_PAGE_13, 0x35),
    E_REG_P13_GMD_1_PB19_RW          = SPA(E_SNONE            , E_PAGE_13, 0x36),
    E_REG_P13_GMD_1_PB20_RW          = SPA(E_SNONE            , E_PAGE_13, 0x37),
    E_REG_P13_GMD_1_PB21_RW          = SPA(E_SNONE            , E_PAGE_13, 0x38),
    E_REG_P13_GMD_1_PB22_RW          = SPA(E_SNONE            , E_PAGE_13, 0x39),
    E_REG_P13_GMD_1_PB23_RW          = SPA(E_SNONE            , E_PAGE_13, 0x3A),
    E_REG_P13_GMD_1_PB24_RW          = SPA(E_SNONE            , E_PAGE_13, 0x3B),
    E_REG_P13_GMD_1_PB25_RW          = SPA(E_SNONE            , E_PAGE_13, 0x3C),
    E_REG_P13_GMD_1_PB26_RW          = SPA(E_SNONE            , E_PAGE_13, 0x3D),
    E_REG_P13_GMD_1_PB27_RW          = SPA(E_SNONE            , E_PAGE_13, 0x3E)
};
#undef SPR

/**
 * Register bitfield masks, with a macro to allow binary initializers.
 * Enum names are derived directly from TDA998x register and bitfield names.
 */
#define BINARY(d7,d6,d5,d4,d3,d2,d1,d0) \
    (((d7)<<7)|((d6)<<6)|((d5)<<5)|((d4)<<4)|((d3)<<3)|((d2)<<2)|((d1)<<1)|(d0))

enum _eMaskReg
{
    E_MASKREG_NONE                          = BINARY(0,0,0,0, 0,0,0,0),
    E_MASKREG_ALL                           = BINARY(1,1,1,1, 1,1,1,1),

    /* N4 features flags read from version register:
     * not_h = no HDCP support
     * not_s = no scaler support
     *
     * N5 = a flag that is not a register bit, but is derived by the
     * driver from the new N5 registers DWIN_RE_DE and DWIN_FE_DE,
     * because the N5 device still uses the N4 version register value.
     * This bit position would clash with version register, so is not
     * present in the driver's copy (uDeviceVersion) of the version 
     * register, but only in the driver's features byte (uDeviceFeatures).
     */

     /* CEC Masks*/

    E_MASKREG_CEC_INTERRUPTSTATUS_hdmi_int  = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_CEC_INTERRUPTSTATUS_cec_int   = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_CEC_RXSHPDINTENA_ena_hpd_int  = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_CEC_RXSHPDINTENA_ena_rxs_int  = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_CEC_RXSHPDINT_hpd_int         = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_CEC_RXSHPDINT_rxs_int         = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_CEC_RXSHPDLEV_hpd_level       = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_CEC_RXSHPDLEV_rxs_level       = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_CEC_ENAMODS_dis_fro           = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_CEC_ENAMODS_dis_cclk          = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_CEC_ENAMODS_ena_rxs           = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_CEC_ENAMODS_ena_hdmi          = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_CEC_ENAMODS_ena_cec           = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_CEC_FRO_IM_CLK_CTRL_ghost_dis = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_CEC_FRO_IM_CLK_CTRL_ena_otp   = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_CEC_FRO_IM_CLK_CTRL_imclk_sel = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_CEC_FRO_IM_CLK_CTRL_fro_div   = BINARY(0,0,0,0, 0,0,0,1),

     /* HDMI Masks*/
    E_MASKREG_P00_VERSION_not_h             = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P00_VERSION_not_s             = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P00_FEATURE_N5                = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_MAIN_CNTRL0_scaler        = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_MAIN_CNTRL0_cehs          = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P00_MAIN_CNTRL0_cecs          = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_MAIN_CNTRL0_dehs          = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P00_MAIN_CNTRL0_decs          = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P00_MAIN_CNTRL0_sr            = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_SR_REG_sr_i2c_ms          = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P00_SR_REG_sr_audio           = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_DDC_DISABLE_ddc_dis       = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_CCLK_ON_cclk_ddc_on       = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_I2C_MASTER_app_strt_lat    = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P00_I2C_MASTER_dis_filt        = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P00_I2C_MASTER_dis_mm          = BINARY(0,0,0,0, 0,0,0,1),

#ifdef TMFL_TDA19989
    E_MASKREG_FEAT_POWER_DOWN_spdif          = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_FEAT_POWER_DOWN_otp            = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_FEAT_POWER_DOWN_csc            = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_FEAT_POWER_DOWN_prefilt        = BINARY(0,0,0,0, 0,0,0,1),
    E_MASKREG_FEAT_POWER_DOWN_all            = BINARY(0,0,0,0, 1,1,1,1),
#endif

    E_MASKREG_P00_INT_FLAGS_0_r0            = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_INT_FLAGS_0_pj            = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_INT_FLAGS_0_sha_1         = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P00_INT_FLAGS_0_bstatus       = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P00_INT_FLAGS_0_bcaps         = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_INT_FLAGS_0_t0            = BINARY(0,0,0,0, 0,1,0,0),
    /*E_MASKREG_P00_INT_FLAGS_0_hpd           = BINARY(0,0,0,0, 0,0,1,0),*/
    E_MASKREG_P00_INT_FLAGS_0_encrypt       = BINARY(0,0,0,0, 0,0,0,1),

    /*E_MASKREG_P00_INT_FLAGS_1_hpd_in        = BINARY(1,0,0,0, 0,0,0,0),*/
    E_MASKREG_P00_INT_FLAGS_1_sw_int        = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_INT_FLAGS_1_sc_deil       = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P00_INT_FLAGS_1_sc_vid        = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P00_INT_FLAGS_1_sc_out        = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_INT_FLAGS_1_sc_in         = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P00_INT_FLAGS_1_otp           = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P00_INT_FLAGS_1_vs_rpt        = BINARY(0,0,0,0, 0,0,0,1),
    /*E_MASKREG_P00_INT_FLAGS_2_rx_sense      = BINARY(0,0,0,0, 0,0,0,1),*/
    E_MASKREG_P00_INT_FLAGS_2_edid_blk_rd   = BINARY(0,0,0,0, 0,0,1,0),

    /*E_MASKREG_P00_INT_FLAGS_3_rxs_fil       = BINARY(0,0,0,0, 0,0,0,1),*/
    
    E_MASKREG_P00_SW_INT_sw_int             = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_ENA_ACLK_ena_aclk         = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_GND_ACLK_gnd_aclk         = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_ENA_VP_0_ena_vp7          = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_ENA_VP_0_ena_vp6          = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_ENA_VP_0_ena_vp5          = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P00_ENA_VP_0_ena_vp4          = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P00_ENA_VP_0_ena_vp3          = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_ENA_VP_0_ena_vp2          = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P00_ENA_VP_0_ena_vp1          = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P00_ENA_VP_0_ena_vp0          = BINARY(0,0,0,0, 0,0,0,1),
    
    E_MASKREG_P00_ENA_VP_1_ena_vp15         = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_ENA_VP_1_ena_vp14         = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_ENA_VP_1_ena_vp13         = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P00_ENA_VP_1_ena_vp12         = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P00_ENA_VP_1_ena_vp11         = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_ENA_VP_1_ena_vp10         = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P00_ENA_VP_1_ena_vp9          = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P00_ENA_VP_1_ena_vp8          = BINARY(0,0,0,0, 0,0,0,1),
    
    E_MASKREG_P00_ENA_VP_2_ena_vp23         = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_ENA_VP_2_ena_vp22         = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_ENA_VP_2_ena_vp21         = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P00_ENA_VP_2_ena_vp20         = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P00_ENA_VP_2_ena_vp19         = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_ENA_VP_2_ena_vp18         = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P00_ENA_VP_2_ena_vp17         = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P00_ENA_VP_2_ena_vp16         = BINARY(0,0,0,0, 0,0,0,1),
    
    E_MASKREG_P00_ENA_AP_ena_ap7            = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_ENA_AP_ena_ap6            = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_ENA_AP_ena_ap5            = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P00_ENA_AP_ena_ap4            = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P00_ENA_AP_ena_ap3            = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_ENA_AP_ena_ap2            = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P00_ENA_AP_ena_ap1            = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P00_ENA_AP_ena_ap0            = BINARY(0,0,0,0, 0,0,0,1),
    
    E_MASKREG_P00_VIP_CNTRL_0_mirr_a        = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_0_swap_a        = BINARY(0,1,1,1, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_0_mirr_b        = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_0_swap_b        = BINARY(0,0,0,0, 0,1,1,1),

    E_MASKREG_P00_VIP_CNTRL_1_mirr_c        = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_1_swap_c        = BINARY(0,1,1,1, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_1_mirr_d        = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_1_swap_d        = BINARY(0,0,0,0, 0,1,1,1),

    E_MASKREG_P00_VIP_CNTRL_2_mirr_e        = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_2_swap_e        = BINARY(0,1,1,1, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_2_mirr_f        = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_2_swap_f        = BINARY(0,0,0,0, 0,1,1,1),

#ifdef TMFL_TDA19989
    E_MASKREG_P00_MUX_VP_VIP_OUT_red        = BINARY(0,0,1,1, 0,0,0,0),
    E_MASKREG_P00_MUX_VP_VIP_OUT_green      = BINARY(0,0,0,0, 1,1,0,0),
    E_MASKREG_P00_MUX_VP_VIP_OUT_blue       = BINARY(0,0,0,0, 0,0,1,1),
#endif

    E_MASKREG_P00_VIP_CNTRL_3_edge          = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_3_de_int        = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_3_sp_sync       = BINARY(0,0,1,1, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_3_emb           = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_3_v_tgl         = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P00_VIP_CNTRL_3_h_tgl         = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P00_VIP_CNTRL_3_x_tgl         = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_VIP_CNTRL_4_tst_pat       = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_4_tst_656       = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_4_656_alt       = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_4_ccir656       = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P00_VIP_CNTRL_4_blankit       = BINARY(0,0,0,0, 1,1,0,0),
    E_MASKREG_P00_VIP_CNTRL_4_blc           = BINARY(0,0,0,0, 0,0,1,1),

    E_MASKREG_P00_VIP_CNTRL_5_sp_cnt        = BINARY(0,0,0,0, 0,1,1,0),
    E_MASKREG_P00_VIP_CNTRL_5_ckcase        = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_MAT_CONTRL_mat_bp         = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P00_MAT_CONTRL_mat_sc         = BINARY(0,0,0,0, 0,0,1,1),

#ifdef TMFL_TDA19989
    E_MASKREG_P00_VIDFORMAT_3d              = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_VIDFORMAT_3d_neg_vs       = BINARY(0,1,0,0, 0,0,0,0),
#endif
    E_MASKREG_P00_VIDFORMAT_vidformat       = BINARY(0,0,0,1, 1,1,1,1),

    E_MASKREG_P00_TBG_CNTRL_0_sync_once     = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_TBG_CNTRL_0_sync_mthd     = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_TBG_CNTRL_0_frame_dis     = BINARY(0,0,1,0, 0,0,0,0),

    E_MASKREG_P00_TBG_CNTRL_1_dwin_dis      = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_TBG_CNTRL_1_vhx_ext       = BINARY(0,0,1,1, 1,0,0,0),
    E_MASKREG_P00_TBG_CNTRL_1_vhx_ext_vs    = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P00_TBG_CNTRL_1_vhx_ext_hs    = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P00_TBG_CNTRL_1_vhx_ext_de    = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_TBG_CNTRL_1_vh_tgl        = BINARY(0,0,0,0, 0,1,1,1),
    E_MASKREG_P00_TBG_CNTRL_1_vh_tgl_2      = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P00_TBG_CNTRL_1_vh_tgl_1      = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P00_TBG_CNTRL_1_vh_tgl_0      = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_I2C_TIMER_RI              = BINARY(0,0,0,0, 1,1,1,1),
    E_MASKREG_P00_I2C_TIMER_PJ              = BINARY(1,1,1,1, 0,0,0,0),

    E_MASKREG_P00_HVF_CNTRL_0_sm            = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P00_HVF_CNTRL_0_rwb           = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_HVF_CNTRL_0_prefil        = BINARY(0,0,0,0, 1,1,0,0),
    E_MASKREG_P00_HVF_CNTRL_0_intpol        = BINARY(0,0,0,0, 0,0,1,1),

    E_MASKREG_P00_HVF_CNTRL_1_semi_planar   = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_HVF_CNTRL_1_pad           = BINARY(0,0,1,1, 0,0,0,0),
    E_MASKREG_P00_HVF_CNTRL_1_vqr           = BINARY(0,0,0,0, 1,1,0,0),
    E_MASKREG_P00_HVF_CNTRL_1_yuvblk        = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P00_HVF_CNTRL_1_for           = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_TIMER_H_wd_clksel         = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_TIMER_H_tim_h             = BINARY(0,0,0,0, 0,0,1,1),

    E_MASKREG_P00_DEBUG_PROBE_sel           = BINARY(0,0,1,1, 0,0,0,0),
    E_MASKREG_P00_DEBUG_PROBE_bypass        = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P00_DEBUG_PROBE_vid_de        = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P00_DEBUG_PROBE_di_de         = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P00_DEBUG_PROBE_woo_en        = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P00_I2S_FORMAT_i2s_format     = BINARY(0,0,0,0, 0,0,1,1),

    E_MASKREG_P00_I2S_FORMAT_i2s_data_size  = BINARY(0,0,0,0, 1,1,0,0),

    E_MASKREG_P00_AIP_CLKSEL_dst_rate       = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P00_AIP_CLKSEL_sel_aip_SHIFT  = 3,
    E_MASKREG_P00_AIP_CLKSEL_sel_aip        = BINARY(0,0,1,1, 1,0,0,0),
    E_MASKREG_P00_AIP_CLKSEL_sel_pol_clk    = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P00_AIP_CLKSEL_sel_fs         = BINARY(0,0,0,0, 0,0,1,1),
    E_MASKREG_P01_SC_VIDFORMAT_lut_sel      = BINARY(1,1,0,0, 0,0,0,0),
    E_MASKREG_P01_SC_VIDFORMAT_vid_format_o = BINARY(0,0,1,1, 1,0,0,0),
    E_MASKREG_P01_SC_VIDFORMAT_vid_format_i = BINARY(0,0,0,0, 0,1,1,1),

    E_MASKREG_P01_SC_CNTRL_phases_h         = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P01_SC_CNTRL_il_out_on        = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P01_SC_CNTRL_phases_v         = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P01_SC_CNTRL_vs_on            = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P01_SC_CNTRL_deil_on          = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P01_VIDFORMAT_vidformat       = BINARY(0,0,0,0, 0,1,1,1),

    E_MASKREG_P01_TBG_CNTRL_0_sync_once     = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P01_TBG_CNTRL_0_sync_mthd     = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P01_TBG_CNTRL_0_frame_dis     = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P01_TBG_CNTRL_0_top_ext       = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P01_TBG_CNTRL_0_de_ext        = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P01_TBG_CNTRL_0_top_sel       = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P01_TBG_CNTRL_0_top_tgl       = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P02_PLL_SERIAL_1_srl_man_iz   = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P02_PLL_SERIAL_1_srl_iz       = BINARY(0,0,0,0, 0,1,1,0),
    E_MASKREG_P02_PLL_SERIAL_1_srl_fdn      = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P02_PLL_SERIAL_2_srl_pr       = BINARY(1,1,1,1, 0,0,0,0),
    E_MASKREG_P02_PLL_SERIAL_2_srl_nosc     = BINARY(0,0,0,0, 0,0,1,1),

    E_MASKREG_P02_PLL_SERIAL_3_srl_pxin_sel = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P02_PLL_SERIAL_3_srl_de       = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P02_PLL_SERIAL_3_srl_ccir     = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P02_SERIALIZER_srl_phase3     = BINARY(1,1,1,1, 0,0,0,0),
    E_MASKREG_P02_SERIALIZER_srl_phase2     = BINARY(0,0,0,0, 1,1,1,1),

    E_MASKREG_P02_BUFFER_OUT_srl_force      = BINARY(0,0,0,0, 1,1,0,0),
    E_MASKREG_P02_BUFFER_OUT_srl_clk        = BINARY(0,0,0,0, 0,0,1,1),

    E_MASKREG_P02_PLL_SCG1_scg_fdn          = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P02_PLL_SCG2_bypass_scg       = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P02_PLL_SCG2_selpllclkin      = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P02_PLL_SCG2_scg_nosc         = BINARY(0,0,0,0, 0,0,1,1),

    E_MASKREG_P02_VAI_PLL_pllde_hvp         = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P02_VAI_PLL_pllscg_hvp        = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P02_VAI_PLL_pllsrl_hvp        = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P02_VAI_PLL_pllscg_lock       = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P02_VAI_PLL_pllsrl_lock       = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P02_AUDIO_DIV_audio_div       = BINARY(0,0,0,0, 0,1,1,1),

    E_MASKREG_P02_TEST1_srldat              = BINARY(1,1,0,0, 0,0,0,0),
    E_MASKREG_P02_TEST1_tst_nosc            = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P02_TEST1_tst_enahvp          = BINARY(0,0,0,0, 0,0,0,1),

    //E_MASKREG_P02_TEST2_pwd1v8              = BINARY(0,0,0,0, 0,0,1,0),
    //E_MASKREG_P02_TEST2_divtestoe           = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P02_SEL_CLK_ena_sc_clk        = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P02_SEL_CLK_sel_vrf_clk       = BINARY(0,0,0,0, 0,1,1,0),
    E_MASKREG_P02_SEL_CLK_sel_clk1          = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P02_BUFF_OUT2_force_dat2      = BINARY(0,0,1,1, 0,0,0,0),
    E_MASKREG_P02_BUFF_OUT2_force_dat1      = BINARY(0,0,0,0, 1,1,0,0),
    E_MASKREG_P02_BUFF_OUT2_force_dat0      = BINARY(0,0,0,0, 0,0,1,1),

    E_MASKREG_P09_EDID_CTRL_edid_rd         = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P11_AIP_CNTRL_0_rst_cts       = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P11_AIP_CNTRL_0_acr_man       = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P11_AIP_CNTRL_0_layout        = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P11_AIP_CNTRL_0_swap          = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P11_AIP_CNTRL_0_rst_fifo      = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P11_CA_I2S_hbr_chstat_4       = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P11_CA_I2S_ca_i2s             = BINARY(0,0,0,1, 1,1,1,1),

    E_MASKREG_P11_GC_AVMUTE_set_mute        = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P11_GC_AVMUTE_clr_mute        = BINARY(0,0,0,0, 0,0,0,1),
    E_MASKREG_P11_GC_AVMUTE_setclr_mute     = BINARY(0,0,0,0, 0,0,1,1),

    E_MASKREG_P11_CTS_N_m_sel               = BINARY(0,0,1,1, 0,0,0,0),
    E_MASKREG_P11_CTS_N_k_sel               = BINARY(0,0,0,0, 0,1,1,1),

    E_MASKREG_P11_ENC_CNTRL_ctl_code        = BINARY(0,0,0,0, 1,1,0,0),
    E_MASKREG_P11_ENC_CNTRL_rst_sel         = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P11_ENC_CNTRL_rst_enc         = BINARY(0,0,0,0, 0,0,0,1),
    E_MASKREG_P11_DIP_FLAGS_force_null      = BINARY(1,0,0,0, 0,0,0,0),
    E_MASKREG_P11_DIP_FLAGS_null            = BINARY(0,1,0,0, 0,0,0,0),
    E_MASKREG_P11_DIP_FLAGS_acp             = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P11_DIP_FLAGS_isrc2           = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P11_DIP_FLAGS_isrc1           = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P11_DIP_FLAGS_gc              = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P11_DIP_FLAGS_acr             = BINARY(0,0,0,0, 0,0,0,1),

    E_MASKREG_P11_DIP_IF_FLAGS_if5          = BINARY(0,0,1,0, 0,0,0,0),
    E_MASKREG_P11_DIP_IF_FLAGS_if4          = BINARY(0,0,0,1, 0,0,0,0),
    E_MASKREG_P11_DIP_IF_FLAGS_if3          = BINARY(0,0,0,0, 1,0,0,0),
    E_MASKREG_P11_DIP_IF_FLAGS_if2          = BINARY(0,0,0,0, 0,1,0,0),
    E_MASKREG_P11_DIP_IF_FLAGS_if1          = BINARY(0,0,0,0, 0,0,1,0),

    E_MASKREG_P13_GMD_CONTROL_buf_sel       = BINARY(0,0,0,0, 0,0,1,0),
    E_MASKREG_P13_GMD_CONTROL_enable        = BINARY(0,0,0,0, 0,0,0,1)
};
#undef BINARY

/**
 * 3 enum for the video formats :
 *  - 1 used in the E_REG_P00_VIDFORMAT_W register
 *  - 1 for new format that are not prefetch
 *  - 1 for PC
 */
#define REGVFMT_INVALID 0xFF

enum _eRegVfmt {
   E_REGVFMT_640x480p_60Hz     = 0, /* 1 */
   E_REGVFMT_720x480p_60Hz     ,    /* 2/3 */
   E_REGVFMT_1280x720p_60Hz    ,    /* 4 */
   E_REGVFMT_1920x1080i_60Hz   ,    /* 5 */
   E_REGVFMT_720x480i_60Hz     ,    /* 6/4 */
   E_REGVFMT_720x240p_60Hz     ,    /*NT 8/9 */
   E_REGVFMT_1920x1080p_60Hz   ,    /* 16 */
   E_REGVFMT_720x576p_50Hz     ,    /* 17/18 */
   E_REGVFMT_1280x720p_50Hz    ,    /* 19 */
   E_REGVFMT_1920x1080i_50Hz   ,    /* 20 */
   E_REGVFMT_720x576i_50Hz     ,    /* 21/22 */
   E_REGVFMT_720x288p_50Hz     ,    /* 23/24 */
   E_REGVFMT_1920x1080p_50Hz   ,    /* 31 */
#ifdef TMFL_RGB_DDR_12BITS
   E_REGVFMT_1920x1080p_24Hz   ,    /* 32 */
   E_REGVFMT_1440x576p_50Hz    ,    /* 29/30 */
   E_REGVFMT_1440x480p_60Hz    ,    /* 14/15 */
   E_REGVFMT_2880x480p_60Hz    ,    /* 35/36 */
   E_REGVFMT_2880x576p_50Hz    ,    /* 37/38 */
   E_REGVFMT_2880x480i_60Hz    ,    /* 10/11*/
   E_REGVFMT_2880x480i_60Hz_PR2,    /* 10/11*/
   E_REGVFMT_2880x480i_60Hz_PR4,    /* 10/11*/
   E_REGVFMT_2880x576i_50Hz    ,    /* 25/26 */
   E_REGVFMT_2880x576i_50Hz_PR2,    /* 25/26 */
   E_REGVFMT_720x480p_60Hz_FP  ,    /* 2/3 FP */
   E_REGVFMT_1280x720p_60Hz_FP ,    /* 4 FP */
   E_REGVFMT_720x576p_50Hz_FP  ,    /* 17/18 FP */
   E_REGVFMT_1280x720p_50Hz_FP ,    /* 19 FP */
   E_REGVFMT_1920x1080p_24Hz_FP,    /* 32 FP */
   E_REGVFMT_1920x1080p_25Hz_FP,    /* 33 FP */
   E_REGVFMT_1920x1080p_30Hz_FP,    /* 34 FP */
   E_REGVFMT_1920x1080i_60Hz_FP,    /* 5 FP */
   E_REGVFMT_1920x1080i_50Hz_FP,    /* 20 FP */
#endif
   E_REGVFMT_MAX_PREFETCH      ,
   E_REGVFMT_NUM_PREFETCH      = E_REGVFMT_MAX_PREFETCH
};

enum _eRegVfmtExtra {
#ifndef TMFL_RGB_DDR_12BITS
   E_REGVFMT_1920x1080p_24Hz   = E_REGVFMT_MAX_PREFETCH, /* 32 */
   E_REGVFMT_1920x1080p_25Hz   , /* 33 */
#else
   E_REGVFMT_1920x1080p_25Hz   = E_REGVFMT_MAX_PREFETCH,
#endif
   E_REGVFMT_1920x1080p_30Hz,   /* 34 */
   E_REGVFMT_1280x720p_24Hz,    /* 60 */
   E_REGVFMT_1280x720p_25Hz,    /* 61 */
   E_REGVFMT_1280x720p_30Hz ,   /* 62 */
#ifndef TMFL_RGB_DDR_12BITS
   E_REGVFMT_1280x720p_60Hz_FP,
   E_REGVFMT_1920x1080i_60Hz_FP,
   E_REGVFMT_1280x720p_50Hz_FP,
   E_REGVFMT_1920x1080i_50Hz_FP,
   E_REGVFMT_1920x1080p_24Hz_FP,
   E_REGVFMT_1920x1080p_25Hz_FP,
   E_REGVFMT_1920x1080p_30Hz_FP,
#endif
   E_REGVFMT_1280x720p_24Hz_FP,
   E_REGVFMT_1280x720p_25Hz_FP,
   E_REGVFMT_1280x720p_30Hz_FP,
   E_REGVFMT_MAX_EXTRA         ,
   E_REGVFMT_NUM_EXTRA         = E_REGVFMT_MAX_EXTRA - E_REGVFMT_MAX_PREFETCH
};

#ifdef FORMAT_PC
enum _eRegVfmtPC {
    E_REGVFMT_640x480p_72Hz     = E_REGVFMT_MAX_EXTRA,
    E_REGVFMT_640x480p_75Hz     ,
    E_REGVFMT_640x480p_85Hz     ,
    E_REGVFMT_800x600p_60Hz     ,
    E_REGVFMT_800x600p_72Hz     ,
    E_REGVFMT_800x600p_75Hz     ,
    E_REGVFMT_800x600p_85Hz     ,
    E_REGVFMT_1024x768p_60Hz    ,
    E_REGVFMT_1024x768p_70Hz    ,
    E_REGVFMT_1024x768p_75Hz    ,
    E_REGVFMT_1280x768p_60Hz    ,
    E_REGVFMT_1280x1024p_60Hz   ,
    E_REGVFMT_1360x768p_60Hz    ,
    E_REGVFMT_1400x1050p_60Hz   ,
    E_REGVFMT_1600x1200p_60Hz   ,
    E_REGVFMT_1280x1024p_85Hz   ,
    E_REGVFMT_MAX_PC            ,
    E_REGVFMT_NUM_PC            = E_REGVFMT_MAX_PC - E_REGVFMT_MAX_EXTRA
};
#define E_REGVFMT_MAX E_REGVFMT_MAX_PC
#else
#define E_REGVFMT_MAX E_REGVFMT_MAX_EXTRA
#endif /*FORMAT_PC*/

#define PREFETCH(fmt) ((fmt) < E_REGVFMT_MAX_PREFETCH)
#define EXTRA(fmt) (!PREFETCH(fmt))
#define PCFORMAT(fmt) ((fmt) >= E_REGVFMT_MAX_EXTRA)/* PR1570 FIXED */
#define BASE(fmt) (PREFETCH(fmt)?(fmt):(fmt)-E_REGVFMT_MAX_PREFETCH)


/**
 * An enum for the video input formats used in the E_REG_P01_SC_VIDFORMAT_W
 * register
 */
enum _eRegVfmtScIn
{
    E_REGVFMT_SCIN_480i_60Hz        = 0,
    E_REGVFMT_SCIN_576i_50Hz        = 1,
    E_REGVFMT_SCIN_480p_60Hz        = 2,
    E_REGVFMT_SCIN_576p_50Hz        = 3,
    E_REGVFMT_SCIN_720p_50Hz_60Hz   = 4,
    E_REGVFMT_SCIN_1080i_50Hz_60Hz  = 5,
    E_REGVFMT_SCIN_MAX              = 5,
    E_REGVFMT_SCIN_NUM              = 6,
    E_REGVFMT_SCIN_INVALID          = 6
};
 
/**
 * An enum to list all supported pixel clock frequencies in kHz
 */
enum _ePixClk
{
    E_PIXCLK_25175      = 0,
    E_PIXCLK_25200      = 1,
    E_PIXCLK_27000      = 2,
    E_PIXCLK_27027      = 3,
    E_PIXCLK_54000      = 4,
    E_PIXCLK_54054      = 5,
    E_PIXCLK_59400      = 6,
    E_PIXCLK_74175      = 7,
    E_PIXCLK_74250      = 8,
    E_PIXCLK_148350     = 9,
    E_PIXCLK_148500     = 10,
    E_PIXCLK_108000     = 11,
    E_PIXCLK_108108     = 12,
#ifndef FORMAT_PC
    E_PIXCLK_MAX        = 12,
    E_PIXCLK_INVALID    = 13,
    E_PIXCLK_NUM        = 13
#else /* FORMAT_PC */
    E_PIXCLK_31500      = 13,
    E_PIXCLK_36000      = 14,
    E_PIXCLK_40000      = 15,
    E_PIXCLK_49500      = 16,
    E_PIXCLK_50000      = 17,
    E_PIXCLK_56250      = 18,
    E_PIXCLK_65000      = 19,
    E_PIXCLK_75000      = 20,
    E_PIXCLK_78750      = 21,
    E_PIXCLK_79500      = 22,
    E_PIXCLK_85500      = 23,
    E_PIXCLK_PC_108000  = 24,
    E_PIXCLK_121750     = 25,
    E_PIXCLK_162000     = 26,
    E_PIXCLK_MAX        = 26,
    E_PIXCLK_INVALID    = 27,
    E_PIXCLK_NUM        = 27
#endif /* FORMAT_PC */
};

/**
 * An enum to list all device version codes supported by this driver.
 * The values form a list, with non-zero version codes first in any order.
 * The E_DEV_VERSION_END_LIST must be the last value in the list.
 */
enum _eDevVersion
{
    E_DEV_VERSION_N2            = 0x101,    /**< TDA9989 n2  */
    E_DEV_VERSION_TDA19989      = 0x201,    /**< TDA19989    */
    E_DEV_VERSION_TDA19989_N2   = 0x202,    /**< TDA19989 N2 */
    E_DEV_VERSION_TDA19988      = 0x301,    /**< TDA19988    */
    E_DEV_VERSION_LIST_END      = 0x00,
    E_DEV_VERSION_LIST_NUM      = 5         /**< Number of items in list */
};

/**
 * An enum to list all CEA Data Block Tag Codes we may find in EDID.
 */
enum _eCeaBlockTags
{
    E_CEA_RESERVED_0     = 0x00,
    E_CEA_AUDIO_BLOCK    = 0x01,
    E_CEA_VIDEO_BLOCK    = 0x02,
    E_CEA_VSDB           = 0x03,
    E_CEA_SPEAKER_ALLOC  = 0x04,
    E_CEA_VESA_DTC       = 0x05,
    E_CEA_RESERVED_6     = 0x06,
    E_CEA_EXTENDED       = 0x07
};

/**
 * An enum to list all CEA Data Block Extended Tag Codes we may find in EDID.
 */
enum _eCeaExtendedBlockTags
{
    EXT_CEA_MISC_VIDEO_FIELDS        = 0x00,
    EXT_CEA_VS_VIDEO_DB              = 0x01,
    EXT_CEA_COLORIMETRY_DB           = 0x05,
    EXT_CEA_MISC_AUDIO_FIELDS        = 0x10,
    EXT_CEA_VS_AUDIO_DB              = 0x11
};

/** A typedef for colourspace values */
typedef enum
{
    HDMITX_CS_RGB_FULL              = 0,    /**< RGB Full (PC) */
    HDMITX_CS_RGB_LIMITED           = 1,    /**< RGB Limited (TV) */
    HDMITX_CS_YUV_ITU_BT601         = 2,    /**< YUV ITUBT601 (SDTV) */
    HDMITX_CS_YUV_ITU_BT709         = 3,    /**< YUV ITUBT709 (HDTV) */
    HDMITX_CS_NUM                   = 4     /**< Number Cspaces we support */
} tmbslTDA9989Colourspace_t;

/** Matrix register block size */
#define MATRIX_PRESET_SIZE          31

/** Matrix register block size */
#define MATRIX_PRESET_QTY           12

/** The enum that vectors us into the MatrixPreset table */
enum _eMatrixPresetIndex
{
    E_MATRIX_RGBF_2_RGBL    = 0,
    E_MATRIX_RGBF_2_BT601   = 1,
    E_MATRIX_RGBF_2_BT709   = 2,
    E_MATRIX_RGBL_2_RGBF    = 3,
    E_MATRIX_RGBL_2_BT601   = 4,
    E_MATRIX_RGBL_2_BT709   = 5,
    E_MATRIX_BT601_2_RGBF   = 6,
    E_MATRIX_BT601_2_RGBL   = 7,
    E_MATRIX_BT601_2_BT709  = 8,
    E_MATRIX_BT709_2_RGBF   = 9,
    E_MATRIX_BT709_2_RGBL   = 10,
    E_MATRIX_BT709_2_BT601  = 11
};   

/** EDID i2c address */
#define DDC_EDID_ADDRESS        0xA0

/** EDID alternate i2c address */
#define DDC_EDID_ADDRESS_ALT    0xA2

/** EDID Segment Pointer address */
#define DDC_SGMT_PTR_ADDRESS    0x60

/** EDID DTD block descriptor size */
#define EDID_DTD_BLK_SIZE       0x12

/** number of detailed timing descriptor stored in pDis */
#define NUMBER_DTD_STORED       10

/** MUX_AP audio selection values */
#define MUX_AP_SELECT_I2S     0xE4
#define MUX_AP_SELECT_SPDIF   0x27

#define TDA19989_MUX_AP_SELECT_I2S     0x64
#define TDA19989_MUX_AP_SELECT_SPDIF   0x24

/** VSWING default value */
#define HDMI_TX_VSWING_VALUE 0x09


/**
 * \brief A structure type to form arrays that hold a series of registers and
 * values
 */
typedef struct _tmHdmiTxRegVal_t
{
    UInt16 Reg;
    UInt8  Val;
} tmHdmiTxRegVal_t;

/**
 * \brief A structure type to form arrays that hold a series of registers,
 * bitfield masks and bitfield values
 */
typedef struct _tmHdmiTxRegMaskVal_t
{
    UInt16 Reg;
    UInt8  Mask;
    UInt8  Val;
} tmHdmiTxRegMaskVal_t;

/**
 * \brief A function pointer type to call a function and return a result
 */
typedef tmErrorCode_t (* ptmHdmiTxFunc_t) (tmUnitSelect_t txUnit);

/**
 * \brief The structure of a TM998x object, one per device unit
 ****************************************************************************
 ** Copy changes to kTestDisNames tab in "HDMI Driver - Register List.xls" **
 ****************************************************************************
 */

typedef struct _tmHdmiTxobject_t
{
    /** Component State */
    tmbslTDA9989State_t state;

    /** Count of events ignored by setState() */
    UInt8 nIgnoredEvents;

    /** Device unit number */
    tmUnitSelect_t txUnit;

    /** Device I2C slave address */
    UInt8 uHwAddress;

    /** System function to write to the I2C driver */
    ptmbslHdmiTxSysFunc_t sysFuncWrite;

    /** System function to read from the I2C driver */
    ptmbslHdmiTxSysFunc_t sysFuncRead;

    /** System function to read EDID blocks via the I2C driver */
    ptmbslHdmiTxSysFuncEdid_t sysFuncEdidRead;

    /** System function to run a timer */
    ptmbslHdmiTxSysFuncTimer_t sysFuncTimer;

    /** Array of registered interrupt handler callback functions */
    ptmbslHdmiTxCallback_t funcIntCallbacks[HDMITX_CALLBACK_INT_NUM];

    /** Flags to store disable or enable of interrupts */
    UInt16 InterruptsEnable; /* At moment used only for VS Interrupt */

    /** Device version(s) supported by this component */
    UInt16 uSupportedVersions[E_DEV_VERSION_LIST_NUM];

    /** Device version read from register, with features flags masked out */
    UInt16 uDeviceVersion;

    /** Device features flags read from version register */
    UInt8 uDeviceFeatures;

    /** The device's power state */
    tmbslHdmiTxPowerState_t ePowerState;

    /*=== E D I D ===*/

    /** EDID Use alternative i2c address flag */
    Bool bEdidAlternateAddr;

    /** The sink type set by the user (may or may not match EdidSinkType) */
    tmbslHdmiTxSinkType_t sinkType;

    /** EDID Sink Type for receiver */
    tmbslHdmiTxSinkType_t EdidSinkType;

    /** EDID AI_Support from HDMI VSDB */
    Bool EdidSinkAi;

    /** EDID CEA flags from extension block */
    UInt8 EdidCeaFlags;

    /** EDID CEA flags from colorimetry block */
    UInt8 EdidCeaXVYCCFlags;

    /** EDID latency information */
    tmbslHdmiTxEdidLatency_t EdidLatency;

    /** EDID 3D data structure */
    tmbslHdmiTxEdidExtraVsdbData_t EdidExtraVsdbData;

    /** EDID Read Status */
    UInt8 EdidStatus;

    /** NB DTD stored in EdidDTD */
    UInt8 NbDTDStored;

    /** EDID Detailed Timing Descriptor */
    tmbslHdmiTxEdidDtd_t EdidDTD[NUMBER_DTD_STORED];

    /** EDID First Moniteur descriptor */
    tmbslHdmiTxEdidFirstMD_t EdidFirstMonitorDescriptor;

    /** EDID Second Moniteur descriptor */
    tmbslHdmiTxEdidSecondMD_t EdidSecondMonitorDescriptor;

    /** EDID Other Moniteur descriptor */
    tmbslHdmiTxEdidOtherMD_t EdidOtherMonitorDescriptor;

    /** EDID supported Short Video Descriptors */
    UInt8 EdidVFmts[HDMI_TX_SVD_MAX_CNT];

    /** Counter for supported short video descriptors */
    UInt8 EdidSvdCnt;

    /** EDID supported Short Audio Descriptors */
    tmbslHdmiTxEdidSad_t EdidAFmts[HDMI_TX_SAD_MAX_CNT];

    /** Counter for supported short audio descriptors */
    UInt8 EdidSadCnt;

    /** EDID block workspace */
    UInt8 EdidBlock[EDID_BLOCK_SIZE];

    /** EDID Block Count */
    UInt8 EdidBlockCnt;

    /** CEC Source Address read from EDID as "A.B.C.D" nibbles */
    UInt16 EdidSourceAddress;

    /** EDID block number which is reading */
    UInt8 EdidBlockRequested;

    /** EDID read on going*/
    Bool EdidReadStarted;

    /** Parameter for return edid block requested by application */
    tmbslHdmiTxEdidToApp_t EdidToApp;

    /** EDID Basic Display Parameters */
    tmbslHdmiTxEdidBDParam_t EDIDBasicDisplayParam;
    
#ifdef TMFL_HDCP_SUPPORT
    /*=== H D C P === */

    Bool HDCPIgnoreEncrypt;

    /** Configured DDC I2C slave address */
    UInt8 HdcpSlaveAddress;

    /** Configured mode of our transmitter device */
    tmbslHdmiTxHdcpTxMode_t HdcpTxMode;

    /** Configured HDCP options */
    tmbslHdmiTxHdcpOptions_t HdcpOptions;

    /** BCAPS read from sink */
    UInt8 HdcpBcaps;

    /** BSTATUS read from sink */
    UInt16 HdcpBstatus;

    /** Device value generated for Ri=Ri' comparison */
    UInt16 HdcpRi;

    /** Device HDCP FSM state */
    UInt8 HdcpFsmState;

    /** Device failure state that caused T0 interrupt */
    UInt8 HdcpT0FailState;

    /** Otp Seed key from user*/
    UInt16 HdcpSeed;

    /* Key Selection Vector for transmitter */
    UInt8 HdcpAksv[HDMITX_KSV_BYTES_PER_DEVICE];

    /** Local callback scheduled to be called after HdcpFuncRemainingMs */
    ptmHdmiTxFunc_t HdcpFuncScheduled;

    /** Period in ms after which to call HdcpFuncScheduled; 0=disabled */
    UInt16 HdcpFuncRemainingMs;

    /** Configured period in ms after which to do HDCP check */
    UInt16 HdcpCheckIntervalMs;

    /** Period in ms until next HDCP check */
    UInt16 HdcpCheckRemainingMs;

    /** Number of the HDCP check since HDCP was started; 0=disabled */
    UInt8 HdcpCheckNum;

    /** Configured number of HDCP checks to do after HDCP is started */
    UInt8 HdcpChecksToDo;
#endif /* TMFL_HDCP_SUPPORT */

    /*=== V I D E O ===*/

    /** Current EIA/CEA video input format */
    tmbslHdmiTxVidFmt_t vinFmt;

    /** Current EIA/CEA video output format */
    tmbslHdmiTxVidFmt_t voutFmt;
    
    /** Current pix Rate*/
    tmbslHdmiTxPixRate_t pixRate;

    /** Video input mode */
    tmbslHdmiTxVinMode_t vinMode;

    /** Video output mode */
    tmbslHdmiTxVoutMode_t voutMode;

    /** Vertical output frequency */
    tmbslHdmiTxVfreq_t voutFreq;

    /** Current scaler mode */
    tmbslHdmiTxScaMode_t scaMode;

    /** Current upsampler mode */
    tmbslHdmiTxUpsampleMode_t upsampleMode;

    /** Current pixel repetition count */
    UInt8 pixelRepeatCount;

    /** Status of hot plug detect pin last read at interrupt */
    tmbslHdmiTxHotPlug_t hotPlugStatus;

    /** Status of rx sense detect pin last read at interrupt */
    tmbslHdmiTxRxSense_t rxSenseStatus;

    /** Current register page */
    UInt8 curRegPage;

    /** Shadow copies of write-only registers with bitfields */
    UInt8 shadowReg[E_SNUM];

    /** TRUE: Blue screen is the previous test pattern ; FALSE: is not */
    Bool prevFilterPattern;

    /** TRUE: last screen is test pattern ; FALSE: is not */
    Bool prevPattern;

    /** TRUE: Unit has been initialized; FALSE: not initialized */
    Bool bInitialized;

    tmbslHdmiTxVQR_t        dviVqr;

    /** TRUE: 3D Frame Packing video is ongoing */
    Bool h3dFpOn;

} tmHdmiTxobject_t;

/**
 * \The structure of registers for video format , 
 *  used by PC_formats and chip_unknown formats
 */

typedef struct _tmHdmiTxVidReg_t
{
    UInt16  nPix;
    UInt16  nLine;
    UInt8   VsLineStart;
    UInt16  VsPixStart;
    UInt8   VsLineEnd;
    UInt16  VsPixEnd;
    UInt16  HsStart;
    UInt16  HsEnd;
    UInt8   ActiveVideoStart;
    UInt16  ActiveVideoEnd;
    UInt16  DeStart;
    UInt16  DeEnd;
    UInt16  ActiveSpaceStart;
    UInt16  ActiveSpaceEnd;
} tmHdmiTxVidReg_t;


/*============================================================================*/
/*                       EXTERN DATA DEFINITION                               */
/*============================================================================*/

#include "tmbslTDA9989_local_otp.h"

extern tmHdmiTxobject_t gHdmiTxInstance[HDMITX_UNITS_MAX];
extern UInt8 kPageIndexToPage[E_PAGE_NUM];

/*============================================================================*/
/*                       EXTERN FUNCTION PROTOTYPES                           */
/*============================================================================*/

tmErrorCode_t    checkUnitSetDis (tmUnitSelect_t txUnit,
                                  tmHdmiTxobject_t **ppDis);
tmErrorCode_t    getHwRegisters (tmHdmiTxobject_t *pDis, 
                                 UInt16 regShadPageAddr,
                                 UInt8 *pData, UInt16 lenData);
tmErrorCode_t    getHwRegister (tmHdmiTxobject_t *pDis, 
                                UInt16 regShadPageAddr,
                                UInt8 *pRegValue);
tmErrorCode_t    setHwRegisters (tmHdmiTxobject_t *pDis, 
                                 UInt16 regShadPageAddr,
                                 UInt8 *pData, UInt16 lenData);
tmErrorCode_t    setHwRegisterMsbLsb (tmHdmiTxobject_t *pDis, 
                                      UInt16 regShadPageAddr, 
                                      UInt16 regWord);
tmErrorCode_t    setHwRegister (tmHdmiTxobject_t *pDis, 
                                UInt16 regShadPageAddr,
                                UInt8 regValue);
tmErrorCode_t    setHwRegisterField (tmHdmiTxobject_t *pDis,
                                     UInt16 regShadPageAddr,
                                     UInt8 fieldMask, UInt8 fieldValue);
tmErrorCode_t    setHwRegisterFieldTable(tmHdmiTxobject_t *pDis,
                                     const tmHdmiTxRegMaskVal_t *pTable);
tmErrorCode_t    getCECHwRegister(tmHdmiTxobject_t *pDis, 
                                    UInt16  regAddr,
                                    UInt8   *pRegValue);
tmErrorCode_t    setCECHwRegister(tmHdmiTxobject_t *pDis, 
                                    UInt16  regAddr, 
                                    UInt8   regValue);

tmErrorCode_t    lmemcpy (void *pTable1,
                        const void *pTable2,
                        UInt Size);
tmErrorCode_t    lmemset (void *pTable1,
                        const UInt8 value,
                        UInt Size);


#ifdef __cplusplus
}
#endif

#endif /* TMBSLTDA9989_LOCAL_H */
/*============================================================================*/
/*                            END OF FILE                                     */
/*============================================================================*/

