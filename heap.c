/* Heap — Multi-page Slab Allocator — Nova kernel Phase 3 */
/* 
 * Design:
 *   - 9 slab caches: 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 bytes
 *   - Each cache maintains a linked list of slab pages
 *   - When a slab is full, allocate another page from PMM
 *   - Free list stored WITHIN freed objects (zero overhead)
 *   - kmalloc rounds UP to next slab size
 *   - kmalloc > 4096: allocates multiple pages from PMM (buddy-style)
 */

#include "heap.h"
#include "mm.h"
#include "io.h"
#include "attributes.h"

#define SLAB_PAGE_SIZE  4096
#define FREE_SENTINEL   0xFFFFFFFF

/* ==================== Per-Page Slab Header ==================== */
/* Stored at the START of each slab page (before any objects) */
typedef struct slab_page {
    struct slab_page *next;         /* Next slab in cache (NULL = last) */
    uint32_t          first_free;   /* Index of first free object */
    uint32_t          total_free;   /* Free count in this slab page */
    uint32_t          total_objects;/* Total objects in this page */
} slab_page_t;

/* How many bytes the slab header occupies */
#define SLAB_HEADER_SIZE  sizeof(slab_page_t)

/* ==================== Slab Cache ==================== */
typedef struct slab_cache {
    size_t        object_size;
    slab_page_t   *pages;           /* Linked list of slab pages */
    uint32_t      objects_per_page; /* Objects per 4KB page (minus header) */
} slab_cache_t;

/* One cache per size class */
static slab_cache_t slab_caches[SLAB_COUNT];

/* Size table: 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 */
static const size_t slab_sizes[SLAB_COUNT] = {
    16, 32, 64, 128, 256, 512, 1024, 2048, 4096
};

/* ==================== Slab Index Lookup ==================== */

static int slab_index_for(size_t size) {
    if (size == 0) size = 1;
    for (int i = 0; i < SLAB_COUNT; i++) {
        if (size <= slab_sizes[i]) return i;
    }
    return -1;
}

/* ==================== Slab Page Alloc ==================== */

static slab_page_t *slab_page_alloc(slab_cache_t *cache) {
    /* If no objects fit per page, can't use slab for this size */
    if (cache->objects_per_page == 0) return NULL;

    void *page = pmm_alloc();

    slab_page_t *sp = (slab_page_t *)page;
    sp->next = NULL;

    /* Available bytes per page (after header) */
    size_t available = SLAB_PAGE_SIZE - SLAB_HEADER_SIZE;
    sp->total_objects = (uint32_t)(available / cache->object_size);
    sp->total_free    = sp->total_objects;

    /* Build free list within objects */
    uint8_t *base = (uint8_t *)page + SLAB_HEADER_SIZE;
    for (uint32_t i = 0; i < sp->total_objects - 1; i++) {
        uint32_t *next = (uint32_t *)(base + i * cache->object_size);
        *next = i + 1;
    }
    uint32_t *last = (uint32_t *)(base + (sp->total_objects - 1) * cache->object_size);
    *last = FREE_SENTINEL;

    sp->first_free = 0;

    return sp;
}

/* ==================== Slab Cache Init ==================== */

static void slab_cache_init(slab_cache_t *cache, size_t obj_size) {
    cache->object_size      = obj_size;
    cache->pages            = NULL;
    cache->objects_per_page = 0;

    /* Calculate objects per page (accounting for header) */
    size_t available = SLAB_PAGE_SIZE - SLAB_HEADER_SIZE;
    uint32_t objs = (uint32_t)(available / obj_size);

    /* If no objects fit (e.g., 4096 bytes with 16-byte header = 4080 < 4096),
     * skip slab creation — kmalloc will fall back to page_alloc for this size */
    if (objs == 0) return;

    cache->objects_per_page = objs;

    /* Pre-allocate first page */
    slab_page_t *first = slab_page_alloc(cache);
    if (first) cache->pages = first;
}

/* ==================== Slab Alloc ==================== */

