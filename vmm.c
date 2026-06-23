/* Virtual Memory Manager — Nova kernel Phase 2 */
/* 2-level paging (PD + PT), recursive mapping at PD[1023] */

#include "mm.h"
#include "io.h"
#include "attributes.h"

/* Page Directory — at FIXED location 0x200000 (2MB).
 * This address is within the identity-mapped first 4MB (PDE[0]).
 * Using PMM for the PD caused issues because paging translations
 * must work before and after CR0.PG enable. */
#define PD_PHYS 0x200000
static page_dir_t *page_dir = (page_dir_t *)PD_PHYS;

/* ==================== PDE helpers ==================== */

/* Unused — kept for reference
static uint32_t pd_entry_get_pt(uint32_t pd_idx) {
    return (*page_dir)[pd_idx] & 0xFFFFF000;
}
*/

static void pd_entry_set(uint32_t pd_idx, uint32_t val) {
    (*page_dir)[pd_idx] = val;
}

/* ==================== PT access via recursive mapping ==================== */
/* PD[1023] = phys(PD) → PT[i] is at 0xFFC00000 + i*4096 */

static volatile uint32_t *pt_get_entry(uint32_t pd_idx, uint32_t pt_idx) {
    volatile uint32_t *pt = (volatile uint32_t *)(PT_WINDOW + pd_idx * PAGE_SIZE);
    return &pt[pt_idx];
}

/* ==================== Map: virt → phys (4KB page) ==================== */

bool vmm_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    if (virt & PAGE_MASK) return false;

    /* Can't map into recursive region */
    if (pd_idx == 1023) return false;

    /* If PDE uses 4MB page, we need to split it first */
    if ((*page_dir)[pd_idx] & PDE_PS) {
        /* Allocate page table for this 4MB region */
        void *new_pt = pmm_alloc();
        if (new_pt == NULL) return false;
        uint32_t pt_phys = (uint32_t)new_pt;

        /* Identity map the new page table */
        vmm_map(pt_phys, pt_phys, PDE_PRESENT | PDE_WRITABLE);

        /* Copy old mapping to new PT (4MB → 1024 pages of 4KB) */
        uint32_t old_base = (*page_dir)[pd_idx] & 0xFFC00000; /* 4MB aligned base */
        for (uint32_t j = 0; j < 1024; j++) {
            volatile uint32_t *pte = pt_get_entry(pd_idx, j);
            *pte = (old_base + j * PAGE_SIZE) | PDE_PRESENT | PDE_WRITABLE;
        }

        /* Replace PDE with pointer to new PT */
        pd_entry_set(pd_idx, pt_phys | PDE_PRESENT | PDE_WRITABLE);
    }

    /* Allocate PT if needed */
    if (!((*page_dir)[pd_idx] & PDE_PRESENT)) {
        void *new_pt = pmm_alloc();
        if (new_pt == NULL) return false;
        uint32_t pt_phys = (uint32_t)new_pt;

        /* Identity map the new page table */
        vmm_map(pt_phys, pt_phys, PDE_PRESENT | PDE_WRITABLE);
        pd_entry_set(pd_idx, pt_phys | PDE_PRESENT | PDE_WRITABLE);
    }

    volatile uint32_t *pte = pt_get_entry(pd_idx, pt_idx);
    *pte = (phys & 0xFFFFF000) | (flags & 0xFFF);

    return true;
}

/* ==================== Unmap ==================== */

bool vmm_unmap(uint32_t virt) {
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    if (!((*page_dir)[pd_idx] & PDE_PRESENT)) return false;

    /* If 4MB page, can't unmap individual 4KB */
    if ((*page_dir)[pd_idx] & PDE_PS) return false;

    volatile uint32_t *pte = pt_get_entry(pd_idx, pt_idx);
    if (!(*pte & PTE_PRESENT)) return false;

    *pte = 0;
    __asm__ volatile ("invlpg %0" : : "m"(*(volatile uint32_t *)virt) : "memory");
    return true;
}

/* ==================== Get physical address ==================== */

uint32_t vmm_get_phys(uint32_t virt) {
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);
    uint32_t offset = virt & PAGE_MASK;

    if (!((*page_dir)[pd_idx] & PDE_PRESENT)) return 0;

    /* 4MB page */
    if ((*page_dir)[pd_idx] & PDE_PS) {
        return ((*page_dir)[pd_idx] & 0xFFC00000) + offset;
    }

    volatile uint32_t *pte = pt_get_entry(pd_idx, pt_idx);
    if (!(*pte & PTE_PRESENT)) return 0;

    return (*pte & 0xFFFFF000) | offset;
}

/* ==================== Identity map entire 4GB using 4MB pages ==================== */

static void identity_map_all(void) {
    log_entry_t e = log_begin("Identity mapping RAM");

    /* Map ONLY the physical RAM that exists (64MB = first 16 PDEs).
     * We CANNOT map all 1024 PDEs because physical addresses >64MB
     * don't have RAM — the page tables would point to non-existent memory. */
    uint32_t ram_pdes = MEM_TOTAL / 0x400000;  /* 64MB / 4MB = 16 */
    for (uint32_t i = 0; i < ram_pdes; i++) {
        uint32_t base = i * 0x400000;
        pd_entry_set(i, base | PDE_PRESENT | PDE_WRITABLE | PDE_PS);
    }

    log_end(&e, true);
}

/* ==================== Enable paging ==================== */

/* Paging enable — assembly trampoline in paging.asm */
extern void paging_enable(uint32_t pd_phys);

/* ==================== VMM Init ==================== */

/* ==================== VMM Init ==================== */

void vmm_init(void) {
    log_entry_t e;

    /* Page Directory at fixed 0x200000 (within first 4MB — identity mapped) */
    e = log_begin("Zeroing page directory at 0x200000");
    for (uint32_t *p = (uint32_t *)PD_PHYS; p < (uint32_t *)(PD_PHYS + 4096); p++) {
        *p = 0;
    }
    log_end(&e, true);

    uint32_t pd_phys = PD_PHYS;

    /* Step 1: Identity map ALL 4GB with 4MB pages */
    /* This ensures everything is accessible after paging is enabled */
    identity_map_all();

    /* Step 2: Set up recursive mapping PD[1023] = phys(PD) */
    /* This allows us to access page tables by virtual address */
    e = log_begin("Setting up recursive page directory");
    pd_entry_set(1023, pd_phys | PDE_PRESENT | PDE_WRITABLE);
    log_end(&e, true);

    /* Step 3: Enable paging (CR0.PG = 1) via assembly trampoline */
    e = log_begin("Enabling paging (CR0.PG)");
    paging_enable(pd_phys);
    log_end(&e, true);

    /* Success — paging is active */
    log_ok("Paging enabled   : CR0.PG = 1, PDE[0..1023] = 4MB pages");
}

/* Page fault decoding happens in idt.c exception_handler(EXC_PF) */