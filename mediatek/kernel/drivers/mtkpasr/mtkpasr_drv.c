/*
 * MTKPASR SW Module
 */

#define KMSG_COMPONENT "mtkpasr"
#define pr_fmt(fmt) KMSG_COMPONENT ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bitops.h>
#include <linux/buffer_head.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/highmem.h>
#include <linux/slab.h>
#include <linux/lzo.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/swap.h>
#include <linux/suspend.h>
#include <linux/migrate.h>
#include "mtkpasr_drv.h"

/* MTKPASR Information */
static struct zs_pool *mtkpasr_mem_pool;
static struct table *mtkpasr_table = NULL;
static u64 mtkpasr_disksize = 0;		/* bytes */
static u32 mtkpasr_total_slots = 0;
static u32 mtkpasr_free_slots = 0;

/* Rank & Bank Information */
static struct mtkpasr_bank *mtkpasr_banks = NULL;
static struct mtkpasr_rank *mtkpasr_ranks = NULL;
static int num_banks = 0;
static int num_ranks = 0;
static u32 banks_per_rank;
static unsigned long pages_per_bank = 0, pages_per_rank = 0;
static unsigned long mtkpasr_start_pfn;
static unsigned long mtkpasr_end_pfn;
static unsigned long mtkpasr_total_pfns;

/* Strategy control for PASR SW operation */
#ifdef CONFIG_MTKPASR_MAFL
static unsigned int mtkpasr_ops_invariant = 0;
static unsigned int prev_mafl_count = 0;
static unsigned int before_mafl_count = 0;
#endif

/* For no-PASR-imposed banks */
static struct nopasr_bank *nopasr_banks = NULL;
static int num_nopasr_banks = 0;
 
/* For page migration operation */
static LIST_HEAD(fromlist);
static LIST_HEAD(tolist);
static int fromlist_count;
static int tolist_count;
static unsigned long mtkpasr_migration_end;
static unsigned long mtkpasr_last_scan;
static int mtkpasr_admit_order;

/* Switch : Enabled by default */
int mtkpasr_enable = 1;
int mtkpasr_enable_sr = 1;

/* Debug filter */
#ifdef CONFIG_MT_ENG_BUILD
int mtkpasr_debug_level = 3;
#else
int mtkpasr_debug_level = 1;
#endif

/* Globals */
struct mtkpasr *mtkpasr_device;

/*------------------*/
/*-- page_alloc.c --*/
/*------------------*/

/* Find inuse & free pages */
extern int pasr_find_free_page(struct page *page, struct list_head *freelist);
/* Compute admit order for page allocation */
extern int pasr_compute_safe_order(void);

/* Banksize */
extern unsigned long pasrbank_size;

/*--------------*/
/*-- vmscan.c --*/
/*--------------*/

/* Isolate pages */
#ifdef CONFIG_MTKPASR_ALLEXTCOMP
extern int mtkpasr_isolate_page(struct page *page, int check_swap);
#else
extern int mtkpasr_isolate_page(struct page *page);
#endif
/* Drop pages in file/anon lrus! */
extern int mtkpasr_drop_page(struct page *page);

/* Show mem banks */
int mtkpasr_show_banks(char *buf)
{
	int i, j, len = 0, tmp;

	if (mtkpasr_device->init_done == 0) 
		return sprintf(buf, "MTKPASR is not initialized!\n");

	/* Show ranks & banks */
	for (i=0;i<num_ranks;i++) {
		tmp = sprintf(buf, "Rank[%d] - start_bank[%d] end_bank[%d]\n",i,mtkpasr_ranks[i].start_bank,mtkpasr_ranks[i].end_bank);
		buf += tmp;
		len += tmp;
		for (j=mtkpasr_ranks[i].start_bank;j<=mtkpasr_ranks[i].end_bank;j++) {
			tmp = sprintf(buf, "  Bank[%d] - start_pfn[0x%lx] end_pfn[0x%lx] segment[%d]\n",j,mtkpasr_banks[j].start_pfn,mtkpasr_banks[j].end_pfn-1,mtkpasr_banks[j].segment);
			buf += tmp;
			len += tmp;
		}
	}

	/* Show remaining banks */
	for (i=0;i<num_banks;i++) {
		if (mtkpasr_banks[i].rank == NULL) {
			tmp = sprintf(buf, "Bank[%d] - start_pfn[0x%lx] end_pfn[0x%lx] segment[%d]\n",i,mtkpasr_banks[i].start_pfn,mtkpasr_banks[i].end_pfn-1,mtkpasr_banks[i].segment);
			buf += tmp;
			len += tmp;
		}
	}

	return len;
}

static void mtkpasr_free_page(struct mtkpasr *mtkpasr, size_t index)
{
	unsigned long handle = mtkpasr_table[index].handle;

	if (unlikely(!handle)) {
		/*
		 * No memory is allocated for zero filled pages.
		 * Simply clear zero page flag.
		 */
		return;
	}

	zs_free(mtkpasr_mem_pool, handle);
	
	/* Reset related fields to make it consistent ! */
	mtkpasr_table[index].handle = 0;
	mtkpasr_table[index].size = 0;
	mtkpasr_table[index].obj = NULL;
}

/* 0x5D5D5D5D */
static void handle_zero_page(struct page *page)
{
	void *user_mem;

	user_mem = kmap_atomic(page);
	memset(user_mem, 0x5D, PAGE_SIZE);
	kunmap_atomic(user_mem);

	flush_dcache_page(page);
}

static int mtkpasr_read(struct mtkpasr *mtkpasr, u32 index, struct page *page)
{
	int ret;
	size_t clen;
	unsigned char *user_mem, *cmem;

	/* !! We encapsulate the page into mtkpasr_table !! */
	page = mtkpasr_table[index].obj;
	if (page == NULL) {
		mtkpasr_err("\n\n\n\nNull Page!\n\n\n\n");
		return 0;
	}

	/* Requested page is not present in compressed area */
	if (unlikely(!mtkpasr_table[index].handle)) {
		mtkpasr_err("Not present : page pfn[%ld]\n",page_to_pfn(page));
		handle_zero_page(page);
		return 0;
	}

	user_mem = kmap_atomic(page);
	clen = PAGE_SIZE;
	cmem = zs_map_object(mtkpasr_mem_pool, mtkpasr_table[index].handle, ZS_MM_RO);

	if (mtkpasr_table[index].size == PAGE_SIZE) {
		memcpy(user_mem, cmem, PAGE_SIZE);
		ret = LZO_E_OK;
	} else {
		ret = lzo1x_decompress_safe(cmem, mtkpasr_table[index].size, user_mem, &clen);
	}

	zs_unmap_object(mtkpasr_mem_pool, mtkpasr_table[index].handle);

	kunmap_atomic(user_mem);

	/* Should NEVER happen. Return bio error if it does. */
	if (unlikely(ret != LZO_E_OK)) {
		mtkpasr_err("Decompression failed! err=%d, page=%u, pfn=%lu\n", ret, index, page_to_pfn(page));
		/* Should be zero! */
		/* return 0; */	// to free this slot
	}

	/* Can't use it because maybe some pages w/o actual mapping 
	flush_dcache_page(page); */

	/* Free this object */
	mtkpasr_free_page(mtkpasr, index);

	return 0;
}

static int mtkpasr_write(struct mtkpasr *mtkpasr, u32 index, struct page *page)
{
	int ret;
	size_t clen;
	unsigned long handle;
	unsigned char *user_mem, *cmem, *src;
	
	src = mtkpasr->compress_buffer;

	/*
	 * System overwrites unused sectors. Free memory associated
	 * with this sector now.
	 */

	user_mem = kmap_atomic(page);

	ret = lzo1x_1_compress(user_mem, PAGE_SIZE, src, &clen, mtkpasr->compress_workmem);

	kunmap_atomic(user_mem);

	if (unlikely(ret != LZO_E_OK)) {
		mtkpasr_err("Compression failed! err=%d\n", ret);
		ret = -EIO;
		goto out;
	}

	if (unlikely(clen > max_cmpr_size)) {
		src = NULL;
		clen = PAGE_SIZE;
	}

	handle = zs_malloc(mtkpasr_mem_pool, clen);
	if (!handle) {
		mtkpasr_err("Error allocating memory for compressed "
			"page: %u, size=%zu\n", index, clen);
		ret = -ENOMEM;
		goto out;
	}
	cmem = zs_map_object(mtkpasr_mem_pool, handle, ZS_MM_WO);

	if (clen == PAGE_SIZE)
		src = kmap_atomic(page);
	memcpy(cmem, src, clen);
	if (clen == PAGE_SIZE)
		kunmap_atomic(src);

	zs_unmap_object(mtkpasr_mem_pool, handle);

	/* Update global MTKPASR table */
	mtkpasr_table[index].handle = handle;
	mtkpasr_table[index].size = clen;
	mtkpasr_table[index].obj = page; 

	return 0;

out:
	return ret;
}

/* This is the main entry for active memory compression for PASR */
/*
 * return 0 means success
 */
int mtkpasr_forward_rw(struct mtkpasr *mtkpasr, u32 index, struct page *page, int rw)
{
	int ret = -ENOMEM;

	if (rw == READ) {
		ret = mtkpasr_read(mtkpasr, index, page);
		mtkpasr_free_slots = (!!ret)? mtkpasr_free_slots : (mtkpasr_free_slots + 1); 
	} else {
		/* No free slot! */
		if (mtkpasr_free_slots == 0) {
			mtkpasr_log("No free slots!\n");
			return ret;
		}
		ret = mtkpasr_write(mtkpasr, index, page);
		mtkpasr_free_slots = (!!ret)? mtkpasr_free_slots : (mtkpasr_free_slots - 1); 
	}

	return ret;
}
EXPORT_SYMBOL(mtkpasr_forward_rw);

