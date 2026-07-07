# Compilers
TARGET = x86_64-elf
CC = $(TARGET)-gcc
LD = $(TARGET)-ld
OBJCOPY = $(TARGET)-objcopy
NASM = nasm

# Targets
.PHONY: all clean iso kernel run

all: iso

# --- Kernel ---
kernel:

# --- ISO ---
iso:

# --- Run ---
run:

# --- Clean ---
clean:
