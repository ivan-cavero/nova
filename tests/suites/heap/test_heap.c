/* Heap Runtime Tests — Nova kernel Phase 3 */
/* Tests: basic alloc/free, all sizes, stress, OOM, kcalloc, krealloc */

#include "test_runner.h"
#include "heap.h"
static test_suite_t suite;

static const char *test_kmalloc_16(void) {
    void *p = kmalloc(16);
    if (!p) return "kmalloc(16) returned NULL";
    *(uint32_t *)p = 0xDEADBEEF;
    if (*(uint32_t *)p != 0xDEADBEEF) return "kmalloc(16) write/read failed";
    kfree(p);
    return NULL;
}

static const char *test_kmalloc_64(void) {
    void *p = kmalloc(64);
    if (!p) return "kmalloc(64) returned NULL";
    for (int i = 0; i < 64; i++) ((uint8_t *)p)[i] = (uint8_t)i;
    for (int i = 0; i < 64; i++) {
        if (((uint8_t *)p)[i] != (uint8_t)i) return "kmalloc(64) data corrupt";
    }
    kfree(p);
    return NULL;
}

static const char *test_kmalloc_4096(void) {
    void *p = kmalloc(4096);
    if (!p) return "kmalloc(4096) returned NULL";
    for (int i = 0; i < 4096; i++) ((uint8_t *)p)[i] = (uint8_t)(i & 0xFF);
    for (int i = 0; i < 4096; i++) {
        if (((uint8_t *)p)[i] != (uint8_t)(i & 0xFF)) return "kmalloc(4096) data corrupt";
    }
    kfree(p);
    return NULL;
}

static const char *test_kmalloc_large(void) {
    void *p = kmalloc(8192);
    if (!p) return "kmalloc(8192) returned NULL";
    *(uint64_t *)p = 0x1234567890ABCDEFULL;
    if (*(uint64_t *)p != 0x1234567890ABCDEFULL) return "kmalloc(8192) write/read failed";
    kfree(p);
    return NULL;
}

static const char *test_kmalloc_null(void) {
    if (kmalloc(0) != NULL) return "kmalloc(0) must return NULL";
    return NULL;
}

static const char *test_kfree_null(void) {
    kfree(NULL);
    return NULL;
}

static const char *test_kcalloc(void) {
    void *p = kcalloc(100, 4);
    if (!p) return "kcalloc(100,4) returned NULL";
    for (int i = 0; i < 400; i++) {
        if (((uint8_t *)p)[i] != 0) return "kcalloc not zeroed";
    }
    kfree(p);
    return NULL;
}

static const char *test_krealloc_shrink(void) {
    void *p = kmalloc(256);
    if (!p) return "krealloc shrink: initial alloc failed";
    for (int i = 0; i < 256; i++) ((uint8_t *)p)[i] = (uint8_t)(i & 0xFF);
    void *q = krealloc(p, 64);
    if (!q) return "krealloc shrink failed";
    for (int i = 0; i < 64; i++) {
        if (((uint8_t *)q)[i] != (uint8_t)(i & 0xFF)) return "krealloc shrink data lost";
    }
    kfree(q);
    return NULL;
}

static const char *test_krealloc_grow(void) {
    void *p = kmalloc(64);
    if (!p) return "krealloc grow: initial alloc failed";
    for (int i = 0; i < 64; i++) ((uint8_t *)p)[i] = (uint8_t)(i & 0xFF);
    void *q = krealloc(p, 256);
    if (!q) return "krealloc grow failed";
    for (int i = 0; i < 64; i++) {
        if (((uint8_t *)q)[i] != (uint8_t)(i & 0xFF)) return "krealloc grow data lost";
    }
    kfree(q);
    return NULL;
}

static const char *test_alloc_free_stress(void) {
    /* Allocate and free many times to test slab free list */
    for (int round = 0; round < 5; round++) {
        void *ptrs[100];
        for (int i = 0; i < 100; i++) {
            ptrs[i] = kmalloc(64);
            if (!ptrs[i]) return "stress: alloc failed";
            *(uint32_t *)ptrs[i] = (uint32_t)i;
        }
        for (int i = 0; i < 100; i++) {
            if (*(uint32_t *)ptrs[i] != (uint32_t)i) return "stress: data corrupt";
            kfree(ptrs[i]);
        }
    }
    return NULL;
}

static const char *test_all_sizes(void) {
    size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
    for (int i = 0; i < 11; i++) {
        void *p = kmalloc(sizes[i]);
        if (!p) return "all_sizes: alloc failed";
        *(uint32_t *)p = (uint32_t)sizes[i];
        if (*(uint32_t *)p != (uint32_t)sizes[i]) return "all_sizes: data corrupt";
        kfree(p);
    }
    return NULL;
}

void run_suite_heap(void) {
    suite_init(&suite, "Heap");
    suite_add(&suite, "kmalloc(16)",       "smallest slab",         test_kmalloc_16);
    suite_add(&suite, "kmalloc(64)",       "mid slab",              test_kmalloc_64);
    suite_add(&suite, "kmalloc(4096)",     "largest slab",          test_kmalloc_4096);
    suite_add(&suite, "kmalloc(8192)",     "buddy alloc",           test_kmalloc_large);
    suite_add(&suite, "kmalloc(0)=NULL",   "zero size",             test_kmalloc_null);
    suite_add(&suite, "kfree(NULL)",       "null free safe",        test_kfree_null);
    suite_add(&suite, "kcalloc zeroed",    "zeroed alloc",          test_kcalloc);
    suite_add(&suite, "krealloc shrink",   "256->64",               test_krealloc_shrink);
    suite_add(&suite, "krealloc grow",     "64->256",               test_krealloc_grow);
    suite_add(&suite, "stress 5x100",      "500 alloc/free",        test_alloc_free_stress);
    suite_add(&suite, "all sizes",         "16..16384",             test_all_sizes);
    suite_run(&suite);
}void run_suite_heap(void);