/* Acquire the number of free slots */
int mtkpasr_acquire_frees(void)
{
	return mtkpasr_free_slots;
}
EXPORT_SYMBOL(mtkpasr_acquire_frees);

/* Acquire the number of total slots */
int mtkpasr_acquire_total(void)
{
	return mtkpasr_total_slots;
}
EXPORT_SYMBOL(mtkpasr_acquire_total);

/* This is a recovery step for invalid PASR status */
void mtkpasr_reset_slots(void)
{
	size_t index;

	/* Free all pages that are still in this mtkpasr device */
	for (index = 0;index < mtkpasr_total_slots;index++) {
		unsigned long handle = mtkpasr_table[index].handle;
		if (!handle)
			continue;

		zs_free(mtkpasr_mem_pool, handle);

		/* Reset related fields to make it consistent ! */
		mtkpasr_table[index].handle = 0;
		mtkpasr_table[index].size = 0;
		mtkpasr_table[index].obj = NULL;

		/* Add it */
		mtkpasr_free_slots++;
	}

#ifdef CONFIG_MTKPASR_DEBUG
	if (mtkpasr_free_slots != mtkpasr_total_slots) {
		BUG();
	}
#endif
}

/*******************************/
/* MTKPASR Core Implementation */
/*******************************/

/* Helper function for page migration (Runnint under IRQ-disabled environment ) */
/* To avoid fragmentation through mtkpasr_admit_order */
static struct page *mtkpasr_alloc(struct page *migratepage, unsigned long data, int **result)
{
#if 1	/* FAST PATH */
	struct page *page = NULL, *end_page;
	struct zone *z;
	/*unsigned long flags;*/
	int found;
	int order;

	/* No admission on page allocation */
	if (unlikely(mtkpasr_admit_order < 0)) {
		return NULL;
	}
retry:
	/* We still have some free pages */
	if (!list_empty(&tolist)) {
		page = list_entry(tolist.next, struct page, lru);
		list_del(&page->lru);
#ifdef CONFIG_MTKPASR_DEBUG
		--tolist_count;
#endif
	} else {
		/* Check whether mtkpasr_last_scan meets the end */
		if (mtkpasr_last_scan < mtkpasr_migration_end) {
			mtkpasr_last_scan = mtkpasr_start_pfn - pageblock_nr_pages;
			return NULL;
		}
		/* To collect free pages */
		page = pfn_to_page(mtkpasr_last_scan);
		end_page = pfn_to_page(mtkpasr_last_scan + pageblock_nr_pages);
		z = page_zone(page);
		while (page < end_page) {
			/* Lock this zone */
			/*** spin_lock_irqsave(&z->lock, flags); ***/
			/* Find free pages */
			if (!PageBuddy(page)) {
				/*** spin_unlock_irqrestore(&z->lock, flags); ***/
				++page;
				continue;
			}
			/* Is this ok? */
			order = PAGE_ORDER(page);
			if (order > mtkpasr_admit_order) {
				/*** spin_unlock_irqrestore(&z->lock, flags); ***/
				page += (1 << order);
				continue;
			}
			/* Found! */
			found = pasr_find_free_page(page, &tolist);
			/* Unlock this zone */
			/*** spin_unlock_irqrestore(&z->lock, flags); ***/
			/* Update found */
#ifdef CONFIG_MTKPASR_DEBUG
			tolist_count += found;
#endif
			page += found;
		}
		/* Update mtkpasr_last_scan*/
		mtkpasr_last_scan -= pageblock_nr_pages;

		/* Retry */
		goto retry;
	}

	return page;
#else
	/* With __GFP_HIGHMEM? */
	return alloc_pages(__GFP_HIGHMEM|GFP_ATOMIC, 0);
#endif
}

/* Return whether current system has enough free memory to save it from congestion */
#define SAFE_ORDER	(THREAD_SIZE_ORDER + 1)
static struct zone *nz = NULL;
static unsigned long safe_level = 0;
static int pasr_check_free_safe(void)
{
	unsigned long free = zone_page_state(nz, NR_FREE_PAGES);

	if (free > safe_level) {
		return 0;
	}

	return -1;
}

/*-- MTKPASR INTERNAL-USED PARAMETERS --*/

/* Map of in-use pages: For a bank with 128MB, we need 32 pages. */
static void *src_pgmap = NULL;
/* Maps for pages in external compression */
static unsigned long *extcomp = NULL;
/* With the size equal to src_pgmap */
static unsigned long *sorted_for_extcomp = NULL;	
/* MTKPASR state */
static enum mtkpasr_phase mtkpasr_status = MTKPASR_OFF;
/* Atomic variable to indicate MTKPASR slot */
static atomic_t sloti;

#ifdef CONFIG_MTKPASR_MAFL
static unsigned long mafl_total_count = 0;
unsigned long mtkpasr_show_page_reserved(void)
{
	return mafl_total_count;
}
bool mtkpasr_no_phaseone_ops(void)
{
	int safe_mtkpasr_pfns;

	safe_mtkpasr_pfns = mtkpasr_total_pfns >> 1;
	return ((prev_mafl_count == mafl_total_count) || (mafl_total_count > safe_mtkpasr_pfns));
}
bool mtkpasr_no_ops(void)
{
	return ((mafl_total_count == mtkpasr_total_pfns) || (mtkpasr_ops_invariant > MAX_OPS_INVARIANT));
}
#endif

/* Reset state to MTKPASR_OFF */
void mtkpasr_reset_state(void)
{
	mtkpasr_reset_slots();
	mtkpasr_status = MTKPASR_OFF;
}

/* Enabling SR or Turning off DPD. It should be called after syscore_resume
 * Actually, it only does reset on the status of all ranks & banks!
 *
 * Return - MTKPASR_WRONG_STATE (Should bypass it & go to next state)
 * 	    MTKPASR_SUCCESS
 */
enum mtkpasr_phase mtkpasr_enablingSR(void)
{
	enum mtkpasr_phase result = MTKPASR_SUCCESS;
	int check_dpd = 0;
	int i, j;

	/* Sanity Check */
	if (mtkpasr_status != MTKPASR_ON && mtkpasr_status != MTKPASR_DPD_ON) {
		mtkpasr_err("Error Current State [%d]!\n",mtkpasr_status);
		return MTKPASR_WRONG_STATE;
	} else {
		check_dpd = (mtkpasr_status == MTKPASR_DPD_ON)? 1 : 0;
		/* Go to MTKPASR_ENABLINGSR state */
		mtkpasr_status = MTKPASR_ENABLINGSR;
	}

	/* From which state */
	if (check_dpd) {
		for (i=0;i<num_ranks;i++) {
			if (mtkpasr_ranks[i].inused == MTKPASR_DPDON) {
				/* Clear rank */
				mtkpasr_ranks[i].inused = 0;
				/* Clear all related banks */
				for (j=mtkpasr_ranks[i].start_bank;j<=mtkpasr_ranks[i].end_bank;j++) {
					mtkpasr_banks[j].inused = 0;
				}
				mtkpasr_info("Call DPDOFF API!\n");
			}
		}
	}

	/* Check all banks */
	for (i=0;i<num_banks;i++) {
		if (mtkpasr_banks[i].inused == MTKPASR_SROFF) {
			/* Clear bank */
			mtkpasr_banks[i].inused = 0;
			mtkpasr_info("Call SPM SR/ON API on bank[%d]!\n",i);
		} else {
			mtkpasr_info("Bank[%d] free[%d]!\n",i,mtkpasr_banks[i].inused);
		}
	}
	
	/* Go to MTKPASR_EXITING state if success(Always being success!) */
	if (result == MTKPASR_SUCCESS) {
		mtkpasr_status = MTKPASR_EXITING;
	}

	return result;
}

/* Decompress all immediate data. It should be called right after mtkpasr_enablingSR
 *
 * Return - MTKPASR_WRONG_STATE
 *          MTKPASR_SUCCESS
 *          MTKPASR_FAIL (Some fatal error!)
 */
enum mtkpasr_phase mtkpasr_exiting(void)
{
	enum mtkpasr_phase result = MTKPASR_SUCCESS;
	int current_index;
	int ret = 0;
	struct mtkpasr *mtkpasr;
	int should_flush_cache = 0;
#ifdef CONFIG_MTKPASR_DEBUG 
	int decompressed = 0;
#endif

	mtkpasr_info("\n");

	/* Sanity Check */
	if (mtkpasr_status != MTKPASR_EXITING) {
		mtkpasr_err("Error Current State [%d]!\n",mtkpasr_status);
		/* 
		 * Failed to exit PASR!! - This will cause some user processes died unexpectedly! 
		 * We don't do anything here because it is harmless to kernel.
		 */
		return MTKPASR_WRONG_STATE;
	}

	/* Main thread is here */
	mtkpasr = &mtkpasr_device[0];

	/* Do decompression */
	current_index = atomic_dec_return(&sloti);
	should_flush_cache = current_index;
	while (current_index >= 0) {
		ret = mtkpasr_forward_rw(mtkpasr, current_index, NULL, READ);
		/* Unsuccessful decompression */
		if (unlikely(ret)) {
			break;
		}
#ifdef CONFIG_MTKPASR_DEBUG 
		++decompressed;
#endif
		/* Next */
		current_index = atomic_dec_return(&sloti);
	}
	
	/* Check decompression result */
	if (ret) {
		mtkpasr_err("Failed Decompression!\n");
		/* 
		 * Failed to exit PASR!! - This will cause some user processes died unexpectedly! 
		 * We don't do anything here because it is harmless to kernel.
		 */
		result = MTKPASR_FAIL;
	}
	
	/* Go to MTKPASR_OFF state if success */
	if (result == MTKPASR_SUCCESS) {
		mtkpasr_status = MTKPASR_OFF;
	}

	/* Check whether we should flush cache */
	if (should_flush_cache >= 0)
		flush_cache_all();

#ifdef CONFIG_MTKPASR_DEBUG 
	mtkpasr_info("Decompressed pages [%d]\n",decompressed);
#endif
	return result;
}

