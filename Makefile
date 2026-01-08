# Compilers
TARGET = x86_64-elf
NASM = nasm
GCC = $(TARGET)-gcc
LD = $(TARGET)-ld
OBJCOPY = $(TARGET)-objcopy

# Flags
C_FLAGS = -ffreestanding -m32 -O1 -Wall -Wextra -Werror -fno-builtin -fno-stack-protector -I$(KERNEL_DIR)/include
LD_FLAGS = -m elf_i386

# Directories
SRC_DIR = src
BUILD_DIR = build
BOOTLOADER_DIR = $(SRC_DIR)/bootloader
KERNEL_DIR = $(SRC_DIR)/kernel

# Files
## Bootloader
BOOT16_STAGE1_SRC = $(BOOTLOADER_DIR)/boot16_stage1.asm
BOOT16_STAGE1_BIN = $(BUILD_DIR)/boot16_stage1.bin
BOOT16_STAGE2_SRC = $(BOOTLOADER_DIR)/boot16_stage2.asm
BOOT16_STAGE2_BIN = $(BUILD_DIR)/boot16_stage2.bin
BOOT32_STAGE3_SRC = $(BOOTLOADER_DIR)/boot32_stage3.asm
BOOT32_STAGE3_BIN = $(BUILD_DIR)/boot32_stage3.bin
BOOTLOADER_BIN = $(BUILD_DIR)/bootloader.bin
## Kernel
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d)) # recursive wildcard function
KERNEL_SOURCES = $(call rwildcard,$(KERNEL_DIR),*.c)
KERNEL_OBJECTS = $(patsubst $(KERNEL_DIR)/%.c, $(BUILD_DIR)/%.o, $(KERNEL_SOURCES))
KERNEL_ASM_SOURCES = $(call rwildcard,$(KERNEL_DIR),*.asm)
KERNEL_ASM_OBJECTS = $(patsubst $(KERNEL_DIR)/%.asm, $(BUILD_DIR)/%.o, $(KERNEL_ASM_SOURCES))
KERNEL_LINKER_SCRIPT = $(KERNEL_DIR)/linker.ld
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin

# OS Image
OS_IMAGE = $(BUILD_DIR)/nanoos.img

.PHONY: all os kernel bootloader clean

all: os

os: $(OS_IMAGE)

$(OS_IMAGE): $(BOOTLOADER_BIN) $(KERNEL_BIN) | build_directory
	cat $(BOOTLOADER_BIN) $(KERNEL_BIN) > $(OS_IMAGE)

kernel: $(KERNEL_BIN)

$(KERNEL_BIN): $(KERNEL_ELF) | build_directory
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)

$(KERNEL_ELF): $(KERNEL_OBJECTS) $(KERNEL_ASM_OBJECTS) $(KERNEL_LINKER_SCRIPT) | build_directory
	$(LD) $(LD_FLAGS) -T $(KERNEL_LINKER_SCRIPT) -o $(KERNEL_ELF) $(KERNEL_OBJECTS) $(KERNEL_ASM_OBJECTS)

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | build_directory
	@mkdir -p $(dir $@)
	$(GCC) $(C_FLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.asm | build_directory
	@mkdir -p $(dir $@)
	$(NASM) -f elf32 $< -o $@

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
