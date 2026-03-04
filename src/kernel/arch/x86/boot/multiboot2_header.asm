[BITS 32]

MAGIC equ 0xE85250D6
ARCH  equ 0 ; protected mode i386

section .multiboot
align 8
global multiboot2_header

multiboot2_header:
    dd MAGIC
    dd ARCH
    dd header_end - multiboot2_header
    dd 0x100000000 - (MAGIC + ARCH + (header_end - multiboot2_header))

    ; Framebuffer Tag
    align 8
    dw 5 ; type
    dw 0 ; flags
    dd 20 ; size
    dd 0 ; width
    dd 0 ; height
    dd 0 ; depth

    ; End Tag
    align 8
    dw 0 ; type
    dw 0 ; flags
    dd 8 ; size
header_end:
