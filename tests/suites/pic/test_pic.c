#include "suites.h"
/* PIC Runtime Tests — Nova kernel */
#include "test_runner.h"
#include "idt.h"
#include "ports.h"
static test_suite_t suite;

static const char *test_imr_master(void) {
    uint8_t m = inb(PIC1_DATA);
    if (m & ((1U<<0)|(1U<<1))) return "IRQ0/IRQ1 masked";
    if (m != (uint8_t)~((1U<<0)|(1U<<1))) return "PIC1 mask unexpected";
    return NULL;
}
static const char *test_imr_slave(void) {
    if (inb(PIC2_DATA) != 0xFF) return "PIC2 not fully masked";
    return NULL;
}
static const char *test_irq0_present(void) {
    struct { uint16_t lim; uint32_t base; } PACKED idtr;
    __asm__ volatile ("sidt %0" : "=m"(idtr));
    volatile uint8_t *g = (volatile uint8_t *)(idtr.base + 32 * 8);
    if (!(g[5] & 0x80)) return "IRQ0 gate not present";
    return NULL;
}
static const char *test_eoi_safe(void) {
    outb(PIC1_CMD, 0x20); outb(PIC2_CMD, 0x20);
    return NULL;
}
void run_suite_pic(void) {
    suite_init(&suite, "PIC");
    suite_add(&suite, "PIC1 IMR",   "IRQ0+1 unmasked",   test_imr_master);
    suite_add(&suite, "PIC2 IMR",   "all masked",         test_imr_slave);
    suite_add(&suite, "IRQ0 gate",  "IDT[32] present",    test_irq0_present);
    suite_add(&suite, "EOI safe",   "no crash",           test_eoi_safe);
    suite_run(&suite);
}