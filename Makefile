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
SRC_DIR = src
BUILD_DIR = build
BOOTLOADER_DIR = $(SRC_DIR)/bootloader
KERNEL_DIR = $(SRC_DIR)/kernel

# Files
BOOT16_STAGE1_SRC = $(BOOTLOADER_DIR)/boot16_stage1.asm
BOOT16_STAGE1_BIN = $(BUILD_DIR)/boot16_stage1.bin
BOOT16_STAGE2_SRC = $(BOOTLOADER_DIR)/boot16_stage2.asm
BOOT16_STAGE2_BIN = $(BUILD_DIR)/boot16_stage2.bin
BOOT32_STAGE3_SRC = $(BOOTLOADER_DIR)/boot32_stage3.asm
BOOT32_STAGE3_BIN = $(BUILD_DIR)/boot32_stage3.bin
BOOTLOADER_BIN = $(BUILD_DIR)/bootloader.bin

# OS Image
OS_IMAGE = $(BUILD_DIR)/nanoos.img

.PHONY: all os bootloader clean

all: os

os: $(OS_IMAGE)

$(OS_IMAGE): $(BOOTLOADER_BIN) | build_directory
	cat $(BOOTLOADER_BIN) > $(OS_IMAGE)

bootloader: $(BOOTLOADER_BIN)

$(BOOTLOADER_BIN): $(BOOT16_STAGE1_SRC) $(BOOT16_STAGE2_SRC) $(BOOT32_STAGE3_SRC) | build_directory
	$(NASM) -f bin $(BOOT16_STAGE1_SRC) -o $(BOOT16_STAGE1_BIN)
	$(NASM) -f bin $(BOOT16_STAGE2_SRC) -o $(BOOT16_STAGE2_BIN)
	$(NASM) -f bin $(BOOT32_STAGE3_SRC) -o $(BOOT32_STAGE3_BIN)
	cat $(BOOT16_STAGE1_BIN) $(BOOT16_STAGE2_BIN) $(BOOT32_STAGE3_BIN) > $(BOOTLOADER_BIN)

build_directory:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
