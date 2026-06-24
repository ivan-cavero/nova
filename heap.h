/* Heap — Slab allocator — Nova kernel Phase 3 */
/* Fixed-size slab caches (16B..4KB) + PMM fallback for large allocs */

#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ==================== Slab Size Classes ==================== */
/* Each is a power of 2, fitting exactly into 4KB pages */
#define SLAB_MIN        16
#define SLAB_MAX        4096
#define SLAB_COUNT      9       /* 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 */

/* ==================== Opaque types ==================== */
/* slab_cache_t is defined in heap.c — we only need a forward decl */
typedef struct slab_cache slab_cache_t;

/* ==================== Public API ==================== */

/* Initialize the heap — must be called once after paging is enabled */
void heap_init(void);

/* Allocate 'size' bytes. Returns 4-byte aligned pointer, or NULL on OOM. */
void *kmalloc(size_t size);

/* Free a pointer returned by kmalloc */
void kfree(void *ptr);

/* Reallocate: change size, preserving content */
void *krealloc(void *ptr, size_t new_size);

/* Allocate zeroed memory */
void *kcalloc(size_t num, size_t size);

/* Debug: print slab cache statistics to VGA + serial */
void heap_dump_stats(void);

#endif /* HEAP_H */