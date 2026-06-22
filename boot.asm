section .multiboot

MB2_MAGIC  equ 0xe85250d6
MB2_ARCH   equ 0

align 8
mb2_header_start:
    dd MB2_MAGIC
    dd MB2_ARCH
    dd mb2_header_end - mb2_header_start
    dd 0x100000000 - (MB2_MAGIC + MB2_ARCH + (mb2_header_end - mb2_header_start))

align 8
    dw 0
    dw 0
    dd 8
mb2_header_end:

section .text
bits 32

global _start
extern kernel_main

_start:
    mov esp, stack_top

    push ebx
    push eax

    call kernel_main

    cli
.hang:
    hlt
    jmp .hang

section .note.GNU-stack noalloc noexec nowrite progbits

section .bss
align 16
stack_bottom:
    resb 16384
stack_top: