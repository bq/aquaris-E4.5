

#ifndef __AP_EDITOR_PARA_H__
#define __AP_EDITOR_PARA_H__

#ifdef __cplusplus
extern "C"
{
#endif
    /********************************************
    * Generic Primitives for AP_Editor READ/WRITE
    ********************************************/

    typedef struct
    {
        FT_H			header;         //the header of ft module
        unsigned short  file_idx;       // file lid
        unsigned short  para;           //record id
    } FT_AP_Editor_read_req;

    typedef struct
    {
        FT_H			header;         //the header of ft module
        unsigned short  file_idx;       //file lid
        unsigned short  para;           //record id
        unsigned char   read_status;    //read nvram file status: 0 is fail
        unsigned char	status;         //the status of ft module:  0 is success
    } FT_AP_Editor_read_cnf;


    typedef struct
    {
        FT_H			header;         //the header of ft module
        unsigned short  file_idx;       //file lid
        unsigned short  para;           //record id
    } FT_AP_Editor_write_req;

    typedef struct
    {
        FT_H			header;         //the header of ft module
        unsigned short  file_idx;       //file lid
        unsigned short  para;           //record id
        unsigned char   write_status;   //the write status
        unsigned char	status;         //the status of ft module:  0 is success
    } FT_AP_Editor_write_cnf;


    typedef struct
    {
        FT_H			header;         //the header of ft module
        unsigned char	reset_category;	//0xff indicate reset all files
        unsigned short	file_idx;   	//0xffff indicate reset all files
    } FT_AP_Editor_reset_req;

    typedef struct
    {
        FT_H			header;         //the header of ft module
        unsigned char	reset_status;   //the status of reset file to default value
        unsigned char	status;         //the status of ft module, 0 is success
    } FT_AP_Editor_reset_cnf;


    /* implement these functions in AP_Editor.LIB  */
    /********************************************************************************
    //FUNCTION:
    //		META_Editor_Init
    //DESCRIPTION:
    //		this function is called to initial the meta_editor module.
    //
    //PARAMETERS:
    //		None
    //
    //RETURN VALUE:
    //		TRUE: is scuccess, otherwise is fail
    //
    //DEPENDENCY:
    //		None
    //
    //GLOBALS AFFECTED
    //		None
    ********************************************************************************/
    bool 					META_Editor_Init(void);


    /********************************************************************************
    //FUNCTION:
    //		META_Editor_Deinit
    //DESCRIPTION:
    //		this function is called to de-initial the meta_editor module.
    //
    //PARAMETERS:
    //		None
    //
    //RETURN VALUE:
    //		TRUE: is scuccess, otherwise is fail
    //
    //DEPENDENCY:
    //		META_Editor_Init must have been called
    //
    //GLOBALS AFFECTED
    //		None
    ********************************************************************************/
    bool 					META_Editor_Deinit(void);

    /********************************************************************************
    //FUNCTION:
    //		META_Editor_ReadFile_OP
    //DESCRIPTION:
    //		this function is called to Read a record of NvRam file from Target side to PC.
    //
    //PARAMETERS:
    //		req:
    //
    //RETURN VALUE:
    //		TRUE: is scuccess, otherwise is fail. the data will be send to PC in the function body
    //
    //DEPENDENCY:
    //		META_Editor_Init must have been called
    //
    //GLOBALS AFFECTED
    ********************************************************************************/
    bool					META_Editor_ReadFile_OP(FT_AP_Editor_read_req *req);

    /********************************************************************************
    //FUNCTION:
    //		META_Editor_WriteFile_OP
    //DESCRIPTION:
    //		this function is called to write a record of NvRam file from PC side to Target.
    //
    //PARAMETERS:
    //		None
    //
    //RETURN VALUE:
    //		refers to the definition of "FT_AP_Editor_write_cnf"
    //
    //DEPENDENCY:
    //		META_Editor_Init must have been called
    //
    //GLOBALS AFFECTED
    //		None
    ********************************************************************************/
    FT_AP_Editor_write_cnf	META_Editor_WriteFile_OP(FT_AP_Editor_write_req *req,
            char *peer_buf,
            unsigned short peer_len);

    /********************************************************************************
    //FUNCTION:
    //		META_Editor_ResetFile_OP
    //DESCRIPTION:
    //		this function is called to reset a NvRam to default value.
    //
    //PARAMETERS:
    //		None
    //
    //RETURN VALUE:
    //		refers to the definition of "FT_AP_Editor_reset_cnf"
    //
    //DEPENDENCY:
    //		META_Editor_Init must have been called
    //
    //GLOBALS AFFECTED
    //		None
    ********************************************************************************/
    FT_AP_Editor_reset_cnf	META_Editor_ResetFile_OP(FT_AP_Editor_reset_req *req);

    /********************************************************************************
    //FUNCTION:
    //		META_Editor_ResetAllFile_OP
    //DESCRIPTION:
    //		this function is called to Reset all of NvRam files to default value.
    //
    //PARAMETERS:
    //		None
    //
    //RETURN VALUE:
    //		refers to the definition of "FT_AP_Editor_reset_cnf"
    //
    //DEPENDENCY:
    //		META_Editor_Init must have been called
    //
    //GLOBALS AFFECTED
    //		None
    ********************************************************************************/
    FT_AP_Editor_reset_cnf	META_Editor_ResetAllFile_OP(FT_AP_Editor_reset_req *req);


#ifdef __cplusplus
}
#endif

#endif