static void *slab_alloc_from(slab_cache_t *cache) {
    slab_page_t *sp = cache->pages;

    /* Walk pages to find one with free space */
    while (sp) {
        if (sp->first_free != FREE_SENTINEL && sp->total_free > 0) {
            /* Found a page with space */
            uint32_t idx = sp->first_free;
            uint8_t  *obj = (uint8_t *)sp + SLAB_HEADER_SIZE + idx * cache->object_size;

            /* Read next free index from the object itself */
            sp->first_free = *(uint32_t *)obj;
            sp->total_free--;

            return (void *)obj;
        }
        sp = sp->next;
    }

    /* All pages are full — allocate a new one */
    slab_page_t *new_sp = slab_page_alloc(cache);
    if (new_sp == NULL) return NULL;

    /* Prepend to list */
    new_sp->next = cache->pages;
    cache->pages = new_sp;

    /* Allocate from the new page */
    uint32_t idx = new_sp->first_free;
    uint8_t *obj = (uint8_t *)new_sp + SLAB_HEADER_SIZE + idx * cache->object_size;
    new_sp->first_free = *(uint32_t *)obj;
    new_sp->total_free--;

    return (void *)obj;
}

/* ==================== Slab Free ==================== */

static void slab_free_to(slab_cache_t *cache, void *ptr) {
    /* Find which slab page this pointer belongs to */
    slab_page_t *sp = cache->pages;
    while (sp) {
        uint8_t *start = (uint8_t *)sp;
        uint8_t *end   = start + SLAB_PAGE_SIZE;

        if ((uint8_t *)ptr >= start && (uint8_t *)ptr < end) {
            uint32_t offset = (uint32_t)((uint8_t *)ptr - (start + SLAB_HEADER_SIZE));
            uint32_t idx = offset / cache->object_size;

            if (idx >= sp->total_objects) return;  /* Invalid */

            /* Store current first_free in this object, then update */
            *(uint32_t *)ptr = sp->first_free;
            sp->first_free = idx;
            sp->total_free++;
            return;
        }
        sp = sp->next;
    }
}

/* ==================== Page Alloc (for > 4096 bytes) ==================== */

static void *page_alloc(size_t size) {
    uint32_t num_pages = (uint32_t)((size + SLAB_PAGE_SIZE - 1) / SLAB_PAGE_SIZE);

    uint32_t *first_page = (uint32_t *)pmm_alloc();
    if (first_page == NULL) return NULL;
    first_page[0] = num_pages;

    for (uint32_t i = 1; i < num_pages; i++) {
        void *p = pmm_alloc();
        if (p == NULL) {
            for (uint32_t j = 0; j < i; j++)
                pmm_free((void *)((uint32_t)first_page + j * SLAB_PAGE_SIZE));
            return NULL;
        }
    }
    return (void *)((uint8_t *)first_page + sizeof(uint32_t));
}

static void page_free(void *ptr) {
    uint32_t *meta = (uint32_t *)((uint8_t *)ptr - sizeof(uint32_t));
    uint32_t n = meta[0];
    uint32_t base = (uint32_t)meta;
    for (uint32_t i = 0; i < n; i++)
        pmm_free((void *)(base + i * SLAB_PAGE_SIZE));
}

/* ==================== Public API ==================== */

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    if (size <= SLAB_MAX) {
        int idx = slab_index_for(size);
        if (idx < 0) goto large;
        void *p = slab_alloc_from(&slab_caches[idx]);
        if (p) return p;
        /* Slab alloc failed (e.g., size too large for slab page) */
    }
large:
    return page_alloc(size);
}

void kfree(void *ptr) {
    if (ptr == NULL) return;
    for (int i = 0; i < SLAB_COUNT; i++) {
        slab_cache_t *cache = &slab_caches[i];
        if (cache->pages == NULL) continue;

        slab_page_t *sp = cache->pages;
        while (sp) {
            uint8_t *start = (uint8_t *)sp;
            uint8_t *end   = start + SLAB_PAGE_SIZE;
            if ((uint8_t *)ptr >= start && (uint8_t *)ptr < end) {
                slab_free_to(cache, ptr);
                return;
            }
            sp = sp->next;
        }
    }
    page_free(ptr);
}

