#ifndef CAMERA_SYSRAM_H
#define CAMERA_SYSRAM_H
//-----------------------------------------------------------------------------
#define SYSRAM_DEV_NAME     "camera-sysram"
#define SYSRAM_MAGIC_NO     'p'
//-----------------------------------------------------------------------------
typedef enum
{
    SYSRAM_USER_VIDO,
    SYSRAM_USER_GDMA,
    SYSRAM_USER_SW_FD,
    SYSRAM_USER_AMOUNT,
    SYSRAM_USER_NONE
}SYSRAM_USER_ENUM;
//
typedef struct
{
    unsigned long       Alignment;
    unsigned long       Size;
    SYSRAM_USER_ENUM    User;
    unsigned long       Addr; // In/Out : address
    unsigned long       TimeoutMS; // In : millisecond
}SYSRAM_ALLOC_STRUCT;
//
typedef enum
{
    SYSRAM_CMD_ALLOC,
    SYSRAM_CMD_FREE,
    SYSRAM_CMD_DUMP
}SYSRAM_CMD_ENUM;
//-----------------------------------------------------------------------------
#define SYSRAM_ALLOC    _IOWR(  SYSRAM_MAGIC_NO,    SYSRAM_CMD_ALLOC,   SYSRAM_ALLOC_STRUCT)
#define SYSRAM_FREE     _IOWR(  SYSRAM_MAGIC_NO,    SYSRAM_CMD_FREE,    SYSRAM_USER_ENUM)
#define SYSRAM_DUMP     _IO(    SYSRAM_MAGIC_NO,    SYSRAM_CMD_DUMP)
//-----------------------------------------------------------------------------
#endif

