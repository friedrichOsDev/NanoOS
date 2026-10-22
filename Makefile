# META
NAME = "NanoOS"
VERSION = 0.0.2

# Compilers
TARGET = x86_64-elf
CC = $(TARGET)-gcc
LD = $(TARGET)-ld
OBJCOPY = $(TARGET)-objcopy
NASM = nasm

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Directories
SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso
GRUB_DIR = grub
KERNEL_DIR = $(SRC_DIR)/kernel
INCLUDE_DIR = $(KERNEL_DIR)/include

# Files
KERNEL_ELF   = $(BUILD_DIR)/kernel.elf
KERNEL_DEBUG = $(BUILD_DIR)/kernel.debug
ISO_IMAGE    = $(BUILD_DIR)/nanoos.iso
ISO_DEBUG    = $(BUILD_DIR)/nanoos-debug.iso
LINKER       = $(KERNEL_DIR)/arch/x86_64/linker.ld

ALL_INCLUDE_DIRS = $(sort $(dir $(call rwildcard,$(INCLUDE_DIR),*/)))
INCLUDE_FLAGS = -I$(INCLUDE_DIR) $(foreach dir,$(ALL_INCLUDE_DIRS),-I$(dir))

C_SOURCES   = $(call rwildcard,$(KERNEL_DIR),*.c)
ASM_SOURCES = $(call rwildcard,$(KERNEL_DIR),*.asm)
S_SOURCES := $(call rwildcard,$(KERNEL_DIR),*.s)

C_OBJECTS       = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/release/%.o,$(C_SOURCES))
ASM_OBJECTS     = $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/release/%.o,$(ASM_SOURCES))
S_OBJECTS       = $(patsubst $(KERNEL_DIR)/%.s,$(BUILD_DIR)/release/%.o,$(S_SOURCES))
OBJECTS         = $(C_OBJECTS) $(ASM_OBJECTS) $(S_OBJECTS)

DEBUG_C_OBJECTS   = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/debug/%.o,$(C_SOURCES))
DEBUG_ASM_OBJECTS = $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/debug/%.o,$(ASM_SOURCES))
DEBUG_S_OBJECTS   = $(patsubst $(KERNEL_DIR)/%.s,$(BUILD_DIR)/debug/%.o,$(S_SOURCES))
DEBUG_OBJECTS     = $(DEBUG_C_OBJECTS) $(DEBUG_ASM_OBJECTS) $(DEBUG_S_OBJECTS)

# Precompiled Header
PCH_SRC         = $(INCLUDE_DIR)/pch.h
PCH_REL         = $(BUILD_DIR)/release/pch.h.gch
PCH_DBG         = $(BUILD_DIR)/debug/pch.h.gch

# COMPILER-WARNUNGEN & KERNEL-FLAGS
# --- Basis ---
WARN_BASE  = -std=c23 -Wall -Wextra -Werror #  -Wpedantic

# --- Typen & Konvertierungen ---
WARN_TYPES = -Wconversion -Wsign-conversion -Warith-conversion \
             -Wcast-qual -Wcast-align=strict -Wbad-function-cast \
             -Wdouble-promotion -Wfloat-equal -Wpointer-arith \
             -Wwrite-strings -Wenum-conversion -Wunsuffixed-float-constants

# --- Deklarationen & Präprozessor ---
WARN_DECL  = -Wshadow -Wmissing-prototypes -Wmissing-declarations \
             -Wredundant-decls -Wnested-externs -Wundef -Wunused-macros \
             -Wunused-const-variable=2 -Wmissing-include-dirs -Wdate-time

# --- Kontrollfluss ---
WARN_FLOW  = -Wswitch-enum -Wswitch-default -Wimplicit-fallthrough=5 \
             -Wlogical-op -Wduplicated-cond -Wduplicated-branches \
             -Wjump-misses-init -Wnull-dereference -Wstrict-overflow=2 \
             -Wtrampolines

# --- Speicher & Stack (16-KiB-Kernelstacks!) ---
WARN_MEM   = -Warray-bounds=2 -Wstringop-overflow=4 -Wstringop-truncation \
             -Wvla -Walloca -Walloc-zero -Wstack-usage=4096 \
             -Wframe-larger-than=4096 -Wpacked -Wattribute-alias=2

# --- Format (printf-artige Funktionen) ---
WARN_FMT   = -Wformat=2 -Wformat-signedness -Wformat-overflow=2 \
             -Wformat-truncation=2

# --- Codegenerierung (Kernel-spezifisch) ---
CG_FLAGS   = -fno-omit-frame-pointer -fno-delete-null-pointer-checks \
             -fwrapv -fno-common -fno-pic -fno-pie -fcf-protection=none \
             -fno-asynchronous-unwind-tables

KERNEL_CORE_FLAGS = -ffreestanding -m64 -mcmodel=kernel -mno-red-zone -mno-mmx \
                    -fno-stack-protector -fno-builtin -fno-strict-aliasing -nostdlib

# Precompiled Header
PCH_SRC         = $(INCLUDE_DIR)/pch.h
PCH_REL         = $(BUILD_DIR)/release/pch.h.gch
PCH_DBG         = $(BUILD_DIR)/debug/pch.h.gch

