#include "suites.h"
/* GDT Runtime Tests — Nova kernel */
#include "test_runner.h"
#include "gdt.h"
#include "attributes.h"
static test_suite_t suite;

static const char *test_gdt_limit(void) {
    struct { uint16_t lim; uint32_t base; } PACKED gdtr;
    __asm__ volatile ("sgdt %0" : "=m"(gdtr));
    if (gdtr.lim != 39) return "GDTR.limit != 39 (5*8-1)";
    return NULL;
}
static const char *test_gdt_addr(void) {
    struct { uint16_t lim; uint32_t base; } PACKED gdtr;
    __asm__ volatile ("sgdt %0" : "=m"(gdtr));
    if (gdtr.base == 0) return "GDT not in memory";
    if (gdtr.base & 7)  return "GDT not 8-byte aligned";
    return NULL;
}
static const char *test_null_desc(void) {
    struct { uint16_t lim; uint32_t base; } PACKED gdtr;
    __asm__ volatile ("sgdt %0" : "=m"(gdtr));
    volatile uint32_t *e = (volatile uint32_t *)gdtr.base;
    if (e[0] != 0 || e[1] != 0) return "Null desc not zero";
    return NULL;
}
static const char *test_cs(void) {
    uint16_t cs; __asm__ volatile ("mov %%cs,%0" : "=r"(cs));
    if (cs != 0x08) return "CS != 0x08";
    return NULL;
}
static const char *test_ds(void) {
    uint16_t ds; __asm__ volatile ("mov %%ds,%0" : "=r"(ds));
    if (ds != 0x10) return "DS != 0x10";
    return NULL;
}
static const char *test_pe(void) {
    uint32_t cr0; __asm__ volatile ("mov %%cr0,%0" : "=r"(cr0));
    if (!(cr0 & 1)) return "CR0.PE = 0";
    return NULL;
}
static const char *test_pg(void) {
    uint32_t cr0; __asm__ volatile ("mov %%cr0,%0" : "=r"(cr0));
    if (cr0 & (1U<<31)) return "CR0.PG = 1";
    return NULL;
}
static const char *test_kcode_entry(void) {
    struct { uint16_t lim; uint32_t base; } PACKED gdtr;
    __asm__ volatile ("sgdt %0" : "=m"(gdtr));
    volatile uint8_t *e = (volatile uint8_t *)(gdtr.base + 8);
    if (e[5] != 0x9A) return "Kernel Code access != 0x9A";
    if (e[6] != 0xCF) return "Kernel Code granularity != 0xCF";
    return NULL;
}
void run_suite_gdt(void) {
    suite_init(&suite, "GDT");
    suite_add(&suite, "GDTR limit=39","sgdt reads 5 entries", test_gdt_limit);
    suite_add(&suite, "GDTR valid",    "base nonzero aligned", test_gdt_addr);
    suite_add(&suite, "Null desc",     "first 8 bytes zeroed", test_null_desc);
    suite_add(&suite, "Kcode entry",   "access=0x9A,gran=0xCF",test_kcode_entry);
    suite_add(&suite, "CS = 0x08",     "kernel code selector", test_cs);
    suite_add(&suite, "DS = 0x10",     "kernel data selector", test_ds);
    suite_add(&suite, "CR0.PE = 1",    "protected mode on",    test_pe);
    suite_add(&suite, "CR0.PG = 0",    "paging disabled",      test_pg);
    suite_run(&suite);
}