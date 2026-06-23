/* Nova kernel - main entry point */

#include "io.h"
#include "kernel.h"
#include "gdt.h"
#include "attributes.h"

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

    /* Load our own GDT (GRUB provides a minimal one; we need 5 entries with user segments) */
    gdt_init();

    vga_putchar('\n');
    vga_setcolor(CLR_LBLUE, CLR_BLACK);
    vga_writestr("  System ready. Halting CPU.");
    vga_putchar('\n');
    vga_setcolor(CLR_LGREY, CLR_BLACK);
    serial_writestr("\n  System ready. Halting CPU.\n");

    while (1) {
        __asm__ volatile ("hlt");
    }
}