# NanoOS

NanoOS is a simple kernel development project that I develop in my free time.

## Features
- GRUB as bootloader
- IDT and IRQ management
- Physical and virtual memory manager
- Dynamic memory allocator
- Simple framebuffer driver with psf2 font support
- A printf libary implementation
- Other hardware-specific drivers

## Building
1. Build the docker image using the [Build Docker](scripts/linux/build_docker.sh) script
2. Build the os ISO using the [Run Docker](scripts/linux/run_docker.sh) script
3. Execute the os:
   - Flash to USB Drive using the [Flash ISO](scripts/linux/flash_iso.sh) script
   - Run with QEMU using the [Run QEMU](scripts/linux/run_qemu.sh) script
