# Compilers
TARGET = i686-elf
CC = $(TARGET)-gcc
LD = $(TARGET)-ld
OBJCOPY = $(TARGET)-objcopy
NASM = nasm

# Directories
SRC_DIR = src
ASSET_DIR = assets
BUILD_DIR = build
ISO_DIR = iso
GRUB_DIR = grub
KERNEL_DIR  = $(SRC_DIR)/kernel

# Files
KERNEL_ELF   = $(BUILD_DIR)/kernel.elf
ISO_IMAGE    = $(BUILD_DIR)/nanoos.iso
DISK_IMAGE_1 = $(BUILD_DIR)/disk1.img
DISK_IMAGE_2 = $(BUILD_DIR)/disk2.img
LINKER       = $(KERNEL_DIR)/linker.ld

# recursive wildcard
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

INCLUDE_DIRS = $(shell find $(KERNEL_DIR)/include -type d)
INCLUDE_FLAGS = $(foreach dir,$(INCLUDE_DIRS),-I$(dir))

C_SOURCES   = $(call rwildcard,$(KERNEL_DIR),*.c)
ASM_SOURCES = $(call rwildcard,$(KERNEL_DIR),*.asm)
ASSET_SOURCES = $(call rwildcard,$(ASSET_DIR),*.psf)

C_OBJECTS   = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS = $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
ASSET_OBJECTS = $(patsubst $(ASSET_DIR)/%.psf,$(BUILD_DIR)/%.o,$(ASSET_SOURCES))
OBJECTS     = $(C_OBJECTS) $(ASM_OBJECTS) $(ASSET_OBJECTS)

# Flags
CFLAGS = -ffreestanding -m32 -O1 -Wall -Wextra -Werror \
         -fno-stack-protector -fno-builtin -fno-strict-aliasing -nostdlib \
         $(INCLUDE_FLAGS)
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

$(BUILD_DIR)/%.o: $(ASSET_DIR)/%.psf
	@mkdir -p $(dir $@)
	$(OBJCOPY) -I binary -O elf32-i386 -B i386 $< $@

# --- ISO ---
iso: $(KERNEL_ELF)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp $(GRUB_DIR)/grub.cfg $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_IMAGE) $(ISO_DIR)

# --- Disks ---
disks:
	@mkdir -p $(BUILD_DIR)

	qemu-img create -f raw $(DISK_IMAGE_1) 64M
	parted --script $(DISK_IMAGE_1) mklabel msdos
	parted --script $(DISK_IMAGE_1) mkpart primary fat32 1MiB 100%
	mkfs.vfat --offset=2048 -F 32 $(DISK_IMAGE_1)
	mmd -i $(DISK_IMAGE_1)@@1M ::/test_dir
	echo "Hello from Disk 1" > $(BUILD_DIR)/test1.txt
	mcopy -i $(DISK_IMAGE_1)@@1M $(BUILD_DIR)/test1.txt ::/test_dir/test.txt
	echo "Root file 1" > $(BUILD_DIR)/root1_1.txt
	mcopy -i $(DISK_IMAGE_1)@@1M $(BUILD_DIR)/root1_1.txt ::/root1_1.txt
	echo "Root file 2" > $(BUILD_DIR)/root1_2.txt
	mcopy -i $(DISK_IMAGE_1)@@1M $(BUILD_DIR)/root1_2.txt ::/root1_2.txt
	mmd -i $(DISK_IMAGE_1)@@1M ::/extra_dir
	echo "Extra 1" > $(BUILD_DIR)/extra1_1.txt
	echo "Extra 2" > $(BUILD_DIR)/extra1_2.txt
	echo "Extra 3" > $(BUILD_DIR)/extra1_3.txt
	mcopy -i $(DISK_IMAGE_1)@@1M $(BUILD_DIR)/extra1_1.txt ::/extra_dir/file1.txt
	mcopy -i $(DISK_IMAGE_1)@@1M $(BUILD_DIR)/extra1_2.txt ::/extra_dir/file2.txt
	mcopy -i $(DISK_IMAGE_1)@@1M $(BUILD_DIR)/extra1_3.txt ::/extra_dir/file3.txt
	
	qemu-img create -f raw $(DISK_IMAGE_2) 1G
	parted --script $(DISK_IMAGE_2) mklabel gpt
	parted --script $(DISK_IMAGE_2) mkpart root fat32 1MiB 100%
	mkfs.vfat --offset=2048 -F 32 $(DISK_IMAGE_2)
	mmd -i $(DISK_IMAGE_2)@@1M ::/data
	echo "Hello from Disk 2" > $(BUILD_DIR)/test3.txt
	mcopy -i $(DISK_IMAGE_2)@@1M $(BUILD_DIR)/test3.txt ::/data/readme.txt
	echo "Root file A" > $(BUILD_DIR)/root2_a.txt
	mcopy -i $(DISK_IMAGE_2)@@1M $(BUILD_DIR)/root2_a.txt ::/root2_a.txt
	echo "Root file B" > $(BUILD_DIR)/root2_b.txt
	mcopy -i $(DISK_IMAGE_2)@@1M $(BUILD_DIR)/root2_b.txt ::/root2_b.txt
	mmd -i $(DISK_IMAGE_2)@@1M ::/more_data
	echo "Data 1" > $(BUILD_DIR)/extra2_1.txt
	echo "Data 2" > $(BUILD_DIR)/extra2_2.txt
	echo "Data 3" > $(BUILD_DIR)/extra2_3.txt
	mcopy -i $(DISK_IMAGE_2)@@1M $(BUILD_DIR)/extra2_1.txt ::/more_data/data1.bin
	mcopy -i $(DISK_IMAGE_2)@@1M $(BUILD_DIR)/extra2_2.txt ::/more_data/data2.bin
	mcopy -i $(DISK_IMAGE_2)@@1M $(BUILD_DIR)/extra2_3.txt ::/more_data/data3.bin

# --- Run ---
run: iso disks
	qemu-system-i386 -m 4G -vga std -cdrom $(ISO_IMAGE) -boot d -drive file=$(DISK_IMAGE_1),format=raw,if=ide -drive file=$(DISK_IMAGE_2),format=raw,if=ide -no-reboot -d int,cpu_reset -D q.log -serial file:serial.log

# --- Clean ---
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)
	rm -rf q.log
	rm -rf serial.log