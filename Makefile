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
rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Files
KERNEL_ELF   = $(BUILD_DIR)/kernel.elf
KERNEL_DEBUG = $(BUILD_DIR)/kernel.debug
ISO_IMAGE    = $(BUILD_DIR)/nanoos.iso
ISO_DEBUG    = $(BUILD_DIR)/nanoos-debug.iso
LINKER       = $(KERNEL_DIR)/arch/x86_64/linker.ld

ALL_INCLUDE_DIRS = $(sort $(dir $(call rwildcard,$(KERNEL_DIR)/include,*/)))
INCLUDE_FLAGS = -Isrc/kernel/include $(foreach dir,$(ALL_INCLUDE_DIRS),-I$(dir))

C_SOURCES   = $(call rwildcard,$(KERNEL_DIR),*.c)
ASM_SOURCES = $(call rwildcard,$(KERNEL_DIR),*.asm)

C_OBJECTS       = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/release/%.o,$(C_SOURCES))
ASM_OBJECTS     = $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/release/%.o,$(ASM_SOURCES))
OBJECTS         = $(C_OBJECTS) $(ASM_OBJECTS)

DEBUG_C_OBJECTS   = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/debug/%.o,$(C_SOURCES))
DEBUG_ASM_OBJECTS = $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/debug/%.o,$(ASM_SOURCES))
DEBUG_OBJECTS     = $(DEBUG_C_OBJECTS) $(DEBUG_ASM_OBJECTS)

# Default flags (Release)
CFLAGS = -ffreestanding -m64 -O1 -Wall -Wextra -Werror \
         -fno-stack-protector -fno-builtin -fno-strict-aliasing \
         -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -nostdlib \
         $(INCLUDE_FLAGS)

# Default flags (Debug)
DEBUG_CFLAGS = -ffreestanding -m64 -O0 -g -Wall -Wextra -Werror \
               -fno-stack-protector -fno-builtin -fno-strict-aliasing \
               -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -nostdlib \
               $(INCLUDE_FLAGS)

LDFLAGS = -m elf_x86_64 -T $(LINKER)

# Targets
.PHONY: all clean iso iso-debug kernel debug run run-debug gdb

all: iso

# --- Kernel (Release) ---
kernel: $(KERNEL_ELF)

$(KERNEL_ELF): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

$(BUILD_DIR)/release/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/release/%.o: $(KERNEL_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 $< -o $@

# --- Kernel (Debug) ---
debug: $(KERNEL_DEBUG)

$(KERNEL_DEBUG): $(DEBUG_OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(DEBUG_OBJECTS)

$(BUILD_DIR)/debug/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(DEBUG_CFLAGS) -c $< -o $@

$(BUILD_DIR)/debug/%.o: $(KERNEL_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) -g -F dwarf -f elf64 $< -o $@

# --- ISOs ---
iso: $(KERNEL_ELF)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp $(GRUB_DIR)/grub.cfg $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_IMAGE) $(ISO_DIR)

iso-debug: $(KERNEL_DEBUG)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_DEBUG) $(ISO_DIR)/boot/kernel.elf
	cp $(GRUB_DIR)/grub.cfg $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_DEBUG) $(ISO_DIR)

# --- Run ---
run: iso
	qemu-system-x86_64 -smp 4 -bios ./uefi/OVMF.fd -m 8G -vga std -display gtk,gl=off -cdrom $(ISO_IMAGE) -no-reboot -d int,cpu_reset -D qemu.log -serial file:serial.log

# --- Run with debug ---
run-debug: iso-debug
	qemu-system-x86_64 -smp 4 -bios ./uefi/OVMF.fd -m 8G -vga std -display gtk,gl=off -cdrom $(ISO_DEBUG) -no-reboot -d int,cpu_reset -D qemu.log -serial file:serial.log -S -s

gdb:
	gdb build/kernel.debug -ex "target remote localhost:1234"

# --- Clean ---
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)
	rm -rf qemu.log serial.log