# Compiler Flags (Release and Debug)
CFLAGS       = $(KERNEL_CORE_FLAGS) -O1 $(WARN_BASE) \
               $(WARN_FMT) $(CG_FLAGS) $(INCLUDE_FLAGS)

DEBUG_CFLAGS = $(KERNEL_CORE_FLAGS) -O0 -g -DKERNEL_DEBUG_BUILD $(WARN_BASE) \
               $(WARN_FMT) \
			   $(CG_FLAGS) $(INCLUDE_FLAGS)

# Static Analysis: make analyze flags
ANALYZE_FLAGS = -fanalyzer -fanalyzer-checker=taint

LDFLAGS = -m elf_x86_64 -T $(LINKER)

# Targets
.PHONY: all clean iso iso-debug debug release run run-debug gdb analyze

all: iso

# --- Static Analysis ---
analyze:
	@echo " [ANALYZE] Running static analysis with GCC Analyzer..."
	@for file in $(C_SOURCES); do \
		echo " Analysing $$file..."; \
		$(CC) $(CFLAGS) $(ANALYZE_FLAGS) -fsyntax-only $$file || exit 1; \
	done

# --- Kernel (Release) ---
release: $(KERNEL_ELF)

$(KERNEL_ELF): $(PCH_REL) $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

# PCH GCH (Release)
$(PCH_REL): $(PCH_SRC)
	@mkdir -p $(dir $@)
	@echo " [PCH] Compiling Precompiled Header (Release)..."
	$(CC) $(CFLAGS) -x c-header $< -o $@

$(BUILD_DIR)/release/%.o: $(KERNEL_DIR)/%.c $(PCH_REL)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -include $(PCH_SRC) -c $< -o $@

$(BUILD_DIR)/release/%.o: $(KERNEL_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 $< -o $@

$(BUILD_DIR)/release/%.o: $(KERNEL_DIR)/%.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -include $(PCH_SRC) -c $< -o $@

# --- Kernel (Debug) ---
debug: $(KERNEL_DEBUG)

$(KERNEL_DEBUG): $(PCH_DBG) $(DEBUG_OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(DEBUG_OBJECTS)

# PCH GCH (Debug)
$(PCH_DBG): $(PCH_SRC)
	@mkdir -p $(dir $@)
	@echo " [PCH] Compiling Precompiled Header (Debug)..."
	$(CC) $(DEBUG_CFLAGS) -x c-header $< -o $@

$(BUILD_DIR)/debug/%.o: $(KERNEL_DIR)/%.c $(PCH_DBG)
	@mkdir -p $(dir $@)
	$(CC) $(DEBUG_CFLAGS) -include $(PCH_SRC) -c $< -o $@

$(BUILD_DIR)/debug/%.o: $(KERNEL_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) -g -F dwarf -f elf64 $< -o $@

$(BUILD_DIR)/debug/%.o: $(KERNEL_DIR)/%.s $(PCH_DBG)
	@mkdir -p $(dir $@)
	$(CC) $(DEBUG_CFLAGS) -include $(PCH_SRC) -c $< -o $@

# --- ISOs ---
iso: release
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp $(GRUB_DIR)/grub.cfg $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_IMAGE) $(ISO_DIR)

iso-debug: debug
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_DEBUG) $(ISO_DIR)/boot/kernel.elf
	cp $(GRUB_DIR)/grub.cfg $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_DEBUG) $(ISO_DIR)

# --- Run ---
run: iso
	qemu-system-x86_64 \
		-machine q35,pflash0=ovmf-code,pflash1=ovmf-vars \
		-blockdev node-name=ovmf-code,driver=file,filename=./uefi/OVMF_CODE.4m.fd,read-only=on \
		-blockdev node-name=ovmf-vars,driver=file,filename=./uefi/OVMF_VARS.4m.fd \
		-smp 4 \
		-m 8G \
		-vga virtio \
		-display gtk,gl=off,zoom-to-fit=off \
		-cdrom $(ISO_IMAGE) \
		-no-reboot \
		-no-shutdown \
		-d int,cpu_reset \
		-D qemu.log \
		-serial file:serial.log

# --- Run with debug ---
run-debug: iso-debug
	qemu-system-x86_64 \
		-machine q35,pflash0=ovmf-code,pflash1=ovmf-vars \
		-blockdev node-name=ovmf-code,driver=file,filename=./uefi/OVMF_CODE.4m.fd,read-only=on \
		-blockdev node-name=ovmf-vars,driver=file,filename=./uefi/OVMF_VARS.4m.fd \
		-smp 4 \
		-m 8G \
		-vga virtio \
		-display gtk,gl=off,zoom-to-fit=off \
		-cdrom $(ISO_DEBUG) \
		-no-reboot \
		-no-shutdown \
		-d int,cpu_reset \
		-D qemu.log \
		-serial file:serial.log \
		-S -s

gdb:
	gdb build/kernel.debug -ex "target remote localhost:1234"

# --- Clean ---
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)
	rm -rf qemu.log serial.log