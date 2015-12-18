/*
* Copyright (C) 2011-2014 MediaTek Inc.
* 
* This program is free software: you can redistribute it and/or modify it under the terms of the 
* GNU General Public License version 2 as published by the Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DFO_BOOT_H
#define DFO_BOOT_H

#define ATAG_DFO_DATA 0x41000805
#define DFO_BOOT_COUNT 14

typedef struct
{
    char name[DFO_BOOT_COUNT][32];   // kernel dfo name array
    int value[DFO_BOOT_COUNT];       // kernel dfo value array
} tag_dfo_boot;

#endif
