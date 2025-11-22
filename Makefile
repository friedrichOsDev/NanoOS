# Compilers
ARGET = x86_64-elf
NASM = nasm
GCC = $(TARGET)-gcc
LD = $(TARGET)-ld
OBJCOPY = $(TARGET)-objcopy

# Flags
C_FLAGS = -ffreestanding -m32 -O1 -Wall -Wextra -Werror -fno-builtin -fno-stack-protector
LD_FLAGS = -m elf_i386

# Directories
SRCDIR = src
BOOTDIR = $(SRCDIR)/boot
KERNELDIR = $(SRCDIR)/kernel
BUILDDIR = build

# Bootloader Files
BOOT16_ASM = $(BOOTDIR)/boot16.asm
BOOT16_BIN = $(BUILDDIR)/boot16.bin
BOOT32_ASM = $(BOOTDIR)/boot32.asm
BOOT32_BIN = $(BUILDDIR)/boot32.bin

# OS Image 
OS_IMG = $(BUILDDIR)/os.img

# Phony Targets
.PHONY: all os clean

all: clean os

os: $(OS_IMG)

$(OS_IMG): $(BOOT16_BIN) $(BOOT32_BIN) | $(BUILDDIR)
	cat $(BOOT16_BIN) $(BOOT32_BIN) > $(OS_IMG)

$(BOOT16_BIN): $(BOOT16_ASM) | $(BUILDDIR)
	$(NASM) -f bin $< -o $@

$(BOOT32_BIN): $(BOOT32_ASM) | $(BUILDDIR)
	$(NASM) -f bin $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)
