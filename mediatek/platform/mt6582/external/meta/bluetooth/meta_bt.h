
#ifndef __META_BT_H__
#define __META_BT_H__
#include "meta_common.h"
#include "FT_Public.h"
#include "Meta_bt_Para.h"

typedef void (*BT_CNF_CB)(BT_CNF *cnf, void *buf, unsigned int size);

extern void META_BT_Register(BT_CNF_CB callback);

#endif 



