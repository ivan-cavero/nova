/* Physical Memory Manager — Nova kernel Phase 2 */
/* Stack-based: O(1) alloc/free, cache-hot frame numbers */

#include "mm.h"
#include "io.h"
#include "attributes.h"

/* PMM state — global singleton */
pmm_t pmm;

/* Read index for O(1) alloc-from-bottom */
static uint32_t pmm_alloc_idx = 0;

/* The PMM stack sits right after kernel BSS */
/* Max 16384 pages × 4 bytes = 64 KB */
static uint32_t pmm_stack[PMM_MAX_PAGES] __attribute__((section(".bss")));

/* ==================== Init ==================== */

void pmm_init(void) {
    log_entry_t e = log_begin("Initializing PMM");

    /* Get kernel boundaries from linker */
    uint32_t kernel_start = (uint32_t)&_kernel_start;   /* 0x100000 */
    uint32_t kernel_end   = (uint32_t)&_kernel_end;

    /* Total memory: 64MB in QEMU (configurable) */
    uint32_t total_memory = MEM_TOTAL;
    uint32_t total_pages  = total_memory / PAGE_SIZE;

    /* Stack setup */
    pmm.stack           = pmm_stack;
    pmm.stack_capacity  = PMM_MAX_PAGES;
    pmm.stack_top       = 0;
    pmm.total_pages     = total_pages;
    pmm.free_pages      = 0;

    /* Mark all pages as used initially, then free usable ones */
    /* Strategy: everything starts reserved, we explicitly free safe regions */

    /* Free pages from kernel_end to end of RAM */
    uint32_t free_start = kernel_end;
    uint32_t free_end   = total_memory;

    /* Align to page boundary */
    free_start = (free_start + PAGE_SIZE - 1) & (uint32_t)(~(uint32_t)(PAGE_SIZE - 1));

    for (uint32_t addr = free_start; addr < free_end; addr += PAGE_SIZE) {
        uint32_t pfn = addr / PAGE_SIZE;

        /* Skip if in reserved hole (first 1MB) */
        if (addr < MEM_HOLE_END) continue;

        /* Skip if within kernel image (already accounted for) */
        if (addr >= kernel_start && addr < kernel_end) continue;

        /* This page is usable */
        if (pmm.stack_top < pmm.stack_capacity) {
            pmm.stack[pmm.stack_top++] = pfn;
            pmm.free_pages++;
        }
    }

    /* Reset alloc index */
    pmm_alloc_idx = 0;

    log_end(&e, true);
}

/* ==================== Alloc ==================== */

void *pmm_alloc(void) {
    if (pmm_alloc_idx >= pmm.stack_top) {
        return NULL;  /* Out of memory */
    }

    /* Pop from BOTTOM → lowest free addresses first.
     * This keeps allocations near the kernel in identity-mapped 4MB. */
    uint32_t pfn = pmm.stack[pmm_alloc_idx++];
    pmm.free_pages--;

    /* Zero the page */
    uint32_t *page = (uint32_t *)(pfn * PAGE_SIZE);
    for (uint32_t i = 0; i < PAGE_SIZE / sizeof(uint32_t); i++) {
        page[i] = 0;
    }

    return (void *)page;
}

/* ==================== Alloc + Zero ==================== */

void *pmm_allocz(void) {
    /* pmm_alloc already zeros, so this is just alias */
    return pmm_alloc();
}

/* ==================== Free ==================== */

void pmm_free(void *page) {
    if (page == NULL) return;
    if (pmm.stack_top >= pmm.stack_capacity) return;  /* Stack full */

    uint32_t addr = (uint32_t)page;
    uint32_t pfn  = addr / PAGE_SIZE;

    pmm.stack[pmm.stack_top++] = pfn;
    pmm.free_pages++;
}

/* ==================== Query ==================== */

uint32_t pmm_free_count(void) {
    return pmm.free_pages;
}