void *krealloc(void *ptr, size_t new_size) {
    if (ptr == NULL) return kmalloc(new_size);
    if (new_size == 0) { kfree(ptr); return NULL; }

    /* Find which slab cache */
    for (int i = 0; i < SLAB_COUNT; i++) {
        slab_cache_t *cache = &slab_caches[i];
        if (cache->pages == NULL) continue;
        slab_page_t *sp = cache->pages;
        while (sp) {
            uint8_t *s = (uint8_t *)sp, *e = s + SLAB_PAGE_SIZE;
            if ((uint8_t *)ptr >= s && (uint8_t *)ptr < e) {
                size_t old_size = cache->object_size;
                if (new_size <= old_size) return ptr;
                void *new_ptr = kmalloc(new_size);
                if (!new_ptr) return NULL;
                for (size_t j = 0; j < old_size; j++)
                    ((uint8_t *)new_ptr)[j] = ((uint8_t *)ptr)[j];
                slab_free_to(cache, ptr);
                return new_ptr;
            }
            sp = sp->next;
        }
    }
    void *new_ptr = kmalloc(new_size);
    if (!new_ptr) return NULL;
    for (size_t j = 0; j < new_size; j++)
        ((uint8_t *)new_ptr)[j] = ((uint8_t *)ptr)[j];
    page_free(ptr);
    return new_ptr;
}

void *kcalloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = kmalloc(total);
    if (!ptr) return NULL;
    uint8_t *p = (uint8_t *)ptr;
    for (size_t i = 0; i < total; i++) p[i] = 0;
    return ptr;
}

void heap_init(void) {
    vga_writestr("  [..]  Initializing heap... ");
    for (int i = 0; i < SLAB_COUNT; i++)
        slab_cache_init(&slab_caches[i], slab_sizes[i]);
    vga_setcolor(CLR_LBLUE, CLR_BLACK);
    vga_writestr("[OK]\n");
    vga_setcolor(CLR_LGREY, CLR_BLACK);
    serial_writestr("  [..]  Initializing heap... [OK]\n");
}

void heap_dump_stats(void) {
    vga_writestr("\n  --- Heap ---\n");
    serial_writestr("\n--- Heap ---\n");
    for (int i = 0; i < SLAB_COUNT; i++) {
        slab_cache_t *c = &slab_caches[i];
        uint32_t pages = 0, free = 0, total = 0;
        slab_page_t *sp = c->pages;
        while (sp) { pages++; free += sp->total_free; total += sp->total_objects; sp = sp->next; }
        
        /* VGA */
        vga_writestr("  Size ");
        { char b[12]; int x = 10; uint64_t v = c->object_size; b[11]=0;
          if (!v) b[--x]='0'; while(v&&x>0){b[--x]=(char)('0'+v%10);v/=10;} vga_writestr(&b[x]); }
        vga_writestr(": ");
        { char b[12]; int x = 10; uint64_t v = free; b[11]=0;
          if (!v) b[--x]='0'; while(v&&x>0){b[--x]=(char)('0'+v%10);v/=10;} vga_writestr(&b[x]); }
        vga_writestr("/");
        { char b[12]; int x = 10; uint64_t v = total; b[11]=0;
          if (!v) b[--x]='0'; while(v&&x>0){b[--x]=(char)('0'+v%10);v/=10;} vga_writestr(&b[x]); }
        vga_writestr(" free (");
        { char b[12]; int x = 10; uint64_t v = pages; b[11]=0;
          if (!v) b[--x]='0'; while(v&&x>0){b[--x]=(char)('0'+v%10);v/=10;} vga_writestr(&b[x]); }
        vga_writestr(" pages)\n");

        /* Serial */
        serial_writestr("  Size ");
        { char b[12]; int x = 10; uint64_t v = c->object_size; b[11]=0;
          if (!v) b[--x]='0'; while(v&&x>0){b[--x]=(char)('0'+v%10);v/=10;} serial_writestr(&b[x]); }
        serial_writestr(": ");
        { char b[12]; int x = 10; uint64_t v = free; b[11]=0;
          if (!v) b[--x]='0'; while(v&&x>0){b[--x]=(char)('0'+v%10);v/=10;} serial_writestr(&b[x]); }
        serial_writestr("/");
        { char b[12]; int x = 10; uint64_t v = total; b[11]=0;
          if (!v) b[--x]='0'; while(v&&x>0){b[--x]=(char)('0'+v%10);v/=10;} serial_writestr(&b[x]); }
        serial_writestr(" free (");
        { char b[12]; int x = 10; uint64_t v = pages; b[11]=0;
          if (!v) b[--x]='0'; while(v&&x>0){b[--x]=(char)('0'+v%10);v/=10;} serial_writestr(&b[x]); }
        serial_writestr(" pages)\n");
    }
}