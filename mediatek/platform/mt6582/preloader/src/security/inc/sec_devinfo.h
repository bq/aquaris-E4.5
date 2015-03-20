#ifndef DEVINFO_H
#define DEVINFO_H

/******************************************************************************
 * CONSTANT DEFINITIONS                                                       
 ******************************************************************************/
#define INCORRECT_INDEX          0xFFFFFFFF    /* incorrect register index */

/******************************************************************************
 * TYPE DEFINITIONS                                                            
******************************************************************************/
typedef enum
{   
    E_AREA0,    
    E_AREA1,    
    E_AREA2,    
    E_AREA3,    
    E_AREA4,    
    E_AREA5,    
    E_AREA6,    
    E_AREA7,    
    E_AREA8,    
    E_AREA9,    
    E_AREA10,   
    E_AREA11,   
    E_AREA12,   
    E_AREA13,   
    E_AREA14,   
    E_AREA15,   
    E_AREA16,   
    E_AREA17,   
    E_AREA18,   
    E_AREA19,   
    E_AREA20,   
    E_AREA_MAX
} E_INDEX;


/******************************************************************************
 * EXPORT FUNCTION
 ******************************************************************************/
extern u32 seclib_get_devinfo_with_index(u32 index);


#endif /* DEVINFO_H*/