/* If something error happens at MTKPASR_ENTERING/MTKPASR_DISABLINGSR, then call it */
void mtkpasr_restoring(void)
{
	mtkpasr_info("\n");

	/* Sanity Check */
	if (mtkpasr_status != MTKPASR_ENTERING && mtkpasr_status != MTKPASR_DISABLINGSR) {
		mtkpasr_err("Error Current State [%d]!\n",mtkpasr_status);
		return;
	} else {
		/* Go to MTKPASR_RESTORING state */
		mtkpasr_status = MTKPASR_RESTORING;
	}

	/* No matter which status it reaches, we only need to do is to reset all slots here!(Data stored is not corrupted!) */
	mtkpasr_reset_slots();
	
	/* Go to MTKPASR_OFF state */
	mtkpasr_status = MTKPASR_OFF;
	
	mtkpasr_info("(END)\n");
}

/*
 * Check whether current page is compressed
 * return 1 means found
 *        0       not found
 */
static int check_if_compressed(long start, long end, int pfn)
{
	long mid;
	int found = 0;

	/* Needed! */
	end = end - 1;

	/* Start to search */
	while (start <= end) {
		mid = (start + end) >> 1;
		if (pfn == extcomp[mid]) {
			found = 1;
			break;
		} else if (pfn > extcomp[mid]) {
			start = mid + 1;
		} else {
			end = mid - 1;
		}
	}

	return found;
}

/* Return the number of inuse pages */
static u32 check_inused(unsigned long start, unsigned long end, long comp_start, long comp_end)
{
	int inused = 0;
	struct page *page;

	for (;start < end;start++) {
		page = pfn_to_page(start);
		if (page_count(page) != 0) {
			if(check_if_compressed(comp_start, comp_end, start) == 0) {
				++inused;
			}
		}
	}

	return inused;
}
	
/* Compute bank inused! */
static void compute_bank_inused(int all)
{
	int i;

#ifdef CONFIG_MTKPASR_MAFL
	/* fast path */
	if (mtkpasr_no_ops()) {
		for (i = 0;i < num_banks;i++) {
			if (mtkpasr_banks[i].inmafl == mtkpasr_banks[i].valid_pages) {
				mtkpasr_banks[i].inused = 0;
			} else {
				/* Rough estimation */
				mtkpasr_banks[i].inused = mtkpasr_banks[i].valid_pages - mtkpasr_banks[i].inmafl;
			}
		}
		goto fast_path;
	}
#endif

	/* 
	 * Drain pcp LRU lists to free some "unused" pages! 
	 * (During page migration, there may be some OLD pages be in pcp pagevec! To free them!)
	 * To call lru_add_drain_all();
	 *
	 * Drain pcp free lists to free some hot/cold pages into buddy!
	 * To call drain_all_pages();
	 */
	MTKPASR_FLUSH();

	/* Scan banks */
	for (i = 0;i < num_banks;i++) {
#ifdef CONFIG_MTKPASR_MAFL
		mtkpasr_banks[i].inused = check_inused(mtkpasr_banks[i].start_pfn + mtkpasr_banks[i].inmafl, mtkpasr_banks[i].end_pfn,
				mtkpasr_banks[i].comp_start, mtkpasr_banks[i].comp_end);
#else
		mtkpasr_banks[i].inused = check_inused(mtkpasr_banks[i].start_pfn, mtkpasr_banks[i].end_pfn,
				mtkpasr_banks[i].comp_start, mtkpasr_banks[i].comp_end);
#endif
	}

#ifdef CONFIG_MTKPASR_MAFL
fast_path:
#endif
	/* Should we compute no-PASR-imposed banks? */
	if (all != 0) {
		/* Excluding 1st nopasr_banks (Kernel resides here.)*/
		for (i = 1;i < num_nopasr_banks;i++) {
			nopasr_banks[i].inused = check_inused(nopasr_banks[i].start_pfn, nopasr_banks[i].end_pfn, 0, 0);
		}
	}
}

#ifdef CONFIG_MTKPASR_MAFL
/* Test whether it can be removed from buddy temporarily */
static void remove_bank_from_buddy(int bank)
{
	int has_extcomp = mtkpasr_banks[bank].comp_start - mtkpasr_banks[bank].comp_end;
	struct page *spage, *epage;
	struct zone *z;
	unsigned long flags;
	unsigned int order;
	int free_count;
	struct list_head *mafl;
	int *inmafl;

	/* mafl is full */
	inmafl = &mtkpasr_banks[bank].inmafl;
	if (*inmafl == mtkpasr_banks[bank].valid_pages) {
		return;
	}

	/* This bank can't be removed! Don't consider banks with external compression. */
	if (has_extcomp != 0) {
		return;
	}

	spage = pfn_to_page(mtkpasr_banks[bank].start_pfn);
	spage += *inmafl;
	epage = pfn_to_page(mtkpasr_banks[bank].end_pfn);
	z = page_zone(spage);

	/* Lock this zone */
	spin_lock_irqsave(&z->lock, flags);

	/* Check whether remaining pages are in buddy */
	while (spage < epage) {
		/* Not in buddy, exit */
		if (!PageBuddy(spage)) {
			spin_unlock_irqrestore(&z->lock, flags);
			return;
		}
		/* Check next page block */
		free_count = 1 << PAGE_ORDER(spage);
		spage += free_count;
	}

	/* Remove it from buddy to bank's mafl */
	mafl = &mtkpasr_banks[bank].mafl;
	spage = pfn_to_page(mtkpasr_banks[bank].start_pfn);
	spage += *inmafl;
	while (spage < epage) {
		/* Delete it from buddy */
		list_del(&spage->lru);
		order = PAGE_ORDER(spage);
		z->free_area[order].nr_free--;
		/* No removal on page block's order - rmv_PAGE_ORDER(spage); */
		__mod_zone_page_state(z, NR_FREE_PAGES, -(1UL << order));
		/* Add it to mafl */
		list_add_tail(&spage->lru, mafl);
		/* Check next page block */
		free_count = 1 << order;
		spage += free_count;
		/* Update statistics */
		*inmafl += free_count;
		mafl_total_count += free_count;
	}

	/* UnLock this zone */
	spin_unlock_irqrestore(&z->lock, flags);

#ifdef  CONFIG_MTKPASR_DEBUG
	if (mtkpasr_banks[bank].inmafl != mtkpasr_banks[bank].valid_pages) {
		BUG();
	}
#endif
}

#define MTKPASR_EXHAUSTED	0x4		/* # * 2^(MAX_ORDER-1) : 16MB */
static bool mtkpasr_no_exhausted(void)
{
	int order = MAX_ORDER - 1;
	unsigned long free = 0, exhausted_level = MTKPASR_EXHAUSTED;
	struct free_area *area;
	
	for (; order >= 0; --order) {
		/* Go through MTKPASR range */
		area = &(MTKPASR_ZONE->free_area[order]);
		free += area->nr_free;
		/* Early check for whether it is exhausted */
		if (free > exhausted_level) {
			return true;
		}
		/* Shift order */
		free <<= 1;
		exhausted_level <<= 1;
	}

	return false;
}

static struct shrinker mtkpasr_shrinker;
/* Shrinker callback */
static int shrink_mtkpasr_memory(struct shrinker *shrink, struct shrink_control *sc)
{
	int current_bank;
	struct list_head *mafl = NULL;
	struct page *page;
	struct zone *z;
	unsigned long flags;
	unsigned int order;
	int free_count;

	/* Don't apply shrink if it is ZONE_NORMAL - TO CHECK */
	if (gfp_zone(sc->gfp_mask) == ZONE_NORMAL) {
		return 0;
	}

	/* How many pages we can shrink */
	if (sc->nr_to_scan <= 0) {
		/* No pending objs */
		atomic_long_set(&mtkpasr_shrinker.nr_in_batch, 0);
		return mafl_total_count;
	}

	/* Find a non-empty bank */
#ifndef CONFIG_MTKPASR_RDIRECT
	current_bank = num_banks - 1;
#else
	current_bank = 0;
#endif

#ifndef CONFIG_MTKPASR_RDIRECT
	while (current_bank >= 0) {
#else
	while (current_bank < num_banks) {
#endif
		mafl = &mtkpasr_banks[current_bank].mafl;
		if (!list_empty(mafl)) {
			break;
		}
#ifndef CONFIG_MTKPASR_RDIRECT
		--current_bank;
#else
		++current_bank;
#endif
	}

	/* Avoid uninitialized */
	if (mafl == NULL)
		return -1;
	
	/* Putback page blocks to buddy */
	z = page_zone(pfn_to_page(mtkpasr_banks[current_bank].start_pfn));
	
	/* Try lock this zone */
	if (!spin_trylock_irqsave(&z->lock, flags)) {
		return -1;
	}

	/* Test whether mtkpasr is under suitable level */
	if (mtkpasr_no_exhausted()) {
		/* UnLock this zone */
		spin_unlock_irqrestore(&z->lock, flags);
		return -1;
	}

	/* It may be empty here due to page reclaiming! */
	if (list_empty(mafl)) {
		/* UnLock this zone */
		spin_unlock_irqrestore(&z->lock, flags);
		/* Only put one bank to buddy a time! Someone has done it. */
		return -1;
	}

	/* Put the last page block back */
	page = list_entry(mafl->prev, struct page, lru);
	list_del(&page->lru);
	order = PAGE_ORDER(page);
	
	/* Add to tail!! */
	list_add_tail(&page->lru, &z->free_area[order].free_list[MIGRATE_MTKPASR]);
	__mod_zone_page_state(z, NR_FREE_PAGES, 1UL << order);
	z->free_area[order].nr_free++;
	
	/* Update statistics */
	free_count = 1 << order;
	mtkpasr_banks[current_bank].inmafl -= free_count;
	mafl_total_count -= free_count;
	
	/* UnLock this zone */
	spin_unlock_irqrestore(&z->lock, flags);

	/* Sanity check */
	if (mtkpasr_banks[current_bank].inmafl < 0) {
		BUG();
	}

#ifdef  CONFIG_MTKPASR_DEBUG
	if (mafl_total_count >= 0) {
		mtkpasr_info("Remaining MAFL [%ld]!\n",mafl_total_count);
	} else {
		mtkpasr_err("Error : Remaining MAFL [%ld]!\n",mafl_total_count);
	}
#endif

	/* Only put one page block to buddy a time */
	return -1;
}

static struct shrinker mtkpasr_shrinker = {
	.shrink = shrink_mtkpasr_memory,
	.seeks = DEFAULT_SEEKS,
	/* .batch = 0, // by default */
};

/* Register shrinker callback */
void mtkpasr_register_shrinker(void)
{
	register_shrinker(&mtkpasr_shrinker);
}

/* Shrinking mtkpasr_banks[bank]'s mafl totally */
static void shrink_mafl_all(int bank)
{
	struct list_head *mafl = NULL;
	int *inmafl;
	struct page *page, *next;
	struct zone *z;
	unsigned long flags;
	unsigned int order;
#ifdef  CONFIG_MTKPASR_DEBUG
	int free_count = 0;
#endif

	mafl = &mtkpasr_banks[bank].mafl;
	if (list_empty(mafl)) {
		return;
	}

	inmafl = &mtkpasr_banks[bank].inmafl;
	z = page_zone(pfn_to_page(mtkpasr_banks[bank].start_pfn));

	/* Lock this zone */
	spin_lock_irqsave(&z->lock, flags);

	/* Put them back */
	list_for_each_entry_safe(page, next, mafl, lru) {
		list_del(&page->lru);
		order = PAGE_ORDER(page);
		/* Add to tail!! */
		list_add_tail(&page->lru, &z->free_area[order].free_list[MIGRATE_MTKPASR]);
		__mod_zone_page_state(z, NR_FREE_PAGES, 1UL << order);
		z->free_area[order].nr_free++;
#ifdef  CONFIG_MTKPASR_DEBUG
		free_count += (1 << order);
#endif
	}

#ifdef  CONFIG_MTKPASR_DEBUG
	/* Test whether they are equal */
	if (free_count != *inmafl) {
		BUG();
	}
#endif
		
	/* Update statistics */
	mafl_total_count -= *inmafl;
	*inmafl = 0;
	
	/* UnLock this zone */
	spin_unlock_irqrestore(&z->lock, flags);
}

/* Shrink all mtkpasr memory */
void shrink_mtkpasr_all(void)
{
	int i;

	/* Go through all banks */
	for (i=0;i<num_banks;i++) {
		shrink_mafl_all(i);
	}
}

/* Shrink mtkpasr memory from late resume */
void shrink_mtkpasr_late_resume(void)
{
	unsigned long start_bank;
	unsigned long shrink_banks;

	/* Check whether it is an early resume (No MTKPASR is triggered) */
	if (!is_mtkpasr_triggered()) {
		return;
	}

	/* Reset ops invariant */
	mtkpasr_ops_invariant = 0;

	/* Check whether it is empty */
	if (!mafl_total_count) {
		return;
	}

	/* Compute start & amount */
	start_bank = (mtkpasr_total_pfns - mafl_total_count) / pages_per_bank;
	shrink_banks = (mafl_total_count >> 1) / pages_per_bank;

	/* We should shrink at least 1 bank */
	if (!shrink_banks)
		shrink_banks = 1;

	/* Start shrinking */
	shrink_banks = start_bank + shrink_banks;
	for (; start_bank < shrink_banks; start_bank++) {
		shrink_mafl_all(start_bank);
	}

	/* Clear triggered */
	clear_mtkpasr_triggered();
}
#else // CONFIG_MTKPASR_MAFL

void shrink_mtkpasr_all(void) { do {} while(0); }
void shrink_mtkpasr_late_resume(void) { do {} while(0); } 

#endif // CONFIG_MTKPASR_MAFL

/* 
 * Scan Bank Information & disable its SR or DPD full rank if possible. It should be called after syscore_suspend
 * - sr , indicate which segment will be SR-offed.
 *   dpd, indicate which package will be dpd.
 *
 * Return - MTKPASR_WRONG_STATE
 *          MTKPASR_GET_WAKEUP
 * 	    MTKPASR_SUCCESS
 */
enum mtkpasr_phase mtkpasr_disablingSR(u32 *sr, u32 *dpd)
{
	enum mtkpasr_phase result = MTKPASR_SUCCESS;
	int i, j;
	int enter_dpd = 0;
	u32 banksr = 0x0; 	/* From SPM's specification, 0 means SR-on, 1 means SR-off */
	bool keep_ops = true;

	/* Reset SR */
	*sr = banksr;

	/* Sanity Check */
	if (mtkpasr_status != MTKPASR_DISABLINGSR) {
		mtkpasr_err("Error Current State [%d]!\n",mtkpasr_status);
		return MTKPASR_WRONG_STATE;
	}
	
	/* Any incoming wakeup sources? */
	if (CHECK_PENDING_WAKEUP) {
		mtkpasr_log("Pending Wakeup Sources!\n");
		return MTKPASR_GET_WAKEUP;
	}

	/* Scan banks */
	compute_bank_inused(0);
	
	for (i=0;i<num_ranks;i++) {
		mtkpasr_ranks[i].inused = 0;
		for (j=mtkpasr_ranks[i].start_bank;j<=mtkpasr_ranks[i].end_bank;j++) {
			if (mtkpasr_banks[j].inused != 0) {
				++mtkpasr_ranks[i].inused;
			}
		}
	}

	/* Check whether a full rank is cleared */
	for (i=0;i<num_ranks;i++) {
		if (mtkpasr_ranks[i].inused == 0) {
			mtkpasr_info("DPD!\n");
			/* Set MTKPASR_DPDON */
			mtkpasr_ranks[i].inused = MTKPASR_DPDON;
			/* Set all related banks as MTKPASR_RDPDON */
			for (j=mtkpasr_ranks[i].start_bank;j<=mtkpasr_ranks[i].end_bank;j++) {
				mtkpasr_banks[j].inused = MTKPASR_RDPDON;
				/* Disable its SR */
				banksr = banksr | (0x1 << (mtkpasr_banks[j].segment & MTKPASR_SEGMENT_CH0)); 
			}
			enter_dpd = 1;
		}
	}

	/* Check whether "other" banks are cleared */
	for (i=0;i<num_banks;i++) {
		if (mtkpasr_banks[i].inused == 0) {
			/* Set MTKPASR_SROFF */
			mtkpasr_banks[i].inused = MTKPASR_SROFF;
			/* Disable its SR */
			banksr = banksr | (0x1 << (mtkpasr_banks[i].segment & MTKPASR_SEGMENT_CH0));
#ifdef CONFIG_MTKPASR_MAFL
			/* Test whether it can be removed from buddy temporarily */
			remove_bank_from_buddy(i);
#endif
			mtkpasr_info("SPM SR/OFF[%d]!\n",i);
		} else {
			mtkpasr_log("Bank[%d] %s[%d]!\n",i,(mtkpasr_banks[i].inused == MTKPASR_RDPDON)? "RDPDON":"inused",mtkpasr_banks[i].inused);
		}
	
		/* To check whether we should do aggressive PASR SW in the future(no external compression) */
		if (keep_ops) {
			if (mtkpasr_banks[i].comp_pos != 0) {
				keep_ops = false;
			}
		}
	}

	/* Go to MTKPASR_ON state if success */
	if (result == MTKPASR_SUCCESS) {
		mtkpasr_status = enter_dpd? MTKPASR_DPD_ON : MTKPASR_ON;
		*sr = banksr;
	}

#ifdef CONFIG_MTKPASR_MAFL
	/* Update strategy control */
	if (before_mafl_count == mafl_total_count) {	/* Ops-invariant */
		/* It hints not to do ops */
		if (!keep_ops) {
			mtkpasr_ops_invariant = KEEP_NO_OPS;
		}
		/* Check whether it is hard to apply PASR :( */
		if (mtkpasr_ops_invariant != KEEP_NO_OPS) {
			++mtkpasr_ops_invariant;
			if (mtkpasr_ops_invariant > MAX_NO_OPS_INVARIANT) {
				mtkpasr_ops_invariant = MAX_OPS_INVARIANT;
			}
		}
	} else {
		mtkpasr_ops_invariant = 0;
	}
	prev_mafl_count = mafl_total_count;
#endif
	
	mtkpasr_log("Ops_invariant[%u] result [%s] mtkpasr_status [%s]\n",mtkpasr_ops_invariant,
			(result == MTKPASR_SUCCESS)? "MTKPASR_SUCCESS": "MTKPASR_FAIL",
			(enter_dpd == 1)? "MTKPASR_DPD_ON": "MTKPASR_ON");

	return result;
}

#define BANK_INUSED(i)	(mtkpasr_banks[i].inused)
static struct page *compacting_alloc(struct page *migratepage, unsigned long data, int **result)
{
	struct page *page = NULL;

	if (!list_empty(&tolist)) {
		page = list_entry(tolist.next, struct page, lru);
		list_del(&page->lru);
		--tolist_count;
	}

	return page;
}

/* Collect free pages - Is it too long?? */
static int collect_free_pages_for_compacting_banks(struct mtkpasr_bank_cc *bcc)
{
	struct page *page, *end_page;
	struct zone *z;
	unsigned long flags;
	int found;

	/* Sanity check - 20131126 */
	if (bcc->to_cursor == 0) {
		return tolist_count;
	}

	/* We have enough free pages */
	if (tolist_count >= fromlist_count) {
		return tolist_count;
	}

	/* To gather free pages */
	page = pfn_to_page(bcc->to_cursor);
	z = page_zone(page);
	end_page = pfn_to_page(mtkpasr_banks[bcc->to_bank].end_pfn);
	while (page < end_page) {
		/* Lock this zone */
		spin_lock_irqsave(&z->lock, flags);
		/* Find free pages */
		if (!PageBuddy(page)) {
			spin_unlock_irqrestore(&z->lock, flags);
			++page;
			continue;
		}
		found = pasr_find_free_page(page, &tolist);
		/* Unlock this zone */
		spin_unlock_irqrestore(&z->lock, flags);
		/* Update found */
		tolist_count += found;
		page += found;
		/* Update to_cursor & inused */
		bcc->to_cursor += (page - pfn_to_page(bcc->to_cursor));
		BANK_INUSED(bcc->to_bank) += found;
		/* Enough free pages? */
		if (tolist_count >= fromlist_count) {
			break;
		}
		/* Is to bank full? */
		if (BANK_INUSED(bcc->to_bank) == mtkpasr_banks[bcc->to_bank].valid_pages) {
			bcc->to_cursor = 0;
			break;
		}
	}	
	
	if (page == end_page) {
		mtkpasr_info("\"To\" bank[%d] is full!\n",bcc->to_bank);
		bcc->to_cursor = 0;
	}
	
	return tolist_count;
}

/* Release pages from freelist */
static unsigned long putback_free_pages(struct list_head *freelist)
{
	struct page *page, *next;
	unsigned long count = 0;

	list_for_each_entry_safe(page, next, freelist, lru) {
		list_del(&page->lru);
		__free_page(page);
		count++;
	}

	return count;
}

#define COMPACTING_COLLECT()										\
	{												\
		if (collect_free_pages_for_compacting_banks(&bank_cc) >= fromlist_count) { 					\
			if (migrate_pages(&fromlist, compacting_alloc, 0, false, MIGRATE_ASYNC) != 0) {	\
				mtkpasr_log("Bank[%d] can't be cleared!\n",from);			\
				ret = -1;								\
				goto next;								\
			}										\
			/* Migration is done for this batch */						\
			fromlist_count = 0;								\
		} else {										\
			ret = 1;									\
			goto next;									\
		}											\
	}

/* 
 * Migrate pages from "from" to "to"
 * =0, success
 * >0, success but no new free bank generated
 *     (Above two conditions will update cursors.)
 * <0, fail(only due to CHECK_PENDING_WAKEUP)
 */
static int compacting_banks(int from, int to, unsigned long *from_cursor, unsigned long *to_cursor)
{
	struct page *page;
	unsigned long fc = *from_cursor;
	unsigned long ec = mtkpasr_banks[from].end_pfn;
	int to_be_migrated = 0;
	int ret = 0;
	struct mtkpasr_bank_cc bank_cc = {
		.to_bank = to,
		.to_cursor = *to_cursor,
	};

	/* Any incoming wakeup sources? */
	if (CHECK_PENDING_WAKEUP) {
		mtkpasr_log("Pending Wakeup Sources!\n");
		return -EBUSY;
	}
	
	/* Do reset */
	if (list_empty(&fromlist)) {
		fromlist_count = 0;
	}
	if (list_empty(&tolist)) {
		tolist_count = 0;
	}

	/* Migrate MTKPASR_CHECK_MIGRATE pages per batch */
	while (fc < ec) {
		/* Any incoming wakeup sources? */
		if ((fc % MTKPASR_CHECK_ABORTED) == 0) {
			if (CHECK_PENDING_WAKEUP) {
				mtkpasr_log("Pending Wakeup Sources!\n");
				ret = -EBUSY; 
				break; 
			}
		}
		/* Scan inuse pages */
		page = pfn_to_page(fc);
		if (page_count(page) != 0) {
			/* Check whether this page is compressed */
			if(check_if_compressed(mtkpasr_banks[from].comp_start, mtkpasr_banks[from].comp_end, fc)) {
				++fc;
				continue;
			}
			/* To isolate it */
#ifdef CONFIG_MTKPASR_ALLEXTCOMP
			if (!mtkpasr_isolate_page(page, 0x0)) {
#else
			if (!mtkpasr_isolate_page(page)) {
#endif
				list_add(&page->lru, &fromlist);
				++fromlist_count;
				++to_be_migrated;
				--BANK_INUSED(from);
			} else {
				/* This bank can't be cleared! */
				mtkpasr_log("Bank[%d] can't be cleared!\n",from);
				ret = -1;
				break;
			}
		} else {
			++fc;
			continue;
		}
		/* To migrate */
		if ((to_be_migrated % MTKPASR_CHECK_MIGRATE) == 0) {
			if (!list_empty(&fromlist)) {
				COMPACTING_COLLECT();
			}
		}
		/* Is from bank empty (Earlier leaving condition than (fc == ec)) */
		if (BANK_INUSED(from) == 0) {
			fc = 0;
			if (!list_empty(&fromlist)) {
				ret = 1;
			} else {
				/* A new free bank is generated! */
				ret = 0;
			}
			break;
		}
		/* Update fc */
		++fc;
	}

	/* From bank is scanned completely (Should always be false!) */
	if (fc == ec) {
		mtkpasr_err("Should always be false!\n");
		fc = 0;
		if (!list_empty(&fromlist)) {
			ret = 1;
		} else {
			/* A new free bank is generated! */
			ret = 0;
		}
	}

	/* Complete remaining compacting */
	if (ret > 0 && !list_empty(&fromlist)) {
		COMPACTING_COLLECT();
	}

next:
	/* Should we put all pages in fromlist back */
	if (ret == -1) {
		/* We should put all pages from fromlist back */
		putback_lru_pages(&fromlist);
		fromlist_count = 0;								\
		/* We can't clear this bank. Go to the next one! */
		fc = 0;
		ret = 1;
	}

	/* Update cursors */
	*from_cursor = fc;
	*to_cursor = bank_cc.to_cursor;

	return ret;
}

/* Main entry of compacting banks (No compaction on the last one(modem...)) */
static enum mtkpasr_phase mtkpasr_compact_banks(int toget)
{
	int to_be_sorted = num_banks;
	int asort_banks[to_be_sorted];
	int i, j, tmp, ret;
	unsigned long from_cursor = 0, to_cursor = 0;
	enum mtkpasr_phase result = MTKPASR_SUCCESS;
	
	/* Any incoming wakeup sources? */
	if (CHECK_PENDING_WAKEUP) {
		mtkpasr_log("Pending Wakeup Sources!\n");
		return MTKPASR_GET_WAKEUP;
	}

	/* Initialization */
	for (i = 0;i < to_be_sorted;++i) {
		asort_banks[i] = i;
	}
	
	/* Sort banks by inused - ascending */
	for (i = to_be_sorted;i > 1;--i) {
		for (j = 0;j < i-1;++j) {
			if (BANK_INUSED(asort_banks[j]) >= BANK_INUSED(asort_banks[j+1])) {
				tmp = asort_banks[j];
				asort_banks[j] = asort_banks[j+1];
				asort_banks[j+1] = tmp;
			}
		} 
	}
	
#ifdef  CONFIG_MTKPASR_DEBUG 
	for (i = 0;i < to_be_sorted;++i) {
		printk(KERN_ALERT "[%d] - (%d) - inused(%d)\n",i,asort_banks[i],BANK_INUSED(asort_banks[i]));
	}
#endif

	/* Go through banks */
	i = 0;			// from-bank
	j = to_be_sorted - 1;	// to-bank
	while (i < j) {
		/* Whether from-bank is empty */
		if (BANK_INUSED(asort_banks[i]) == 0) {
			++i;
			continue;
		}
		/* Whether to-bank is full */
		if (BANK_INUSED(asort_banks[j]) == mtkpasr_banks[asort_banks[j]].valid_pages) {
			--j;
			continue;
		}
		/* Set compacting position if needed */
		if (!from_cursor) {
			from_cursor = mtkpasr_banks[asort_banks[i]].start_pfn;
#ifdef CONFIG_MTKPASR_MAFL
			from_cursor += mtkpasr_banks[asort_banks[i]].inmafl;
#endif
		}
		if (!to_cursor) {
#ifdef CONFIG_MTKPASR_MAFL
			/* Shrinking (remaining) mafl totally */
			shrink_mafl_all(asort_banks[j]);
#endif
			to_cursor = mtkpasr_banks[asort_banks[j]].start_pfn;
		}
		/* Start compaction on banks */
		ret = compacting_banks(asort_banks[i], asort_banks[j], &from_cursor, &to_cursor);
		if (ret >= 0) {
			if (!from_cursor) {
				++i;
			}
			if (!to_cursor) {
				--j;
			}
			if (!ret) {
				--toget;
			} else {
				continue;
			}
		} else {
			/* Error occurred! */
			mtkpasr_err("Error occurred during the compacting on banks!\n");
			if (ret == -EBUSY) {
				result = MTKPASR_GET_WAKEUP;
			} else {
				result = MTKPASR_FAIL;
#ifdef  CONFIG_MTKPASR_DEBUG
				BUG();
#endif
			}
			break;
		}
		/* Should we stop the compaction! */
		if (!toget) {
			break;
		}
	}

	/* Putback & release pages if needed */
	putback_lru_pages(&fromlist);
	if (putback_free_pages(&tolist) != tolist_count) {
		mtkpasr_err("Should be the same!\n");
	}
	tolist_count = 0;

	return result;
}

/* Apply compaction on banks */
static enum mtkpasr_phase mtkpasr_compact(void)
{
	int i;
	int no_compact = 0;
	int total_free = 0;
	int free_banks;

	/* Scan banks for inused */
	compute_bank_inused(0);
	
	/* Any incoming wakeup sources? */
	if (CHECK_PENDING_WAKEUP) {
		mtkpasr_log("Pending Wakeup Sources!\n");
		return MTKPASR_GET_WAKEUP;
	}

	/* Check whether we should do compaction on banks */
	for (i=0;i<num_banks;++i) {
		/* View invalid pages as inused here! */
		total_free += ((mtkpasr_banks[i].valid_pages << 1) - mtkpasr_banks[i].inused - pages_per_bank);
		if (mtkpasr_banks[i].inused == 0) {
			++no_compact;
		}
	}
	
	/* How many free banks we could get */
	free_banks = total_free / pages_per_bank;
	if (no_compact >= free_banks) {
		mtkpasr_info("No need to do compaction on banks!\n");
		/* No need to do compaction on banks */
		return MTKPASR_SUCCESS;
	}

	/* Actual compaction on banks */
	return mtkpasr_compact_banks(free_banks - no_compact);	
}

/* 
 * Return the number of pages which need to be compressed & do reset.
 *
 * src_map	- store the pages' pfns which need to be compressed.
 * start	- Start PFN of current scanned bank
 * end		- End PFN of current scanned bank
 * sorted	- Buffer for recording which bank(by offset) is externally compressed.
 */
static int pasr_scan_memory(void *src_map, unsigned long start, unsigned long end, unsigned long *sorted)
{
	unsigned long start_pfn;
	unsigned long end_pfn;
	struct page *page;
	unsigned long *pgmap = (unsigned long*)src_map;
	int need_compressed = 0;

	/* Initialize start/end */
	start_pfn = start;
	end_pfn = end;

	/* We don't need to go through following loop because there is no page to be processed. */
	if (start == end) {
		return need_compressed;
	}

	/* Start to scan inuse pages */
	do {
		page = pfn_to_page(start_pfn);
		if (page_count(page) != 0) {
			*pgmap++ = start_pfn;
			++need_compressed;
		}
	} while (++start_pfn < end_pfn);

	/* Clear sorted (we only need to clear (end-start) entries.)*/
	memset(sorted, 0, (end-start)*sizeof(unsigned long));		//memset(sorted, 0, pages_per_bank*sizeof(unsigned long));

	mtkpasr_info("@@@ start_pfn[0x%lx] end_pfn[0x%lx] - to process[%d] @@@\n",start,end,need_compressed);

	return need_compressed;
}

/* Implementation of MTKPASR Direct Compression! DON'T MODIFY IT.  */
#define MTKPASR_DIRECT_COMPRESSION()							\
	{										\
		ret = 0;								\
		if (!trylock_page(page)) {						\
			mtkpasr_err("FL!\n");						\
			ret = -1;							\
			break;								\
		}									\
		if (likely(compressed < MTKPASR_MAX_EXTCOMP)) {				\
			/* Next MTKPASR slot */						\
			current_index = atomic_inc_return(&sloti);			\
			/* Forward page to MTKPASR pool */				\
			ret = mtkpasr_forward_rw(mtkpasr, current_index, page , WRITE);	\
			/* Unsuccessful compression? */					\
			if (unlikely(ret)) {						\
				unlock_page(page);					\
				atomic_dec(&sloti);					\
				mtkpasr_err("FFRW!\n");					\
				break;							\
			}								\
			/* Record the pfn for external compression */			\
			sorted_for_extcomp[page_to_pfn(page)-bank_start_pfn] = 1;	\
			++compressed;							\
		}									\
		unlock_page(page);							\
	}

/* 
 * Drop, Compress, Migration, Compaction
 * 
 * Return - MTKPASR_WRONG_STATE
 *          MTKPASR_GET_WAKEUP
 * 	    MTKPASR_SUCCESS
 */
enum mtkpasr_phase mtkpasr_entering(void)
{
#define SAFE_CONDITION_CHECK() {										\
		/* Is there any incoming ITRs from wake-up sources? If yes, then abort it. */			\
		if (CHECK_PENDING_WAKEUP) {									\
			mtkpasr_log("Pending Wakeup Sources!\n");						\
			ret = -EBUSY;										\
			break;											\
		}												\
		/* Check whether current system is safe! */							\
		if (unlikely(pasr_check_free_safe())) {								\
			mtkpasr_log("Unsafe System Status!\n");							\
			ret = -1;										\
			break;											\
		}												\
	}

	int ret = 0;
	struct mtkpasr *mtkpasr;
	struct page *page;
	int current_bank, current_pos, current_index;
	unsigned long which_pfn, bank_start_pfn, bank_end_pfn;
#ifdef CONFIG_MTKPASR_DEBUG 
	int drop_cnt, to_be_migrated, splitting, no_migrated; 
#endif
	int compressed = 0, val;
	unsigned long *start;
	int pgmi;
	LIST_HEAD(to_migrate);
	LIST_HEAD(batch_to_migrate);
	enum mtkpasr_phase result = MTKPASR_SUCCESS;
	struct zone *zone;
	unsigned long isolated_file, isolated_anon;
	long bias_file, bias_anon;

	/* Sanity Check */
	if (mtkpasr_status != MTKPASR_OFF) {
		mtkpasr_err("Error Current State [%d]!\n",mtkpasr_status);
		return MTKPASR_WRONG_STATE;
	} else {
		/* Go to MTKPASR_ENTERING state */
		mtkpasr_status = MTKPASR_ENTERING;
	}
	
	/* Any incoming wakeup sources? */
	if (CHECK_PENDING_WAKEUP) {
		mtkpasr_log("Pending Wakeup Sources!\n");
		return MTKPASR_GET_WAKEUP;
	}

	/* Reset "CROSS-OPS" variables: extcomp position index, extcomp start & end positions */
	atomic_set(&sloti, -1);
	for (current_bank = 0;current_bank < num_banks;++current_bank) {
		mtkpasr_banks[current_bank].comp_pos = 0;
	}

#ifdef CONFIG_MTKPASR_MAFL
	/* Set for verification of ops-invariant */
	before_mafl_count = mafl_total_count;

	/* Transition-invariant */
	if (prev_mafl_count == mafl_total_count) {	
		if (mtkpasr_no_ops()) {
			/* Go to the next state */
			mtkpasr_status = MTKPASR_DISABLINGSR;
			goto fast_path;
		}
	} else {
		/* Transition is variant. Clear ops-invariant to proceed it.*/
		mtkpasr_ops_invariant = 0;
	}
#endif

	/* Preliminary work before actual PASR SW operation */
	MTKPASR_FLUSH();

	/*****************************/
	/* PASR SW Operation starts! */
	/*****************************/

#ifdef CONFIG_MTKPASR_DEBUG 
	drop_cnt = to_be_migrated = splitting = no_migrated = 0; 
#endif
	
	/* Record original number of zone isolated pages! IMPORTANT! */
	zone = MTKPASR_ZONE;
	isolated_file = zone_page_state(zone, NR_ISOLATED_FILE);
	isolated_anon = zone_page_state(zone, NR_ISOLATED_ANON);
	
	/* Indicate start bank */
#ifndef CONFIG_MTKPASR_RDIRECT
	current_bank = 0;
#else
	current_bank = num_banks - 1;
#endif

	/* Check whether current system is safe! */
	if (unlikely(pasr_check_free_safe())) {
		mtkpasr_log("Unsafe System Status!\n");
		goto no_safe;
	}
	
	/* Current admit order */
	mtkpasr_admit_order = pasr_compute_safe_order();
	mtkpasr_info("mtkpasr_admit_order is [%d]\n",mtkpasr_admit_order);

	/* Main thread is here */
	mtkpasr = &mtkpasr_device[0];

	/* Indicate mtkpasr_last_scan */
	mtkpasr_last_scan = mtkpasr_start_pfn - pageblock_nr_pages;

next_bank:	
	/* Set start pos at extcomp */
	mtkpasr_banks[current_bank].comp_start = (s16)compressed;

	/* Scan MTKPASR-imposed pages */
#ifdef CONFIG_MTKPASR_MAFL
	bank_start_pfn = mtkpasr_banks[current_bank].start_pfn + mtkpasr_banks[current_bank].inmafl;
#else
	bank_start_pfn = mtkpasr_banks[current_bank].start_pfn;
#endif
	bank_end_pfn = mtkpasr_banks[current_bank].end_pfn;
	val = pasr_scan_memory(src_pgmap, bank_start_pfn, bank_end_pfn, sorted_for_extcomp);
	start = src_pgmap;
	
	/* Reset scan index */
	pgmi = -1;

	/* Reset ret!(Important) */
	ret = 0;

	/* Start compression, dropping & migration */
	current_pos = ++pgmi;
	while (current_pos < val) {
		/* Don't process CHECK_PENDING_WAKEUP in every loop(It maybe time-consuming!) */
		/* Don't process pasr_check_free_safe in every loop(It maybe time-consuming!) */
		if ((current_pos % MTKPASR_CHECK_MIGRATE) == 0) {
			SAFE_CONDITION_CHECK();
		}
		/* Query & process */
		page = pfn_to_page(start[current_pos]);
		if (page != NULL) {
			/* To compress: !PageLRU, PageUnevictable, !page_evictable */
			if (!PageLRU(page) || PageUnevictable(page) || !page_evictable(page, NULL)) {
				MTKPASR_DIRECT_COMPRESSION();
				goto next_page;
			}
			/* To drop */
			ret = mtkpasr_drop_page(page);
			if (ret == 0) {
#ifdef CONFIG_MTKPASR_DEBUG 
				++drop_cnt;
#endif
			} else if (ret == -EAGAIN) {
				ret = -1;
				break;
			} else {
				if (unlikely(ret == -EACCES)) {
					/* This kind of pages still in LRU or not evictable! */
					MTKPASR_DIRECT_COMPRESSION();
				} else {
					/* This kind of pages are removed from LRU! Link failedly-dropped pages together & prepare migration! */
					list_add_tail(&page->lru, &to_migrate);
#ifdef CONFIG_MTKPASR_DEBUG 
					++to_be_migrated;
#endif
					/* Reset ret!(Important) */
					ret = 0;
				}
			}
		}
next_page:
		current_pos = ++pgmi;
	}

	/* To migrate */
	while (!ret && !list_empty(&to_migrate)) {
		struct list_head *this, *split_start;
		/* Select MTKPASR_CHECK_MIGRATE pages */
		current_pos = 0;
		split_start = to_migrate.next;
		list_for_each(this, &to_migrate) {
			++current_pos;
			if (current_pos == MTKPASR_CHECK_MIGRATE) {
				break;
			}
		}
		/* Check should be aborted */
		SAFE_CONDITION_CHECK();
		/* Split pages */
		if (current_pos == MTKPASR_CHECK_MIGRATE) {
			to_migrate.next = this->next;
			this->next->prev = &to_migrate;
			batch_to_migrate.next = split_start;
			split_start->prev = &batch_to_migrate;
			batch_to_migrate.prev = this;
			this->next = &batch_to_migrate;
			this = &batch_to_migrate;
		} else {
			this = &to_migrate;
		}
#ifdef CONFIG_MTKPASR_DEBUG
		splitting += current_pos;
#endif
		/* Migrate pages */
		if(migrate_pages(this, mtkpasr_alloc, 0, false, MIGRATE_ASYNC)) {
			/* Failed migration on remaining pages! No list add/remove operations! */
			list_for_each_entry(page, this, lru) {
#ifdef CONFIG_MTKPASR_DEBUG
				++no_migrated;
#endif
				MTKPASR_DIRECT_COMPRESSION();
				/* Clear ret here! No needs to leave this loop due to fail compression */
				ret = 0;
			}
			putback_lru_pages(this);
		}
		/* Check should be aborted */
		SAFE_CONDITION_CHECK();
	}

	/* Put remaining pages back! */
	putback_lru_pages(&batch_to_migrate);
	putback_lru_pages(&to_migrate);

#ifdef CONFIG_MTKPASR_DEBUG
	if (putback_free_pages(&tolist) != tolist_count) {
		mtkpasr_err("Should be the same!\n");
	}
	tolist_count = 0;
#else
	putback_free_pages(&tolist);
#endif
	
	/* Set pos next to the last one at extcomp */
	mtkpasr_banks[current_bank].comp_end = (s16)compressed;
	mtkpasr_info("bank[%d] - comp_start[%d] comp_end[%d]\n",
			current_bank,mtkpasr_banks[current_bank].comp_start,mtkpasr_banks[current_bank].comp_end);

	/* Update extcomp if needed */
	if (mtkpasr_banks[current_bank].comp_start < mtkpasr_banks[current_bank].comp_end) {
		which_pfn = 0;
		bank_end_pfn = bank_end_pfn - bank_start_pfn;
		start = extcomp + mtkpasr_banks[current_bank].comp_start;
		do {
			/* Is this page compressed */
			if (sorted_for_extcomp[which_pfn] == 1) {
				*start++ = which_pfn + bank_start_pfn;
			}
		} while (++which_pfn < bank_end_pfn);

#ifdef CONFIG_MTKPASR_DEBUG 
		/* Sanity check */
		if (start != (extcomp + compressed)) {
			mtkpasr_err("\n\n\n\n\n\n Oh no!\n\n\n\n\n\n");
		}
#endif
	}

	/* Check whether we should go to the next bank */
	/* Because PASR only takes effect on continuous PASR banks, we should add "!ret" to avoid unnecessary works */
#ifndef CONFIG_MTKPASR_RDIRECT
	if (!ret && (++current_bank) < num_banks)
#else
	if (!ret && (--current_bank) >= 0)
#endif
		goto next_bank; 

	/* Updated to be the position next to the last occupied slot */
	atomic_inc(&sloti);

	/* Check MTKPASR result */
	if (ret == -EBUSY) {
		mtkpasr_log("Failed MTKPASR due to pending wakeup source!\n");
		/* Some error handling: It means failed to enter PASR! - Need to enter MTKPASR_RESTORING */
		result = MTKPASR_GET_WAKEUP;
	} else if (ret == -1) {
		mtkpasr_log("Failed compression or no safe amount of free space! Go ahead to SPM!\n");
	}

no_safe:
	/* Go to MTKPASR_DISABLINGSR state if success */
	if (result == MTKPASR_SUCCESS) {
		/* Compaction on banks */
#ifndef CONFIG_MTKPASR_RDIRECT
		if (current_bank < (num_banks - 1))
#else
		if (current_bank > 0)
#endif
			result = mtkpasr_compact();
		/* Successful PASR ops */
		if (result == MTKPASR_SUCCESS) {
			/* Go to the next state */
			mtkpasr_status = MTKPASR_DISABLINGSR;
		}
		/* This should be called whether it is a successful PASR?? IMPORTANT! */
		flush_cache_all();
	}

	/* Recover zone isolate statistics to original ones! IMPORTANT! */
	bias_file = isolated_file - zone_page_state(zone, NR_ISOLATED_FILE);
	bias_anon = isolated_anon - zone_page_state(zone, NR_ISOLATED_ANON);
	mod_zone_page_state(zone, NR_ISOLATED_FILE, bias_file);
	mod_zone_page_state(zone, NR_ISOLATED_ANON, bias_anon);

#ifdef CONFIG_MTKPASR_DEBUG 
	mtkpasr_info("dropped [%d] - compressed [%d] - to_be_migrated [%d] - splitting [%d] - no_migrated [%d]\n"
			,drop_cnt,compressed,to_be_migrated,splitting,no_migrated);
#endif

#ifdef CONFIG_MTKPASR_MAFL
fast_path:
#endif

	mtkpasr_log("result [%s]\n\n",(result == MTKPASR_SUCCESS)? "MTKPASR_SUCCESS": ((result == MTKPASR_FAIL)? "MTKPASR_FAIL": "MTKPASR_GET_WAKEUP"));

	return result;
}

/* Reset */
void __mtkpasr_reset_device(struct mtkpasr *mtkpasr)
{
	mtkpasr->init_done = 0;

	/* Free various per-device buffers */
	if (mtkpasr->compress_workmem != NULL) {
		kfree(mtkpasr->compress_workmem);
		mtkpasr->compress_workmem = NULL;
	}
	if (mtkpasr->compress_buffer != NULL) {
		free_pages((unsigned long)mtkpasr->compress_buffer, 1);
		mtkpasr->compress_buffer = NULL;
	}
}

void mtkpasr_reset_device(struct mtkpasr *mtkpasr)
{
	down_write(&mtkpasr->init_lock);
	__mtkpasr_reset_device(mtkpasr);
	up_write(&mtkpasr->init_lock);
}

void mtkpasr_reset_global(void)
{
	mtkpasr_reset_slots();

	/* Free table */
	kfree(mtkpasr_table);
	mtkpasr_table = NULL;
	mtkpasr_disksize = 0;

	/* Destroy pool */
	zs_destroy_pool(mtkpasr_mem_pool);
	mtkpasr_mem_pool = NULL;
}

int mtkpasr_init_device(struct mtkpasr *mtkpasr)
{
	int ret;

	down_write(&mtkpasr->init_lock);

	if (mtkpasr->init_done) {
		up_write(&mtkpasr->init_lock);
		return 0;
	}

	mtkpasr->compress_workmem = kzalloc(LZO1X_MEM_COMPRESS, GFP_KERNEL);
	if (!mtkpasr->compress_workmem) {
		pr_err("Error allocating compressor working memory!\n");
		ret = -ENOMEM;
		goto fail;
	}

	mtkpasr->compress_buffer = (void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 1);
	if (!mtkpasr->compress_buffer) {
		pr_err("Error allocating compressor buffer space\n");
		ret = -ENOMEM;
		goto fail;
	}

	mtkpasr->init_done = 1;
	up_write(&mtkpasr->init_lock);
	
	pr_debug("Initialization done!\n");
	return 0;

fail:
	__mtkpasr_reset_device(mtkpasr);
	up_write(&mtkpasr->init_lock);
	pr_err("Initialization failed: err=%d\n", ret);
	return ret;
}

/* Adjust 1st rank to exclude Normal zone! */
static void __init mtkpasr_construct_bankrank(void)
{	
	unsigned long rank_start_pfn;
	int i, j, k;
	unsigned long spfn, epfn;

	/******************************/
	/* PASR default imposed range */
	/******************************/

	/* Reset total pfns */
	mtkpasr_total_pfns = 0;

	/* Basic bank & rank information */
	pages_per_bank = (mtkpasr_end_pfn - mtkpasr_start_pfn)/num_banks;
	pages_per_rank = pages_per_bank * banks_per_rank;
	
	/* Compute rank start pfn (Rank-alignment) */
	if (!num_ranks) {
		rank_start_pfn = ULONG_MAX; 
	} else {
		rank_start_pfn = (mtkpasr_start_pfn + pages_per_rank - 1) & ~(pages_per_rank - 1);
	}

	/* Associate banks with ranks */
	for (i = 0, j = 0, k = 0; i < num_banks; i++) {
		mtkpasr_banks[i].start_pfn = mtkpasr_start_pfn + i * pages_per_bank;
		mtkpasr_banks[i].end_pfn = mtkpasr_banks[i].start_pfn + pages_per_bank;
		mtkpasr_banks[i].inused = 0;
		mtkpasr_banks[i].segment = pasr_bank_to_segment(mtkpasr_banks[i].start_pfn, mtkpasr_banks[i].end_pfn); 
		mtkpasr_banks[i].comp_pos = 0;
		/* Set bank's rank */
		if (mtkpasr_banks[i].start_pfn >= rank_start_pfn) {
			mtkpasr_banks[i].rank = &mtkpasr_ranks[j];
			++k;
		} else {
			mtkpasr_banks[i].rank = NULL;			/* No related rank! */
		}
		/* Assign rank range */
		if (k == banks_per_rank) {
			mtkpasr_ranks[j].start_bank = (u16)(i + 1 - banks_per_rank);
			mtkpasr_ranks[j].end_bank = (u16)i;
			/* mtkpasr_ranks[j].hw_rank = 0; */
			mtkpasr_ranks[j++].inused = 0;
			rank_start_pfn += pages_per_rank;
			k = 0;
		}
#ifdef CONFIG_MTKPASR_MAFL
		/* Mark it As Free by removing pages from buddy allocator to its List */
		INIT_LIST_HEAD(&mtkpasr_banks[i].mafl);
		mtkpasr_banks[i].inmafl = 0;
#endif
		/* "Simple" adjustment on banks to exclude invalid PFNs */
		spfn = mtkpasr_banks[i].start_pfn;
		epfn = mtkpasr_banks[i].end_pfn;
		/* Find out the 1st valid PFN */
		while (spfn < epfn) {
			if (pfn_valid(spfn)) {
				mtkpasr_banks[i].start_pfn = spfn++;
				break;
			}
			++spfn;
		}
		/* From spfn, to find out the 1st invalid PFN */
		while (spfn < epfn) {
			if (!pfn_valid(spfn)) {
				mtkpasr_banks[i].end_pfn = spfn;
				break;
			}
			++spfn;
		}
		/* Update valid_pages */
		mtkpasr_banks[i].valid_pages = mtkpasr_banks[i].end_pfn - mtkpasr_banks[i].start_pfn;

		/* To fix mtkpasr_total_pfns(only contain valid pages) */
		mtkpasr_total_pfns += mtkpasr_banks[i].valid_pages;

		mtkpasr_info("Bank[%d] - start_pfn[0x%lx] end_pfn[0x%lx] valid_pages[%u] rank[%p]\n",
				i,mtkpasr_banks[i].start_pfn,mtkpasr_banks[i].end_pfn,mtkpasr_banks[i].valid_pages,mtkpasr_banks[i].rank);
	}

	/**************************/
	/* PASR non-imposed range */
	/**************************/

#ifdef CONFIG_MTKPASR_MAFL
	/* Try to remove some pages from buddy to enhance the PASR performance */
	compute_bank_inused(0);

	/* Reserve half first */
	i = num_banks >> 1;
	for (; i < num_banks; i++) {
		if (mtkpasr_banks[i].inused == 0) {
			remove_bank_from_buddy(i);
		}
	}

	prev_mafl_count = mafl_total_count;
#endif
}

/* mtkpasr initcall */
static int __init mtkpasr_init(void)
{
	int ret;

	/* MTKPASR table of slot information for external compression (MTKPASR_MAX_EXTCOMP) */
	mtkpasr_total_slots = MTKPASR_MAX_EXTCOMP + 1;	// Leave a slot for buffering
	mtkpasr_free_slots = mtkpasr_total_slots;
	mtkpasr_disksize = mtkpasr_total_slots << PAGE_SHIFT;
	mtkpasr_table = kzalloc(mtkpasr_total_slots * sizeof(*mtkpasr_table), GFP_KERNEL);
	if (!mtkpasr_table) {
		mtkpasr_err("Error allocating mtkpasr address table\n");
		ret = -ENOMEM;
		goto fail;
	}

	/* Create MTKPASR mempool. */
	mtkpasr_mem_pool = zs_create_pool("mtkpasr", GFP_ATOMIC|__GFP_HIGHMEM);
	if (!mtkpasr_mem_pool) {
		mtkpasr_err("Error creating memory pool\n");
		ret = -ENOMEM;
		goto no_mem_pool;
	}

	/* We have only 1 mtkpasr device. */
	mtkpasr_device = kzalloc(sizeof(struct mtkpasr), GFP_KERNEL);
	if (!mtkpasr_device) {
		mtkpasr_err("Failed to create mtkpasr_device\n");
		ret = -ENOMEM;
		goto out;
	}

	/* To allocate memory for src_pgmap if needed (corresponding to one bank size) */
	if (src_pgmap == NULL) {
		src_pgmap = (void*)__get_free_pages(GFP_KERNEL, get_order((pasrbank_size >> PAGE_SHIFT)* sizeof(unsigned long)));
		if (src_pgmap == NULL) {
			mtkpasr_err("Failed to allocate (order:?) memory!\n");
			ret = -ENOMEM;
			goto free_devices;
		}
	}

	/* To allocate memory for keeping external compression information */
	if (extcomp == NULL) {
		extcomp = (unsigned long*)__get_free_pages(GFP_KERNEL, get_order(MTKPASR_MAX_EXTCOMP * sizeof(unsigned long)));
		if (extcomp == NULL) {
			mtkpasr_err("Failed to allocate memory for extcomp!\n");
			ret = -ENOMEM;
			goto no_memory;
		}
		sorted_for_extcomp = (unsigned long*)__get_free_pages(GFP_KERNEL, get_order((pasrbank_size >> PAGE_SHIFT)* sizeof(unsigned long)));
		if (sorted_for_extcomp == NULL) {
			free_pages((unsigned long)extcomp, get_order(MTKPASR_MAX_EXTCOMP * sizeof(unsigned long)));
			mtkpasr_err("Failed to allocate memory for extcomp!\n");
			ret = -ENOMEM;
			goto no_memory;
		}
	}

	/* Basic initialization */
	init_rwsem(&mtkpasr_device->init_lock);
	spin_lock_init(&mtkpasr_device->stat64_lock);
	mtkpasr_device->init_done = 0;

	/* Create working buffers */
	ret = mtkpasr_init_device(mtkpasr_device);
	if (ret < 0) {
		mtkpasr_err("Failed to initialize mtkpasr device\n");
		goto reset_devices;
	}
	
	/* Create SYSFS interface */
	ret = sysfs_create_group(power_kobj, &mtkpasr_attr_group);
	if (ret < 0) {
		mtkpasr_err("Error creating sysfs group\n");
		goto reset_devices;
	}
	
	/* Construct memory rank & bank information */
	num_banks = compute_valid_pasr_range(&mtkpasr_start_pfn, &mtkpasr_end_pfn, &banks_per_rank, &num_ranks);
	if (num_banks < 0) {
		mtkpasr_err("No valid PASR range!\n");
		ret = -EINVAL;
		goto free_banks_ranks;
	}
	mtkpasr_total_pfns = mtkpasr_end_pfn - mtkpasr_start_pfn;
	mtkpasr_banks = kzalloc(num_banks * sizeof(struct mtkpasr_bank), GFP_KERNEL);
	if (!mtkpasr_banks) {
		mtkpasr_err("Error allocating mtkpasr banks information!\n");
		ret = -ENOMEM;
		goto free_banks_ranks;
	}
	mtkpasr_ranks = kzalloc(num_ranks * sizeof(struct mtkpasr_rank), GFP_KERNEL);
	if (!mtkpasr_ranks) {
		mtkpasr_err("Error allocating mtkpasr ranks information!\n");
		ret = -ENOMEM;
		goto free_banks_ranks;
	}
	mtkpasr_construct_bankrank();

	/* Indicate migration end */
	mtkpasr_migration_end = NODE_DATA(0)->node_start_pfn + pages_per_bank;

#if defined(CONFIG_MTKPASR_MAFL)
	mtkpasr_register_shrinker();
#endif

	mtkpasr_info("num_banks[%d] num_ranks[%d] mtkpasr_start_pfn[%ld] mtkpasr_end_pfn[%ld] mtkpasr_total_pfns[%ld] banks_per_rank[%d]\n",
			num_banks,num_ranks,mtkpasr_start_pfn,mtkpasr_end_pfn,mtkpasr_total_pfns,banks_per_rank);

	/* Setup others */
	nz = &NODE_DATA(0)->node_zones[ZONE_NORMAL];
	safe_level = low_wmark_pages(nz);
#ifdef CONFIG_HIGHMEM	
	safe_level += nz->lowmem_reserve[ZONE_HIGHMEM];
#endif

	return 0;

free_banks_ranks:
	if (mtkpasr_banks != NULL) {
		kfree(mtkpasr_banks);
		mtkpasr_banks = NULL;
	}
	if (mtkpasr_ranks != NULL) {
		kfree(mtkpasr_ranks);
		mtkpasr_ranks = NULL;
	}
	sysfs_remove_group(power_kobj, &mtkpasr_attr_group);

reset_devices:
	mtkpasr_reset_device(mtkpasr_device);
	free_pages((unsigned long)extcomp, get_order(MTKPASR_MAX_EXTCOMP * sizeof(unsigned long)));
	free_pages((unsigned long)sorted_for_extcomp, get_order((pasrbank_size >> PAGE_SHIFT)* sizeof(unsigned long)));

no_memory:
	free_pages((unsigned long)src_pgmap, get_order((pasrbank_size >> PAGE_SHIFT)* sizeof(unsigned long)));

free_devices:
	kfree(mtkpasr_device);

out:
	zs_destroy_pool(mtkpasr_mem_pool);

no_mem_pool:
	kfree(mtkpasr_table);
	mtkpasr_table = NULL;
	mtkpasr_disksize = 0;

fail:
	/* Disable MTKPASR */
	mtkpasr_enable = 0;
	mtkpasr_enable_sr = 0;
	num_banks = 0;

	return ret;
}

static void __exit mtkpasr_exit(void)
{
	sysfs_remove_group(power_kobj, &mtkpasr_attr_group);
	if (mtkpasr_device->init_done)
		mtkpasr_reset_device(mtkpasr_device);

	mtkpasr_reset_global();

	kfree(mtkpasr_device);
	pr_debug("Cleanup done!\n");
}

//module_init(mtkpasr_init);
subsys_initcall_sync(mtkpasr_init);
module_exit(mtkpasr_exit);

MODULE_AUTHOR("Chinwen Change <chinwen.chang@mediatek.com>");
MODULE_DESCRIPTION("MTK proprietary PASR driver");
