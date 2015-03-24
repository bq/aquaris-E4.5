#ifndef _DU_TIMER_OBSERVER_H
#define _DU_TIMER_OBSERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================*/
// Include files
/*=============================================================*/
// system includes

// project includes

// local includes

// forward references


/*=============================================================*/
// Macro definition
/*=============================================================*/


/*=============================================================*/
// Type definition
/*=============================================================*/


/*=============================================================*/
// Global variable definition
/*=============================================================*/

/*=============================================================*/
// Global function definition
/*=============================================================*/
/*
extern int to_init(int index, char * name, unsigned long interval, void (*callback)(void));
extern int to_init_all(char * name, unsigned long interval);
extern int to_update_jiffies(int index);
*/
#define to_init(index, name, interval, callback)
#define to_init_all(name, interval)
#define to_update_jiffies(index)

#ifdef __cplusplus
}
#endif

#endif //#ifndef _DU_TIMER_OBSERVER_H
