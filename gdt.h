#ifndef GDT_H
#define GDT_H

#include <stdint.h>
#include "attributes.h"

/* GDT entry (8 bytes per descriptor) */
typedef struct {
    uint16_t limit_low;   /* Limit [15:0]  */
    uint16_t base_low;    /* Base  [15:0]  */
    uint8_t  base_mid;    /* Base  [23:16] */
    uint8_t  access;      /* Access flags   */
    uint8_t  granularity; /* G+D/B+L+AVL + Limit[19:16] */
    uint8_t  base_high;   /* Base  [31:24] */
} PACKED gdt_entry_t;

/* GDTR (6 bytes, passed to lgdt instruction) */
typedef struct {
    uint16_t limit;       /* sizeof(GDT) - 1 */
    uint32_t base;        /* Linear address of GDT table */
} PACKED gdtr_t;

/* Selector indices */
#define GDT_NULL   0   /* 0x00 */
#define GDT_KCODE  1   /* 0x08 */
#define GDT_KDATA  2   /* 0x10 */
#define GDT_UCODE  3   /* 0x18 */
#define GDT_UDATA  4   /* 0x20 */
#define GDT_ENTRIES 5

/* Access byte helpers (bit positions) */
#define GDT_ACCESS_P      0x80  /* Present */
#define GDT_ACCESS_DPL0   0x00  /* Ring 0 */
#define GDT_ACCESS_DPL3   0x60  /* Ring 3 */
#define GDT_ACCESS_S      0x10  /* Code/data descriptor (not system) */
#define GDT_ACCESS_EXEC   0x08  /* Executable (code segment) */
#define GDT_ACCESS_DATA   0x00  /* Data segment */
#define GDT_ACCESS_RW     0x02  /* Readable (code) / Writable (data) */

/* Granularity byte helpers */
#define GDT_GRAN_4K       0x80  /* 4 KB page granularity */
#define GDT_GRAN_32BIT    0x40  /* 32-bit protected mode */
#define GDT_GRAN_LIMIT_H  0x0F  /* Limit[19:16] = 0xF (full 4 GB) */

/* Pre-built access bytes */
#define GDT_ACCESS_KCODE (GDT_ACCESS_P | GDT_ACCESS_DPL0 | GDT_ACCESS_S | GDT_ACCESS_EXEC | GDT_ACCESS_RW) /* 0x9A */
#define GDT_ACCESS_KDATA (GDT_ACCESS_P | GDT_ACCESS_DPL0 | GDT_ACCESS_S | GDT_ACCESS_DATA | GDT_ACCESS_RW) /* 0x92 */
#define GDT_ACCESS_UCODE (GDT_ACCESS_P | GDT_ACCESS_DPL3 | GDT_ACCESS_S | GDT_ACCESS_EXEC | GDT_ACCESS_RW) /* 0xFA */
#define GDT_ACCESS_UDATA (GDT_ACCESS_P | GDT_ACCESS_DPL3 | GDT_ACCESS_S | GDT_ACCESS_DATA | GDT_ACCESS_RW) /* 0xF2 */

/* Pre-built granularity for flat 4 GB segments */
#define GDT_GRAN_FLAT (GDT_GRAN_4K | GDT_GRAN_32BIT | GDT_GRAN_LIMIT_H) /* 0xCF */

/* Convenience macro to build a GDT entry */
#define GDT_ENTRY(base, limit, access, gran) { \
    (uint16_t)((limit)       & 0xFFFF),        \
    (uint16_t)((base)        & 0xFFFF),        \
    (uint8_t )(((base) >> 16) & 0xFF),         \
    (uint8_t )(access),                         \
    (uint8_t )((gran) | (((limit) >> 16) & 0x0F)), \
    (uint8_t )(((base) >> 24) & 0xFF),         \
}

/* Public API */
void gdt_init(void);

#endif /* GDT_H */