# Nova

Minimal x86 operating system kernel. Boots in QEMU via GRUB with multiboot2.

```
╔════════════════════════════════════╗
║                                    ║
║  #   #  ###   #   #    #          ║
║  ##  # #   #  #   #  # #          ║
║  # # # #   #   # #   #   #        ║
║  #  ## #   #    #    #####        ║
║  #   #  ###     #    #   #        ║
║                                    ║
║   Nova v0.2 - x86 Edition          ║
╚════════════════════════════════════╝
  Boot Sequence
  -------------
  [OK]  Bootloader      @ 0x100000
  [..]  Architecture    : x86 / 32-bit protected mode
  [..]  Multiboot       : v2 (magic 0x36D76289)
  [OK]  CPU             : i686 class, FPU on-board
  [OK]  Memory          : 0x00000000 - 0x03FFFFFF (64 MB)
  [OK]  GDT             : 5-entry (null, code, data, user code, user data)
```

## Build

```bash
make          # compile kernel
make iso      # create bootable ISO
make run      # launch in QEMU
make debug    # launch with GDB
make clean    # remove build artifacts
```

## Requirements

- gcc (freestanding)
- nasm
- make
- grub-mkrescue (for ISO creation)
- qemu-system-x86_64 (for testing)

## Structure

```
kernel/
├── boot.asm       # Multiboot2 header + entry
├── kernel.c       # Main kernel entry
├── io.c           # VGA text mode + serial + boot log
├── io.h
├── linker.ld      # Linker script
├── Makefile
├── ports.h        # I/O port access
├── attributes.h   # Compiler attribute macros
└── boot/grub/     # GRUB config for ISO
```

## License

MIT