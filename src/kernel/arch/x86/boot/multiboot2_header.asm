[BITS 32]

section .multiboot
align 8
global multiboot2_header

multiboot2_header:
    dd 0xE85250D6 ; magic
    dd 0 ; architecture (0 = i386)
    dd header_end - multiboot2_header
    dd - (0xE85250D6 + 0 + (header_end - multiboot2_header))
    dw 0 ; type
    dw 0 ; flags
    dd 8 ; size
header_end:
