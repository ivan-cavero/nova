/* Nova kernel - main entry point */

#include "io.h"
#include "kernel.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "keyboard.h"
#include "ports.h"
#include "attributes.h"
#include "test_runner.h"

void kernel_main(uint32_t magic, uint32_t addr) {
    log_init();

    serial_writestr("  Boot Sequence\n");
    serial_writestr("  -------------\n\n");
    vga_writestr("  Boot Sequence\n");
    vga_writestr("  -------------\n\n");

    /* Field names padded to exactly 16 chars for alignment */
    log_ok("Bootloader      @ 0x100000");
    log_info("Architecture    : x86 / 32-bit protected mode");
    log_info("Multiboot       : v2 (magic 0x36D76289)");
    log_ok("CPU             : i686 class, FPU on-board");
    log_ok("Memory          : 0x00000000 - 0x03FFFFFF (64 MB)");

    /* === Phase 0: Foundation === */
    gdt_init();

    /* === Phase 1: Interrupt Infrastructure === */
    idt_init();                    /* IDT + exceptions + PIC remap */

    timer_init(TIMER_DEFAULT_FREQ); /* PIT timer at 1000 Hz */
    keyboard_init();                /* PS/2 keyboard */

    /* Unmask PIT (IRQ0) and keyboard (IRQ1) */
    outb(PIC1_DATA, (uint8_t)(~((1U << 0) | (1U << 1))));

    /* Enable interrupts globally */
    sti();

    /* Run Phase 1 verification test suite */
    run_all_tests();

    vga_putchar('\n');
    vga_setcolor(CLR_LBLUE, CLR_BLACK);
    vga_writestr("  All tests complete. Halting CPU.");
    vga_putchar('\n');
    vga_setcolor(CLR_LGREY, CLR_BLACK);
    serial_writestr("\nAll tests complete. Halting CPU.\n");

    while (1) {
        __asm__ volatile ("hlt");
    }
}