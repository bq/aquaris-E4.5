
#ifndef __META_CPU_PARA_H__
#define __META_CPU_PARA_H__

#include <meta_common.h>
#include <FT_Public.h>

#ifdef __cplusplus
extern "C"
{
#endif


    /************************CPU register cmd structure define***********************/
    typedef struct
    {
        unsigned int		addr;		//baseband register address
        unsigned char		bytenum;	//the lenght length
    } CPU_REG_READ_REQ;

    typedef struct
    {
        unsigned int		value;		//the returned value
        unsigned char		status;		//read status: 0 is fail
    } CPU_REG_READ_CNF;


    typedef struct
    {
        unsigned int		addr;		//baseband register address
        unsigned int		value;		//write baseband reister value
        unsigned char		bytenum;	//the register length
    } CPU_REG_WRITE_REQ;

    typedef struct
    {
        unsigned char		status;		//write status: 0 is fail
    } CPU_REG_WRITE_CNF;

    typedef struct
    {
        FT_H				header;
        CPU_REG_READ_REQ 	req;
    } FT_REG_READ_REQ;

    typedef struct
    {
        FT_H				header;
        CPU_REG_READ_CNF	cnf;
        unsigned char		status;
    } FT_REG_READ_CNF;

    typedef struct
    {
        FT_H				header;
        CPU_REG_WRITE_REQ	req;
    } FT_REG_WRITE_REQ;

    typedef struct
    {
        FT_H				header;
        CPU_REG_WRITE_CNF	cnf;
        unsigned char		status;
    } FT_REG_WRITE_CNF;

    /************************RTC register cmd structure define***********************/
    typedef struct
    {
        unsigned short		interval;
    } WatchDog_REQ;

    typedef struct
    {
        unsigned short		rtc_sec;
        unsigned short		rtc_min;
        unsigned short		rtc_hour;
        unsigned short		rtc_day;
        unsigned short		rtc_mon;
        unsigned short		rtc_wday;
        unsigned short		rtc_year;
        unsigned short		status;

    } WatchDog_CNF;


    /********************************************************************************
    //FUNCTION:
    //		META_CPUReg_Init
    //DESCRIPTION:
    //		this function is used to init meta cpu module, but now it is reserved.
    //
    //PARAMETERS:
    //		None
    //
    //RETURN VALUE:
    //		TRUE is success, otherwise is fail
    //
    //DEPENDENCY:
    //		None
    //
    //GLOBALS AFFECTED
    //		None
    ********************************************************************************/
    BOOL 					META_CPUReg_Init(void);

    /********************************************************************************
    //FUNCTION:
    //		META_CPUReg_Deinit
    //DESCRIPTION:
    //		this function is called to deinit the meta cpu module, but now it is reserved.
    //
    //PARAMETERS:
    //		None
    //
    //RETURN VALUE:
    //		TRUE is success, otherwise is fail
    //
    //DEPENDENCY:
    //		META_CPUReg_Init must have been called
    //
    //GLOBALS AFFECTED
    //		None
    ********************************************************************************/
    BOOL 					META_CPUReg_Deinit(void);


    /********************************************************************************
    //FUNCTION:
    //		META_CPURegR_OP
    //DESCRIPTION:
    //		this function is called to read chip registor.
    //
    //PARAMETERS:
    //		ft_req:	[IN]	refers to the define of "FT_REG_READ_REQ"
    //
    //RETURN VALUE:
    //		refers to the define of "FT_REG_READ_CNF"
    //
    //DEPENDENCY:
    //		META_CPUReg_Init must have been called
    //
    //GLOBALS AFFECTED
    //		None
    ********************************************************************************/
    FT_REG_READ_CNF			META_CPURegR_OP(FT_REG_READ_REQ *ft_req);


    /********************************************************************************
    //FUNCTION:
    //		META_CPURegW_OP
    //DESCRIPTION:
    //		this function is called to write related value to the cpu registor
    //
    //PARAMETERS:
    //		ft_req:	[IN] refers to the define of "FT_REG_WRITE_REQ"
    //
    //RETURN VALUE:
    //		refers to the define of "FT_REG_WRITE_CNF"
    //
    //DEPENDENCY:
    //		META_CPUReg_Init must have been called
    //
    //GLOBALS AFFECTED
    //		None
    ********************************************************************************/
    FT_REG_WRITE_CNF		META_CPURegW_OP(FT_REG_WRITE_REQ  *ft_req );

    /********************************************************************************
    //FUNCTION:
    //		META_RTCRead_OP
    //DESCRIPTION:
    //		this function is called to read RTC value
    //
    //PARAMETERS:
    //		ft_req: [IN] refers to the define of "WatchDog_REQ"
    //
    //RETURN VALUE:
    //		refers to the define of "WatchDog_CNF"
    //
    //DEPENDENCY:
    //		None
    //
    //GLOBALS AFFECTED
    //		None
    ********************************************************************************/
    WatchDog_CNF			META_RTCRead_OP(WatchDog_REQ  ft_req );


#ifdef __cplusplus
}
#endif

#endif


