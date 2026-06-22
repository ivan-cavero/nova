# Nova

A minimal x86 operating system kernel — built from scratch.

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
```

## Features

- **32-bit protected mode** (x86)
- **GRUB multiboot2** boot protocol
- **VGA text mode** output with colored boot log
- **Serial console** (COM1, 115200 baud)
- **GDT** with 5 entries (null, kernel code/data, user code/data)
- **Boot log system** with `[OK]`, `[..]`, `[!!]`, `[FAIL]` status markers
- **Optimized** with `-O3`, strict compilation, minimal binary size

## Requirements

| Tool | Purpose |
|------|---------|
| `gcc` | C compiler (freestanding) |
| `nasm` | x86 assembler |
| `make` | Build system |
| `grub-mkrescue` | ISO creation (part of `grub-pc-bin`) |
| `xorriso` | ISO creation |
| `qemu-system-x86_64` | Emulator for testing |

---

## Quick start

### Linux (Ubuntu / Debian)

```bash
# Install dependencies
sudo apt update
sudo apt install build-essential nasm make xorriso grub-pc-bin qemu-system-x86

# Clone and build
git clone https://github.com/ivan-cavero/nova.git
cd nova
make run
```

### Windows (WSL2 + Debian/Ubuntu)

#### 1. Install WSL2

Open **PowerShell as Administrator** and run:

```powershell
wsl --install -d Debian
```

Restart your PC when prompted.

#### 2. Install dependencies inside WSL

```bash
sudo apt update
sudo apt install build-essential nasm make xorriso grub-pc-bin qemu-system-x86
```

#### 3. Install QEMU for Windows (for GUI display)

```powershell
# Download from: https://qemu.org
# Or install via winget:
winget install QEMU
```

#### 4. Clone and build

```bash
git clone https://github.com/ivan-cavero/nova.git
cd nova
make run
```

> **Note**: `make run` launches QEMU inside WSL with serial output (text-only).
> For the graphical window, use the `run.bat` script (double-click from Windows Explorer),
> or launch QEMU from Windows directly:
> ```powershell
> & "C:\Program Files\qemu\qemu-system-x86_64.exe" -cdrom "\\wsl.localhost\Debian\home\<user>\nova\myKernel.iso" -m 64
> ```

### Setting up on a new PC

1. Install Git
2. Install WSL2 (Windows) or ensure you're on a Linux machine
3. Install the dependencies listed above
4. Clone the repository
5. Run `make run`

No cross-compiler needed — the host `gcc` works with freestanding flags.

---

## Build commands

| Command | Description |
|---------|-------------|
| `make` | Compile kernel (`myKernel.bin`) |
| `make iso` | Create bootable ISO (`myKernel.iso`) |
| `make run` | Build + launch in QEMU |
| `make debug` | Build + launch with GDB attached |
| `make clean` | Remove build artifacts |

---

## Project structure

```
nova/
├── boot.asm           # Multiboot2 header + 32-bit entry point
├── kernel.c           # Main kernel entry
├── kernel.h
├── io.c               # VGA text mode + serial + boot log
├── io.h
├── ports.h            # I/O port access (inb, outb, etc.)
├── attributes.h       # Compiler macros (likely, NORETURN, etc.)
├── linker.ld          # Linker script (loads at 1MB)
├── Makefile
├── .gitignore
├── README.md
├── run.bat            # Windows launcher (double-click)
├── run.ps1            # PowerShell launcher
└── boot/
    └── grub/
        └── grub.cfg   # GRUB configuration
```

---

## Boot log

When the kernel boots, it displays:

```
  Boot Sequence
  -------------
  [OK]  Bootloader      @ 0x100000
  [..]  Architecture    : x86 / 32-bit protected mode
  [..]  Multiboot       : v2 (magic 0x36D76289)
  [OK]  CPU             : i686 class, FPU on-board
  [OK]  Memory          : 0x00000000 - 0x03FFFFFF (64 MB)
  [OK]  GDT             : 5-entry (null, code, data, user code, user data)

  System ready. Halting CPU.
```

### Log levels

| Tag | Color | Meaning |
|-----|-------|---------|
| `[OK]` | Blue | Component initialized successfully |
| `[..]` | Grey | Informational message |
| `[!!]` | Brown/Yellow | Warning |
| `[FAIL]` | Red | Component failed |
| `[>]` | Magenta | Step in progress |

---

## Low-level overview

### Boot flow

```
Power on
  └→ CPU starts in Real Mode (16-bit)
      └→ BIOS loads GRUB from ISO
          └→ GRUB switches to Protected Mode (32-bit)
              └→ GRUB reads Multiboot2 header from boot.asm
                  └→ GRUB loads kernel ELF at 0x100000 (1 MB)
                      └→ GRUB jumps to _start
                          └→ _start sets up stack, calls kernel_main
                              └── kernel_main initializes VGA + serial
                                  └── displays boot log
                                      └── HLT (waits for interrupts)
```

### Memory layout

| Address | Region |
|---------|--------|
| `0x00000000` - `0x0007FFFF` | BIOS / Real Mode IVT / BDA |
| `0x0007E000` - `0x0007FFFF` | GRUB stage 2 |
| `0x00080000` - `0x0009FFFF` | Kernel stack + data |
| `0x000A0000` - `0x000BFFFF` | VGA video memory |
| `0x000B8000` - `0x000BFFFF` | VGA text mode buffer |
| `0x00100000` | **Kernel loaded here** |
| `0x00100000` - `0x03FFFFFF` | Available RAM (64 MB in QEMU) |

---

## License

MIT