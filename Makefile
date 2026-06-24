CC      = gcc
LD      = ld
ASM     = nasm

CFLAGS  = -ffreestanding -nostdlib -nostartfiles -nodefaultlibs \
          -m32 -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
          -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes \
          -Wmissing-declarations -Wredundant-decls -Wnested-externs \
          -Wcast-qual -Wcast-align -Wwrite-strings -Wpointer-arith \
          -Wstrict-aliasing=3 -Werror=vla -Wno-unused-parameter -Wno-missing-prototypes \
          -O3 -march=i686 -fomit-frame-pointer -fno-stack-protector \
          -fno-strict-aliasing -fno-exceptions -fno-asynchronous-unwind-tables \
          -fno-pic -fno-pie -mno-red-zone -g -I. -Itests \
          -MMD -MP

LDFLAGS = -m elf_i386 -T linker.ld -nostdlib -z max-page-size=0x1000
ASFLAGS = -f elf32 -g -F dwarf -Ox

KERNEL  = myKernel.bin
ISO     = myKernel.iso

OBJS = boot.o kernel.o io.o gdt.o gdt_flush.o idt.o interrupt.o timer.o keyboard.o \
       pmm.o vmm.o paging.o

# Test suite — recursive wildcard
TEST_SRCS := $(shell find tests -name '*.c' 2>/dev/null)
TEST_OBJS := $(TEST_SRCS:.c=.o)

OBJS += $(TEST_OBJS)

.PHONY: all clean run iso debug size

all: $(KERNEL)

# Compile any .c → .o, including subdirectories
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(ASM) $(ASFLAGS) $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "  KERNEL: $@ ($$(wc -c < $@) bytes)"

iso: $(KERNEL)
	mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/
	cp boot/grub/grub.cfg iso/boot/grub/
	grub-mkrescue -o $(ISO) iso 2>&1
	rm -rf iso
	@echo "  ISO: $(ISO) ($$(wc -c < $(ISO)) bytes)"

run: iso
	qemu-system-x86_64 -cdrom $(ISO) -serial stdio -m 64 \
		-no-reboot -no-shutdown -d cpu_reset 2>&1

debug: iso
	qemu-system-x86_64 -cdrom $(ISO) -serial stdio -m 64 -s -S -no-reboot &
	@gdb -ex "target remote localhost:1234" \
	     -ex "symbol-file $(KERNEL)" \
	     -ex "break kernel_main" \
	     -ex "continue"

size: $(KERNEL)
	@echo "--- Sections ---"
	@objdump -h $(KERNEL) 2>/dev/null || true
	@echo ""
	@echo "--- Symbols ---"
	@nm -S --size-sort $(KERNEL) 2>/dev/null || true

clean:
	rm -f *.o *.d $(KERNEL) $(ISO)
	rm -f $(shell find tests -name '*.o' -o -name '*.d' 2>/dev/null)
	rm -rf iso

-include $(OBJS:.o=.d)