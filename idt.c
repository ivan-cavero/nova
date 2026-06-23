/* IDT + PIC initialization — Nova kernel Phase 1 */

#include "idt.h"
#include "gdt.h"
#include "io.h"
#include "ports.h"
#include "attributes.h"

/* ==================== IDT Table ==================== */

/* 256 entries × 8 bytes = 2048 bytes */
static idt_entry_t idt[256];

/* IDTR pointing to our table */
static const idtr_t idtr = {
    .limit = sizeof(idt) - 1,   /* 2047 */
    .base  = (uint32_t)&idt,
};

/* Stub arrays exported from interrupt.asm */
extern const uint32_t isr_stubs[32];
extern const uint32_t irq_stubs[16];

/* Load IDTR (assembly) */
extern void lidt_load(const idtr_t *idtr_ptr);

/* ==================== Helpers ==================== */

static void idt_set_gate(uint8_t vector, uint32_t handler, uint8_t flags) {
    idt[vector].offset_lo = (uint16_t)(handler & 0xFFFF);
    idt[vector].selector  = GDT_KCODE * 8;   /* 0x08 — kernel code segment */
    idt[vector].ist       = 0;                /* Not used in 32-bit mode */
    idt[vector].flags     = flags;
    idt[vector].offset_hi = (uint16_t)((handler >> 16) & 0xFFFF);
}

/* ==================== Exception Names ==================== */

const char * const exc_names[32] = {
    "#DE  Divide Error",                /* 0  */
    "#DB  Debug",                        /* 1  */
    "#NMI Non-maskable Interrupt",      /* 2  */
    "#BP  Breakpoint",                   /* 3  */
    "#OF  Overflow",                     /* 4  */
    "#BR  Bound Range Exceeded",         /* 5  */
    "#UD  Invalid Opcode",               /* 6  */
    "#NM  Device Not Available",         /* 7  */
    "#DF  Double Fault",                 /* 8  */
    "Coprocessor Segment Overrun",      /* 9  */
    "#TS  Invalid TSS",                  /* 10 */
    "#NP  Segment Not Present",          /* 11 */
    "#SS  Stack-Segment Fault",          /* 12 */
    "#GP  General Protection Fault",     /* 13 */
    "#PF  Page Fault",                   /* 14 */
    "Reserved (15)",                     /* 15 */
    "#MF  x87 FPU Error",                /* 16 */
    "#AC  Alignment Check",              /* 17 */
    "#MC  Machine Check",                /* 18 */
    "#XM  SIMD Floating-Point",          /* 19 */
    "Reserved (20)",                     /* 20 */
    "Reserved (21)",                     /* 21 */
    "Reserved (22)",                     /* 22 */
    "Reserved (23)",                     /* 23 */
    "Reserved (24)",                     /* 24 */
    "Reserved (25)",                     /* 25 */
    "Reserved (26)",                     /* 26 */
    "Reserved (27)",                     /* 27 */
    "Reserved (28)",                     /* 28 */
    "Reserved (29)",                     /* 29 */
    "#SX  Security Exception",           /* 30 */
    "Reserved (31)",                     /* 31 */
};

/* ==================== Exception Handler ==================== */

void exception_handler(registers_t *r) {
    const char *name = (r->vector < 32) ? exc_names[r->vector] : "Unknown";

    /* === VGA output === */
    vga_setcolor(CLR_RED, CLR_BLACK);
    vga_writestr("\n\n  *** KERNEL PANIC ***\n");
    vga_setcolor(CLR_LRED, CLR_BLACK);
    vga_writestr("  Unhandled exception: ");
    vga_writestr(name);
    vga_putchar('\n');

    /* === Serial output === */
    serial_writestr("\n\n  *** KERNEL PANIC ***\n");
    serial_writestr("  Unhandled exception: ");
    serial_writestr(name);
    serial_putchar('\n');

    /* Print error code if non-zero */
    if (r->error_code != 0) {
        vga_writestr("  Error code         : 0x");
        serial_writestr("  Error code         : 0x");
        /* Simple hex print — 8 hex digits */
        for (int shift = 28; shift >= 0; shift -= 4) {
            uint8_t nibble = (uint8_t)((r->error_code >> shift) & 0xF);
            char hex = (char)(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
            vga_putchar(hex);
            serial_putchar(hex);
        }
        vga_putchar('\n');
        serial_putchar('\n');
    }

    /* Print EIP */
    vga_writestr("  EIP/Return addr    : 0x");
    serial_writestr("  EIP/Return addr    : 0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        uint8_t nibble = (uint8_t)((r->eip >> shift) & 0xF);
        char hex = (char)(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
        vga_putchar(hex);
        serial_putchar(hex);
    }
    vga_putchar('\n');
    serial_putchar('\n');

    /* Extra info for Page Fault */
    if (r->vector == EXC_PF) {
        uint32_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
        vga_writestr("  CR2 (fault address): 0x");
        serial_writestr("  CR2 (fault address): 0x");
        for (int shift = 28; shift >= 0; shift -= 4) {
            uint8_t nibble = (uint8_t)((cr2 >> shift) & 0xF);
            char hex = (char)(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
            vga_putchar(hex);
            serial_putchar(hex);
        }
        vga_putchar('\n');
        serial_putchar('\n');

        /* Page fault flags */
        vga_writestr("  Reason: ");
        serial_writestr("  Reason: ");
        if (!(r->error_code & 1)) { vga_writestr("page not present");  serial_writestr("page not present"); }
        else                      { vga_writestr("page protection");   serial_writestr("page protection");  }
        if (r->error_code & 2)    { vga_writestr(", write");            serial_writestr(", write");           }
        else                      { vga_writestr(", read");             serial_writestr(", read");            }
        if (r->error_code & 4)    { vga_writestr(", user mode");        serial_writestr(", user mode");       }
        if (r->error_code & 8)    { vga_writestr(", reserved bit");     serial_writestr(", reserved bit");    }
        if (r->error_code & 16)   { vga_writestr(", instruction fetch"); serial_writestr(", instruction fetch"); }
        vga_putchar('\n');
        serial_putchar('\n');
    }

    vga_writestr("\n  System halted.\n");
    serial_writestr("\n  System halted.\n");

    /* Disable interrupts and halt forever */
    cli();
    while (1) { halt(); }
}

/* External dispatch targets — registered by timer.c, keyboard.c */
extern void timer_tick(void);
extern void keyboard_irq(void);

/* ==================== IRQ Handler ==================== */

void irq_handler(registers_t *r) {
    /* Dispatch to device-specific handlers */
    switch (r->vector) {
    case 32:  /* IRQ0 — PIT Timer */
        timer_tick();
        break;

    case 33:  /* IRQ1 — PS/2 Keyboard */
        keyboard_irq();
        break;

    default:
        break;
    }

    /* Send End-of-Interrupt to PIC */
    if (r->vector >= 40) {
        outb(PIC2_CMD, 0x20);   /* Slave EOI */
    }
    outb(PIC1_CMD, 0x20);       /* Master EOI */
}

/* ==================== PIC Remap ==================== */

void pic_remap(void) {
    /* ICW1: begin initialization (edge-triggered, cascade, ICW4 needed) */
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);

    /* ICW2: remap IRQs to INT 32-47 */
    outb(PIC1_DATA, PIC1_OFFSET);   /* Master: INT 32-39 */
    outb(PIC2_DATA, PIC2_OFFSET);   /* Slave:  INT 40-47 */

    /* ICW3: tell master there's a slave at IRQ2 */
    outb(PIC1_DATA, 0x04);          /* Slave is connected to IRQ2 (bit 2) */
    outb(PIC2_DATA, 0x02);          /* Slave cascade identity */

    /* ICW4: 80x86 mode, automatic EOI disabled */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* Mask all interrupts (we'll unmask selectively) */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/* ==================== IDT Initialization ==================== */

void idt_init(void) {
    log_entry_t e;
    unsigned int i;

    /* Set exception gates (vectors 0-31) from assembly stubs */
    e = log_begin("Installing exception handlers");
    for (i = 0; i < 32; i++) {
        idt_set_gate((uint8_t)i, isr_stubs[i], IDT_FLAG_INT32_DPL0);
    }
    log_end(&e, true);

    /* Set IRQ gates (vectors 32-47) from assembly stubs */
    e = log_begin("Installing IRQ handlers");
    for (i = 0; i < 16; i++) {
        idt_set_gate((uint8_t)(32 + i), irq_stubs[i], IDT_FLAG_INT32_DPL0);
    }
    log_end(&e, true);

    /* Load the IDT into the CPU */
    e = log_begin("Loading IDT");
    lidt_load(&idtr);
    log_end(&e, true);

    /* Remap PIC (IRQ0-15 → INT 32-47) */
    e = log_begin("Remapping PIC");
    pic_remap();
    log_end(&e, true);
}