/* GDT initialization — Nova kernel */

#include "gdt.h"
#include "io.h"
#include "attributes.h"

/* GDT table: 5 entries, 8 bytes each = 40 bytes total */
static gdt_entry_t gdt_table[GDT_ENTRIES] = {
    /* 0x00 — Null descriptor (required by CPU) */
    GDT_ENTRY(0, 0, 0, 0),

    /* 0x08 — Kernel Code: Ring 0, 32-bit, readable, 4 GB flat */
    GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_KCODE, GDT_GRAN_FLAT),

    /* 0x10 — Kernel Data: Ring 0, writable, 4 GB flat */
    GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_KDATA, GDT_GRAN_FLAT),

    /* 0x18 — User Code: Ring 3, 32-bit, readable, 4 GB flat */
    GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_UCODE, GDT_GRAN_FLAT),

    /* 0x20 — User Data: Ring 3, writable, 4 GB flat */
    GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_UDATA, GDT_GRAN_FLAT),
};

/* GDTR: points to our GDT table */
static gdtr_t gdtr = {
    .limit = sizeof(gdt_table) - 1,
    .base  = (uint32_t)&gdt_table,
};

/* Assembly routine: loads GDTR and reloads segment registers */
extern void gdt_flush(const gdtr_t *gdtr_ptr);

void gdt_init(void) {
    /* Load the GDT via assembly trampoline */
    gdt_flush(&gdtr);

    /* Log success — this is now backed by real hardware state */
    log_ok("GDT             : 5-entry (null, code, data, user code, user data)");
}
