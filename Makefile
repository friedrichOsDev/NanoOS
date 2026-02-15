# Compilers
TARGET = i686-elf
CC = $(TARGET)-gcc
LD = $(TARGET)-ld
OBJCOPY = $(TARGET)-objcopy
NASM = nasm

# Directories
SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso
GRUB_DIR = grub
KERNEL_DIR  = $(SRC_DIR)/kernel

# Files
KERNEL_ELF  = $(BUILD_DIR)/kernel.elf
ISO_IMAGE   = $(BUILD_DIR)/nanoos.iso

LINKER      = $(KERNEL_DIR)/linker.ld

# recursive wildcard
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

C_SOURCES   = $(call rwildcard,$(KERNEL_DIR),*.c)
ASM_SOURCES = $(call rwildcard,$(KERNEL_DIR),*.asm)

C_OBJECTS   = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS = $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

OBJECTS     = $(C_OBJECTS) $(ASM_OBJECTS)

# Flags
CFLAGS = -ffreestanding -m32 -O2 -Wall -Wextra -Werror -fno-stack-protector -fno-builtin -nostdlib -I$(KERNEL_DIR)/include
LDFLAGS = -m elf_i386 -T $(LINKER)

# Targets
.PHONY: all clean iso kernel run

all: iso

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
	$(NASM) -f elf32 $< -o $@

# --- ISO ---
iso: $(KERNEL_ELF)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp $(GRUB_DIR)/grub.cfg $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_IMAGE) $(ISO_DIR)

# --- Run ---
run: iso
	qemu-system-i386 -m 4G -cdrom $(ISO_IMAGE) -no-reboot -d int,cpu_reset -D q.log -serial file:serial.log

# --- Clean ---
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)
	rm -rf q.log
	rm -rf serial.log