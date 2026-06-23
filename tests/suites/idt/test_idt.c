#include "suites.h"
/* IDT Runtime Tests — Nova kernel */
#include "test_runner.h"
#include "idt.h"
#include "attributes.h"
static test_suite_t suite;

static const char *test_idt_limit(void) {
    struct { uint16_t lim; uint32_t base; } PACKED idtr;
    __asm__ volatile ("sidt %0" : "=m"(idtr));
    if (idtr.lim != 2047) return "IDTR.limit != 2047";
    return NULL;
}
static const char *test_idt_addr(void) {
    struct { uint16_t lim; uint32_t base; } PACKED idtr;
    __asm__ volatile ("sidt %0" : "=m"(idtr));
    if (idtr.base == 0) return "IDT not in memory";
    if (idtr.base & 7)  return "IDT not 8-byte aligned";
    return NULL;
}
static const char *test_exc_present(void) {
    struct { uint16_t lim; uint32_t base; } PACKED idtr;
    __asm__ volatile ("sidt %0" : "=m"(idtr));
    for (uint32_t i = 0; i < 32; i++) {
        volatile uint8_t *g = (volatile uint8_t *)(idtr.base + i * 8);
        if (!(g[5] & 0x80)) return "Exception gate not present";
    }
    return NULL;
}
static const char *test_irq_present(void) {
    struct { uint16_t lim; uint32_t base; } PACKED idtr;
    __asm__ volatile ("sidt %0" : "=m"(idtr));
    for (uint32_t i = 32; i < 48; i++) {
        volatile uint8_t *g = (volatile uint8_t *)(idtr.base + i * 8);
        if (!(g[5] & 0x80)) return "IRQ gate not present";
    }
    return NULL;
}
static const char *test_gate_sel(void) {
    struct { uint16_t lim; uint32_t base; } PACKED idtr;
    __asm__ volatile ("sidt %0" : "=m"(idtr));
    for (uint32_t i = 0; i < 48; i++) {
        volatile uint16_t *s = (volatile uint16_t *)(idtr.base + i * 8 + 2);
        if (*s != 0x08) return "Gate selector != 0x08";
    }
    return NULL;
}
static const char *test_gate_type(void) {
    struct { uint16_t lim; uint32_t base; } PACKED idtr;
    __asm__ volatile ("sidt %0" : "=m"(idtr));
    for (uint32_t i = 0; i < 48; i++) {
        volatile uint8_t *f = (volatile uint8_t *)(idtr.base + i * 8 + 5);
        if ((f[0] & 0x0F) != 0x0E) return "Gate type != 0xE";
    }
    return NULL;
}
void run_suite_idt(void) {
    suite_init(&suite, "IDT");
    suite_add(&suite, "IDTR limit",    "sidt returns 2047",   test_idt_limit);
    suite_add(&suite, "IDTR base",     "nonzero, aligned",    test_idt_addr);
    suite_add(&suite, "32 exc gates",  "all present",         test_exc_present);
    suite_add(&suite, "16 IRQ gates",  "all present",         test_irq_present);
    suite_add(&suite, "Selector 0x08", "kernel code",         test_gate_sel);
    suite_add(&suite, "Gate type 0xE", "32-bit interrupt",    test_gate_type);
    suite_run(&suite);
}