#include "cmdq_mmp.h"

static CMDQ_MMP_Events_t CMDQ_MMP_Events;
#if 0
extern void MMProfileEnable(int enable);
extern void MMProfileStart(int start);
#else
        __weak void MMProfileEnableEvent(MMP_Event event, long enable) { return 0; }
        __weak void MMProfileEnableFTraceEvent(MMP_Event event, long enable, long ftrace) { return 0; }
        __weak void MMProfileEnableEventRecursive(MMP_Event event, long enable) { return 0; }
        __weak void MMProfileEnableFTraceEventRecursive(MMP_Event event, long enable, long ftrace) { return 0; }
        __weak long MMProfileQueryEnable(MMP_Event event) { return 1; }
        __weak void MMProfileLog(MMP_Event event, MMP_LogType type) { return 0; }
        __weak void MMProfileLogEx(MMP_Event event, MMP_LogType type, unsigned long data1,  unsigned long data2) { return 0; }
        __weak long MMProfileLogMeta(MMP_Event event, MMP_LogType type, MMP_MetaData_t *pMetaData) { return 1; }
        __weak long MMProfileLogMetaString(MMP_Event event, MMP_LogType type, const char *str) { return 1; }
        __weak long MMProfileLogMetaStringEx(MMP_Event event, MMP_LogType type, unsigned long data1,
                                     unsigned long data2, const char *str) { return 1; }
        __weak long MMProfileLogMetaStructure(MMP_Event event, MMP_LogType type,
                                      MMP_MetaDataStructure_t *pMetaData) { return 1; }
        __weak long MMProfileLogMetaBitmap(MMP_Event event, MMP_LogType type,MMP_MetaDataBitmap_t *pMetaData) { return 1; }

        __weak void MMProfileEnable(int a){ return 0; }
        __weak void MMProfileStart(int a){ return 0; }
        __weak MMP_Event MMProfileRegisterEvent(MMP_Event parent, const char* name){ return 0; }
#endif

CMDQ_MMP_Events_t *cmdq_mmp_get_event(void)
{
	return &CMDQ_MMP_Events;
}

void cmdq_mmp_init(void)
{
#if CMDQ_PROFILE_MMP
	MMProfileEnable(1);
	if (CMDQ_MMP_Events.CMDQ == 0) {
		CMDQ_MMP_Events.CMDQ = MMProfileRegisterEvent(MMP_RootEvent, "CMDQ");
		CMDQ_MMP_Events.thread_en =
		    MMProfileRegisterEvent(CMDQ_MMP_Events.CMDQ, "thread_en");
		CMDQ_MMP_Events.CMDQ_IRQ = MMProfileRegisterEvent(CMDQ_MMP_Events.CMDQ, "CMDQ_IRQ");
		CMDQ_MMP_Events.warning = MMProfileRegisterEvent(CMDQ_MMP_Events.CMDQ, "warning");
		CMDQ_MMP_Events.loopBeat = MMProfileRegisterEvent(CMDQ_MMP_Events.CMDQ, "loopBeat");

		CMDQ_MMP_Events.autoRelease_add =
		    MMProfileRegisterEvent(CMDQ_MMP_Events.CMDQ, "autoRelease_add");
		CMDQ_MMP_Events.autoRelease_done =
		    MMProfileRegisterEvent(CMDQ_MMP_Events.CMDQ, "autoRelease_done");
		CMDQ_MMP_Events.consume_add =
		    MMProfileRegisterEvent(CMDQ_MMP_Events.CMDQ, "consume_add");
		CMDQ_MMP_Events.consume_done =
		    MMProfileRegisterEvent(CMDQ_MMP_Events.CMDQ, "consume_done");
		CMDQ_MMP_Events.alloc_task =
		    MMProfileRegisterEvent(CMDQ_MMP_Events.CMDQ, "alloc_task");
		CMDQ_MMP_Events.wait_task =
		    MMProfileRegisterEvent(CMDQ_MMP_Events.CMDQ, "wait_task");
		CMDQ_MMP_Events.wait_thread =
		    MMProfileRegisterEvent(CMDQ_MMP_Events.CMDQ, "wait_thread");
		CMDQ_MMP_Events.MDP_reset =
		    MMProfileRegisterEvent(CMDQ_MMP_Events.CMDQ, "MDP_reset");

		MMProfileEnableEventRecursive(CMDQ_MMP_Events.CMDQ, 1);
	}
	MMProfileStart(1);
#endif
}
