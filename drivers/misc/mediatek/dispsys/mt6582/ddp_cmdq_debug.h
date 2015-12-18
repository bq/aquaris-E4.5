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

#ifndef __DDP_CMDQ_DEBUG_H__
#define __DDP_CMDQ_DEBUG_H__

#include "ddp_cmdq.h"

uint32_t* cmdq_core_get_pc(const TaskStruct *pTask, uint32_t thread, uint32_t insts[4]);
const char* cmdq_core_get_event_name(uint32_t event);
const char* cmdq_core_parse_subsys(uint32_t argA, uint32_t argB);
const char* cmdq_core_parse_op(uint32_t opCode);
void cmdq_core_parse_error(struct TaskStruct *pTask, uint32_t thread,
                           const char **moduleName, uint32_t *instA, uint32_t *instB);

uint32_t* cmdq_core_dump_pc(const TaskStruct *pTask, int thread, const char *tag);
void cmdq_core_dump_mdp_status(uint32_t engineFlag, int32_t logLevel);

#endif // __DDP_CMDQ_DEBUG_H__