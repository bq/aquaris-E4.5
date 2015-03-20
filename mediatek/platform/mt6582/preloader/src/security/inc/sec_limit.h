
#ifndef SEC_LIMIT_H
#define SEC_LIMIT_H

/******************************************************************************
 * FORBIDEN MODE
 ******************************************************************************/
typedef enum 
{
    F_FACTORY_MODE      = 0x0001,
} FORBIDDEN_MODE;

/******************************************************************************
 * LIMITATION
 ******************************************************************************/
#define SEC_LIMIT_MAGIC  0x4C4C4C4C // LLLL

typedef struct 
{
    unsigned int magic_num;
    FORBIDDEN_MODE forbid_mode;
} SEC_LIMIT;

#endif /* SEC_LIMIT_H */



