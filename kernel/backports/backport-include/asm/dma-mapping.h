#ifndef __BACKPORT_ASM_DMA_MAPPING_H
#define __BACKPORT_ASM_DMA_MAPPING_H
#include_next <asm/dma-mapping.h>
#include <linux/version.h>

#if defined(CONFIG_BACKPORT_BPAUTO_BUILD_DMA_SHARED_HELPERS)
#define dma_common_get_sgtable LINUX_BACKPORT(dma_common_get_sgtable)
int
dma_common_get_sgtable(struct device *dev, struct sg_table *sgt,
		       void *cpu_addr, dma_addr_t dma_addr, size_t size);
#endif /* defined(CONFIG_BACKPORT_BPAUTO_BUILD_DMA_SHARED_HELPERS) */

#endif /* __BACKPORT_ASM_DMA_MAPPING_H */
