# Compilers
TARGET = x86_64-elf
CC = $(TARGET)-gcc
LD = $(TARGET)-ld
OBJCOPY = $(TARGET)-objcopy
NASM = nasm

# Directories
SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso
GRUB_DIR = grub
KERNEL_DIR = $(SRC_DIR)/kernel

# Recursive Wildcard
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Files
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
ISO_IMAGE = $(BUILD_DIR)/nanoos.iso
LINKER = $(KERNEL_DIR)/arch/x86_64/linker.ld

INCLUDE_DIRS = $(shell find $(KERNEL_DIR)/include -type d)
INCLUDE_FLAGS = $(foreach dir,$(INCLUDE_DIRS),-I$(dir))

C_SOURCES   = $(call rwildcard,$(KERNEL_DIR),*.c)
ASM_SOURCES = $(call rwildcard,$(KERNEL_DIR),*.asm)

C_OBJECTS   = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS = $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
OBJECTS     = $(C_OBJECTS) $(ASM_OBJECTS)

CFLAGS = -ffreestanding -m64 -O1 -Wall -Wextra -Werror \
         -fno-stack-protector -fno-builtin -fno-strict-aliasing \
         -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -nostdlib \
         $(INCLUDE_FLAGS)

LDFLAGS = -m elf_x86_64 -T $(LINKER)

# Targets
.PHONY: all clean iso kernel

all: clean iso

# --- Kernel ---
kernel: $(KERNEL_ELF)

$(KERNEL_ELF): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 $< -o $@

# --- ISO ---
iso: $(KERNEL_ELF)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp $(GRUB_DIR)/grub.cfg $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_IMAGE) $(ISO_DIR)

# --- Clean ---
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)