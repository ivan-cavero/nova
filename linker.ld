OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

ENTRY(_start)

SECTIONS
{
    . = 1M;
    _kernel_start = .;

    .multiboot ALIGN(8) : {
        KEEP(*(.multiboot))
    } :text

    .text ALIGN(4K) : {
        *(.text .text.*)
    } :text

    .rodata ALIGN(4K) : {
        *(.rodata .rodata.*)
    } :rodata

    .data ALIGN(4K) : {
        *(.data .data.*)
    } :data

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    _kernel_end = ALIGN(4K);

    /DISCARD/ : {
        *(.comment)
        *(.comment.*)
        *(.note.*)
        *(.eh_frame)
        *(.eh_frame_hdr)
    }
}

PHDRS
{
    text    PT_LOAD FLAGS(5);
    rodata  PT_LOAD FLAGS(4);
    data    PT_LOAD FLAGS(6);
}