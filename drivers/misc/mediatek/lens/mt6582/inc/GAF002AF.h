#ifndef _GAF002AF_H
#define _GAF002AF_H

#include <linux/ioctl.h>
//#include "kd_imgsensor.h"

#define GAF002AF_MAGIC 'A'
//IOCTRL(inode * ,file * ,cmd ,arg )

//Structures
typedef struct {
//current position
unsigned long u4CurrentPosition;
//macro position
unsigned long u4MacroPosition;
//Infiniti position
unsigned long u4InfPosition;
//Motor Status
bool          bIsMotorMoving;
//Motor Open?
bool          bIsMotorOpen;
//Support SR?
bool          bIsSupportSR;
} stGAF002AF_MotorInfo;


//Control commnad
//S means "set through a ptr"
//T means "tell by a arg value"
//G means "get by a ptr"
//Q means "get by return a value"
//X means "switch G and S atomically"
//H means "switch T and Q atomically"
#define GAF002AFIOC_G_MOTORINFO _IOR(GAF002AF_MAGIC,0,stGAF002AF_MotorInfo)

#define GAF002AFIOC_T_MOVETO _IOW(GAF002AF_MAGIC,1,unsigned long)

#define GAF002AFIOC_T_SETINFPOS _IOW(GAF002AF_MAGIC,2,unsigned long)

#define GAF002AFIOC_T_SETMACROPOS _IOW(GAF002AF_MAGIC,3,unsigned long)

#define GAF002AFIOC_T_SETPARA _IOW(GAF002AF_MAGIC,5,unsigned long)

#else
#endif
