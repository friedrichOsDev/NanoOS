# Compilers
TARGET = x86_64-elf
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
BOOT16_STAGE1_ASM = $(BOOTDIR)/boot16_stage1.asm
BOOT16_STAGE1_BIN = $(BUILDDIR)/boot16_stage1.bin
BOOT16_STAGE2_ASM = $(BOOTDIR)/boot16_stage2.asm
BOOT16_STAGE2_BIN = $(BUILDDIR)/boot16_stage2.bin
BOOT32_STAGE3_ASM = $(BOOTDIR)/boot32_stage3.asm
BOOT32_STAGE3_BIN = $(BUILDDIR)/boot32_stage3.bin

# OS Image 
OS_IMG = $(BUILDDIR)/os.img

# Phony Targets
.PHONY: all os clean

all: clean os

os: $(OS_IMG)

$(OS_IMG): $(BOOT16_STAGE1_BIN) $(BOOT16_STAGE2_BIN) $(BOOT32_STAGE3_BIN) | $(BUILDDIR)
	cat $(BOOT16_STAGE1_BIN) $(BOOT16_STAGE2_BIN) $(BOOT32_STAGE3_BIN) > $@

$(BOOT16_STAGE1_BIN): $(BOOT16_STAGE1_ASM) | $(BUILDDIR)
	$(NASM) -f bin $< -o $@

$(BOOT16_STAGE2_BIN): $(BOOT16_STAGE2_ASM) | $(BUILDDIR)
	$(NASM) -f bin $< -o $@

$(BOOT32_STAGE3_BIN): $(BOOT32_STAGE3_ASM) | $(BUILDDIR)
	$(NASM) -f bin $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)
