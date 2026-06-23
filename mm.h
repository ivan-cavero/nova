/* Memory Management — Nova kernel Phase 2 */
/* PMM (stack-based) + VMM (2-level paging, recursive map) */

#ifndef MM_H
#define MM_H

#include <stdint.h>
#include <stdbool.h>
#include "attributes.h"

/* ==================== Constants ==================== */
#define PAGE_SIZE       4096        /* 4 KB per page */
#define PAGE_SHIFT      12
#define PAGE_MASK       (PAGE_SIZE - 1)

/* Memory hole: first 1MB is reserved (BIOS, VGA, EBDA) */
#define MEM_HOLE_END    0x00100000  /* 1 MB */

/* QEMU default RAM (can be overridden with -m) */
#define MEM_TOTAL       (64UL * 1024 * 1024)  /* 64 MB */

/* ==================== PMM — Physical Memory Manager ==================== */

/* Stack-based: O(1) alloc/free */
#define PMM_MAX_PAGES   16384       /* 64 MB / 4 KB */

typedef struct {
    uint32_t *stack;                /* Array of free frame numbers */
    uint32_t  stack_capacity;       /* Max entries */
    uint32_t  stack_top;            /* Current top (0 = empty) */
    uint32_t  total_pages;
    uint32_t  free_pages;
} pmm_t;

extern pmm_t pmm;

void  pmm_init(void);
void *pmm_alloc(void);              /* Returns 4KB-aligned page, zeroed */
void  pmm_free(void *page);         /* Returns page to pool */
void *pmm_allocz(void);             /* Alloc + zero */
uint32_t pmm_free_count(void);

/* ==================== VMM — Virtual Memory Manager ==================== */

/* Page directory entry flags (32-bit) */
#define PDE_PRESENT     (1U << 0)
#define PDE_WRITABLE    (1U << 1)
#define PDE_USER        (1U << 2)
#define PDE_WRITETHROUGH (1U << 3)
#define PDE_CACHE_DIS   (1U << 4)
#define PDE_ACCESSED    (1U << 5)
#define PDE_DIRTY       (1U << 6)
#define PDE_PS          (1U << 7)   /* Page size: 4MB page */
#define PDE_GLOBAL      (1U << 8)

/* Page table entry flags (32-bit) */
#define PTE_PRESENT     (1U << 0)
#define PTE_WRITABLE    (1U << 1)
#define PTE_USER        (1U << 2)
#define PTE_WRITETHROUGH (1U << 3)
#define PTE_CACHE_DIS   (1U << 4)
#define PTE_ACCESSED    (1U << 5)
#define PTE_DIRTY       (1U << 6)
#define PTE_PAT         (1U << 7)
#define PTE_GLOBAL      (1U << 8)

/* Recursive mapping: PD[1023] points to PD itself */
#define PD_INDEX(virt)  (((uint32_t)(virt) >> 22) & 0x3FF)
#define PT_INDEX(virt)  (((uint32_t)(virt) >> 12) & 0x3FF)

/* Virtual address of recursive PD entry (accessed as PT) */
#define PD_RECURSIVE    (0xFFFFF000) /* PD[1023] → PD */

/* Virtual address window for page table access */
#define PT_WINDOW       (0xFFC00000) /* 4MB: all 1024 PTs accessible */

#define PAGING_FLAG     (1U << 31)  /* CR0.PG bit */

/* Page directory: 1024 entries × 4 bytes = 4KB */
typedef uint32_t page_dir_t[1024] ALIGNED(PAGE_SIZE);
typedef uint32_t page_table_t[1024] ALIGNED(PAGE_SIZE);

/* VMM API */
void  vmm_init(void);
bool  vmm_map(uint32_t virt, uint32_t phys, uint32_t flags);
bool  vmm_unmap(uint32_t virt);
uint32_t vmm_get_phys(uint32_t virt);
void  vmm_enable(void);

/* Extern kernel boundaries from linker */
extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

#endif /* MM_H */